// Echtzeit-Prüfung eines CLAP-Plugins wie in einem Host (Linux und macOS, nativ und unter Rosetta):
//   rtcheck plugin.clap [Sekunden je Phase, Standard 20] [Blockgröße, Standard 128]
// Misst je Phase die Rechenzeit jedes Blocks relativ zur Blockdauer (100 % = ein Kern voll) und meldet Mittel, 99,9-Perzentil,
// Maximum, Blöcke über 100 % und nicht endliche Ausgaben; dazu die Dauer von init/activate/deactivate/destroy (Haupt-Thread).
// Phasen: Grundstellung, Stereo verschieden, Zufalls-Automation aller Parameter (mit Pausen: Schlaf/Aufwachen),
// Transportlücken (Logic/REAPER rufen process bei Stillstand nicht auf → Neubeginn), Ein/Aus (stop/start_processing),
// Denormal-Abklingen (Ton, dann exakte Nullen bzw. winzige Werte), Zustand laden/speichern im Haupt-Thread während der Audio-Thread rechnet,
// Wechsel der Abtastrate (44,1/96/192 kHz). Ausgabe: Zeilen "RESULT ..." (für CI-Hinweise) und am Ende "WORST ...".
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <time.h>
#include <vector>
using Clock = std::chrono::steady_clock;
static double secsSince(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }
// Rechenzeit des eigenen Threads (ohne Zeiten, in denen das System den Thread verdrängt hat): trennt Plugin-Arbeit von Aussetzern der (virtuellen) Maschine
static double threadCpu() { timespec ts; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }

static std::atomic<int> flushReq{0};
static void req_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void req_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static void req_flush(const clap_host_t *) { flushReq++; }
static const clap_host_params_t HP = {req_rescan, req_clear, req_flush};
static bool reg_timer(const clap_host_t *, uint32_t, clap_id *id) { *id = 7; return true; } static bool unreg_timer(const clap_host_t *, clap_id) { return true; }
static const clap_host_timer_support_t HT = {reg_timer, unreg_timer};
static const void *host_ext(const clap_host_t *, const char *id) { if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &HP; if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &HT; return nullptr; }
static void host_nop(const clap_host_t *) {}
static std::atomic<bool> cbReq{false}; static void host_cb(const clap_host_t *) { cbReq = true; }   // request_callback: nach dem Block on_main_thread (wie ein Host im Hauptfaden)

static std::vector<clap_event_param_value_t> Q;
static uint32_t q_size(const clap_input_events_t *) { return (uint32_t)Q.size(); }
static const clap_event_header_t *q_get(const clap_input_events_t *, uint32_t i) { return &Q[i].header; }
static bool out_push(const clap_output_events_t *, const clap_event_header_t *) { return true; }
static clap_event_param_value_t ev(uint32_t id, double v, uint32_t t = 0) { clap_event_param_value_t e{}; e.header.size = sizeof e; e.header.time = t; e.header.space_id = CLAP_CORE_EVENT_SPACE_ID; e.header.type = CLAP_EVENT_PARAM_VALUE; e.param_id = id; e.value = v; e.note_id = -1; e.port_index = -1; e.channel = -1; e.key = -1; return e; }

struct OStream { std::string s; clap_ostream_t os; OStream() { os.ctx = this; os.write = [](const clap_ostream_t *o, const void *b, uint64_t n) -> int64_t { ((OStream*)o->ctx)->s.append((const char*)b, n); return (int64_t)n; }; } };
struct IStream { std::string s; size_t p = 0; clap_istream_t is; IStream(std::string x) : s(std::move(x)) { is.ctx = this; is.read = [](const clap_istream_t *i, void *b, uint64_t n) -> int64_t { IStream *t = (IStream*)i->ctx; size_t k = std::min<size_t>(n, t->s.size() - t->p); std::memcpy(b, t->s.data() + t->p, k); t->p += k; return (int64_t)k; }; } };

struct Stat { std::vector<double> r, rc; int nan = 0; double worstMs = 0, worstCpuMs = 0; std::string worstWhy;
  void add(double ratio, double ms, const std::string &why) { r.push_back(ratio); if (ms > worstMs) { worstMs = ms; worstWhy = "Block " + std::to_string(r.size() - 1) + ": " + why; } } };
// SPIKELOG=<Faktor>: Blöcke über diesem Vielfachen der Echtzeit mit Parametern und den Änderungen davor ausgeben (1 → 3-fach)
static double spikeLim() { const char *e = std::getenv("SPIKELOG"); double v = e ? std::atof(e) : 0; return v > 1 ? v : 3; }
static unsigned rs = 20261103;
static double urand() { rs = rs * 1664525u + 1013904223u; return (rs >> 8) / 16777216.0; }

struct Host {
  void *lib = nullptr; const clap_plugin_entry_t *entry = nullptr; const clap_plugin_t *p = nullptr; const clap_plugin_params_t *par = nullptr; const clap_plugin_state_t *st = nullptr;
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "rtcheck", "tonestack", "", "1", host_ext, host_nop, host_nop, host_cb};
  double fs = 48000; int B = 128; std::vector<clap_param_info_t> info; std::string name;
  std::vector<float> L, R, OL, OR; long pos = 0; bool asleep = false;
  bool open(const char *path) {
    lib = dlopen(path, RTLD_NOW | RTLD_LOCAL); if (!lib) { std::printf("dlopen: %s\n", dlerror()); return false; }
    entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); if (!entry || !entry->init(path)) { std::printf("clap_entry fehlt\n"); return false; }
    auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0); name = std::string(d->name) + " " + d->version;
    auto t0 = Clock::now(); p = fac->create_plugin(fac, &host, d->id); bool ok = p && p->init(p); report("init", secsSince(t0)); if (!ok) return false;
    par = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); st = (const clap_plugin_state_t*)p->get_extension(p, CLAP_EXT_STATE);
    for (uint32_t i = 0; i < par->count(p); i++) { clap_param_info_t in; par->get_info(p, i, &in); info.push_back(in); }
    return true; }
  void report(const char *what, double s) { std::printf("RESULT kind=main what=%s ms=%.1f\n", what, s * 1000); std::fflush(stdout); }
  void activate(double rate) { fs = rate; auto t0 = Clock::now(); p->activate(p, fs, 32, 4096); char w[48]; std::snprintf(w, sizeof w, "activate@%.0f", fs); report(w, secsSince(t0)); p->start_processing(p);
    L.assign(4096, 0); R.assign(4096, 0); OL.assign(4096, 0); OR.assign(4096, 0); }
  void deactivate() { p->stop_processing(p); auto t0 = Clock::now(); p->deactivate(p); report("deactivate", secsSince(t0)); }
  void dumpParams() { std::string t; for (auto &pi : info) { double v; par->get_value(p, pi.id, &v); char b[300]; std::snprintf(b, sizeof b, "%u=%g ", pi.id, v); t += b; } std::printf("  params: %s\n", t.c_str()); }
  // ein Block; Signal: f(Zeit) für links, rechts = links (same) oder phasenverschoben
  template <class F> clap_process_status block(int n, F sig, bool same, Stat &S, int ch = 2) {
    for (int i = 0; i < n; i++) { double t = (pos + i) / fs; double a, b; sig(t, a, b); L[i] = (float)a; R[i] = same ? (float)a : (float)b; }
    float *ins[2] = {L.data(), R.data()}, *outs[2] = {OL.data(), OR.data()};
    clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = ch; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = ch;
    clap_input_events_t ie = {nullptr, q_size, q_get}; clap_output_events_t oe = {nullptr, out_push};
    clap_process_t pr{}; pr.frames_count = n; pr.steady_time = pos; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
    std::string why; for (auto &e : Q) { char b[300]; std::snprintf(b, sizeof b, "%s=%g ", info[e.param_id].name, e.value); why += b; }
    auto t0 = Clock::now(); double c0 = threadCpu(); clap_process_status r = p->process(p, &pr); double dt = secsSince(t0), dc = threadCpu() - c0;
    S.rc.push_back(dc / (n / fs)); S.worstCpuMs = std::max(S.worstCpuMs, dc * 1000);
    S.add(dt / (n / fs), dt * 1000, why.empty() ? (asleep ? "(schlief)" : "-") : why); asleep = r == CLAP_PROCESS_SLEEP;
    if (std::getenv("SPIKELOG") && dt / (n / fs) > spikeLim()) { std::printf("SPIKE2 pos=%.3fs ratio=%.0f%% ms=%.2f cpu_ms=%.2f status=%d\n", pos / fs, dt / (n / fs) * 100, dt * 1000, dc * 1000, (int)r); dumpParams(); }
    for (int c = 0; c < ch; c++) for (int i = 0; i < n; i++) if (!std::isfinite((c ? OR : OL)[i])) { S.nan++; break; }
    Q.clear(); pos += n; if (cbReq.exchange(false)) p->on_main_thread(p); return r; }
};
static double bass(double t) { double env = 0.5 * std::exp(-2.5 * std::fmod(t, 0.8)); return env * (std::sin(2 * M_PI * 55 * t) + 0.3 * std::sin(2 * M_PI * 110 * t + 0.4)); }
static Stat worstAll; static std::string worstPhase; static double worstMax = 0;
static void summary(const char *phase, Stat &S, double fs) {
  if (S.r.empty()) return; std::vector<double> v = S.r; std::sort(v.begin(), v.end()); double mean = 0; for (double x : v) mean += x; mean /= v.size();
  double p999 = v[std::min(v.size() - 1, (size_t)(v.size() * 0.999))]; int over = 0; for (double x : v) if (x > 1) over++;
  std::vector<double> vc = S.rc; std::sort(vc.begin(), vc.end()); int overc = 0; for (double x : vc) if (x > 1) overc++;
  std::printf("RESULT kind=rt phase=%s fs=%.0f blocks=%zu mean=%.1f p999=%.1f max=%.1f over100=%d cpu_max=%.1f cpu_over100=%d nan=%d worst_ms=%.2f worst_cpu_ms=%.2f worst_at=\"%s\"\n", phase, fs, v.size(), mean * 100, p999 * 100, v.back() * 100, over,
    vc.empty() ? 0 : vc.back() * 100, overc, S.nan, S.worstMs, S.worstCpuMs, S.worstWhy.c_str());
  std::fflush(stdout); if (v.back() > worstMax) { worstMax = v.back(); worstPhase = phase; worstAll = S; } }

int main(int argc, char **argv) {
  if (argc < 2) { std::printf("rtcheck plugin.clap [Sekunden] [Block]\n"); return 2; }
  Host H; if (!H.open(argv[1])) return 1; double secs = argc > 2 ? std::atof(argv[2]) : 20; H.B = argc > 3 ? std::atoi(argv[3]) : 128;
  std::printf("Plugin: %s, %zu Parameter, Block %d\n", H.name.c_str(), H.info.size(), H.B);
  auto nBlocks = [&](double s) { return (long)(s * H.fs / H.B); };
  auto sigBass = [](double t, double &a, double &b) { a = bass(t); b = bass(t + 0.0007); };
  H.activate(48000);
  { Stat S; for (long k = 0; k < nBlocks(secs / 2); k++) H.block(H.B, sigBass, true, S); summary("grundstellung_mono_gleich", S, H.fs); }
  { Stat S; for (long k = 0; k < nBlocks(secs / 2); k++) H.block(H.B, sigBass, false, S); summary("stereo_verschieden", S, H.fs); }
  // Zufalls-Automation: alle sichtbaren Parameter, Stufenparameter ganzzahlig, alle 20–300 ms ein bis drei Änderungen; Signal mit Pausen (Schlaf, Aufwachen)
  { Stat S; long n = nBlocks(secs * 3); long next = 0; std::vector<std::pair<long, clap_event_param_value_t>> recent; std::map<std::string, int> spikeBy;
    for (long k = 0; k < n; k++) {
      if (k >= next) { int m = 1 + (int)(urand() * 3); for (int j = 0; j < m; j++) { const clap_param_info_t &pi = H.info[(size_t)(urand() * H.info.size())]; if (pi.flags & CLAP_PARAM_IS_HIDDEN) continue;
          double v = pi.min_value + urand() * (pi.max_value - pi.min_value); if (pi.flags & CLAP_PARAM_IS_STEPPED) v = std::round(v); Q.push_back(ev(pi.id, v, (uint32_t)(urand() * H.B))); }
        std::sort(Q.begin(), Q.end(), [](auto &a, auto &b) { return a.header.time < b.header.time; }); next = k + (long)((0.02 + urand() * 0.28) * H.fs / H.B); }
      bool pause = (k / nBlocks(4)) % 3 == 2;   // jede dritte 4-s-Strecke still: Schlaf nach 2 s, danach Aufwachen
      std::vector<clap_event_param_value_t> sent = Q;
      H.block(H.B, [&](double t, double &a, double &b) { a = pause ? 0 : bass(t); b = pause ? 0 : bass(t + 0.0007); }, (k / nBlocks(7)) % 2 == 0, S);
      for (auto &e : sent) recent.push_back({k, e});
      while (!recent.empty() && recent.front().first < k - (long)(0.2 * H.fs / H.B)) recent.erase(recent.begin());
      if (std::getenv("SPIKELOG") && S.r.back() > spikeLim()) { std::string t; for (auto &r : recent) { char b[340]; std::snprintf(b, sizeof b, "[-%ld] %s=%g; ", k - r.first, H.info[r.second.param_id].name, r.second.value); t += b; }
        std::printf("SPIKE block=%ld ratio=%.0f%% ms=%.2f pause=%d after: %s\n", k, S.r.back() * 100, S.r.back() * H.B / H.fs * 1000, (int)pause, t.c_str()); }
      if (S.r.back() > 1.0) { for (auto &r : recent) { char b[300]; std::snprintf(b, sizeof b, "%s", H.info[r.second.param_id].name); spikeBy[b]++; } if (recent.empty()) spikeBy["(keine Änderung in 200 ms)"]++; } }
    summary("zufall_automation", S, H.fs);
    std::vector<std::pair<int, std::string>> top; for (auto &x : spikeBy) top.push_back({x.second, x.first}); std::sort(top.rbegin(), top.rend());
    std::string t; for (size_t i = 0; i < top.size() && i < 8; i++) { char b[340]; std::snprintf(b, sizeof b, "%s:%d ", top[i].second.c_str(), top[i].first); t += b; }
    std::printf("RESULT kind=spikes blocks_over100_preceded_by=\"%s\"\n", t.c_str()); }
  // Transportlücken: 1,5 s spielen, 0,35 s keine Aufrufe (Wanduhr), weiter; wie Logic bei Stopp/Start oder REAPER-Spur ohne Wiedergabe
  { Stat S, F; for (int r = 0; r < std::max(4, (int)(secs / 2)); r++) { for (long k = 0; k < nBlocks(1.5); k++) { Stat &T = k == 0 && r > 0 ? F : S; H.block(H.B, sigBass, true, T); }
      if (std::getenv("SPIKELOG")) { std::printf("GAP at %.3fs\n", H.pos / H.fs); std::fflush(stdout); }
      std::this_thread::sleep_for(std::chrono::milliseconds(350)); }
    summary("transport_laufend", S, H.fs); summary("transport_erster_block_nach_luecke", F, H.fs); }
  // Ein/Aus wie REAPER-FX-Schalter: stop/start_processing (Haupt-Thread-Seite hier im selben Thread, wie bei den meisten Hosts zwischen Blöcken)
  { Stat S, F; for (int r = 0; r < 8; r++) { H.p->stop_processing(H.p); std::this_thread::sleep_for(std::chrono::milliseconds(50)); H.p->start_processing(H.p);
      for (long k = 0; k < nBlocks(0.5); k++) H.block(H.B, sigBass, true, k == 0 ? F : S); }
    summary("ein_aus_laufend", S, H.fs); summary("ein_aus_erster_block", F, H.fs); }
  // Denormals: 1 s Ton, 1,9 s exakte Nullen (vor dem Einschlafen nach 2 s), 1 s Ton, 1,9 s winzige Werte (±1e-30, unter der Schlafschwelle)
  { Stat T, Z, W; for (int r = 0; r < 3; r++) {
      for (long k = 0; k < nBlocks(1); k++) H.block(H.B, sigBass, true, T);
      for (long k = 0; k < nBlocks(1.9); k++) H.block(H.B, [](double, double &a, double &b) { a = b = 0; }, true, Z);
      for (long k = 0; k < nBlocks(1); k++) H.block(H.B, sigBass, true, T);
      for (long k = 0; k < nBlocks(1.9); k++) H.block(H.B, [&](double, double &a, double &b) { a = b = (k & 1 ? 1e-30 : -1e-30); }, true, W); }
    summary("denormal_ton", T, H.fs); summary("denormal_nullen", Z, H.fs); summary("denormal_winzig", W, H.fs);
    auto mean = [](Stat &s) { double m = 0; for (double x : s.r) m += x; return m / std::max<size_t>(1, s.r.size()); };
    std::printf("RESULT kind=denorm ratio_nullen=%.2f ratio_winzig=%.2f\n", mean(Z) / mean(T), mean(W) / mean(T)); }
  // Zustand speichern/laden im Haupt-Thread, während der Audio-Thread rechnet (CLAP erlaubt beides gleichzeitig)
  { Stat S; std::atomic<bool> stop{false}; std::atomic<long> loads{0}; std::mutex m; double maxLoad = 0;
    std::thread mainT([&]() { while (!stop) { OStream o; H.st->save(H.p, &o.os); std::string s = o.s;
        // jede zweite Runde zufällige Werte in den Zustand (wie ein anderes Preset)
        if (loads % 2) { std::string t; for (auto &pi : H.info) { if (pi.flags & CLAP_PARAM_IS_HIDDEN) continue; double v = pi.min_value + urand() * (pi.max_value - pi.min_value); if (pi.flags & CLAP_PARAM_IS_STEPPED) v = std::round(v); char b[64]; std::snprintf(b, sizeof b, "%u=%.17g\n", pi.id, v); t += b; }
          if (s.find('=') != std::string::npos) s = t; }   // nur Textzustände (alle drei Plugins: id=wert je Zeile)
        IStream i(s); auto t0 = Clock::now(); H.st->load(H.p, &i.is); double d = secsSince(t0); { std::lock_guard<std::mutex> lk(m); maxLoad = std::max(maxLoad, d); } loads++;
        char txt[64]; for (auto &pi : H.info) { double v; H.par->get_value(H.p, pi.id, &v); H.par->value_to_text(H.p, pi.id, v, txt, sizeof txt); }
        std::this_thread::sleep_for(std::chrono::milliseconds(150)); } });
    for (long k = 0; k < nBlocks(secs); k++) H.block(H.B, sigBass, true, S);
    stop = true; mainT.join(); summary("zustand_laden_parallel", S, H.fs); std::printf("RESULT kind=main what=state_load_max ms=%.1f loads=%ld\n", maxLoad * 1000, loads.load()); }
  H.deactivate();
  for (double r : {44100.0, 96000.0, 192000.0, 48000.0}) { H.activate(r); Stat S; long n = nBlocks(secs / 4), next = 0;
    for (long k = 0; k < n; k++) { if (k >= next) { const clap_param_info_t &pi = H.info[(size_t)(urand() * H.info.size())]; if (!(pi.flags & CLAP_PARAM_IS_HIDDEN)) { double v = pi.min_value + urand() * (pi.max_value - pi.min_value); if (pi.flags & CLAP_PARAM_IS_STEPPED) v = std::round(v); Q.push_back(ev(pi.id, v)); } next = k + (long)(0.25 * H.fs / H.B); }
      H.block(H.B, sigBass, true, S); }
    char ph[48]; std::snprintf(ph, sizeof ph, "rate_%.0f", r); summary(ph, S, H.fs); H.deactivate(); }
  // Abbau direkt nach dem Aktivieren (Hintergrund-Thread läuft evtl. noch): darf nicht hängen oder abstürzen
  { auto t0 = Clock::now(); H.p->activate(H.p, 96000, 32, 4096); H.p->deactivate(H.p); H.report("activate+deactivate_sofort@96000", secsSince(t0)); }
  { auto t0 = Clock::now(); H.p->destroy(H.p); H.report("destroy", secsSince(t0)); }
  H.entry->deinit();
  std::printf("WORST phase=%s max=%.1f%% worst_ms=%.2f at=\"%s\"\n", worstPhase.c_str(), worstMax * 100, worstAll.worstMs, worstAll.worstWhy.c_str());
  return 0; }
