// Cultec EQ-1A: Nachbau des Pultec EQP-1A (passives Netz, HS-56, HS-29, 12AX7/12AU7 im Gegentakt, S-217-D mit Gegenkopplung) als Schaltung im Zeitbereich
// (engine/src/cultec_core.hpp, docs/cultec/SCHALTPLAN.md) als CLAP-Plugin (Linux, Windows; auf dem Mac über clap-wrapper als VST3/Audio Unit).
// Ein Regler-Satz für beide Kanäle; MODEL wählt ab 1.1 zwischen EQP-1A und EQP-1R (eigene Wahlschalter-Parameter, Neubau des Paars wie bei OVERSAMPLING). Modell: LEVEL (0 dBFS = x dBu an der Eingangsklemme), OVERSAMPLING (intern höchstens etwa 200 kHz), OUTPUT.
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "1.5"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "cultec_core.hpp"
#include "platform.hpp"
#include "numtext.hpp"
#include "ports.hpp"
#ifndef TS_NO_GUI
#include "cultecgui.hpp"
#else
namespace ctgui { struct Gui; }
#endif
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
#include "x11safe.hpp"
#endif
#if !defined(TS_NO_GUI) && !defined(TS_OWNVIEW)
#define TSX_GUARD tsx::Guard xg_
#else
#define TSX_GUARD (void)0
#endif

enum { P_LB, P_LA, P_LF, P_HB, P_BW, P_HF, P_HC, P_HCS, P_IN, P_LEVEL, P_OS, P_OUT, P_MODEL, P_LFR, P_HFR, P_COUNT };   // ab 1.1 angehängt: Modell, Wahlschalter des EQP-1R
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_LF[] = {"20 Hz", "30 Hz", "60 Hz", "100 Hz"};
static const char *N_HF[] = {"3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz"};
static const char *N_HC[] = {"5 kHz", "10 kHz", "20 kHz"};
static const char *N_IN[] = {"Out", "In"};
static const char *N_OS[] = {"1x", "2x", "4x"};
static const char *N_MODEL[] = {"EQP-1A", "EQP-1R"};
static const char *N_LFR[] = {"30 Hz", "60 Hz", "100 Hz"};
static const char *N_HFR[] = {"3 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz"};
static const PDef PD[P_COUNT] = {
  {"Low frequency boost", 0, 10, 0, 0, 0}, {"Low frequency atten", 0, 10, 0, 0, 0}, {"Low frequency (cps)", 0, 3, 2, N_LF, 4},
  {"High frequency boost", 0, 10, 0, 0, 0}, {"Bandwidth (0 sharp - 10 broad)", 0, 10, 5, 0, 0}, {"High frequency (kcs)", 0, 6, 2, N_HF, 7},
  {"High frequency atten", 0, 10, 0, 0, 0}, {"Atten select (kcs)", 0, 2, 1, N_HC, 3}, {"Equalizer in", 0, 1, 1, N_IN, 2},
  {"Level: 0 dBFS = dBu at the input", 0, 30, 18, 0, 0}, {"Oversampling", 0, 2, 2, N_OS, 3}, {"Output level (dB)", -24, 24, 0, 0, 0},
  {"Model", 0, 1, 0, N_MODEL, 2}, {"EQP-1R low frequency (cps)", 0, 2, 1, N_LFR, 3}, {"EQP-1R high frequency (kcs)", 0, 4, 1, N_HFR, 5} };
static int osOf(double v) { int k = (int)std::lround(v); return k <= 0 ? 1 : k == 1 ? 2 : 4; }

// Zwei Kanäle; beim Umschalten der Überabtastung oder des Modells baut der Hauptfaden ein neues Paar (eingeschwungen) und der Audiofaden übernimmt es am Blockanfang.
struct Pair { cultec::Unit ch[2]; int os = 0, model = 0; bool linked = true; };   // linked: beide Kanäle gleich (gleiche Eingänge seit dem Start) → nur einer wird gerechnet
struct Plugin {
  PortCfg ports;
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true}, fresh{true}, active{false};
  std::atomic<Pair*> cur{nullptr}, pend{nullptr}, old{nullptr}; std::atomic<bool> wantOs{false}; std::mutex bm;   // bm: Bau eines neuen Paars (Hauptfaden)
  Pair *fadeFrom = nullptr; long fadeN = 0, fadeLen = 1, reqAge = 0;   // Audiofaden: beim Wechsel läuft das alte Paar weiter, bis das neue nach seiner Latenz (LAT) Ton gibt, dann 30 ms Überblendung
  std::atomic<double> load{0}; std::vector<float> m1, m2, m3, m4; double outg = 1, outCur = -1; long quiet = 0; std::atomic<bool> sleeping{false}; cultec::Setting set;
  std::atomic<double> pk[3]; double pkHold[3] = {0, 0, 0};   // Ausgang (V), 12AU7, 12AX7
  ctgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; double guiW = 1200, guiH = 0;   // 1200 × 765 (Frontplatte höher als bei den anderen)
#ifdef _WIN32
  double guiScale = 1;   // Windows: Anzeigeskalierung des Hosts (set_scale); Fenstergrößen sind dort Gerätepixel
#endif
  std::string diagMode, diagTxt; int diagCnt = 0;
  int model() const { return val[P_MODEL] > 0.5 ? 1 : 0; }
  // Einstellung für ein Modell: Wahlschalter-Stellungen aus den Parametern dieses Modells (EQP-1R: eigene CPS/KCS, Höhen-Atten fest)
  cultec::Setting target(int m) const { cultec::Setting s; s.lb = val[P_LB] / 10; s.la = val[P_LA] / 10; s.hb = val[P_HB] / 10; s.bw = val[P_BW] / 10; s.hc = val[P_HC] / 10;
    if (m == 1) { s.lf = (int)std::lround(val[P_LFR]); s.hf = (int)std::lround(val[P_HFR]); s.hcf = 0; }
    else { s.lf = (int)std::lround(val[P_LF]); s.hf = (int)std::lround(val[P_HF]); s.hcf = (int)std::lround(val[P_HCS]); }
    s.in = val[P_IN] > 0.5; return s; }
  double vin() const { return cultec::dbuToVin(val[P_LEVEL]); }
  Pair *build(int os, int m) { Pair *p = new Pair(); p->os = os; p->model = m; cultec::Setting s = target(m); p->ch[0].vin = vin(); p->ch[0].init(fs, os, cultec::partsFor(m), s); p->ch[1] = p->ch[0]; return p; }   // einmal einschwingen, rechts als Kopie
  ~Plugin() { delete cur.exchange(nullptr); delete pend.exchange(nullptr); delete old.exchange(nullptr); delete fadeFrom; }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }
// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", i == P_LFR || i == P_HFR ? "Cultec EQ-1A/EQP-1R" : "Cultec EQ-1A"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); }
  else if (id == P_OUT) std::snprintf(buf, n, "%+.1f dB", v);
  else if (id == P_LEVEL) { double r = tsnum::rnd(v, 0); std::snprintf(buf, n, "%+.0f dBu (0 dBFS = %.2f V peak)", r, cultec::dbuToVin(r)); }   // Spannung aus dem gezeigten dBu-Wert
  else std::snprintf(buf, n, "%.2f", v);
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
  tsnum::fixDot(buf, len); int done = 0; while (done < len) { int64_t w = s->write(s, buf + done, len - done); if (w <= 0) return false; done += (int)w; } return true; }
// Nach dem Laden des Zustands (ab 2026-11-27): dem Host melden, dass sich Parameterwerte geändert haben (CLAP_PARAM_RESCAN_VALUES) – sonst zeigt er
// bis zur nächsten Änderung die alten Werte (clap-validator state-reproducibility-*); clap-wrapper gibt das an VST3 als kParamValuesChanged weiter
static void rescanValues(Plugin *pl) { auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp && hp->rescan) hp->rescan(pl->host, CLAP_PARAM_RESCAN_VALUES); }
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); if (all.size() > 65536) return false; }
  if (all.empty()) return false;   // leerer Zustand (ab 2026-11-27): nichts laden, Fehler melden (clap-validator state-invalid-empty)
  bool seen[P_COUNT] = {};
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = tsnum::parse(line.substr(q + 1).c_str()); if (id >= 0 && id < P_COUNT && std::isfinite(v)) { P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); seen[id] = true; } }
  for (int i = 0; i < P_COUNT; i++) if (!seen[i]) P(p)->val[i].store(PD[i].def);
  P(p)->dirty = true; rescanValues(P(p)); return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
static bool pl_init(const clap_plugin_t *) { return true; }
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
  { std::lock_guard<std::mutex> lk(pl->bm); }   // ein laufender Bau (Hauptfaden) ist fertig
  delete pl; }
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); std::lock_guard<std::mutex> lk(pl->bm); pl->fs = sr;
  delete pl->cur.exchange(pl->build(osOf(pl->val[P_OS]), pl->model())); delete pl->pend.exchange(nullptr); delete pl->old.exchange(nullptr); delete pl->fadeFrom; pl->fadeFrom = nullptr; pl->wantOs = false;
  pl->m1.assign(maxf + 16, 0); pl->m2.assign(maxf + 16, 0); pl->m3.assign(maxf + 16, 0); pl->m4.assign(maxf + 16, 0); pl->fresh = true; pl->active = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { P(p)->active = false; }
static bool pl_start(const clap_plugin_t *p) { P(p)->fresh = true; return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { P(p)->fresh = true; }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  uint32_t n = pr->frames_count; if (!pl->active || !pl->cur.load() || pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  if (n == 0) return CLAP_PROCESS_CONTINUE;
  // Neues Paar (andere Überabtastung) übernehmen; das alte räumt der Hauptfaden weg
  // Das alte läuft weiter (Latenz des neuen, dann 30 ms) und wird ausgeblendet, das neue eingeblendet (kein Knacks beim Modell- oder Überabtastungswechsel)
  if (!pl->old.load() && !pl->fadeFrom) { Pair *np = pl->pend.exchange(nullptr); if (np) { pl->fadeFrom = pl->cur.exchange(np); pl->fadeLen = std::max(1L, (long)(0.03 * pl->fs)); pl->fadeN = pl->fadeLen + cultec::Unit::LAT; pl->dirty = true; pl->fresh = true; } }
  Pair &pr2 = *pl->cur.load();
  // Neubau anfordern; ging die Anforderung verloren (Host ruft on_main_thread nicht), nach 0,5 s erneut
  { const bool need = osOf(pl->val[P_OS]) != pr2.os || pl->model() != pr2.model;
    if (need && !pl->wantOs.exchange(true)) { pl->reqAge = 0; pl->host->request_callback(pl->host); }
    else if ((need || pl->old.load()) && (pl->reqAge += n) > (long)(pl->fs / 2)) { pl->reqAge = 0; pl->wantOs = need; pl->host->request_callback(pl->host); } }
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->m1.size() < n || pl->m3.size() < n) { pl->m1.assign(n + 16, 0); pl->m2.assign(n + 16, 0); pl->m3.assign(n + 16, 0); pl->m4.assign(n + 16, 0); }
  float *a = pl->m1.data(), *b = pl->m2.data(); bool stereo = in.channel_count > 1 && out.channel_count > 1;
  if (pl->dirty.exchange(false)) { pl->set = pl->target(pr2.model); for (int k = 0; k < 2; k++) { pr2.ch[k].setTarget(pl->set); pr2.ch[k].vin = pl->vin(); } pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->quiet = 0; }
  // Neubeginn (activate, start, reset): Zustände aus der Ruhe, Regler sofort am Ziel
  if (pl->fresh.exchange(false)) { pl->set = pl->target(pr2.model); for (int k = 0; k < 2; k++) { pr2.ch[k].vin = pl->vin(); pr2.ch[k].c = pr2.ch[k].c0; pr2.ch[k].setTarget(pl->set); pr2.ch[k].snap(); } pr2.linked = true; pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->outCur = -1; pl->quiet = 0; pl->sleeping = false; }
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = in.channel_count > 1 ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = in.channel_count > 1 ? in.data64[1][i] : x; } a[i] = (float)x; b[i] = (float)x2; }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(a[i]), std::fabs(b[i])));
  if (pl->sleeping && inPk >= 1e-6f) { pl->sleeping = false; pl->quiet = 0; }
  if (pl->sleeping && pl->fadeFrom) { pl->old.store(pl->fadeFrom); pl->fadeFrom = nullptr; pl->host->request_callback(pl->host); }
  if (pl->sleeping) { for (uint32_t c = 0; c < out.channel_count; c++) { if (out.data32) std::memset(out.data32[c], 0, n * sizeof(float)); else if (out.data64) std::memset(out.data64[c], 0, n * sizeof(double)); } return CLAP_PROCESS_CONTINUE; }
  auto t0 = std::chrono::steady_clock::now();
  bool same = !stereo || std::memcmp(a, b, n * sizeof(float)) == 0;
  auto runPair = [&](Pair &q, float *l, float *r) {
    if (stereo && q.linked && !same) { q.ch[1] = q.ch[0]; q.linked = false; }   // bis hierher waren beide gleich: rechten Kanal vom linken übernehmen
    q.ch[0].process(l, (int)n); if (stereo) { if (q.linked) std::memcpy(r, l, n * sizeof(float)); else q.ch[1].process(r, (int)n); }
    double q0[3], q1[3] = {0, 0, 0}; q.ch[0].takePeaks(q0); if (stereo && !q.linked) q.ch[1].takePeaks(q1); for (int k = 0; k < 3; k++) { double m = std::max(q0[k], q1[k]); if (m > pl->pk[k].load()) pl->pk[k].store(m); } };
  float *fa = pl->m3.data(), *fb = pl->m4.data(); if (pl->fadeFrom) { std::memcpy(fa, a, n * sizeof(float)); std::memcpy(fb, b, n * sizeof(float)); }
  runPair(pr2, a, b);
  if (pl->fadeFrom) { runPair(*pl->fadeFrom, fa, fb);
    for (uint32_t i = 0; i < n; i++) { double w = pl->fadeN >= pl->fadeLen ? 1 : pl->fadeN > 0 ? std::sin(0.5 * cultec::PI * (double)pl->fadeN / pl->fadeLen) : 0, u = std::sqrt(std::max(0.0, 1 - w * w)); if (pl->fadeN > 0) pl->fadeN--;
      a[i] = (float)(a[i] * u + fa[i] * w); b[i] = (float)(b[i] * u + fb[i] * w); }
    if (pl->fadeN <= 0) { pl->old.store(pl->fadeFrom); pl->fadeFrom = nullptr; pl->host->request_callback(pl->host); } }
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
// Hauptfaden: altes Paar freigeben; bei geänderter Überabtastung oder geändertem Modell ein neues bauen (eingeschwungen, etwa 0,2 s) und dem Audiofaden übergeben
static void pl_main(const clap_plugin_t *p) { Plugin *pl = P(p); DenormGuard dg;
  try { std::lock_guard<std::mutex> lk(pl->bm); delete pl->old.exchange(nullptr);
    if (pl->wantOs.exchange(false) && pl->active && !pl->pend.load()) { int os = osOf(pl->val[P_OS]), m = pl->model(); Pair *c = pl->cur.load(); if (!c || c->os != os || c->model != m) pl->pend.store(pl->build(os, m)); }
  } catch (...) {} }
#ifndef TS_NO_GUI
static void gui_tick(Plugin *pl);
static void gui_setp(Plugin *pl, int id, double v) { if (!std::isfinite(v)) return; v = std::max(PD[id].lo, std::min(PD[id].hi, v)); if (pl->val[id].load() == v) return; pl->val[id].store(v); pl->dirty = true;
  { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static void gui_gesture(Plugin *pl, int id, bool begin) { if (id < 0 || id >= P_COUNT) return;
  { std::lock_guard<std::mutex> lk(pl->qm); if (begin) { if (pl->gestId >= 0) pl->guiQ.push_back({pl->gestId, 0, 2}); pl->gestId = id; pl->guiQ.push_back({id, 0, 1}); } else if (pl->gestId == id) { pl->gestId = -1; pl->guiQ.push_back({id, 0, 2}); } else return; }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static bool g_supported(const clap_plugin_t *, const char *api, bool floating) { return !std::strcmp(api, TS_WINAPI) && !floating; }
static bool g_preferred(const clap_plugin_t *, const char **api, bool *floating) { *api = TS_WINAPI; *floating = false; return true; }
// Frequenzgang (Kleinsignal im Arbeitspunkt, mit Übertragern und Verstärker), 0 dB = alle Regler 0 bei 1 kHz. Eigener Kern, einmal eingeschwungen (Hauptfaden, etwa 30 ms).
static cultec::Core &plotCore(int m) { static cultec::Core c[2] = {[] { cultec::Core k; k.warp = false; k.init(96000, cultec::partsFor(0), cultec::Setting()); k.settle(0.2); return k; }(),
  [] { cultec::Core k; k.warp = false; k.init(96000, cultec::partsFor(1), cultec::Setting()); k.settle(0.2); return k; }()}; return c[m ? 1 : 0]; }
static void curves(Plugin *pl, std::vector<double> &cur, std::vector<double> &ref) { const int m = pl->model(); cultec::Core c = plotCore(m); cur.resize(ctgui::NF); ref.resize(ctgui::NF);
  c.cur = cultec::Setting(); double r0 = std::abs(c.response(1000)); for (int i = 0; i < ctgui::NF; i++) ref[i] = 20 * std::log10(std::max(1e-12, std::abs(c.response(ctgui::freqAt(i))) / r0));
  c.cur = pl->target(m); for (int i = 0; i < ctgui::NF; i++) cur[i] = 20 * std::log10(std::max(1e-12, std::abs(c.response(ctgui::freqAt(i))) / r0)); }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); TSX_GUARD; if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef TS_OWNVIEW
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { plotCore(0); pl->gui = new ctgui::Gui(); pl->gui->peaks = [pl](double *o) { for (int k = 0; k < 3; k++) { double p = pl->pk[k].exchange(0); pl->pkHold[k] = std::max(p, pl->pkHold[k] * 0.85); o[k] = pl->pkHold[k]; } }; pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->computeCurves = [pl](std::vector<double> &cur, std::vector<double> &ref) { curves(pl, cur, ref); }; }
  pl->guiH = pl->guiW * ctgui::TOTALH / ctgui::FW;
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
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; upd_diag(pl); }
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
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)ctgui::FW; h->aspect_ratio_height = (uint32_t)ctgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { double ww = std::max(700.0 * gscale(p), std::min(2600.0 * gscale(p), (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * ctgui::TOTALH / ctgui::FW); return true; }
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
static uint32_t g_latency(const clap_plugin_t *) { return cultec::Unit::LAT; }   // Überabtastfilter (bei 1x als Verzögerung), fest für alle Stellungen
static const clap_plugin_latency_t EXT_LATENCY = {g_latency};
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
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS) || !std::strcmp(id, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS_COMPAT)) return &EXT_CAP; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_EQUALIZER, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.cultec-eq1a", "Cultec EQ-1A Program Equalizer (after Pultec EQP-1A)", "tonestack", "", "", "", TS_VERSION, "Passive program equalizer with tube make-up amplifier simulated from the schematic of the Pultec EQP-1A: low boost and atten at 20/30/60/100 Hz, high boost 3-16 kHz with bandwidth, high atten 5/10/20 kHz, input, interstage and output transformers, 12AX7/12AU7 push-pull with feedback; second model after the EQP-1R (30/60/100 Hz, 3-12 kHz, fixed 10 kHz atten, 12AY7)", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def); 
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST
extern "C" bool cultec_entry_init(const char *p) { return entry_init(p); }
extern "C" void cultec_entry_deinit() { entry_deinit(); }
extern "C" const void *cultec_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
