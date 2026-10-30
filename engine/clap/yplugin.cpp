// Yamaha B100-115II (PA-Board, Endstufe, Lautsprecher) als CLAP-Plugin (Linux x86-64)
#include "clap-1.2.2/include/clap/clap.h"
#include "yamaha_core.hpp"
#include "ygui.hpp"
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#include <xmmintrin.h>
#include "x11safe.hpp"
using namespace b100;

enum { P_INP, P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4, P_TAPER, P_PWR, P_VIN, P_SPK, P_OUT, P_COUNT };
struct PDef { const char *name; double lo, hi, def; const char *const *names; int nn; };
static const char *N_INP[] = {"HIGH", "LOW (-12 dB)"}; static const char *N_ONOFF[] = {"Off", "On"}; static const char *N_TAP[] = {"Log 15 %", "Log 10 %", "Linear"};
static const PDef PD[P_COUNT] = {
  {"Input jack", 0, 1, 0, N_INP, 2}, {"Volume", 0, 10, 5, 0, 0}, {"Treble", 0, 10, 5, 0, 0}, {"Middle", 0, 10, 5, 0, 0}, {"Bass", 0, 10, 5, 0, 0},
  {"EQ 60 Hz", -5, 5, 0, 0, 0}, {"EQ 150 Hz", -5, 5, 0, 0, 0}, {"EQ 320 Hz", -5, 5, 0, 0, 0}, {"EQ 640 Hz", -5, 5, 0, 0, 0}, {"EQ 1250 Hz", -5, 5, 0, 0, 0},
  {"Pot taper (volume, tone)", 0, 2, 0, N_TAP, 3}, {"Power amp (MA board, clipping at 42 V)", 0, 1, 0, N_ONOFF, 2}, {"Instrument level at 0 dBFS (mV peak)", 1, 1000, 100, 0, 0},
  {"Speaker (15\" CB158, Portaflex-size vented box)", 0, 1, 0, N_ONOFF, 2}, {"Output level (dB)", -36, 24, 0, 0, 0} };

struct Plugin {
  clap_plugin_t plug; const clap_host_t *host; double fs = 48000; YCore *eng = nullptr, *eng2 = nullptr; std::atomic<double> val[P_COUNT]; std::atomic<bool> dirty{true};
  std::atomic<double> load{0}; std::atomic<double> pk{0}; int pkCnt = 0; std::vector<float> mono, mono2; double outg = 1; bool eng2Has = false; YSetting lastT;
  long quiet = 0; std::atomic<bool> sleeping{false};
  ygui::Gui *gui = nullptr; Display *dpy = nullptr; clap_id timerId = CLAP_INVALID_ID; std::mutex qm; std::recursive_mutex gm; /* gm: Oberfläche/X-Verbindung, Host-Aufrufe und Timer nie gleichzeitig (VST3: Timer evtl. aus Hilfs-Thread) */ struct GuiEv { int id; double v; int kind; }; std::deque<GuiEv> guiQ; int gestId = -1; /* kind: 0 Wert, 1 Gestenbeginn, 2 Gestenende; gestId: Parameter, der gerade gezogen wird */ double guiW = 1200, guiH = 0; std::string diagTxt;
  int taperOf() const { return std::max(0, std::min(2, (int)std::lround(val[P_TAPER]))); }
  // Knopfstellungen → Netzgrößen wie setting() der Web-Seite: Potis mit Kennlinie, Volume mit 0,1 % Rest, EQ-Knopf 10 = Anhebung
  YSetting setting(double vol, double t, double m, double b, const double *eqKnob, bool low, bool pwr, bool spk) const {
    YSetting s; int tp = taperOf(); s.t = taper(t / 10, tp); s.b = taper(b / 10, tp); s.m = taper(m / 10, tp); s.vol = std::max(taper(vol / 10, tp), 0.001);
    for (int k = 0; k < 5; k++) s.e[k] = 1 - eqKnob[k] / 10; s.ing = low ? ING_LOW : 1; s.pwr = pwr; s.spk = spk; s.vin = val[P_VIN] / 1000; return s; }
  YSetting target() const { double eq[5]; for (int k = 0; k < 5; k++) eq[k] = val[P_E0 + k] + 5;
    return setting(val[P_VOL], val[P_T], val[P_M], val[P_B], eq, val[P_INP] > 0.5, val[P_PWR] > 0.5, val[P_SPK] > 0.5); }
};
static Plugin *P(const clap_plugin_t *p) { return (Plugin*)p->plugin_data; }

// ---- Parameter ----
static uint32_t par_count(const clap_plugin_t *) { return P_COUNT; }
static bool par_info(const clap_plugin_t *, uint32_t i, clap_param_info_t *info) {
  if (i >= P_COUNT) return false; const PDef &d = PD[i]; std::memset(info, 0, sizeof *info); info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | (d.names ? CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM : 0);
  info->min_value = d.lo; info->max_value = d.hi; info->default_value = d.def; std::snprintf(info->name, sizeof info->name, "%s", d.name); std::snprintf(info->module, sizeof info->module, "%s", "Yamaha B100-II"); return true; }
static bool par_value(const clap_plugin_t *p, clap_id id, double *out) { if (id >= P_COUNT) return false; *out = P(p)->val[id]; return true; }
static bool par_v2t(const clap_plugin_t *, clap_id id, double v, char *buf, uint32_t n) {
  if (id >= P_COUNT) return false; const PDef &d = PD[id];
  if (d.names) { int k = (int)std::lround(v); if (k < 0) k = 0; if (k >= d.nn) k = d.nn - 1; std::snprintf(buf, n, "%s", d.names[k]); }
  else if (id == P_OUT) std::snprintf(buf, n, "%+.1f dB", v); else if (id == P_VIN) std::snprintf(buf, n, "%.0f mV", v); else if (id >= P_E0 && id <= P_E4) std::snprintf(buf, n, "%+.1f", v); else std::snprintf(buf, n, "%.1f", v);
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
static bool st_load(const clap_plugin_t *p, const clap_istream_t *s) { std::string all; char buf[256]; while (true) { int64_t r = s->read(s, buf, sizeof buf); if (r <= 0) break; all.append(buf, (size_t)r); }
  size_t pos = 0; while (pos < all.size()) { size_t e = all.find('\n', pos); if (e == std::string::npos) e = all.size(); std::string line = all.substr(pos, e - pos); pos = e + 1;
    size_t q = line.find('='); if (q == std::string::npos) continue; int id = std::atoi(line.substr(0, q).c_str()); double v = std::atof(line.substr(q + 1).c_str()); if (id >= 0 && id < P_COUNT && std::isfinite(v)) P(p)->val[id].store(std::max(PD[id].lo, std::min(PD[id].hi, v))); }
  P(p)->dirty = true; return true; }
static const clap_plugin_state_t EXT_STATE = {st_save, st_load};

// ---- Plugin ----
static void newEngines(Plugin *pl) { delete pl->eng; delete pl->eng2; pl->eng = new YCore(pl->fs); pl->eng2 = new YCore(pl->fs); pl->dirty = true; pl->eng2Has = false; }
static bool pl_init(const clap_plugin_t *) { return true; }
static void pl_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); { std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; } tsx::close(pl->dpy); pl->dpy = nullptr; } delete pl->eng; delete pl->eng2; delete pl; }
static bool pl_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t maxf) { Plugin *pl = P(p); pl->fs = sr; newEngines(pl); pl->mono.assign(maxf + 16, 0); pl->mono2.assign(maxf + 16, 0); return true; }
static void pl_deactivate(const clap_plugin_t *p) { Plugin *pl = P(p); delete pl->eng; pl->eng = nullptr; delete pl->eng2; pl->eng2 = nullptr; }
static bool pl_start(const clap_plugin_t *) { return true; }
static void pl_stop(const clap_plugin_t *) {}
static void pl_reset(const clap_plugin_t *p) { Plugin *pl = P(p); if (pl->eng) newEngines(pl); }
static void zeroOut(const clap_process_t *pr) { for (uint32_t k = 0; k < pr->audio_outputs_count; k++) { clap_audio_buffer_t &o = pr->audio_outputs[k]; for (uint32_t c = 0; c < o.channel_count; c++) { if (o.data32) std::memset(o.data32[c], 0, pr->frames_count * sizeof(float)); else if (o.data64) std::memset(o.data64[c], 0, pr->frames_count * sizeof(double)); } } }
static clap_process_status pl_process_impl(const clap_plugin_t *p, const clap_process_t *pr) {
  Plugin *pl = P(p); apply_events(pl, pr->in_events); emit_gui(pl, pr->out_events);
  if (!pl->eng) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  uint32_t n = pr->frames_count; if (pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) { zeroOut(pr); return CLAP_PROCESS_CONTINUE; }
  const clap_audio_buffer_t &in = pr->audio_inputs[0]; clap_audio_buffer_t &out = pr->audio_outputs[0];
  if (pl->mono.size() < n) { pl->mono.assign(n + 16, 0); pl->mono2.assign(n + 16, 0); }
  float *m = pl->mono.data(), *m2 = pl->mono2.data(); bool stereo = in.channel_count > 1 && out.channel_count > 1;
  if (pl->dirty.exchange(false)) { pl->lastT = pl->target(); pl->eng->setTarget(YParts::orig(), pl->lastT); pl->eng2Has = false; pl->outg = std::pow(10, pl->val[P_OUT] / 20); pl->quiet = 0; }
  if (stereo && !pl->eng2Has) { pl->eng2->setTarget(YParts::orig(), pl->lastT); pl->eng2Has = true; }   // zweite Engine nur bei Stereo mitführen
  auto t0 = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < n; i++) { double x = 0, x2 = 0; if (in.data32) { x = in.data32[0][i]; x2 = in.channel_count > 1 ? in.data32[1][i] : x; } else if (in.data64) { x = in.data64[0][i]; x2 = in.channel_count > 1 ? in.data64[1][i] : x; } m[i] = (float)x; m2[i] = (float)x2; }
  float inPk = 0; for (uint32_t i = 0; i < n; i++) inPk = std::max(inPk, std::max(std::fabs(m[i]), std::fabs(m2[i])));
  if (pl->sleeping && inPk >= 1e-6f) { pl->sleeping = false; pl->quiet = 0; }
  if (pl->sleeping) {   // Schlaf auch ohne Mitwirkung des Hosts: lineares Netz ist abgeklungen, Stille ausgeben
    for (uint32_t c = 0; c < out.channel_count; c++) { if (out.data32) std::memset(out.data32[c], 0, n * sizeof(float)); else if (out.data64) std::memset(out.data64[c], 0, n * sizeof(double)); }
    return CLAP_PROCESS_SLEEP; }
  pl->eng->block(m, (int)n); if (stereo) pl->eng2->block(m2, (int)n);
  double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), ratio = dt / (n / pl->fs); pl->load = pl->load * 0.9 + ratio * 0.1;
  for (uint32_t c = 0; c < out.channel_count; c++) { const float *src = (c == 1 && stereo) ? m2 : m; if (out.data32) for (uint32_t i = 0; i < n; i++) out.data32[c][i] = (float)(src[i] * pl->outg); else if (out.data64) for (uint32_t i = 0; i < n; i++) out.data64[c][i] = src[i] * pl->outg; }
  if ((pl->pkCnt += n) >= pl->fs / 10) { pl->pkCnt = 0; double q = pl->eng->takePeak(); if (stereo) q = std::max(q, pl->eng2->takePeak()); pl->pk = q; }
  // Schlafen: Eingang still (< −120 dBFS) und Ausgang abgeklungen (< −100 dBFS) für 0,5 s (längste Zeitkonstante des Netzes: 47 µF-Koppelkondensatoren, unter 0,5 s)
  double outPk = 0; for (uint32_t i = 0; i < n; i++) outPk = std::max(outPk, std::max(std::fabs((double)m[i]), stereo ? std::fabs((double)m2[i]) : 0.0) * pl->outg);
  if (inPk < 1e-6f && outPk < 1e-5) pl->quiet += n; else pl->quiet = 0;
  if (pl->quiet >= (long)(0.5 * pl->fs)) { if (!pl->sleeping) { pl->sleeping = true; pl->load = 0; pl->pk = 0; } return CLAP_PROCESS_SLEEP; }
  return CLAP_PROCESS_CONTINUE;
}
// Hülle: Denormals abschalten (FTZ/DAZ, danach wiederherstellen); eine Ausnahme darf nie die C-Grenze zum Host überschreiten
static clap_process_status pl_process(const clap_plugin_t *p, const clap_process_t *pr) {
  unsigned csr = _mm_getcsr(); _mm_setcsr(csr | 0x8040); clap_process_status r;
  try { r = pl_process_impl(p, pr); } catch (...) { zeroOut(pr); r = CLAP_PROCESS_CONTINUE; }
  _mm_setcsr(csr); return r; }
// ---- Oberfläche (X11, eingebettet) ----
static void gui_setp(Plugin *pl, int id, double v) { v = std::max(PD[id].lo, std::min(PD[id].hi, v)); if (pl->val[id].load() == v) return; pl->val[id].store(v); pl->dirty = true;
  { std::lock_guard<std::mutex> lk(pl->qm); bool g = pl->gestId == id; if (!g) pl->guiQ.push_back({id, 0, 1}); pl->guiQ.push_back({id, v, 0}); if (!g) pl->guiQ.push_back({id, 0, 2}); }   // Geste, damit der Host (VST3: performEdit, Automation „Touch“) die Änderung zuordnet
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
// Ziehen an einem Knopf: eine Geste vom Drücken bis zum Loslassen
static void gui_gesture(Plugin *pl, int id, bool begin) { if (id < 0 || id >= P_COUNT) return;
  { std::lock_guard<std::mutex> lk(pl->qm); if (begin) { if (pl->gestId >= 0) pl->guiQ.push_back({pl->gestId, 0, 2}); pl->gestId = id; pl->guiQ.push_back({id, 0, 1}); } else if (pl->gestId == id) { pl->gestId = -1; pl->guiQ.push_back({id, 0, 2}); } else return; }
  auto *hp = (const clap_host_params_t*)pl->host->get_extension(pl->host, CLAP_EXT_PARAMS); if (hp) hp->request_flush(pl->host); }
static bool g_supported(const clap_plugin_t *, const char *api, bool floating) { return !std::strcmp(api, CLAP_WINDOW_API_X11) && !floating; }
static bool g_preferred(const clap_plugin_t *, const char **api, bool *floating) { *api = CLAP_WINDOW_API_X11; *floating = false; return true; }
static bool g_create(const clap_plugin_t *p, const char *api, bool floating) { Plugin *pl = P(p); if (std::strcmp(api, CLAP_WINDOW_API_X11) || floating) return false;
  { std::lock_guard<std::recursive_mutex> glk(pl->gm);
  if (!pl->dpy) pl->dpy = tsx::open(); if (!pl->dpy) return false; if (!pl->gui) { pl->gui = new ygui::Gui(); pl->gui->getP = [pl](int i) { return pl->val[i].load(); }; pl->gui->setP = [pl](int i, double v) { gui_setp(pl, i, v); }; pl->gui->gesture = [pl](int i, bool b) { gui_gesture(pl, i, b); }; pl->gui->diag = [pl]() { return pl->diagTxt; };
    pl->gui->computeCurves = [pl](std::vector<double> &cur, std::vector<double> &ref) {   // Kleinsignal-Frequenzgang wie im Web-Simulator (ohne Endstufe)
      YSetting t = pl->target(); t.pwr = false; double fs = pl->fs; YParts v = YParts::orig();
      auto curveOf = [&](const YSetting &s, std::vector<double> &out) { LinNet o = netY(v, s, fs); double r = ymag(o, 1000, s); out.resize(ygui::NF); for (int i = 0; i < ygui::NF; i++) out[i] = 20 * std::log10(std::max(1e-12, ymag(o, ygui::freqAt(i), s) / r)); };
      curveOf(t, cur);
      const double flat[5] = {5, 5, 5, 5, 5}; YSetting rs = pl->setting(10, 10, 10, 10, flat, t.ing < 1, false, t.spk); curveOf(rs, ref); }; }
  pl->guiH = pl->guiW * ygui::TOTALH / YFW;
  }   // Timer außerhalb der Sperre anmelden (der Host kann darin schon einen Takt auslösen)
  auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId == CLAP_INVALID_ID) ht->register_timer(pl->host, 33, &pl->timerId);
  return true; }
static void g_destroy(const clap_plugin_t *p) { Plugin *pl = P(p); if (pl->gestId >= 0) gui_gesture(pl, pl->gestId, false); auto *ht = (const clap_host_timer_support_t*)pl->host->get_extension(pl->host, CLAP_EXT_TIMER_SUPPORT); if (ht && pl->timerId != CLAP_INVALID_ID) { ht->unregister_timer(pl->host, pl->timerId); pl->timerId = CLAP_INVALID_ID; }
  std::lock_guard<std::recursive_mutex> glk(pl->gm); if (pl->gui) { pl->gui->destroy(); delete pl->gui; pl->gui = nullptr; } if (pl->dpy) { XSync(pl->dpy, False); } }
static void upd_diag(Plugin *pl) { char b[160]; std::snprintf(b, sizeof b, "CPU %.0f %% of one core · %.0f Hz · stereo%s", pl->load.load() * 100, pl->fs, pl->sleeping ? " · idle (silent input)" : ""); pl->diagTxt = b; }
static bool g_set_scale(const clap_plugin_t *, double) { return false; }
static bool g_get_size(const clap_plugin_t *p, uint32_t *w, uint32_t *h) { Plugin *pl = P(p); *w = (uint32_t)std::lround(pl->guiW); *h = (uint32_t)std::lround(pl->guiH); return true; }
static bool g_can_resize(const clap_plugin_t *) { return true; }
static bool g_resize_hints(const clap_plugin_t *, clap_gui_resize_hints_t *h) { h->can_resize_horizontally = true; h->can_resize_vertically = true; h->preserve_aspect_ratio = true; h->aspect_ratio_width = (uint32_t)YFW; h->aspect_ratio_height = (uint32_t)ygui::TOTALH; return true; }
static bool g_adjust_size(const clap_plugin_t *, uint32_t *w, uint32_t *h) { double ww = std::max(600.0, std::min(2600.0, (double)*w)); *w = (uint32_t)std::lround(ww); *h = (uint32_t)std::lround(ww * ygui::TOTALH / YFW); return true; }
static bool g_set_size(const clap_plugin_t *p, uint32_t w, uint32_t h) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); pl->guiW = w; pl->guiH = h; if (pl->gui && pl->gui->win) { pl->gui->resize((int)w, (int)h); pl->gui->draw(); } return true; }
static bool g_set_parent(const clap_plugin_t *p, const clap_window_t *win) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui || !pl->dpy) return false; if (pl->gui->win) pl->gui->destroy(); return pl->gui->init(pl->dpy, (Window)win->x11, (int)std::lround(pl->guiW), (int)std::lround(pl->guiH)); }
static bool g_set_transient(const clap_plugin_t *, const clap_window_t *) { return false; }
static void g_suggest_title(const clap_plugin_t *, const char *) {}
static bool g_show(const clap_plugin_t *p) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->show(); pl->gui->draw(); return true; }
static bool g_hide(const clap_plugin_t *p) { Plugin *pl = P(p); std::lock_guard<std::recursive_mutex> glk(pl->gm); if (!pl->gui) return false; pl->gui->hide(); return true; }
static const clap_plugin_gui_t EXT_GUI = {g_supported, g_preferred, g_create, g_destroy, g_set_scale, g_get_size, g_can_resize, g_resize_hints, g_adjust_size, g_set_size, g_set_parent, g_set_transient, g_suggest_title, g_show, g_hide};
static uint32_t g_latency(const clap_plugin_t *) { return 0; }   // keine Überabtastung, keine Filterlaufzeit
static const clap_plugin_latency_t EXT_LATENCY = {g_latency};
static void on_timer(const clap_plugin_t *p, clap_id) { Plugin *pl = P(p); std::unique_lock<std::recursive_mutex> glk(pl->gm, std::try_to_lock); if (!glk.owns_lock()) return;
  try { if (pl->gui) { upd_diag(pl); pl->gui->meterOn = pl->val[P_PWR].load() > 0.5 && !pl->sleeping; pl->gui->meter = std::max(pl->pk.load(), pl->gui->meter * 0.85); pl->gui->pump(); } } catch (...) {} }
static const clap_plugin_timer_support_t EXT_TIMER = {on_timer};

static const void *pl_ext(const clap_plugin_t *, const char *id) {
  if (!std::strcmp(id, CLAP_EXT_GUI)) return &EXT_GUI; if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &EXT_TIMER;
  if (!std::strcmp(id, CLAP_EXT_LATENCY)) return &EXT_LATENCY; if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &EXT_PARAMS; if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &EXT_PORTS; if (!std::strcmp(id, CLAP_EXT_STATE)) return &EXT_STATE; return nullptr; }
static void pl_main(const clap_plugin_t *) {}
static const char *FEATURES[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_EQUALIZER, CLAP_PLUGIN_FEATURE_STEREO, nullptr};
static const clap_plugin_descriptor_t DESC = {CLAP_VERSION_INIT, "de.tonestack.yamaha-b100ii", "Yamaha B100-II Bass Amp", "tonestack", "", "", "", "1.4", "PA board (tone controls, volume, 5-band EQ) from the 1979 service manual, with MA board power amp clipping and 15\" speaker", FEATURES};
static const clap_plugin_t *create(const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
  if (std::strcmp(id, DESC.id)) return nullptr; Plugin *pl = new Plugin(); pl->host = host; for (int i = 0; i < P_COUNT; i++) pl->val[i].store(PD[i].def);
  pl->plug = {&DESC, pl, pl_init, pl_destroy, pl_activate, pl_deactivate, pl_start, pl_stop, pl_reset, pl_process, pl_ext, pl_main}; return &pl->plug; }
static uint32_t fac_count(const clap_plugin_factory_t *) { return 1; }
static const clap_plugin_descriptor_t *fac_desc(const clap_plugin_factory_t *, uint32_t i) { return i == 0 ? &DESC : nullptr; }
static const clap_plugin_factory_t FACTORY = {fac_count, fac_desc, create};
static bool entry_init(const char *) { return true; } static void entry_deinit() {}
static const void *entry_factory(const char *id) { return std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &FACTORY; }
#ifdef TONESTACK_CLAP_FIRST   // VST3 über clap-wrapper (engine/vst3): Einstieg als normale Funktionen, clap_entry legt der Wrapper an
extern "C" bool yamaha_entry_init(const char *p) { return entry_init(p); }
extern "C" void yamaha_entry_deinit() { entry_deinit(); }
extern "C" const void *yamaha_entry_factory(const char *id) { return entry_factory(id); }
#else
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, entry_init, entry_deinit, entry_factory};
#endif
