// Wisent Echoreck: Röhren-Echogerät mit Magnettrommel nach dem Schaltplan des Binson Echorec PE 603 T (wisent_core.hpp, docs/wisent/SCHALTPLAN.md)
// als CLAP-Plugin (Linux x86-64; auf dem Mac über clap-wrapper als VST3/Audio Unit). Eingang mono (bei Stereo die Summe), Ausgang mono auf beiden Kanälen.
// Latenz: 3 Abtastwerte bis 48 kHz (Halbbänder der 2-fach-Überabtastung der Aufnahmeseite), ab 88,2 kHz 0.
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "1.4"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "wisent_core.hpp"
#include "platform.hpp"
#include "ports.hpp"
#ifndef TS_NO_GUI
#include "wisentgui.hpp"
#else
namespace wisentgui { struct Gui; enum { P_INPUT, P_SWELL, P_VOLUME, P_BASS, P_TREBLE, P_MODE, P_OUT1, P_FB1 = P_OUT1 + 4, P_LEVEL = P_FB1 + 4, P_OUTDB, P_MAINS, P_DRY, P_SPEED, P_WOB, P_H1, P_H2, P_H3, P_H4, P_COUNT }; }
#endif
#include <array>
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
#include "x11safe.hpp"
#endif
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
#define TSX_GUARD tsx::Guard xg_
#else
#define TSX_GUARD (void)0
#endif
using namespace wisentgui;
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_ONOFF[] = {"Off", "On"};
static const char *N_MODE[] = {"Echo", "Repeat", "Swell"};
static const char *N_MAINS[] = {"50 Hz", "60 Hz"};
static const PDef PD[P_COUNT] = { {"Input", 0, 10, 2.5, 0, 0}, {"Length of swell", 0, 10, 5, 0, 0}, {"Volume", 0, 10, 10, 0, 0}, {"Bass", 0, 10, 5, 0, 0}, {"Treble", 0, 10, 5, 0, 0},
  {"Mode", 0, 2, 1, N_MODE, 3}, {"Playback head 1", 0, 1, 0, N_ONOFF, 2}, {"Playback head 2", 0, 1, 1, N_ONOFF, 2}, {"Playback head 3", 0, 1, 0, N_ONOFF, 2}, {"Playback head 4", 0, 1, 0, N_ONOFF, 2},
  {"Feedback head 1", 0, 1, 0, N_ONOFF, 2}, {"Feedback head 2", 0, 1, 1, N_ONOFF, 2}, {"Feedback head 3", 0, 1, 0, N_ONOFF, 2}, {"Feedback head 4", 0, 1, 0, N_ONOFF, 2},
  {"Level (mV at 0 dBFS)", 20, 2000, 200, 0, 0}, {"Output (dB)", -24, 24, 0, 0, 0}, {"Mains (drum speed)", 0, 1, 0, N_MAINS, 2},
  {"Dry (trimmer TD)", 0, 10, 6.7, 0, 0}, {"Varispeed (mod)", 0.25, 2, 1, 0, 0},   // 1.1: hinten angehängt (gespeicherte Projekte von 1.0 bleiben gültig)
  {"Wobble (worn drum)", 0, 10, 0, 0, 0},   // 1.2: Taumeln der Scheibe (Verschleiß), hinten angehängt; ab 1.4 leiert die Trommel dabei auch
  // 1.4: Lage der vier Wiedergabeköpfe (Modell), als Verzögerung in ms bei 50 Hz ohne Varispeed; Original 74,4 / 148,8 / 223,2 / 297,6 ms
  {"Head 1 position (ms at 50 Hz)", wisent::Core::HEAD_MIN, wisent::Core::HEAD_MAX, wisent::Core::HEAD_MS0 * 1, 0, 0}, {"Head 2 position (ms at 50 Hz)", wisent::Core::HEAD_MIN, wisent::Core::HEAD_MAX, wisent::Core::HEAD_MS0 * 2, 0, 0},
  {"Head 3 position (ms at 50 Hz)", wisent::Core::HEAD_MIN, wisent::Core::HEAD_MAX, wisent::Core::HEAD_MS0 * 3, 0, 0}, {"Head 4 position (ms at 50 Hz)", wisent::Core::HEAD_MIN, wisent::Core::HEAD_MAX, wisent::Core::HEAD_MS0 * 4, 0, 0} };

struct Plugin {
  PortCfg ports;
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; wisent::Core *core = nullptr; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true}, fresh{true}, active{false};
  std::atomic<double> load{0}, eye{0}; std::vector<float> a, oa; long quiet = 0; std::atomic<bool> sleeping{false}; wisent::Setting cur; double outG = 1;
  wisentgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; double guiW = 1640, guiH = 0;
  std::string diagMode, diagTxt; int diagCnt = 0;
  std::array<double, 4> heads() const { return {val[P_H1].load(), val[P_H2].load(), val[P_H3].load(), val[P_H4].load()}; }   // Kopfstellungen (ms bei 50 Hz, ab 1.4)
  wisent::Setting target() const { wisent::Setting s; s.input = val[P_INPUT].load() / 10; s.swell = val[P_SWELL].load() / 10; s.volume = val[P_VOLUME].load() / 10; s.bass = val[P_BASS].load() / 10; s.treble = val[P_TREBLE].load() / 10; s.dry = val[P_DRY].load() / 10;
    s.mode = std::max(0, std::min(2, (int)std::lround(val[P_MODE].load()))); for (int k = 0; k < 4; k++) { s.out[k] = val[P_OUT1 + k].load() > 0.5; s.fb[k] = val[P_FB1 + k].load() > 0.5; } return s; }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "Wisent Echoreck"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); return true; }
  switch (id) { case P_LEVEL: std::snprintf(buf, n, "%.0f mV", v); break; case P_OUTDB: std::snprintf(buf, n, "%+.1f dB", v); break; case P_SPEED: std::snprintf(buf, n, "%.2fx", v); break; default: std::snprintf(buf, n, "%.1f", v); }
  return true; }
static bool par_t2v(const clap_plugin_t *, clap_id id, const char *txt, double *out) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { for (int k = 0; k < d.nn; k++) if (std::strncmp(txt, d.names[k], std::strlen(d.names[k])) == 0) { *out = k; return true; } }
  *out = std::atof(txt); return true; }
static void apply_events(Plugin *pl, const clap_input_events_t *in) {
  if (!in) return; uint32_t n = in->size(in);
  for (uint32_t i = 0; i < n; i++) { const clap_event_header_t *h = in->get(in, i);
    if (h->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    if (h->type == CLAP_EVENT_PARAM_VALUE) { const clap_event_param_value_t *e = (const clap_event_param_value_t*)h; if (e->param_id < P_COUNT && std::isfinite(e->value)) { pl->val[e->param_id].store(std::max(PD[e->param_id].lo, std::min(PD[e->param_id].hi, e->value))); pl->dirty = true; } } }
}
static void emit_gui(Plugin *pl, const clap_output_events_t *out) {
  std::lock_guard<std::mutex> lk(pl->qm);
  while (!pl->guiQ.empty()) { auto q = pl->guiQ.front(); pl->guiQ.pop_front(); if (!out) continue;
    if (q.kind) { clap_event_param_gesture_t g{}; g.header.size = sizeof g; g.header.type = q.kind == 1 ? CLAP_EVENT_PARAM_GESTURE_BEGIN : CLAP_EVENT_PARAM_GESTURE_END; g.header.space_id = CLAP_CORE_EVENT_SPACE_ID; g.param_id = q.id; out->try_push(out, &g.header); continue; }
    clap_event_param_value_t e{}; e.header.size = sizeof e; e.header.type = CLAP_EVENT_PARAM_VALUE; e.header.space_id = CLAP_CORE_EVENT_SPACE_ID; e.param_id = q.id; e.value = q.v; e.note_id = -1; e.port_index = -1; e.channel = -1; e.key = -1; out->try_push(out, &e.header); }
}
static void par_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) { Plugin *pl = P(p); apply_events(pl, in); emit_gui(pl, out); }
static const clap_plugin_params_t EXT_PARAMS = {par_count, par_info, par_value, par_v2t, par_t2v, par_flush};

// ---- Audio-Ports ----
static uint32_t ap_count(const clap_plugin_t *, bool) { return 1; }
static bool ap_get(const clap_plugin_t *p, uint32_t i, bool is_input, clap_audio_port_info_t *info) { return portInfo(P(p)->ports, i, is_input, info); }
static bool cap_can(const clap_plugin_t *, const clap_audio_port_configuration_request *r, uint32_t n) { return portReqOk(r, n); }
static bool cap_apply(const clap_plugin_t *p, const clap_audio_port_configuration_request *r, uint32_t n) { if (!portReqOk(r, n)) return false; portApply(P(p)->ports, r, n); return true; }
static const clap_plugin_configurable_audio_ports_t EXT_CAP = {cap_can, cap_apply};
static const clap_plugin_audio_ports_t EXT_PORTS = {ap_count, ap_get};

// ---- Zustand ----
static bool st_save(const clap_plugin_t *p, const clap_ostream_t *s) { char buf[1024]; int len = 0; for (int i = 0; i < P_COUNT; i++) len += std::snprintf(buf + len, sizeof buf - len, "%d=%.17g\n", i, P(p)->val[i].load());
  int done = 0; while (done < len) { int64_t w = s->write(s, buf + done, len - done); if (w <= 0) return false; done += (int)w; } return true; }
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); if (all.size() > 65536) return false; }
  bool seen[P_COUNT] = {};
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = std::atof(line.substr(q + 1).c_str()); if (id >= 0 && id < P_COUNT && std::isfinite(v)) { P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); seen[id] = true; } }
  for (int i = 0; i < P_COUNT; i++) if (!seen[i]) P(p)->val[i].store(PD[i].def);
  P(p)->dirty = true; return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
static bool pl_init(const clap_plugin_t *) { return true; }
static void stopAll(Plugin *pl) { delete pl->core; pl->core = nullptr; }
static void pl_destroy(const clap_plugin_t *p) { Plugin *pl = P(p);
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
  { auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; } }
#endif
#ifndef TS_NO_GUI
  { TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; }
#ifndef __APPLE__
  tsx::close(pl->dpy); pl->dpy = nullptr;
#endif
  }
#endif
  stopAll(pl); delete pl; }
// Hauptfaden: Netze übersetzen, Ruhezustand einschwingen (etwa 0,1 s), Trommel anlegen (etwa 1 s Draht bei doppelter Rate)
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->fs = sr; stopAll(pl);
  try { pl->cur = pl->target(); pl->core = new wisent::Core(); pl->core->init(sr, pl->cur, pl->val[P_MAINS].load() > 0.5 ? 60 : 50, pl->val[P_LEVEL].load() / 1000, pl->val[P_SPEED].load()); pl->core->setHeads(pl->heads().data(), true); }
  catch (...) { stopAll(pl); return false; }
  for (auto *v : {&pl->a, &pl->oa}) v->assign(maxf + 16, 0); pl->fresh = true; pl->active = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { Plugin *pl = P(p); pl->active = false; stopAll(pl); }
static bool pl_start(const clap_plugin_t *p) { P(p)->fresh = true; return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { P(p)->fresh = true; }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
// Reglerstellungen gleiten (etwa 70 ms über den ganzen Weg), Netze werden höchstens alle 32 Abtastwerte (bei 48 kHz; gleiche Zeit bei anderen Raten) neu übersetzt (meist Rang-1-Änderungen);
// Tasten wirken sofort
static void follow(Plugin *pl) { if (wisent::glideTo(pl->cur, pl->target())) pl->core->apply(pl->cur);
  pl->core->setSpeed(pl->val[P_MAINS].load() > 0.5 ? 60 : 50, pl->val[P_SPEED].load()); pl->core->setWobble(pl->val[P_WOB].load() / 10); pl->core->setHeads(pl->heads().data()); pl->core->vin = pl->val[P_LEVEL].load() / 1000; pl->outG = std::pow(10.0, pl->val[P_OUTDB].load() / 20); }
static bool settled(const Plugin *pl) { return wisent::sameKnobs(pl->cur, pl->target()); }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  uint32_t n = pr->frames_count; if (!pl->active || !pl->core || pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  if (n == 0) return CLAP_PROCESS_CONTINUE;
  wisent::Core &c = *pl->core; const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->a.size() < n) for (auto *v : {&pl->a, &pl->oa}) v->assign(n + 16, 0);
  float *a = pl->a.data(), *oa = pl->oa.data();
  if (pl->fresh.exchange(false)) { c.drum.clear(); pl->quiet = 0; pl->sleeping = false; }   // Neubeginn (activate, start, reset): Trommel leeren
  for (uint32_t i = 0; i < n; i++) { double x = 0; uint32_t nc = std::max(1u, in.channel_count); for (uint32_t ch = 0; ch < nc && ch < in.channel_count; ch++) x += in.data32 ? in.data32[ch][i] : in.data64 ? in.data64[ch][i] : 0; a[i] = (float)(x / nc); }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::fabs(a[i]));
  bool busy = pl->dirty.load() || !settled(pl);
  if (pl->sleeping && (inPk >= 1e-6f || busy)) { pl->sleeping = false; pl->quiet = 0; }
  if (pl->sleeping) { pl->eye = 0; zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  auto t0 = std::chrono::steady_clock::now(); double outPk = 0;
  const uint32_t ck = (uint32_t)std::max(32L, std::lround(32 * pl->fs / 48000)); for (uint32_t i0 = 0; i0 < n; i0 += ck) { if (pl->dirty.exchange(false) || !settled(pl)) follow(pl); uint32_t e = std::min(n, i0 + ck);
    for (uint32_t i = i0; i < e; i++) { double y = c.tick(a[i]) * pl->outG; if (!std::isfinite(y)) y = 0; oa[i] = (float)y; outPk = std::max(outPk, std::fabs(y)); } }
  pl->eye = c.eye;
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  for (uint32_t ch = 0; ch < out.channel_count; ch++) for (uint32_t i = 0; i < n; i++) { if (out.data32) out.data32[ch][i] = oa[i]; else if (out.data64) out.data64[ch][i] = oa[i]; }
  // Schlafen: Eingang still (< −120 dBFS) und Ausgang unter −70 dBFS (Rauschen des Drahts liegt darunter) für eine Trommelumdrehung + 2 s
  if (inPk < 1e-6f && outPk < 3e-4) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)((c.drum.trev + 2) * pl->fs) && !pl->sleeping) { pl->sleeping = true; pl->load = 0; c.drum.clear(); }
  return CLAP_PROCESS_CONTINUE;
}
static clap_process_status pl_process(const clap_plugin_t *p, const clap_process_t *pr) {
  DenormGuard dg; clap_process_status r;
  try { r = pl_process_impl(p, pr); } catch (...) { zeroOut(pr); r = CLAP_PROCESS_CONTINUE; }
  return r; }
static void pl_main(const clap_plugin_t *) {}
#ifndef TS_NO_GUI
static void gui_tick(Plugin *pl);
static void gui_setp(Plugin *pl, int id, double v) { v = std::max(PD[id].lo, std::min(PD[id].hi, v)); if (pl->val[id].load() == v) return; pl->val[id].store(v); pl->dirty = true;
    { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static void gui_gesture(Plugin *pl, int id, bool begin) { if (id < 0 || id >= P_COUNT) return;
  { std::lock_guard<std::mutex> lk(pl->qm); if (begin) { if (pl->gestId >= 0) pl->guiQ.push_back({pl->gestId, 0, 2}); pl->gestId = id; pl->guiQ.push_back({id, 0, 1}); } else if (pl->gestId == id) { pl->gestId = -1; pl->guiQ.push_back({id, 0, 2}); } else return; }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
#ifdef __APPLE__
#define TS_WINAPI CLAP_WINDOW_API_COCOA
#else
#define TS_WINAPI CLAP_WINDOW_API_X11
#endif
static bool g_supported(const clap_plugin_t *, const char *api, bool floating) { return !std::strcmp(api, TS_WINAPI) && !floating; }
static bool g_preferred(const clap_plugin_t *, const char **api, bool *floating) { *api = TS_WINAPI; *floating = false; return true; }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); TSX_GUARD; if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef __APPLE__
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { pl->gui = new wisentgui::Gui(); pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
  }
  pl->guiH = pl->guiW * wisentgui::TOTALH / wisentgui::FW;
  }
#ifdef __APPLE__
  { std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) pl->gui->onTick = [pl]() { gui_tick(pl); }; }
#else
  auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId == CLAP_INVALID_ID) ht->register_timer(pl->host, 33, &pl->timerId);
#endif
  return true; }
static void g_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); TSX_GUARD; if (pl->gestId >= 0) gui_gesture(pl, pl->gestId, false); auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; }
  std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; }
#ifndef __APPLE__
  if (pl->dpy) { XSync(pl->dpy, False); }
#endif
  }
static void upd_diag(Plugin *pl) { char b[160]; std::snprintf(b, sizeof b, "CPU %.1f %% of one core · %.0f Hz%s", pl->load.load() * 100, pl->fs, pl->sleeping ? " · idle (silent input)" : "");
  { const char *m = std::strstr(b, " · "); std::string mode = m ? m : ""; if (++pl->diagCnt < 15 && mode == pl->diagMode && !pl->diagTxt.empty()) return; pl->diagCnt = 0; pl->diagMode = mode; pl->diagTxt = b; } }
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; upd_diag(pl);
  pl->gui->eye = pl->active && !pl->sleeping ? pl->eye.load() : 0; }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)wisentgui::FW; h->aspect_ratio_height = (uint32_t)wisentgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h) { double ww = std::max(700.0, std::min(2600.0, (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * wisentgui::TOTALH / wisentgui::FW); return true; }
static bool g_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); pl->guiW = w; pl->guiH = h; if (pl->gui && pl->gui->win) { pl->gui->resize((int)w, (int)h); pl->gui->draw(); } return true; }
static bool g_set_parent(const clap_plugin_t *p, const clap_window_t *win) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; if (pl->gui->win) pl->gui->destroy();
#ifdef __APPLE__
  return pl->gui->attach(win->cocoa, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
#else
  if (!pl->dpy) return false; return pl->gui->init(pl->dpy, (Window)win->x11, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
#endif
static bool g_set_transient(const clap_plugin_t *, const clap_window_t *) { return false; }
static void g_suggest_title(const clap_plugin_t *, const char *) {}
static bool g_show(const clap_plugin_t *p) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->show(); pl->gui->draw(); return true; }
static bool g_hide(const clap_plugin_t *p) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->hide(); return true; }
static const clap_plugin_gui_t EXT_GUI = {g_supported, g_preferred, g_create, g_destroy, g_set_scale, g_get_size, g_can_resize, g_resize_hints, g_adjust_size, g_set_size, g_set_parent, g_set_transient, g_suggest_title, g_show, g_hide};
#endif
static uint32_t g_latency(const clap_plugin_t *p) { Plugin *pl = P(p); return pl->core ? (uint32_t)pl->core->lat : wisent::Core::LAT; }   // nach activate gültig (Rate)
static const clap_plugin_latency_t EXT_LATENCY = {g_latency};
// Echo klingt nach (CLAP tail): ECHO bis zum letzten Kopf, REPEAT/SWELL mit Rückkopplung länger (schwingt die Schleife: ohne Ende)
// Schwelle der Schleife über der Geschwindigkeit (wisent::loopThreshold, ein Rückkopplungskopf; hier etwas darunter)
static uint32_t g_tail(const clap_plugin_t *p) { Plugin *pl = P(p); int m = (int)std::lround(pl->val[P_MODE].load()); double sw = pl->val[P_SWELL].load() / 10;
  double thr = wisent::loopThreshold((pl->val[P_MAINS].load() > 0.5 ? 1.2 : 1.0) * pl->val[P_SPEED].load()) - 0.02;
  if (m != 0 && sw >= thr) return UINT32_MAX; double hmx = 0; for (int k = 0; k < 4; k++) hmx = std::max(hmx, pl->val[P_H1 + k].load()); const double rl = (pl->val[P_MAINS].load() > 0.5 ? 1.2 : 1.0) * std::max(0.25, pl->val[P_SPEED].load());
  double t = m == 0 ? std::max(0.4 / std::max(0.25, pl->val[P_SPEED].load()), hmx / 1000 / rl + 0.1) : std::max(0.4, hmx / 1000 / rl + 0.1) + 30 * std::pow(sw / std::min(1.0, thr), 3); return (uint32_t)std::lround(t * pl->fs); }   // ab 1.4: späteste Kopfstellung
static const clap_plugin_tail_t EXT_TAIL = {g_tail};
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
static void on_timer(const clap_plugin_t *p, clap_id) { Plugin *pl = P(p); TSX_GUARD; std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock()) return;
  try { if (pl->gui) { gui_tick(pl); pl->gui->pump(); } } catch (...) {} }
static const clap_plugin_timer_support_t EXT_TIMER = {on_timer};
#endif

static const void *pl_ext(const clap_plugin_t *, const char *id) {
#ifndef TS_NO_GUI
  if (!std::strcmp(id, CLAP_EXT_GUI)) return &EXT_GUI;
#endif
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
  if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &EXT_TIMER;
#endif
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_TAIL)) return &EXT_TAIL; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS) || !std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS_COMPAT)) return &EXT_CAP; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DELAY, CLAP_PLUGIN_FEATURE_MONO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.wisent-echoreck", "Wisent Echoreck (after Binson Echorec PE 603 T)", "tonestack", "", "", "", TS_VERSION, "Tube echo with magnetic drum after the schematic of the Binson Echorec PE 603 T: input, mixer and record amplifier as tube circuits (ECC83, ECC82), steel-wire drum with four playback heads (74, 148, 222, 296 ms at 50 Hz), permanent-magnet erase, playback preamps, ECHO/REPEAT/SWELL, playback and feedback head buttons, length of swell, volume, bass, treble, magic eye; level, output and 50/60 Hz drum speed added, trimmer TD as dry control and varispeed (a later modification); mono", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def);
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST
extern "C" bool wisent_entry_init(const char *p) { return entry_init(p); }
extern "C" void wisent_entry_deinit() { entry_deinit(); }
extern "C" const void *wisent_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
