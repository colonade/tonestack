// Blech 140: Hallplatte nach dem EMT 140 als physikalisches Modell (blech_core.hpp, docs/blech/ENTWURF.md) als CLAP-Plugin (Linux x86-64; auf dem Mac
// über clap-wrapper als VST3/Audio Unit). Eingang mono (bei Stereo die Summe für den Hall, trocken je Kanal), Ausgang stereo (zwei Abnehmer); keine Latenz.
// Neue Impulsantworten (DECAY gedreht) rechnet ein Hilfsfaden (etwa 0,7 s), der Audio-Faden zerlegt sie stückweise und blendet über (wie die
// motorgetriebene Dämpfungsplatte des Geräts: der Hall folgt dem Regler mit Verzögerung, Lampe MOTOR).
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "1.7"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "blech_core.hpp"
#include "platform.hpp"
#include "numtext.hpp"
#include "ports.hpp"
#ifndef TS_NO_GUI
#include "blechgui.hpp"
#else
namespace blechgui { struct Gui; }
#endif
#include <deque>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
#include "x11safe.hpp"
#endif
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
#define TSX_GUARD tsx::Guard xg_
#else
#define TSX_GUARD (void)0
#endif

enum { P_DECAY, P_BASS, P_MIX, P_PRE, P_OUT, P_COUNT };
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_BASS[] = {"0 (off)", "1 (100 Hz)", "2 (200 Hz)", "3 (400 Hz)"};
static const PDef PD[P_COUNT] = { {"Decay (s at 500 Hz)", blech::T500LO, blech::T500HI, 2, 0, 0}, {"Bass cut", 0, 3, 0, N_BASS, 4}, {"Mix (%)", 0, 100, 30, 0, 0},
  {"Pre-delay (ms)", 0, 200, 0, 0, 0}, {"Output (dB)", -24, 24, 0, 0, 0} };
static double t01Of(double t500) { return (t500 - blech::T500LO) / (blech::T500HI - blech::T500LO); }

// Hilfsfaden: rechnet Impulsantworten, sobald DECAY 120 ms ruhig steht; Übergabe über ein Postfach (Puffer gleicher Größe werden nur getauscht)
struct Worker {
  std::thread th; std::mutex m; std::condition_variable cv; std::atomic<bool> quit{false}; blech::IRJob job;
  std::atomic<double> want{2}; std::atomic<int> state{0};   // state 0: Postfach frei, 1: Impulsantwort liegt bereit (Audio-Faden holt ab)
  std::vector<float> boxL, boxR; double boxT = 2; std::atomic<bool> busy{false}; double done = 2;
  void start(double fsi, double t) { job.setup(fsi); boxL.assign(job.L, 0.f); boxR.assign(job.L, 0.f); done = t; want = t; state = 0; quit = false; th = std::thread([this] { loop(); }); }
  void stop() { { std::lock_guard<std::mutex> lk(m); quit = true; } cv.notify_all(); if (th.joinable()) th.join(); }   // bricht eine laufende Rechnung nach höchstens einigen ms ab
  void poke() { cv.notify_all(); }
  void loop() {
    std::unique_lock<std::mutex> lk(m);
    while (!quit) {
      double t = want.load();
      if (std::fabs(t - done) <= 0.005 || state.load() != 0) { cv.wait_for(lk, std::chrono::milliseconds(50)); continue; }
      cv.wait_for(lk, std::chrono::milliseconds(120)); if (quit) break; if (want.load() != t) continue;   // Regler bewegt sich noch
      busy = true; lk.unlock(); job.start(t); while (!quit && !job.work(1L << 20)) {} lk.lock(); bool ok = job.phase == blech::IRJob::DONE; job.phase = blech::IRJob::IDLE; busy = false; if (quit || !ok) break;
      if (state.load() == 0) { std::swap(boxL, job.hL); std::swap(boxR, job.hR); boxT = t; done = t; state.store(1, std::memory_order_release); } } }
};

struct Plugin {
  PortCfg ports;
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; blech::Reverb *rev = nullptr; Worker *wk = nullptr; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true}, fresh{true}, active{false};
  std::atomic<double> load{0}; std::vector<float> a, b, oa, ob; long quiet = 0; std::atomic<bool> sleeping{false}; blech::Setting set;
  std::atomic<double> shown{2}; std::atomic<bool> motor{false};   // für die Oberfläche: Nachhallzeit der eingebauten Platte, Dämpfungsplatte fährt
  blechgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; double guiW = 1400, guiH = 0;
#ifdef _WIN32
  double guiScale = 1;   // Windows: Anzeigeskalierung des Hosts (set_scale); Fenstergrößen sind dort Gerätepixel
#endif
  std::string diagMode, diagTxt; int diagCnt = 0;
  blech::Setting target() const { blech::Setting s; s.decay = t01Of(val[P_DECAY].load()); s.mix = val[P_MIX].load() / 100; s.out = (val[P_OUT].load() + 24) / 48; s.pre = std::sqrt(std::max(0.0, val[P_PRE].load()) / 200); s.bass = val[P_BASS].load(); return s; }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "Blech 140"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); return true; }
  switch (id) { case P_DECAY: { blech::Plate pl; std::snprintf(buf, n, "%.2f s at 500 Hz (%.1f s at 125 Hz, %.2f s at 4 kHz)", v, blech::t60At(pl, 125, v), blech::t60At(pl, 4000, v)); break; }
    case P_MIX: std::snprintf(buf, n, "%.1f %%", v); break; case P_PRE: std::snprintf(buf, n, "%.0f ms", v); break; default: std::snprintf(buf, n, "%+.1f dB", v); }
  return true; }
static bool par_t2v(const clap_plugin_t *, clap_id id, const char *txt, double *out) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { for (int k = 0; k < d.nn; k++) if (std::strncmp(txt, d.names[k], std::strlen(d.names[k])) == 0) { *out = k; return true; } }
  *out = tsnum::parse(txt); return std::isfinite(*out); }
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
static void par_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) { Plugin *pl = P(p); apply_events(pl, in); emit_gui(pl, out); if (pl->wk) { pl->wk->want = pl->val[P_DECAY].load(); pl->wk->poke(); } }
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
  tsnum::fixDot(buf, len); int done = 0; while (done < len) { int64_t w = s->write(s, buf + done, len - done); if (w <= 0) return false; done += (int)w; } return true; }
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); if (all.size() > 65536) return false; }
  bool seen[P_COUNT] = {};
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = tsnum::parse(line.substr(q + 1).c_str()); if (id >= 0 && id < P_COUNT && std::isfinite(v)) { P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); seen[id] = true; } }
  for (int i = 0; i < P_COUNT; i++) if (!seen[i]) P(p)->val[i].store(PD[i].def);
  Plugin *pl = P(p); pl->dirty = true; if (pl->wk) { pl->wk->want = pl->val[P_DECAY].load(); pl->wk->poke(); } return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
static bool pl_init(const clap_plugin_t *) { return true; }
static void stopAll(Plugin *pl) { if (pl->wk) { pl->wk->stop(); delete pl->wk; pl->wk = nullptr; } delete pl->rev; pl->rev = nullptr; }
static void pl_destroy(const clap_plugin_t *p) { Plugin *pl = P(p);
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
  { auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; } }
#endif
#ifndef TS_NO_GUI
  { TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; }
#ifndef TS_OWNVIEW
  tsx::close(pl->dpy); pl->dpy = nullptr;
#endif
  }
#endif
  stopAll(pl); delete pl; }
// Hauptfaden: Moden, alle Puffer und die erste Impulsantwort (etwa 0,7 s; über 88 kHz rechnet der Hall bei fs/2 bzw. fs/4), dann der Hilfsfaden
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->fs = sr; stopAll(pl);
  try { double t = pl->val[P_DECAY].load(); pl->rev = new blech::Reverb(); pl->rev->internalJob = false; pl->rev->setup(sr, t01Of(t)); pl->wk = new Worker(); pl->wk->start(pl->rev->fsi, t); pl->shown = t; }
  catch (...) { stopAll(pl); return false; }
  for (auto *v : {&pl->a, &pl->b, &pl->oa, &pl->ob}) v->assign(maxf + 16, 0); pl->fresh = true; pl->active = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { Plugin *pl = P(p); pl->active = false; stopAll(pl); }
static bool pl_start(const clap_plugin_t *p) { P(p)->fresh = true; return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { P(p)->fresh = true; }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  uint32_t n = pr->frames_count; if (!pl->active || !pl->rev || !pl->wk || pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  if (n == 0) return CLAP_PROCESS_CONTINUE;
  blech::Reverb &rv = *pl->rev; Worker &wk = *pl->wk;
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->a.size() < n) for (auto *v : {&pl->a, &pl->b, &pl->oa, &pl->ob}) v->assign(n + 16, 0);
  float *a = pl->a.data(), *b = pl->b.data(), *oa = pl->oa.data(), *ob = pl->ob.data(); bool stIn = in.channel_count > 1;
  if (pl->dirty.exchange(false)) { pl->set = pl->target(); pl->quiet = 0; wk.want.store(pl->val[P_DECAY].load()); }
  if (pl->fresh.exchange(false)) { pl->set = pl->target(); rv.reset(); pl->quiet = 0; pl->sleeping = false; }   // Neubeginn (activate, start, reset): Hall leeren
  if (wk.state.load(std::memory_order_acquire) == 1) { rv.offer(wk.boxL, wk.boxR, wk.boxT); wk.state.store(0, std::memory_order_release); }   // Puffer getauscht, nichts freigegeben
  const double tgt = pl->val[P_DECAY].load(); const bool moving = rv.changing() || wk.busy.load() || std::fabs(tgt - rv.t500Act) > 0.005;
  pl->motor = moving;
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = stIn ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = stIn ? in.data64[1][i] : x; } a[i] = (float)x; b[i] = (float)x2; }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(a[i]), std::fabs(b[i])));
  if (pl->sleeping && (inPk >= 1e-6f || rv.changing())) { pl->sleeping = false; pl->quiet = 0; }   // auch zum Einbauen einer neuen Platte aufwachen
  if (pl->sleeping) { pl->shown = rv.decayShown(); zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  auto t0 = std::chrono::steady_clock::now();
  rv.process(a, stIn ? b : nullptr, oa, ob, (int)n, pl->set);
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  pl->shown = rv.decayShown();
  double outPk = 0; for (uint32_t i = 0; i < n; i++) outPk = std::max(outPk, std::max(std::fabs((double)oa[i]), std::fabs((double)ob[i])));
  for (uint32_t c = 0; c < out.channel_count; c++) {   // Ausgang mono: Mittel beider Abnehmer
    for (uint32_t i = 0; i < n; i++) { float y = out.channel_count == 1 ? 0.5f * (oa[i] + ob[i]) : c == 0 ? oa[i] : c == 1 ? ob[i] : 0.f; if (out.data32) out.data32[c][i] = y; else if (out.data64) out.data64[c][i] = y; } }
  // Schlafen: Eingang still (< −120 dBFS) und Hall abgeklungen (< −90 dBFS) für 2 s, keine Platte im Umbau
  if (inPk < 1e-6f && outPk < 3e-5 && !rv.changing()) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)(2 * pl->fs) && !pl->sleeping) { pl->sleeping = true; pl->load = 0; rv.reset(); }
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
  if (id == P_DECAY && pl->wk) { pl->wk->want = v; pl->wk->poke(); }
  { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static void gui_gesture(Plugin *pl, int id, bool begin) { if (id < 0 || id >= P_COUNT) return;
  { std::lock_guard<std::mutex> lk(pl->qm); if (begin) { if (pl->gestId >= 0) pl->guiQ.push_back({pl->gestId, 0, 2}); pl->gestId = id; pl->guiQ.push_back({id, 0, 1}); } else if (pl->gestId == id) { pl->gestId = -1; pl->guiQ.push_back({id, 0, 2}); } else return; }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static bool g_supported(const clap_plugin_t *, const char *api, bool floating) { return !std::strcmp(api, TS_WINAPI) && !floating; }
static bool g_preferred(const clap_plugin_t *, const char **api, bool *floating) { *api = TS_WINAPI; *floating = false; return true; }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); TSX_GUARD; if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef TS_OWNVIEW
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { pl->gui = new blechgui::Gui(); pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->needle = pl->gui->shown = pl->shown.load(); }
  pl->guiH = pl->guiW * blechgui::TOTALH / blechgui::FW;
  }
#ifdef TS_OWNVIEW
  { std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) pl->gui->onTick = [pl]() { gui_tick(pl); }; }
#else
  auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId == CLAP_INVALID_ID) ht->register_timer(pl->host, 33, &pl->timerId);
#endif
  return true; }
static void g_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); TSX_GUARD; if (pl->gestId >= 0) gui_gesture(pl, pl->gestId, false); auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; }
  std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; }
#ifndef TS_OWNVIEW
  if (pl->dpy) { XSync(pl->dpy, False); }
#endif
  }
static void upd_diag(Plugin *pl) { char b[160]; std::snprintf(b, sizeof b, "CPU %.1f %% of one core · %.0f Hz%s", pl->load.load() * 100, pl->fs, pl->sleeping ? " · idle (silent input)" : "");
  { const char *m = std::strstr(b, " · "); std::string mode = m ? m : ""; if (++pl->diagCnt < 15 && mode == pl->diagMode && !pl->diagTxt.empty()) return; pl->diagCnt = 0; pl->diagMode = mode; pl->diagTxt = b; } }
// Zeiger mit Trägheit (wie ein Drehspulinstrument, etwa 0,15 s), Lampe aus dem Audio-Faden; ohne laufendes Audio (inaktiv) zeigt der Zeiger den Regler
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; upd_diag(pl);
  blechgui::Gui &g = *pl->gui; g.shown = pl->active ? pl->shown.load() : pl->val[P_DECAY].load(); g.motor = pl->active && pl->motor.load();
  double d = g.shown - g.needle; g.needle = std::fabs(d) < 0.002 ? g.shown : g.needle + 0.2 * d; }
#ifdef _WIN32   // bei 150 % Anzeigeskalierung startet die Oberfläche 1,5-mal so groß (CLAP: Größen unter Windows in Gerätepixeln)
static bool g_set_scale(const clap_plugin_t *p, double s) { Plugin *pl = P(p); if (!(s >= 0.5 && s <= 4)) return false; double k = s / pl->guiScale; if (k == 1) return true; pl->guiScale = s; pl->guiW *= k; pl->guiH *= k;
  // Fenster schon offen (z. B. auf einen Monitor mit anderer Skalierung gezogen): neue Größe beim Host anfordern, er antwortet mit set_size
  bool open; { std::lock_guard<std::recursive_mutex> glk(pl->gm); open = pl->gui && pl->gui->win; }
  if (open) { auto *hg = (const clap_host_gui_t*)pl->host->get_extension(pl->host, CLAP_EXT_GUI); if (hg && hg->request_resize) hg->request_resize(pl->host, (uint32_t)std::lround(pl->guiW), (uint32_t)std::lround(pl->guiH)); }
  return true; }
static double gscale(const clap_plugin_t *p) { return P(p)->guiScale; }   // Größengrenzen in Gerätepixeln wachsen mit der Skalierung
#else
static double gscale(const clap_plugin_t *) { return 1; }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
#endif
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)blechgui::FW; h->aspect_ratio_height = (uint32_t)blechgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { double ww = std::max(700.0 * gscale(p), std::min(2600.0 * gscale(p), (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * blechgui::TOTALH / blechgui::FW); return true; }
static bool g_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); pl->guiW = w; pl->guiH = h; if (pl->gui && pl->gui->win) { pl->gui->resize((int)w, (int)h); pl->gui->draw(); } return true; }
static bool g_set_parent(const clap_plugin_t *p, const clap_window_t *win) { Plugin *pl = P(p); TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; if (pl->gui->win) pl->gui->destroy();
#ifdef TS_OWNVIEW
  return pl->gui->attach(win->ptr, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
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
// Hall klingt nach: Impulsantwort 10 s (ab 9 s ausgeblendet) + Vorverzögerung (CLAP tail)
static uint32_t g_tail(const clap_plugin_t *p) { Plugin *pl = P(p); return (uint32_t)std::lround((10.0 + pl->val[P_PRE].load() * 1e-3 + 0.1) * pl->fs); }
static const clap_plugin_tail_t EXT_TAIL = {g_tail};
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
static void on_timer(const clap_plugin_t *p, clap_id) { Plugin *pl = P(p); TSX_GUARD; std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock()) return;
  try { if (pl->gui) { gui_tick(pl); pl->gui->pump(); } } catch (...) {} }
static const clap_plugin_timer_support_t EXT_TIMER = {on_timer};
#endif

static const void *pl_ext(const clap_plugin_t *, const char *id) {
#ifndef TS_NO_GUI
  if (!std::strcmp(id, CLAP_EXT_GUI)) return &EXT_GUI;
#endif
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
  if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &EXT_TIMER;
#endif
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_TAIL)) return &EXT_TAIL; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS) || !std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS_COMPAT)) return &EXT_CAP; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_REVERB, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.blech-140", "Blech 140 (after EMT 140 plate reverb)", "tonestack", "", "", "", TS_VERSION, "Plate reverb after the EMT 140 as a physical model: tensioned steel plate 2 m x 1 m x 0.5 mm, about 23 000 modes up to 18 kHz, thermoelastic and high-frequency loss, damper plate (decay 1 to 5 s at 500 Hz), bass cut 0 to 3, two pickups (stereo); mix, pre-delay and output added; mono in, stereo out, zero latency", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def);
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST
extern "C" bool blech_entry_init(const char *p) { return entry_init(p); }
extern "C" void blech_entry_deinit() { entry_deinit(); }
extern "C" const void *blech_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
