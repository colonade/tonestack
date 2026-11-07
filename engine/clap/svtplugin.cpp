// AmpFett SVT-CL, Nachbau des Ampeg SVT-CL (Service Manual: Vorverstärker 07S519-12, Endstufe 07S419-12; alle Röhren gerechnet) als CLAP-Plugin (Linux x86-64; auf dem Mac über clap-wrapper als VST3/Audio Unit)
#include "clap-1.2.2/include/clap/clap.h"
#define TS_VERSION "1.6"   // Plugin-Version: Deskriptor, Oberfläche, Paketnamen
#include "svt_core.hpp"
#include "platform.hpp"
#include "ports.hpp"
#include "autoin.hpp"
#ifndef TS_NO_GUI
#include "svtgui.hpp"
#else
namespace sgui { struct Gui; }
#endif
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
#include "x11safe.hpp"
#endif
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
#define TSX_GUARD tsx::Guard xg_   // eigene X-Fehlerbehandlung nur während unserer X-Aufrufe (x11safe.hpp)
#else
#define TSX_GUARD (void)0
#endif
using namespace svt;

enum { P_GAIN, P_BASS, P_MID, P_FREQ, P_TREB, P_MASTER, P_UHI, P_ULO, P_JACK, P_TAP, P_STBY, P_PWR, P_VIN, P_OS, P_OUT, P_TAPER, P_PSU, P_BAL, P_MIS, P_INREF, P_COUNT };
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_ONOFF[] = {"Off", "On"}; static const char *N_FREQ[] = {"220 Hz", "450 Hz", "800 Hz", "1.6 kHz", "3 kHz"}; static const char *N_JACK[] = {"0 dB (passive)", "-15 dB (active)"};
static const char *N_TAPO[] = {"4 ohm (load 4 ohm)", "2 ohm (load 2 ohm)"}; static const char *N_TAP[] = {"Log 15 % (original)", "Log 10 %", "Linear"}; static const char *N_OS[] = {"1x"}; static const char *N_PSU[] = {"Simple", "As schematic, 50 Hz mains", "As schematic, 60 Hz mains"}; static const char *N_BAL[] = {"Off", "On (T1 estimated)"};
static const PDef PD[P_COUNT] = {
  {"Gain", 0, 10, 5, 0, 0}, {"Bass", 0, 10, 5, 0, 0}, {"Midrange", 0, 10, 5, 0, 0}, {"Frequency", 0, 4, 2, N_FREQ, 5}, {"Treble", 0, 10, 5, 0, 0}, {"Master", 0, 10, 5, 0, 0},
  {"Ultra Hi", 0, 1, 0, N_ONOFF, 2}, {"Ultra Lo", 0, 1, 0, N_ONOFF, 2}, {"Input", 0, 1, 0, N_JACK, 2}, {"Output tap", 0, 1, 0, N_TAPO, 2},
  {"Standby", 0, 1, 0, N_ONOFF, 2}, {"Power", 0, 1, 1, N_ONOFF, 2},
  {"Instrument level (mV peak)", 1, 1000, 100, 0, 0}, {"Oversampling (1x only since 1.1)", 0, 0, 0, N_OS, 1}, {"Output level (dB)", -36, 24, 0, 0, 0}, {"Pot taper (A pots: gain, bass, treble)", 0, 2, 0, N_TAP, 3},
  {"Power supply", 0, 2, 0, N_PSU, 3}, {"BAL OUT load (T1)", 0, 1, 0, N_BAL, 2},   // ab 1.5
  {"Power tube mismatch (%)", 0, 10, 8, 0, 0}, {"Input reference", 0, 1, 0, tsin::N_REF, 2} };   // ab 1.5: Ruhestrom-Unterschied der beiden 6550-Seiten
static const uint32_t LATENCY = 31;   // fest: Überabtastungsfilter 31 Abtastwerte; bei 1x verzögert das Plugin selbst um 31, damit die Latenz nicht mit dem Modus wechselt
static int osFactor(int) { return 1; }   // ab 1.1 nur noch 1x: 2x/4x brauchten mehr als einen Rechenkern; Aliasing bei Bass-Grundtönen auch bei 1x unter −80 dB (docs/PROJEKTSTAND.md)

// Eingeschwungener Verstärker je Abtastrate und Überabtastung (Netzteil und Arbeitspunkte brauchen 1,5 s Rechenzeit): einmal rechnen, dann kopieren
static std::mutex g_protoM;
// Einschwingen außerhalb der Sperre (andere Instanzen warten nicht); nie im Audio-Faden aufrufen
// cancel (ab 1.14): Hintergrund-Fäden brechen auf Wunsch ab (nullptr zurück, nichts gespeichert), damit deactivate/destroy nicht bis zu Sekunden warten
static Amp *newAmp(double fs, int cfg, const std::atomic<bool> *cancel = nullptr) { static std::map<std::pair<double,int>, std::unique_ptr<Amp>> cache;   // cfg: Überabtastung
  { std::lock_guard<std::mutex> lk(g_protoM); auto it = cache.find({fs, cfg}); if (it != cache.end()) return new Amp(*it->second); }
  std::unique_ptr<Amp> a(new Amp()); Setting s0; s0.psu = (cfg / 10) / 2; s0.bal = (cfg / 10) % 2; a->init(fs, cfg % 10, Parts(), s0, cancel); a->cfg = cfg; if (cancel && cancel->load()) return nullptr;
  std::lock_guard<std::mutex> lk(g_protoM); auto &p = cache[{fs, cfg}]; if (!p) p = std::move(a); return new Amp(*p); }
struct Pend { Amp *a, *proto; };   // fertig eingeschwungener Kern für eine neue Überabtastung samt Ruhezustand (für reset)
struct Dly { float b[LATENCY] = {}; int i = 0; float step(float x) { float y = b[i]; b[i] = x; if (++i == (int)LATENCY) i = 0; return y; } };

struct Plugin {
  tsin::AutoIn ain;   // Eingangsbezug: gemerkte Spitze (autoin.hpp)
  PortCfg ports;   // Kanalzahl Eingang/Ausgang (mono oder stereo)
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; Amp *eng = nullptr, *eng2 = nullptr; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true};
  std::atomic<double> load{0}; std::atomic<double> pk[6] = {}; int pkCnt = 0; std::vector<float> mono, mono2; double outg = 1, outCur = -1; bool monoOK = true, eng2Stale = true;
  Dly d1, d2; long quiet = 0; std::atomic<bool> sleeping{false};
  // Überabtastung umschalten: neuer Kern entsteht in einem Hilfsfaden (Einschwingen dauert), der Audio-Faden übernimmt ihn fertig
  std::atomic<int> cfgAct{1}; std::atomic<Pend*> pending{nullptr}; std::thread worker; std::mutex wm; std::atomic<bool> building{false}, cbAsked{false}; std::atomic<bool> active{false};
  std::atomic<bool> fresh{true}; std::chrono::steady_clock::time_point lastEnd{};   // fresh: beim nächsten process aus dem Ruhezustand neu beginnen (Start, reset, Lücke in der Zeit)
  Amp *proto = nullptr;   // Ruhezustand des laufenden Kerns: reset kopiert ihn (im Audio-Faden kein Einschwingen, keine Sperre)
  sgui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; double guiW = 1200, guiH = 0; std::string diagMode; int diagCnt = 0; std::string diagTxt;
  std::unique_ptr<Core> plotC; double plotFs = 0;   // eigene Instanz für den Frequenzgang (Oberflächen-Faden), zur Abtastrate plotFs
  // ab 1.14: der eingeschwungene 1x-Kern für das Diagramm entsteht im Hintergrund (vorher bis zu Sekunden im Hauptfaden beim ersten Öffnen bei 2x/4x)
  std::thread plotJob; std::atomic<bool> plotBusy{false}, plotRedo{false}, plotStop{false}, cancelJobs{false}; std::mutex plotM; std::unique_ptr<Core> plotReady; double plotReadyFs = 0;
  // Aufbau des Kerns: Überabtastung; ein Wechsel braucht einen neu eingeschwungenen Kern (Hilfsfaden)
  // cfg = Überabtastung + 10 · (2 · Netzteil + BAL OUT): beides ändert den Aufbau des Netzes und braucht einen neu eingeschwungenen Kern (ab 1.5)
  int cfgWant() const { return osFactor((int)std::lround(val[P_OS])) + 10 * (2 * ix(val[P_PSU], 2) + ix(val[P_BAL], 1)); }
  static int ix(double v, int hi) { return std::max(0, std::min(hi, (int)std::lround(v))); }
  Setting setting(double g, double b, double m, double t, double ms) const { Setting s; s.gain = g / 10; s.bass = b / 10; s.mid = m / 10; s.treb = t / 10; s.master = ms / 10;
    s.freq = ix(val[P_FREQ], 4); s.ulo = val[P_ULO] > 0.5; s.uhi = val[P_UHI] > 0.5; s.jack = ix(val[P_JACK], 1); s.tap = ix(val[P_TAP], 1); s.taper = ix(val[P_TAPER], 2); s.bal = ix(val[P_BAL], 1); s.psu = 0; s.mis = std::max(0.0, std::min(10.0, (double)val[P_MIS])) / 100; return s; }   // psu/bal für den Kern: Amp übernimmt sie vom eingeschwungenen Kern (Wechsel nur über cfg)
  Setting target() const { return setting(val[P_GAIN], val[P_BASS], val[P_MID], val[P_TREB], val[P_MASTER]); }
  void applyTo(Amp *a, bool snap = false) const { if (!a) return; a->vin = val[P_VIN] / 1000; a->power = true; a->setTarget(target()); if (snap) a->snap(); }   // STANDBY/POWER ab 1.3 ohne Wirkung (Schalter von der Frontplatte entfernt; Parameter bleiben verborgen, damit alte Projekte laden)
  void joinWorker() { std::lock_guard<std::mutex> lk(wm); cancelJobs = true; if (worker.joinable()) worker.join(); cancelJobs = false; building = false; if (Pend *q = pending.exchange(nullptr)) { delete q->a; delete q->proto; delete q; } }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = (i == P_OS || i == P_STBY || i == P_PWR ? CLAP_PARAM_IS_HIDDEN : CLAP_PARAM_IS_AUTOMATABLE) | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);   // Oversampling ab 1.1 verborgen (Nummer bleibt)
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "AmpFett SVT-CL"); return true; }
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
static void ask_main(Plugin *pl) { if (!pl->cbAsked.exchange(true)) pl->host->request_callback(pl->host); }
static void par_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) { Plugin *pl = P(p); apply_events(pl, in); emit_gui(pl, out); if (pl->active && pl->cfgWant() != pl->cfgAct) ask_main(pl); }
static const clap_plugin_params_t EXT_PARAMS = {par_count, par_info, par_value, par_v2t, par_t2v, par_flush};

// ---- Audio-Ports ----
static uint32_t ap_count(const clap_plugin_t *, bool) { return 1; }
static bool ap_get(const clap_plugin_t *p, uint32_t i, bool is_input, clap_audio_port_info_t *info) { return portInfo(P(p)->ports, i, is_input, info); }
static bool cap_can(const clap_plugin_t *, const clap_audio_port_configuration_request *r, uint32_t n) { return portReqOk(r, n); }
static bool cap_apply(const clap_plugin_t *p, const clap_audio_port_configuration_request *r, uint32_t n) { if (!portReqOk(r, n)) return false; portApply(P(p)->ports, r, n); return true; }   // nur im deaktivierten Zustand (CLAP)
static const clap_plugin_configurable_audio_ports_t EXT_CAP = {cap_can, cap_apply};
static const clap_plugin_audio_ports_t EXT_PORTS = {ap_count, ap_get};

// ---- Zustand ----
static bool st_save(const clap_plugin_t *p, const clap_ostream_t *s) { char buf[1024]; int len = 0; for (int i = 0; i < P_COUNT; i++) len += std::snprintf(buf + len, sizeof buf - len, "%d=%.17g\n", i, P(p)->val[i].load());
  len += tsin::saveLine(buf + len, (int)sizeof buf - len, P(p)->ain.pk.load()); int done = 0; while (done < len) { int64_t w = s->write(s, buf + done, len - done); if (w <= 0) return false; done += (int)w; } return true; }
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); if (all.size() > 65536) return false; }
  bool seen[P_COUNT] = {}; P(p)->ain.pk = 0;
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = std::atof(line.substr(q + 1).c_str()); if (id == tsin::STATE_ID) { P(p)->ain.pk = tsin::clampPk(v); continue; } if (id >= 0 && id < P_COUNT && std::isfinite(v)) { P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); seen[id] = true; } }
  for (int i = 0; i < P_COUNT; i++) if (!seen[i]) P(p)->val[i].store(i == P_MIS ? 0.0 : PD[i].def);   // Projekte vor 1.5 ohne Streuung (klingen wie damals)
  if (!seen[P_INREF]) P(p)->val[P_INREF].store(1.0);   // Projekte vor dem Eingangsbezug: fest 0 dBFS (klingen wie damals)
  Plugin *pl = P(p); pl->dirty = true; if (pl->active && pl->cfgWant() != pl->cfgAct) ask_main(pl); return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
// Hauptfaden (activate): Kerne aus dem eingeschwungenen Vorrat; reset im Audio-Faden kopiert nur noch proto
static void newEngines(Plugin *pl, int cfg) { delete pl->eng; delete pl->eng2; delete pl->proto; pl->proto = newAmp(pl->fs, cfg); pl->eng = new Amp(*pl->proto); pl->eng2 = new Amp(*pl->proto); pl->cfgAct = cfg; pl->dirty = true; pl->monoOK = true; pl->eng2Stale = true; pl->d1 = Dly(); pl->d2 = Dly(); pl->quiet = 0; pl->sleeping = false; pl->fresh = true; pl->lastEnd = {}; }
static bool pl_init(const clap_plugin_t *) { return true; }
static void pl_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); pl->joinWorker(); pl->plotStop = true; if (pl->plotJob.joinable()) pl->plotJob.join();
#if !defined(TS_NO_GUI) && !defined(__APPLE__)
  { auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; } }   // Host ohne gui.destroy: Takt nicht auf ein gelöschtes Plugin laufen lassen
#endif
#ifndef TS_NO_GUI
  { TSX_GUARD; std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; }
#ifndef __APPLE__
  tsx::close(pl->dpy); pl->dpy = nullptr;
#endif
  }
#endif
  delete pl->eng; delete pl->eng2; delete pl->proto; delete pl; }
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->joinWorker(); pl->fs = sr; newEngines(pl, pl->cfgWant()); pl->mono.assign(maxf + 16, 0); pl->mono2.assign(maxf + 16, 0); pl->active = true; return true; }
static void pl_deactivate(const clap_plugin_t *p) { Plugin *pl = P(p); pl->active = false; pl->joinWorker(); delete pl->eng; pl->eng = nullptr; delete pl->eng2; pl->eng2 = nullptr; delete pl->proto; pl->proto = nullptr; }
static bool pl_start(const clap_plugin_t *p) { P(p)->fresh = true; return true; }   // REAPER: FX wieder eingeschaltet
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { P(p)->fresh = true; }   // Audio-Faden: Neubeginn beim nächsten process
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  if (!pl->eng) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  uint32_t n = pr->frames_count; if (pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  if (n == 0) return CLAP_PROCESS_CONTINUE;   // nur Ereignisse (sonst Last = 0/0)
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->mono.size() < n) { pl->mono.assign(n + 16, 0); pl->mono2.assign(n + 16, 0); }
  float *m = pl->mono.data(), *m2 = pl->mono2.data(); bool stereo = in.channel_count > 1 && out.channel_count > 1;
  // fertig eingeschwungener Kern mit neuer Überabtastung
  if (Pend *q = pl->pending.exchange(nullptr)) { if (q->a->cfg == pl->cfgWant()) { delete pl->eng; pl->eng = q->a; delete pl->proto; pl->proto = q->proto; pl->cfgAct = q->a->cfg; pl->eng2Stale = true; pl->monoOK = true; pl->dirty = true; pl->quiet = 0; pl->d1 = Dly(); pl->d2 = Dly(); }
    else { delete q->a; delete q->proto; } delete q; pl->building = false; if (pl->cfgWant() != pl->cfgAct) ask_main(pl); }   // inzwischen anders gewählt: neu anstoßen
  // Neubeginn: eingeschwungener Ruhezustand (wie ein Verstärker, der während der Pause still weiterlief), Regler und Schalter sofort am Ziel.
  // Auslöser: activate, start_processing, reset oder eine Pause zwischen zwei Aufrufen von über 0,2 s (bzw. drei Blocklängen) (FX aus und wieder an, ohne dass der Host es meldet).
  // steady_time taugt dafür nicht: clap-wrapper (VST3, AU) liefert dort die Projektposition, die bei stehendem Transport steht und bei Schleifen springt.
  { auto now = std::chrono::steady_clock::now(); double gap = std::chrono::duration<double>(now - pl->lastEnd).count();
    if (pl->lastEnd.time_since_epoch().count() && gap > std::max(0.2, 3.0 * n / pl->fs) && !pl->sleeping) pl->fresh = true; }   // große Puffer: erst ab dreifacher Blockdauer; im Schlaf darf der Host aussetzen (ab 1.14 wie Laney: kein Neubeginn)
  if (pl->fresh.exchange(false) && pl->proto) { *pl->eng = *pl->proto; *pl->eng2 = *pl->proto; pl->applyTo(pl->eng, true); pl->applyTo(pl->eng2, true); pl->dirty = false;
    pl->d1 = Dly(); pl->d2 = Dly(); pl->monoOK = true; pl->eng2Stale = true; pl->quiet = 0; pl->sleeping = false; pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->outCur = -1; }
  if (pl->dirty.exchange(false)) { pl->applyTo(pl->eng); pl->applyTo(pl->eng2); pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->quiet = 0; if (pl->cfgWant() != pl->cfgAct) ask_main(pl); }
  auto t0 = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = in.channel_count > 1 ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = in.channel_count > 1 ? in.data64[1][i] : x; } m[i] = (float)x; m2[i] = (float)x2; }
  // gleiche Kanäle (Mono-Spur auf Stereo): einmal rechnen; unterscheiden sie sich, übernimmt der zweite Kern den Zustand des ersten
  bool same = stereo && std::memcmp(m, m2, n * sizeof(float)) == 0;
  if (!stereo) { pl->monoOK = true; pl->eng2Stale = true; }
  if (stereo && !same) { pl->monoOK = false; if (pl->eng2Stale) { *pl->eng2 = *pl->eng; pl->d2 = pl->d1; pl->eng2Stale = false; } }
  bool dual = stereo && !(same && pl->monoOK);
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(m[i]), std::fabs(m2[i])));
  if (pl->sleeping && inPk >= 1e-6f) { pl->sleeping = false; pl->quiet = 0; }   // Aufwachen: Kern rechnet vom angehaltenen Ruhezustand weiter
  if (pl->sleeping) { for (uint32_t c = 0; c < out.channel_count; c++) { if (out.data32) std::memset(out.data32[c], 0, n * sizeof(float)); else if (out.data64) std::memset(out.data64[c], 0, n * sizeof(double)); }
    return CLAP_PROCESS_CONTINUE; }
  if (pl->val[P_INREF].load() < 0.5) pl->ain.apply(m, stereo ? m2 : nullptr, n);   // Eingangsbezug: gemerkte Spitze = eingestellte mV (autoin.hpp)
  pl->eng->process(m, (int)n); if (pl->eng->os == 1) for (uint32_t i = 0; i < n; i++) m[i] = pl->d1.step(m[i]);
  if (dual) { pl->eng2->process(m2, (int)n); if (pl->eng2->os == 1) for (uint32_t i = 0; i < n; i++) m2[i] = pl->d2.step(m2[i]);
    if (same) { bool eq = true; for (uint32_t i = 0; i < n; i++) if (std::fabs(m[i] - m2[i]) > 1e-7f) { eq = false; break; } if (eq) { pl->monoOK = true; pl->eng2Stale = true; } } }
  else if (stereo) std::memcpy(m2, m, n * sizeof(float));
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  double outPk = 0; for (uint32_t i = 0; i < n; i++) outPk = std::max(outPk, std::max(std::fabs((double)m[i]), stereo ? std::fabs((double)m2[i]) : 0.0));
  { double g = pl->outCur < 0 ? pl->outg : pl->outCur, gt = pl->outg, k = 1 - std::exp(-1.0 / (0.02 * pl->fs));
    for (uint32_t i = 0; i < n; i++) { g += (gt - g) * k; m[i] = (float)(m[i] * g); if (stereo) m2[i] = (float)(m2[i] * g); } pl->outCur = g; }
  for (uint32_t c = 0; c < out.channel_count; c++) { const float *src = (c == 1 && stereo) ? m2 : m; if (out.data32) for (uint32_t i = 0; i < n; i++) out.data32[c][i] = src[i]; else if (out.data64) for (uint32_t i = 0; i < n; i++) out.data64[c][i] = src[i]; }
  if ((pl->pkCnt += n) >= pl->fs / 10) { pl->pkCnt = 0; double q[6], q2[6]; pl->eng->takePeaks(q); double o = pl->eng->takeOut();
    if (dual) { pl->eng2->takePeaks(q2); o = std::max(o, pl->eng2->takeOut()); for (int i = 0; i < 6; i++) q[i] = std::max(q[i], q2[i]); }
    pl->pk[0] = q[0]; pl->pk[1] = q[1]; pl->pk[2] = q[2]; pl->pk[3] = q[3]; pl->pk[4] = std::max(q[4], q[5]); pl->pk[5] = o * o / 2 / loadOhm(pl->eng->c.cur); }   // Leistung an der gewählten Last
  // Schlafen: Eingang still (< −120 dBFS) und Ausgang fast still (< −70 dBFS) für 2 s; der Kern wird dann angehalten
  // Ab Laney 3.38 / Yamaha 1.17 / AmpFett 1.17 / GK 1.2 / G100 1.5 / B100-III 1.2 / SVT-CL 1.2 nur noch intern: Rückgabe CONTINUE statt SLEEP. REAPER rief das Plugin nach SLEEP
  // bei Live-Eingang (Spur scharf, Transport steht) erst nach einem Druck auf Play wieder auf; der Schlaf kostet so nur die Prüfung des Eingangs.
  if (inPk < 1e-6f && outPk < 3e-4 && !pl->building) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)(2 * pl->fs)) { if (!pl->sleeping) { pl->sleeping = true; pl->load = 0; for (auto &x : pl->pk) x = 0; } return CLAP_PROCESS_CONTINUE; }
  return CLAP_PROCESS_CONTINUE;
}
static clap_process_status pl_process(const clap_plugin_t *p, const clap_process_t *pr) {
  DenormGuard dg; clap_process_status r;
  try { r = pl_process_impl(p, pr); } catch (...) { zeroOut(pr); r = CLAP_PROCESS_CONTINUE; }
  P(p)->lastEnd = std::chrono::steady_clock::now();   // Ende des Aufrufs: Maß für Pausen des Hosts
  return r; }
// Hauptfaden (vom Host nach request_callback): neuen Kern für die gewählte Überabtastung im Hilfsfaden einschwingen lassen
// Sperre wm: pl_main kann vom Host und von der Oberfläche kommen (VST3: Timer evtl. aus einem Hilfsfaden), nie zwei Hilfsfäden zugleich
static void pl_main(const clap_plugin_t *p) { Plugin *pl = P(p); pl->cbAsked = false; std::lock_guard<std::mutex> lk(pl->wm); if (!pl->active) return; int want = pl->cfgWant();
  if (want == pl->cfgAct || pl->building) return; if (pl->worker.joinable()) pl->worker.join();
  pl->building = true; double fs = pl->fs;
  try {
  pl->worker = std::thread([pl, fs, want]() { DenormGuard dg; Pend *q = nullptr;
    try { Amp *a = newAmp(fs, want, &pl->cancelJobs); if (!a) { pl->building = false; return; } q = new Pend{a, nullptr}; q->proto = new Amp(*q->a); } catch (...) { if (q) { delete q->a; delete q; } q = nullptr; pl->building = false; return; }
    if (Pend *old = pl->pending.exchange(q)) { delete old->a; delete old->proto; delete old; } });
  } catch (...) { pl->building = false; } }
#ifndef TS_NO_GUI
static void gui_tick(Plugin *pl);
static void gui_setp(Plugin *pl, int id, double v) { v = std::max(PD[id].lo, std::min(PD[id].hi, v)); if (pl->val[id].load() == v) return; pl->val[id].store(v); pl->dirty = true;
  { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host);
  if (id == P_OS) pl_main(&pl->plug); }   // Oberfläche läuft im Hauptfaden: gleich umschalten
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
// Frequenzgang wie auf der Web-Seite: Kleinsignal im Arbeitspunkt, bezogen auf 1 kHz; Vergleich: alle Knöpfe auf 5, gleiche Schalter
static void startPlotJob(Plugin *pl, double fs) { if (pl->plotBusy.exchange(true)) return; if (pl->plotJob.joinable()) pl->plotJob.join();
  try { pl->plotJob = std::thread([pl, fs]() { DenormGuard dg; std::unique_ptr<Core> c;
      try { Amp *a = newAmp(fs, 1, &pl->plotStop); if (a) { c.reset(new Core(a->c)); delete a; } } catch (...) {}
      if (c) { std::lock_guard<std::mutex> lk(pl->plotM); pl->plotReady = std::move(c); pl->plotReadyFs = fs; }
      pl->plotBusy = false; pl->plotRedo = true; }); } catch (...) { pl->plotBusy = false; } }
static void curves(Plugin *pl, std::vector<double> &cur, std::vector<double> &ref) {
  const bool have = pl->plotC && pl->plotFs == pl->fs;
  if (!have) { std::lock_guard<std::mutex> lk(pl->plotM); if (pl->plotReady && pl->plotReadyFs == pl->fs) { pl->plotC = std::move(pl->plotReady); pl->plotFs = pl->plotReadyFs; } }
  if (!pl->plotC || pl->plotFs != pl->fs) { startPlotJob(pl, pl->fs); cur.clear(); ref.clear(); return; }   // noch nicht da: leeres Diagramm, nach dem Rechnen neu
  Core &C = *pl->plotC;
  auto curveOf = [&](const Setting &s, std::vector<double> &o) { C.apply(s); double r = std::abs(C.response(1000)); o.resize(sgui::NF);
    for (int i = 0; i < sgui::NF; i++) o[i] = 20 * std::log10(std::max(1e-12, std::abs(C.response(sgui::freqAt(i))) / std::max(r, 1e-30))); };
  curveOf(pl->target(), cur); Setting r = pl->setting(5, 5, 5, 5, 5); r.freq = 2; r.ulo = r.uhi = false; curveOf(r, ref); }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); TSX_GUARD; if (std::strcmp(api, TS_WINAPI) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
#ifndef __APPLE__
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false;
#endif
  if (!pl->gui) { pl->gui = new sgui::Gui(); pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->resetPeak = [pl]() { pl->ain.requestReset(); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->computeCurves = [pl](std::vector<double> &cur, std::vector<double> &ref) { curves(pl, cur, ref); }; }
  pl->guiH = pl->guiW * sgui::TOTALH / sgui::FW;
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
static void upd_diag(Plugin *pl) { char b[160]; std::snprintf(b, sizeof b, "CPU %.0f %% of one core · %.0f Hz%s", pl->load.load() * 100, pl->fs, pl->sleeping ? " · idle (silent input)" : ""); { const char *m = std::strstr(b, " · "); std::string mode = m ? m : "";   // CPU-Wert höchstens zweimal je Sekunde neu (jede Änderung zeichnet das Fenster neu), Modus sofort
    if (++pl->diagCnt < 15 && mode == pl->diagMode && !pl->diagTxt.empty()) return; pl->diagCnt = 0; pl->diagMode = mode; pl->diagTxt = b; } }
static void gui_tick(Plugin *pl) { std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock() || !pl->gui) return; pl->gui->inPk = tsin::pkDb(pl->ain.pk.load()); if (pl->plotRedo.exchange(false)) pl->gui->curveKey.clear(); upd_diag(pl);
  pl->gui->meterOn = pl->active && !pl->sleeping; int oa = pl->cfgAct; pl->gui->osActive = oa == 1 ? 0 : oa == 2 ? 1 : 2;
  for (int i = 0; i < 6; i++) pl->gui->meter[i] = std::max(pl->pk[i].load(), pl->gui->meter[i] * 0.85); }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)sgui::FW; h->aspect_ratio_height = (uint32_t)sgui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h) { double ww = std::max(700.0, std::min(2600.0, (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * sgui::TOTALH / sgui::FW); return true; }
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
static uint32_t g_latency(const clap_plugin_t *) { return LATENCY; }
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
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DISTORTION, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.ampfett-svtcl", "AmpFett SVT-CL Bass Amp", "tonestack", "", "", "", TS_VERSION, "Tube bass amp after the SVT-CL service manual: 12AX7 preamp with ultra lo/hi and five-position midrange, 12AX7/12AU7 driver, six 6550 with output transformer and feedback, 300 W; no speaker", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def);
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST
extern "C" bool svt_entry_init(const char *p) { return entry_init(p); }
extern "C" void svt_entry_deinit() { entry_deinit(); }
extern "C" const void *svt_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
