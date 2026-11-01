// Laney AOR 50 Vorverstärker als CLAP-Plugin (Linux x86-64; auf dem Mac über clap-wrapper als VST3/Audio Unit)
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "3.18"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "core.hpp"
#include "platform.hpp"
#ifndef TS_NO_GUI
#include "gui.hpp"
#else
namespace aorgui { struct Gui; }
#endif
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
#include "x11safe.hpp"
#endif
using namespace aor;

enum { P_INP, P_CH, P_V1, P_V2, P_PL, P_T, P_M, P_B, P_MV, P_MID, P_TOP, P_BASS, P_UMBAU, P_PA, P_TAPER, P_OUT, P_NL, P_VIN, P_OS, P_PRES, P_TUBE1, P_TUBE2, P_TUBE3, P_SPK, P_SOFT, P_PAD, P_COUNT };
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_INP[] = {"HIGH (~1 mV)", "LOW (~30 mV)"}; static const char *N_CH[] = {"Preamp 1 (high gain)", "Preamp 2"};
static const char *N_ONOFF[] = {"Off", "On"}; static const char *N_UMB[] = {"Original", "Bass mod C2 C3 C14"};
static const char *N_TAP[] = {"Log 15 %", "Log 10 %", "Linear"}; static const char *N_OS[] = {"1x", "2x", "4x", "8x"};
static const char *N_TUBE[] = {"12AX7 / ECC83 (stock)", "12AT7 / ECC81", "12AU7 / ECC82", "5751 (fitted model)", "12AY7 (fitted model)"};
static const PDef PD[P_COUNT] = {
  {"Input jack", 0, 1, 0, N_INP, 2}, {"Channel", 0, 1, 0, N_CH, 2},
  {"Preamp 1 Volume", 0, 10, 2, 0, 0}, {"Preamp 2 Volume", 0, 10, 2, 0, 0}, {"Preamp 1 Level (Preamp 1 only)", 0, 10, 2, 0, 0},
  {"Treble", 0, 10, 5, 0, 0}, {"Middle", 0, 10, 5, 0, 0}, {"Bass", 0, 10, 5, 0, 0}, {"Master Volume", 0, 10, 5, 0, 0},
  {"Mid Boost (S105)", 0, 1, 1, N_ONOFF, 2}, {"Top Boost (S104)", 0, 1, 1, N_ONOFF, 2}, {"Bass Boost (pull switch)", 0, 1, 1, N_ONOFF, 2},
  {"Bass mod", 0, 1, 1, N_UMB, 2}, {"FX loop + power amp (Presence)", 0, 1, 1, N_ONOFF, 2}, {"Middle/Presence pot taper", 0, 2, 0, N_TAP, 3},
  {"Output level (dB)", -36, 24, 0, 0, 0}, {"Tube distortion", 0, 1, 1, N_ONOFF, 2}, {"Instrument level at 0 dBFS (mV peak)", 1, 1000, 100, 0, 0},
  {"Oversampling (distortion)", 0, 3, 1, N_OS, 4}, {"Presence", 0, 10, 5, 0, 0},
  {"Tube V101", 0, 4, 3, N_TUBE, 5}, {"Tube V102", 0, 4, 3, N_TUBE, 5}, {"Tube V103", 0, 4, 3, N_TUBE, 5},
  {"Speaker (15\" CB158, Portaflex-size vented box)", 0, 1, 0, N_ONOFF, 2},
  {"Soft (mod: C3 at V101B removed)", 0, 1, 1, N_ONOFF, 2}, {"Late (mod: 2.2 MOhm before Preamp 2 Volume)", 0, 1, 0, N_ONOFF, 2} };

static const uint32_t LATENCY = 31;   // feste Latenz: Überabtastungsfilter 31 Abtastwerte; linear/1-fach wird im Kern um 31 verzögert, damit sie nicht mit dem Modus wechselt
struct Plugin {
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; Engine *eng = nullptr, *eng2 = nullptr; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true};   // val: von Oberfläche und Audio-Thread gelesen/geschrieben
  std::atomic<double> load{0}; std::atomic<double> pk[5] = {}; int pkCnt = 0; std::vector<float> mono2; bool eng2Has = false; Target lastT; bool monoOK = true, eng2Stale = true;   // monoOK: beide Kanäle gleich und Kerne gleichauf → einmal rechnen; eng2Stale: zweite Engine veraltet, vor Gebrauch vom linken klonen   // eng2Has: zweite Engine (rechter Kanal) hat das aktuelle Ziel
  long quiet = 0; std::atomic<bool> sleeping{false};   // Stille am Eingang und abgeklungener Ausgang: Host darf die Verarbeitung aussetzen
  std::vector<float> mono; double outg = 1, outCur = -1;   /* outCur: geglättete Ausgangsverstärkung (20 ms), −1 = noch nicht gesetzt */
  aorgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; /* gm: Oberfläche/X-Verbindung, Host-Aufrufe und Timer nie gleichzeitig (VST3: Timer evtl. aus Hilfs-Thread) */ struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; /* kind: 0 Wert, 1 Gestenbeginn, 2 Gestenende; gestId: Parameter, der gerade gezogen wird */ double guiW = 1200, guiH = 0; std::string diagTxt;
  Target target() const { Target t; t.v = val[P_UMBAU] > 0.5 ? Parts::umbau() : Parts::orig(); t.taper = (int)std::lround(val[P_TAPER]);
    t.s.v1 = val[P_V1]/10; t.s.v2 = val[P_V2]/10; t.s.pl = val[P_PL]/10; t.s.t = val[P_T]/10; t.s.m = taper(val[P_M]/10, t.taper); t.s.b = val[P_B]/10; t.s.mv = val[P_MV]/10;
    t.s.pres = taper(val[P_PRES]/10, t.taper); t.s.mid = val[P_MID] > 0.5; t.s.top = val[P_TOP] > 0.5; t.s.bass = val[P_BASS] > 0.5;
    t.s.ch1 = val[P_CH] < 0.5 ? 1 : 0; t.s.low = val[P_INP] > 0.5; t.pa = val[P_PA] > 0.5; t.nl = val[P_NL] > 0.5;
    t.vin = val[P_VIN] / 1000; int o = (int)std::lround(val[P_OS]); t.os = o == 0 ? 1 : o == 1 ? 2 : o == 2 ? 4 : 8;
    for (int i = 0; i < 3; i++) t.s.tube[i] = std::max(0, std::min(NTUBES - 1, (int)std::lround(val[P_TUBE1 + i]))); t.spk = val[P_SPK] > 0.5;
    if (val[P_SOFT] > 0.5) t.v.c3 = 1e-9; t.v.rpad = val[P_PAD] > 0.5 ? 2.2e6 : 0;   // Umbauideen wie im Web: „Sanft“ = C3 an V101B ab, „Später“ = 2,2 MΩ vor Preamp 2 Volume (nur HIGH-Eingang)
    return t; }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }
// Ruhezustand nach langer Stille: Arbeitspunkte wie beim Start, Koppelkondensatoren entladen, Filterspeicher leer.
// Wird beim Aufwachen angewandt; so hängt das Ergebnis weder von der Schlafdauer ab noch davon, ob der Host den Schlaf befolgt.
static void settleRunner(LinRunner &r) { std::fill(r.Ih.begin(), r.Ih.end(), 0.0); std::fill(r.vc.begin(), r.vc.end(), 0.0); r.hasPrev = false; r.blend = 1; }
// Größte Abweichung aller Kondensatorspannungen vom Ruhezustand (V): Maß dafür, ob der Kern innerlich zur Ruhe gekommen ist
static double restDev(Engine *E) {
  if (!E || !E->a || E->b) return 1e9; CoreBase *c = E->a; double d = 0;
  auto lin = [&](LinRunner &r) { for (double x : r.vc) d = std::max(d, std::fabs(x)); };
  if (c->isNl()) { NlCore *n = (NlCore*)c;
    std::string key = NlCore::cacheKey(n->v, n->fs, n->os, n->cur.tube); NlCore::CacheLock lk;
    auto &ca = NlCore::cache(); auto it = ca.find(key); if (it == ca.end()) return 1e9;
    const Snapshot &s = it->second; for (size_t k = 0; k < n->amp.st.size() && k < s.st.size(); k++) for (size_t i = 0; i < n->amp.st[k].vc.size() && i < s.st[k].vc.size(); i++) d = std::max(d, std::fabs(n->amp.st[k].vc[i] - s.st[k].vc[i]));
    if (n->pa) d = std::max(d, n->paC.restDev()); }
  else { LinCore *l = (LinCore*)c; lin(l->r); if (l->pa) lin(l->paC.r); }
  return d;
}
static void settle(Engine *E) {
  if (!E || !E->a || E->b) return; CoreBase *c = E->a;
  if (c->isNl()) { NlCore *n = (NlCore*)c;
    std::string key = NlCore::cacheKey(n->v, n->fs, n->os, n->cur.tube); NlCore::CacheLock lk;
    auto &ca = NlCore::cache(); auto it = ca.find(key); if (it != ca.end()) n->amp.initFrom(n->v, n->cur, it->second); else n->amp.init(n->v, n->cur);
    n->amp.blending = false; std::fill(n->amp.xb.begin(), n->amp.xb.end(), 0.0); std::fill(n->amp.yb.begin(), n->amp.yb.end(), 0.0); n->paC.settle(); }
  else { LinCore *l = (LinCore*)c; settleRunner(l->r); settleRunner(l->paC.r); }
}

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "Laney AOR 50"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); }
  else if (id == P_OUT) std::snprintf(buf, n, "%+.1f dB", v); else if (id == P_VIN) std::snprintf(buf, n, "%.0f mV", v); else std::snprintf(buf, n, "%.1f", v);
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
static void par_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) { apply_events(P(p), in); emit_gui(P(p), out); }
static const clap_plugin_params_t EXT_PARAMS = {par_count, par_info, par_value, par_v2t, par_t2v, par_flush};

// ---- Audio-Ports ----
static uint32_t ap_count(const clap_plugin_t *, bool) { return 1; }
static bool ap_get(const clap_plugin_t *, uint32_t i, bool is_input, clap_audio_port_info_t *info) {
  if (i != 0) return false; std::memset(info, 0, sizeof *info); info->id = 0; std::snprintf(info->name, sizeof info->name, "%s", is_input ? "Input" : "Output");
  info->channel_count = 2; info->flags = CLAP_AUDIO_PORT_IS_MAIN; info->port_type = CLAP_PORT_STEREO; info->in_place_pair = CLAP_INVALID_ID; return true; }
static const clap_plugin_audio_ports_t EXT_PORTS = {ap_count, ap_get};

// ---- Zustand ----
static bool st_save(const clap_plugin_t *p, const clap_ostream_t *s) { char buf[1024]; int len = 0; for (int i = 0; i < P_COUNT; i++) len += std::snprintf(buf + len, sizeof buf - len, "%d=%.17g\n", i, P(p)->val[i].load());
  int done = 0; while (done < len) { int64_t w = s->write(s, buf + done, len - done); if (w <= 0) return false; done += (int)w; } return true; }
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); if (all.size() > 65536) return false; }   // Zustand ist knapp 1 KB; mehr ist kaputt
  bool seen[P_COUNT] = {};
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = std::atof(line.substr(q + 1).c_str()); if (id >= 0 && id < P_COUNT && std::isfinite(v)) { P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); seen[id] = true; } }
  // Projekte älterer Fassungen: Parameter, die es damals nicht gab, so setzen, wie die alte Fassung klang (nicht die heutige Grundstellung)
  for (int i = 0; i < P_COUNT; i++) if (!seen[i]) { double v = PD[i].def; if (i == P_PRES) v = 5; else if (i == P_TUBE1 || i == P_TUBE2 || i == P_TUBE3 || i == P_SPK || i == P_SOFT || i == P_PAD) v = 0; P(p)->val[i].store(v); }
  P(p)->dirty = true; return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
static bool pl_init(const clap_plugin_t *) { warmTubes(); return true; }   // Röhrentabellen jetzt (Haupt-Thread), nicht erst im Audio-Thread
static void pl_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); 
#ifndef TS_NO_GUI
  { std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; } 
#ifndef __APPLE__
  tsx::close(pl->dpy); pl->dpy = nullptr;
#endif
  }
#endif
  delete pl->eng; delete pl->eng2; delete pl; }
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->fs = sr; delete pl->eng; delete pl->eng2; pl->eng = new Engine(sr); pl->eng2 = new Engine(sr); pl->eng->pad = pl->eng2->pad = LATENCY; pl->mono.assign(maxf + 16, 0); pl->mono2.assign(maxf + 16, 0); pl->dirty = true; pl->eng2Has = false; pl->monoOK = true; pl->eng2Stale = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { Plugin *pl = P(p); delete pl->eng; pl->eng = nullptr; delete pl->eng2; pl->eng2 = nullptr; }
static bool pl_start(const clap_plugin_t *) { return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { Plugin *pl = P(p); if (pl->eng) { delete pl->eng; delete pl->eng2; pl->eng = new Engine(pl->fs); pl->eng2 = new Engine(pl->fs); pl->eng->pad = pl->eng2->pad = LATENCY; pl->dirty = true; pl->eng2Has = false; pl->monoOK = true; pl->eng2Stale = true; } }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  if (!pl->eng) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  uint32_t n = pr->frames_count; if (pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->mono.size() < n) { pl->mono.assign(n + 16, 0); pl->mono2.assign(n + 16, 0); }
  float *m = pl->mono.data(), *m2 = pl->mono2.data(); bool stereo = in.channel_count > 1 && out.channel_count > 1;
  if (pl->dirty.exchange(false)) { pl->lastT = pl->target(); pl->eng->setTarget(pl->lastT); pl->eng2Has = false; pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->quiet = 0; }
  auto t0 = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = in.channel_count > 1 ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = in.channel_count > 1 ? in.data64[1][i] : x; } m[i] = (float)x; m2[i] = (float)x2; }
  // Stereo mit gleichem Signal links und rechts (Mono-Spur, Bass-DI auf Stereospur): nur einmal rechnen. Erst wenn die Kanäle sich
  // unterscheiden, übernimmt die zweite Engine den Zustand der ersten (Klon) und rechnet mit; zurück zu einmal rechnen erst, wenn beide
  // Kanäle wieder gleich sind und die Engines dasselbe ausgeben (sonst sprünge der rechte Kanal).
  bool same = stereo && std::memcmp(m, m2, n * sizeof(float)) == 0;
  if (!stereo) { pl->monoOK = true; pl->eng2Stale = true; }   // einkanalig: die zweite Engine läuft nicht mit, ist danach also veraltet
  if (stereo && !same) { pl->monoOK = false; if (pl->eng2Stale) { delete pl->eng2; pl->eng2 = new Engine(*pl->eng); pl->eng2Stale = false; pl->eng2Has = true; } }
  bool dual = stereo && !(same && pl->monoOK);
  if (dual && !pl->eng2Has) { pl->eng2->setTarget(pl->lastT); pl->eng2Has = true; }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(m[i]), std::fabs(m2[i])));
  if (pl->sleeping && inPk >= 1e-6f) { settle(pl->eng); if (dual) settle(pl->eng2); pl->sleeping = false; pl->quiet = 0; }   // Aufwachen: vom Ruhezustand aus weiterrechnen
  if (pl->sleeping) {   // Schlaf auch ohne Mitwirkung des Hosts: Kern übergehen, Stille ausgeben (echter Ausgang läge unter −100 dBFS)
    for (uint32_t c = 0; c < out.channel_count; c++) { if (out.data32) std::memset(out.data32[c], 0, n * sizeof(float)); else if (out.data64) std::memset(out.data64[c], 0, n * sizeof(double)); }
    return CLAP_PROCESS_SLEEP; }
  pl->eng->block(m, (int)n);
  if (dual) { pl->eng2->block(m2, (int)n);
    if (same) { bool eq = true; for (uint32_t i = 0; i < n; i++) if (std::fabs(m[i] - m2[i]) > 1e-7f) { eq = false; break; } if (eq) { pl->monoOK = true; pl->eng2Stale = true; } } }   // gleichauf: ab jetzt einmal rechnen
  else if (stereo) std::memcpy(m2, m, n * sizeof(float));
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  { double g = pl->outCur < 0 ? pl->outg : pl->outCur, gt = pl->outg, k = 1 - std::exp(-1.0 / (0.02 * pl->fs));   // Ausgangspegel gleitet (kein Knacken beim Drehen)
    for (uint32_t i = 0; i < n; i++) { g += (gt - g) * k; m[i] = (float)(m[i] * g); if (stereo) m2[i] = (float)(m2[i] * g); } pl->outCur = g; }
  for (uint32_t c = 0; c < out.channel_count; c++) { const float *src = (c == 1 && stereo) ? m2 : m; if (out.data32) for (uint32_t i = 0; i < n; i++) out.data32[c][i] = src[i]; else if (out.data64) for (uint32_t i = 0; i < n; i++) out.data64[c][i] = src[i]; }
  if ((pl->pkCnt += n) >= pl->fs / 10) { pl->pkCnt = 0; double q[5]; takePeaksFrom(pl->eng, q); for (int i = 0; i < 5; i++) pl->pk[i] = q[i]; }
  // Schlafen: Eingang still (< −120 dBFS), Ausgang abgeklungen (< −100 dBFS), kein Umschaltvorgang; nach 0,5 s darf der Host aussetzen
  double outPk = 0; for (uint32_t i = 0; i < n; i++) outPk = std::max(outPk, std::max(std::fabs((double)m[i]), stereo ? std::fabs((double)m2[i]) : 0.0));
  bool trans = pl->eng->b != nullptr || (dual && pl->eng2->b != nullptr);
  bool rest = inPk < 1e-6f && outPk < 1e-5 && !trans && restDev(pl->eng) < 5e-3 && (!dual || restDev(pl->eng2) < 5e-3);   // auch innen zur Ruhe gekommen (< 5 mV; langsamste Zeitkonstante: C12 mit 3,3 MΩ, etwa 1 s)
  if (rest) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)(0.5 * pl->fs)) { if (!pl->sleeping) { pl->sleeping = true; pl->load = 0; for (int i = 0; i < 5; i++) pl->pk[i] = 0; } return CLAP_PROCESS_SLEEP; }
  return CLAP_PROCESS_CONTINUE;
}
// Hülle: Denormals abschalten (FTZ/DAZ, danach wiederherstellen); eine Ausnahme darf nie die C-Grenze zum Host überschreiten
static clap_process_status pl_process(const clap_plugin_t *p, const clap_process_t *pr) {
  DenormGuard dg; clap_process_status r;
  try { r = pl_process_impl(p, pr); } catch (...) { zeroOut(pr); r = CLAP_PROCESS_CONTINUE; }
  return r; }
#ifndef TS_NO_GUI
// ---- Oberfläche (Linux: X11, macOS: NSView; eingebettet) ----
static void gui_tick(Plugin *pl);
static void gui_setp(Plugin *pl, int id, double v) { v = std::max(PD[id].lo, std::min(PD[id].hi, v)); if (pl->val[id].load() == v) return; pl->val[id].store(v); pl->dirty = true;
  { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }   // Geste, damit der Host (VST3: performEdit, Automation „Touch“) die Änderung zuordnet
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
// Ziehen an einem Knopf: eine Geste vom Drücken bis zum Loslassen
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
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef __APPLE__
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { pl->gui = new aorgui::Gui(); pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->computeCurves = [pl](std::vector<double> &cur, std::vector<double> &ref) {   // Kleinsignal-Frequenzgang wie im Web-Simulator
      Target t = pl->target(); double fs = pl->fs; auto curveOf = [&](const Parts &v, const Setting &s, bool pa, std::vector<double> &out) {
        std::vector<TriSS> ss; const std::vector<TriSS> *sp = nullptr; if (!stockTubes(s)) { ss = smallSignal(v, s, fs); sp = &ss; }
        LinNet o = netLin(v, s, fs, sp), q = netPA(v, s.pres, fs); auto g = [&](double f) { double m = std::abs(responseAt(o, f, "W3")); if (pa) m *= std::abs(responseAt(q, f, "br")); if (t.spk) m *= spkMag(f); return m; };
        double r = g(1000); out.resize(aorgui::NF); for (int i = 0; i < aorgui::NF; i++) out[i] = 20 * std::log10(std::max(1e-12, g(aorgui::freqAt(i)) / r)); };
      curveOf(t.v, t.s, t.pa, cur);
      Setting rs; rs.v1 = rs.v2 = rs.t = rs.b = rs.mv = rs.pl = 0.5; rs.m = taper(0.5, t.taper); rs.pres = taper(0.5, t.taper); rs.ch1 = t.s.ch1; rs.low = t.s.low; curveOf(Parts::orig(), rs, t.pa, ref); }; }
  pl->guiH = pl->guiW * aorgui::TOTALH / FW;
  }   // Timer außerhalb der Sperre anmelden (der Host kann darin schon einen Takt auslösen)
#ifdef __APPLE__
  { std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) pl->gui->onTick = [pl]() { gui_tick(pl); }; }   // die NSView taktet selbst (30/s)
#else
  auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId == CLAP_INVALID_ID) ht->register_timer(pl->host, 33, &pl->timerId);
#endif
  return true; }
static void g_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); if (pl->gestId >= 0) gui_gesture(pl, pl->gestId, false); auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; }
  std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; } 
#ifndef __APPLE__
  if (pl->dpy) { XSync(pl->dpy, False); }
#endif
  }
static void upd_diag(Plugin *pl) { char b[160]; int o = (int)std::lround(pl->val[P_OS]); std::snprintf(b, sizeof b, "CPU %.0f %% of one core · %dx oversampling · %.0f Hz · stereo%s", pl->load.load() * 100, o == 0 ? 1 : o == 1 ? 2 : o == 2 ? 4 : 8, pl->fs, pl->sleeping ? " · idle (silent input)" : ""); pl->diagTxt = b; }
// Anzeigen (Last, Röhrenaussteuerung bzw. Endstufen-Spitze) für die Oberfläche auffrischen; Linux: Host-Timer, macOS: Takt der NSView
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; upd_diag(pl); for (int i = 0; i < 5; i++) pl->gui->meter[i] = std::max(pl->pk[i].load(), pl->gui->meter[i] * 0.85); }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)FW; h->aspect_ratio_height = (uint32_t)aorgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h) { double ww = std::max(600.0, std::min(2600.0, (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * aorgui::TOTALH / FW); return true; }
static bool g_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); pl->guiW = w; pl->guiH = h; if (pl->gui && pl->gui->win) { pl->gui->resize((int)w, (int)h); pl->gui->draw(); } return true; }
static bool g_set_parent(const clap_plugin_t *p, const clap_window_t *win) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; if (pl->gui->win) pl->gui->destroy();
#ifdef __APPLE__
  return pl->gui->attach(win->cocoa, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
#else
  if (!pl->dpy) return false; return pl->gui->init(pl->dpy, (Window)win->x11, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
#endif
static bool g_set_transient(const clap_plugin_t *, const clap_window_t *) { return false; }
static void g_suggest_title(const clap_plugin_t *, const char *) {}
static bool g_show(const clap_plugin_t *p) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->show(); pl->gui->draw(); return true; }
static bool g_hide(const clap_plugin_t *p) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->hide(); return true; }
static const clap_plugin_gui_t EXT_GUI = {g_supported, g_preferred, g_create, g_destroy, g_set_scale, g_get_size, g_can_resize, g_resize_hints, g_adjust_size, g_set_size, g_set_parent, g_set_transient, g_suggest_title, g_show, g_hide};
#endif
// Latenz: Überabtastungsfilter genau 31 Abtastwerte (Amp::step); linear und 1-fach werden im Kern um 31 verzögert (Engine::pad)
static uint32_t g_latency(const clap_plugin_t *) { return LATENCY; }   // fest, unabhängig vom Modus (CLAP: Latenz darf sich nur beim Aktivieren ändern)
static const clap_plugin_latency_t EXT_LATENCY = {g_latency};
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
static void on_timer(const clap_plugin_t *p, clap_id) { Plugin *pl = P(p); std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock()) return;
  try {
  if (pl->gui) { gui_tick(pl); pl->gui->pump(); } } catch (...) {} }
static const clap_plugin_timer_support_t EXT_TIMER = {on_timer};
#endif

static const void *pl_ext(const clap_plugin_t *, const char *id) {
#ifndef TS_NO_GUI
  if (!std::strcmp(id, CLAP_EXT_GUI)) return &EXT_GUI;
#endif
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
  if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &EXT_TIMER;
#endif
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static void pl_main(const clap_plugin_t *) {}
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DISTORTION, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.laney-aor50", "Laney AOR 50 Preamp", "tonestack", "", "", "", TS_VERSION, "Preamp simulation from schematic DRG 1054 with tube distortion, power amp feedback (presence) and selectable tubes", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def);
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST   // VST3 über clap-wrapper (engine/vst3): Einstieg als normale Funktionen, clap_entry legt der Wrapper an
extern "C" bool laney_entry_init(const char *p) { return entry_init(p); }
extern "C" void laney_entry_deinit() { entry_deinit(); }
extern "C" const void *laney_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
