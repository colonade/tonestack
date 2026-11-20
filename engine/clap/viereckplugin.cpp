// Viereck 34: Klangteil des Quad 34 (TILT, BASS Step/Lift, BALANCE, Filter F1/F2/SLOPE) nach dem Schaltplan als Schaltung im Zeitbereich
// (engine/src/viereck_core.hpp, docs/viereck/SCHALTPLAN.md) als CLAP-Plugin (Linux x86-64; auf dem Mac über clap-wrapper als VST3/Audio Unit).
// Ein Regler-Satz für beide Kanäle; BALANCE wirkt gegensinnig (RV4 A links, RV4 B rechts). Modell: TILT stufenlos, Drehpunkt 80 Hz … 4 kHz, LEVEL, OUTPUT.
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "1.0"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "viereck_core.hpp"
#include "platform.hpp"
#include "ports.hpp"
#ifndef TS_NO_GUI
#include "viereckgui.hpp"
#else
namespace vrgui { struct Gui; }
#endif
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

enum { P_TILT, P_BASS, P_BAL, P_FILT, P_SLOPE, P_MODE, P_PIV, P_LEVEL, P_OUT, P_COUNT };
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_BASS[] = {"Step 300 Hz", "Step 170 Hz", "Step 100 Hz", "0 dB (off)", "Lift +3 dB", "Lift +6 dB", "Lift +9 dB"};
static const char *N_FILT[] = {"Off", "Filter 1", "Filter 2"};
static const char *N_ONOFF[] = {"Off", "On"};
static const char *N_MODE[] = {"Steps (original, 7 positions)", "Smooth (continuous)"};
static const PDef PD[P_COUNT] = {
  {"Tilt (+ brighter, - warmer)", -3, 3, 0, 0, 0}, {"Bass", 0, 6, 3, N_BASS, 7}, {"Balance (- left, + right)", -1, 1, 0, 0, 0},
  {"Filter", 0, 2, 0, N_FILT, 3}, {"Filter slope 12 dB/oct", 0, 1, 0, N_ONOFF, 2}, {"Tilt control", 0, 1, 0, N_MODE, 2},
  {"Tilt pivot (Hz)", 80, 4000, 847.13, 0, 0}, {"Level: 0 dBFS (dB re rated output)", -30, 10, -10, 0, 0}, {"Output level (dB)", -24, 24, 0, 0, 0} };
static double vinOf(double lev) { return std::pow(10, lev / 20) * 0.5 * std::sqrt(2.0) / viereck::gain0(); }   // 0 dBFS in Volt Spitze am Eingang der Tilt-Stufe: ein 0-dBFS-Sinus gibt bei flacher Einstellung LEVEL dB re Nennausgang 0,5 V eff. an OUTPUT

struct Plugin {
  PortCfg ports;
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; viereck::Unit ch[2]; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true}, fresh{true}, active{false};
  std::atomic<double> load{0}; std::vector<float> m1, m2; double outg = 1, outCur = -1; long quiet = 0; std::atomic<bool> sleeping{false}; viereck::Setting set[2];
  std::atomic<double> pk[2]; double pkHold[2] = {0, 0};   // Aussteuerung IC12 / IC19 (Anteil der Grenze)
  vrgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; double guiW = 1400, guiH = 0;
  std::string diagMode, diagTxt; int diagCnt = 0;
  // Einstellung je Kanal: BALANCE −1 … +1 dreht RV4 A (links) und RV4 B (rechts) gegensinnig
  viereck::Setting target(int ch) const { viereck::Setting s; s.tilt = val[P_TILT]; s.stepped = val[P_MODE] < 0.5; s.pivot = val[P_PIV]; s.bass = (int)std::lround(val[P_BASS]) - 3;
    s.filter = std::max(0, std::min(2, (int)std::lround(val[P_FILT]))); s.slope = val[P_SLOPE] > 0.5; s.balance = viereck::balPos(val[P_BAL], ch); return s; }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "Viereck 34"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); }
  else if (id == P_OUT) std::snprintf(buf, n, "%+.1f dB", v);
  else if (id == P_LEVEL) std::snprintf(buf, n, "%+.1f dB (0 dBFS = %.2f V peak inside)", v, vinOf(v));
  else if (id == P_PIV) std::snprintf(buf, n, v >= 1000 ? "%.2f kHz" : "%.0f Hz", v >= 1000 ? v / 1000 : v);
  else if (id == P_BAL) std::snprintf(buf, n, std::fabs(v) < 0.005 ? "center" : "%+.2f", v);
  else std::snprintf(buf, n, v == std::round(v) ? "%+.0f" : "%+.2f", v);
  return true; }
static bool par_t2v(const clap_plugin_t *, clap_id id, const char *txt, double *out) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { for (int k = 0; k < d.nn; k++) if (std::strncmp(txt, d.names[k], std::strlen(d.names[k])) == 0) { *out = k; return true; } }
  *out = std::atof(txt); return true; }
static void apply_events(Plugin *pl, const clap_input_events_t *in) {
  if (!in) return; uint32_t n = in->size(in);
  for (uint32_t i = 0; i < n; i++) { const clap_event_header_t *h = in->get(in, i);
    if (h->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;
    if (h->type == CLAP_EVENT_PARAM_VALUE) { const clap_event_param_value_t *e = (const clap_event_param_value_t*)h; if (e->param_id < P_COUNT) { pl->val[e->param_id].store(std::max(PD[e->param_id].lo, std::min(PD[e->param_id].hi, e->value))); pl->dirty = true; } } }
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
  delete pl; }
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->fs = sr; for (int k = 0; k < 2; k++) { pl->set[k] = pl->target(k); pl->ch[k].vin = vinOf(pl->val[P_LEVEL]); pl->ch[k].init(sr, viereck::Parts(), pl->set[k]); } pl->m1.assign(maxf + 16, 0); pl->m2.assign(maxf + 16, 0); pl->fresh = true; pl->active = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { P(p)->active = false; }
static bool pl_start(const clap_plugin_t *p) { P(p)->fresh = true; return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { P(p)->fresh = true; }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  uint32_t n = pr->frames_count; if (!pl->active || pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  if (n == 0) return CLAP_PROCESS_CONTINUE;
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->m1.size() < n) { pl->m1.assign(n + 16, 0); pl->m2.assign(n + 16, 0); }
  float *a = pl->m1.data(), *b = pl->m2.data(); bool stereo = in.channel_count > 1 && out.channel_count > 1;
  if (pl->dirty.exchange(false)) { for (int k = 0; k < 2; k++) { pl->set[k] = pl->target(k); pl->ch[k].setTarget(pl->set[k]); pl->ch[k].vin = vinOf(pl->val[P_LEVEL]); } pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->quiet = 0; }
  // Neubeginn (activate, start, reset): Zustände leeren, Regler sofort am Ziel
  if (pl->fresh.exchange(false)) { for (int k = 0; k < 2; k++) { pl->set[k] = pl->target(k); pl->ch[k].vin = vinOf(pl->val[P_LEVEL]); pl->ch[k].c = pl->ch[k].c0; pl->ch[k].setTarget(pl->set[k]); pl->ch[k].snap(); } pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->outCur = -1; pl->quiet = 0; pl->sleeping = false; }
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = in.channel_count > 1 ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = in.channel_count > 1 ? in.data64[1][i] : x; } a[i] = (float)x; b[i] = (float)x2; }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(a[i]), std::fabs(b[i])));
  if (pl->sleeping && inPk >= 1e-6f) { pl->sleeping = false; pl->quiet = 0; }
  if (pl->sleeping) { for (uint32_t c = 0; c < out.channel_count; c++) { if (out.data32) std::memset(out.data32[c], 0, n * sizeof(float)); else if (out.data64) std::memset(out.data64[c], 0, n * sizeof(double)); } return CLAP_PROCESS_CONTINUE; }
  auto t0 = std::chrono::steady_clock::now();
  pl->ch[0].process(a, (int)n); if (stereo) pl->ch[1].process(b, (int)n);
  { double p[2], q[2] = {0, 0}; pl->ch[0].takePeaks(p); if (stereo) pl->ch[1].takePeaks(q); for (int k = 0; k < 2; k++) { double m = std::max(p[k], q[k]); if (m > pl->pk[k].load()) pl->pk[k].store(m); } }
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  double outPk = 0;
  { double g = pl->outCur < 0 ? pl->outg : pl->outCur, gt = pl->outg, k = 1 - std::exp(-1.0 / (0.02 * pl->fs));
    for (uint32_t i = 0; i < n; i++) { g += (gt - g) * k; a[i] = (float)(a[i] * g); if (stereo) b[i] = (float)(b[i] * g); outPk = std::max(outPk, std::max(std::fabs((double)a[i]), stereo ? std::fabs((double)b[i]) : 0.0)); } pl->outCur = g; }
  for (uint32_t c = 0; c < out.channel_count; c++) { const float *src = (c == 1 && stereo) ? b : a; if (out.data32) for (uint32_t i = 0; i < n; i++) out.data32[c][i] = src[i]; else if (out.data64) for (uint32_t i = 0; i < n; i++) out.data64[c][i] = src[i]; }
  // Schlafen: Eingang still (< −120 dBFS) und Ausgang abgeklungen (< −90 dBFS) für 2 s
  if (inPk < 1e-6f && outPk < 3e-5) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)(2 * pl->fs) && !pl->sleeping) { pl->sleeping = true; pl->load = 0; }
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
// Frequenzgang (Kleinsignal, linker Kanal mit seiner BALANCE-Stellung), 0 dB = flach mit BALANCE in der Mitte bei 1 kHz; Bezug: alles flach, ohne Filter
static void curves(Plugin *pl, std::vector<double> &cur, std::vector<double> &ref) { viereck::Setting s = pl->target(0), r; r.balance = s.balance; viereck::Parts v; cur.resize(vrgui::NF); ref.resize(vrgui::NF);
  for (int i = 0; i < vrgui::NF; i++) { double f = vrgui::freqAt(i); cur[i] = 20 * std::log10(std::max(1e-12, std::abs(viereck::response(v, s, f)) / viereck::gain0())); ref[i] = 20 * std::log10(std::max(1e-12, std::abs(viereck::response(v, r, f)) / viereck::gain0())); } }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); TSX_GUARD; if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef __APPLE__
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { pl->gui = new vrgui::Gui(); pl->gui->peaks = [pl](double *o) { for (int k = 0; k < 2; k++) { double p = pl->pk[k].exchange(0); pl->pkHold[k] = std::max(p, pl->pkHold[k] * 0.85); o[k] = pl->pkHold[k]; } }; pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->computeCurves = [pl](std::vector<double> &cur, std::vector<double> &ref) { curves(pl, cur, ref); }; }
  pl->guiH = pl->guiW * vrgui::TOTALH / vrgui::FW;
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
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; upd_diag(pl); }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)vrgui::FW; h->aspect_ratio_height = (uint32_t)vrgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h) { double ww = std::max(700.0, std::min(2600.0, (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * vrgui::TOTALH / vrgui::FW); return true; }
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
static uint32_t g_latency(const clap_plugin_t *) { return 0; }
static const clap_plugin_latency_t EXT_LATENCY = {g_latency};
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
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS) || !std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS_COMPAT)) return &EXT_CAP; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_EQUALIZER, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.viereck-34", "Viereck 34 Tilt (after Quad 34)", "tonestack", "", "", "", TS_VERSION, "Tone section of the Quad 34 preamplifier simulated from the schematic: TILT (7 positions, +/-3 dB, or smooth), BASS step 300/170/100 Hz and lift +3/+6/+9 dB, BALANCE, filters F1/F2 with SLOPE; model: tilt pivot 80 Hz - 4 kHz", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def); 
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST
extern "C" bool viereck_entry_init(const char *p) { return entry_init(p); }
extern "C" void viereck_entry_deinit() { entry_deinit(); }
extern "C" const void *viereck_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
