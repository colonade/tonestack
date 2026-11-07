// Prüfstand für die CLAP-Plugins (wie ein Host, ohne Oberfläche).
//   plugtest PLUGIN render in.f32 out.f32 "id=wert,..." [fs] [block]   Datei durchrechnen (mono f32 → links f32); block "r256" = zufällige Blockgrößen 1…256
//                                                                     HONOR=1: bei SLEEP stille Blöcke nicht aufrufen (wie ein sparsamer Host)
//   plugtest PLUGIN perf "id=wert,..." [fs] [block]                    Rechenlast: 10 s Bass-Signal, Mittel und längster Block; danach jeder Parameter
//                                                                     während der Wiedergabe umgeschaltet: längster Block je Parameter
//                                                                     DUMP=datei: Dauer jedes Blocks (Position, ms) dorthin schreiben
//   plugtest PLUGIN params                                            Parameterliste
// Ausgaben auf stdout; Rückgabe 0 = gelaufen (Bewertung macht das Auswerteskript).
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

static std::vector<clap_event_param_value_t> EVS;
static uint32_t ev_size(const clap_input_events_t *) { return (uint32_t)EVS.size(); }
static const clap_event_header_t *ev_get(const clap_input_events_t *, uint32_t i) { return &EVS[i].header; }
static bool out_push(const clap_output_events_t *, const clap_event_header_t *) { return true; }
static const void *host_ext(const clap_host_t *, const char *) { return nullptr; }
static void host_nop(const clap_host_t *) {}
static void addParam(uint32_t id, double v) { clap_event_param_value_t e{}; e.header.size = sizeof e; e.header.type = CLAP_EVENT_PARAM_VALUE; e.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  e.param_id = id; e.value = v; e.note_id = -1; e.port_index = -1; e.channel = -1; e.key = -1; EVS.push_back(e); }
static void addParams(const char *spec) { if (!spec || !*spec) return; std::string s(spec); size_t p = 0;
  while (p < s.size()) { size_t c = s.find(',', p); if (c == std::string::npos) c = s.size(); std::string t = s.substr(p, c - p); size_t q = t.find('='); if (q != std::string::npos) addParam((uint32_t)atoi(t.c_str()), atof(t.c_str() + q + 1)); p = c + 1; } }

// Parameter vor dem Aktivieren über den Zustand setzen (ab 2026-11-07): Einstellungen, die den Aufbau ändern (z. B. Netzteil des SVT-CL), wirken sonst erst
// nach einem Rückruf im Hauptfaden, den dieser Prüf-Host nicht gibt. Format „id=wert“ je Zeile wie st_save der Plugins; nicht genannte bleiben auf dem Standard.
struct MemIn { std::string d; size_t pos = 0; };
static int64_t memRead(const clap_istream_t *st, void *buf, uint64_t n) { MemIn *m = (MemIn*)st->ctx; uint64_t k = std::min<uint64_t>(n, m->d.size() - m->pos); std::memcpy(buf, m->d.data() + m->pos, k); m->pos += k; return (int64_t)k; }
static void stateParams(const clap_plugin_t *p, const std::string &spec) { if (spec.empty()) return; auto *st = (const clap_plugin_state_t*)p->get_extension(p, CLAP_EXT_STATE); if (!st) return;
  MemIn m; size_t q = 0; while (q < spec.size()) { size_t c = spec.find(',', q); if (c == std::string::npos) c = spec.size(); m.d += spec.substr(q, c - q) + "\n"; q = c + 1; }
  clap_istream_t is{&m, memRead}; st->load(p, &is); }
struct Host {
  void *lib = nullptr; const clap_plugin_entry_t *entry = nullptr; const clap_plugin_t *p = nullptr; const clap_plugin_params_t *params = nullptr; const clap_plugin_latency_t *lat = nullptr;
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "plugtest", "", "", "1", host_ext, host_nop, host_nop, host_nop};
  std::vector<float> l, r, ol, orr; double fs = 48000;
  bool open(const char *path) { lib = dlopen(path, RTLD_NOW); if (!lib) { printf("dlopen: %s\n", dlerror()); return false; }
    entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(path); auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0); p = fac->create_plugin(fac, &host, d->id); p->init(p);
    params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); lat = (const clap_plugin_latency_t*)p->get_extension(p, CLAP_EXT_LATENCY);
    fprintf(stderr, "Plugin: %s %s\n", d->name, d->version); return true; }
  void activate(double f, uint32_t maxb) { fs = f; p->activate(p, fs, 1, maxb); p->start_processing(p); l.assign(maxb, 0); r.assign(maxb, 0); ol.assign(maxb, 0); orr.assign(maxb, 0); }
  // einen Block rechnen; liefert Rückgabe und Dauer in s
  const float *xr = nullptr; float *yr = nullptr;   // rechter Kanal getrennt (sonst wie links)
  clap_process_status run(const float *x, float *y, int m, double &dt) {
    for (int i = 0; i < m; i++) { l[i] = x ? x[i] : 0.f; r[i] = xr ? xr[i] : l[i]; }
    float *ins[2] = {l.data(), r.data()}, *outs[2] = {ol.data(), orr.data()};
    clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
    clap_input_events_t in_ev = {nullptr, ev_size, ev_get}; clap_output_events_t out_ev = {nullptr, out_push};
    clap_process_t pr{}; pr.frames_count = (uint32_t)m; pr.steady_time = -1; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &in_ev; pr.out_events = &out_ev;
    auto t0 = std::chrono::steady_clock::now(); clap_process_status s = p->process(p, &pr); dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    EVS.clear(); if (y) for (int i = 0; i < m; i++) y[i] = ol[i]; if (yr) for (int i = 0; i < m; i++) yr[i] = orr[i]; return s; }
  void close() { p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); }
};

static std::vector<float> bassSignal(double fs, double secs) {   // gezupfte Bass-Töne mit Obertönen, laut
  std::vector<float> x((size_t)(fs * secs)); const double notes[] = {41.2, 55, 73.4, 98, 61.7, 82.4, 110, 146.8}; double ph = 0;
  for (size_t i = 0; i < x.size(); i++) { double t = i / fs; int k = (int)(t / 0.5); double f = notes[k % 8], e = std::exp(-(t - k * 0.5) * 4); ph += 2 * M_PI * f / fs;
    double s = 0; for (int h = 1; h < 7; h++) s += std::sin(h * ph) / h; x[i] = (float)(0.6 * e * s); }
  return x; }

int main(int argc, char **argv) {
  if (argc < 3) { printf("plugtest PLUGIN render|perf|params ...\n"); return 1; }
  Host H; if (!H.open(argv[1])) return 1; std::string cmd = argv[2];
  if (cmd == "params") { char txt[128]; for (uint32_t i = 0; i < H.params->count(H.p); i++) { clap_param_info_t inf; H.params->get_info(H.p, i, &inf); printf("%u\t%s\t%g\t%g\t%g\n", inf.id, inf.name, inf.min_value, inf.max_value, inf.default_value); (void)txt; } return 0; }
  if (cmd == "render") {
    const char *spec = argc > 5 ? argv[5] : ""; double fs = argc > 6 ? atof(argv[6]) : 48000; std::string bs = argc > 7 ? argv[7] : "256";
    bool rnd = bs[0] == 'r'; int B = atoi(bs.c_str() + (rnd ? 1 : 0)); if (B < 1) B = 256;
    FILE *fi = fopen(argv[3], "rb"); if (!fi) return 2; fseek(fi, 0, SEEK_END); long n = ftell(fi) / 4; fseek(fi, 0, SEEK_SET); std::vector<float> x(n); if (fread(x.data(), 4, n, fi) != (size_t)n) return 2; fclose(fi);
    stateParams(H.p, spec); H.activate(fs, (uint32_t)B); addParams(spec);
    std::vector<float> xR(n, 0.f), yR(n, 0.f); bool hasR = getenv("INR") != nullptr;
    if (hasR) { FILE *f2 = fopen(getenv("INR"), "rb"); if (!f2 || fread(xR.data(), 4, n, f2) != (size_t)n) return 2; fclose(f2); }
    std::vector<float> y(n, 0.f); std::mt19937 rng(7); bool asleep = false; long skipped = 0, sleeps = 0, calls = 0; double tmax = 0, tsum = 0;
    for (long pos = 0; pos < n;) { int m = rnd ? (int)(1 + rng() % B) : B; m = (int)std::min<long>(m, n - pos);
      if (asleep && getenv("HONOR")) { bool silent = true; for (int i = 0; i < m; i++) if (x[pos + i] != 0) silent = false; if (silent) { skipped++; pos += m; continue; } asleep = false; }
      if (getenv("EVENTS")) { std::string ev = getenv("EVENTS"); size_t q = 0; while (q < ev.size()) { size_t e = ev.find(';', q); if (e == std::string::npos) e = ev.size(); std::string t = ev.substr(q, e - q); size_t c = t.find(':');
        if (c != std::string::npos) { long at = (long)(atof(t.c_str()) * fs); if (at >= pos && at < pos + m) addParams(t.c_str() + c + 1); } q = e + 1; } }   // Parameteränderungen zu Zeitpunkten: "t:id=v,...;t:..."
      H.xr = hasR ? &xR[pos] : nullptr; H.yr = &yR[pos]; double dt; clap_process_status s = H.run(&x[pos], &y[pos], m, dt); calls++; tsum += dt; tmax = std::max(tmax, dt); if (getenv("TRACE") && dt > m / fs) fprintf(stderr, "  langsam: Block bei %.3f s (%d Werte) %.2f ms = %.0f%% %s\n", pos / fs, m, dt * 1e3, 100 * dt / (m / fs), asleep ? "(nach Schlaf)" : ""); if (s == CLAP_PROCESS_SLEEP) { asleep = true; sleeps++; } pos += m; }
    FILE *fo = fopen(argv[4], "wb"); fwrite(y.data(), 4, n, fo); fclose(fo);
    if (getenv("OUTR")) { FILE *f3 = fopen(getenv("OUTR"), "wb"); fwrite(yR.data(), 4, n, f3); fclose(f3); }
    uint32_t L = H.lat ? H.lat->get(H.p) : 0;
    printf("latenz_gemeldet=%u aufrufe=%ld sleep=%ld uebersprungen=%ld last=%.4f max_block_ms=%.3f\n", L, calls, sleeps, skipped, tsum / (n / fs), tmax * 1e3);
    H.close(); return 0; }
  if (cmd == "perf") {
    const char *spec = argc > 3 ? argv[3] : ""; double fs = argc > 4 ? atof(argv[4]) : 48000; int B = argc > 5 ? atoi(argv[5]) : 128;
    stateParams(H.p, spec); H.activate(fs, (uint32_t)B); addParams(spec); std::vector<float> x = bassSignal(fs, 10), y(B);
    // Einschwingen 1 s, dann 9 s messen
    double tsum = 0, tmax = 0; std::vector<double> ts; long pos = 0;
    FILE *dump = getenv("DUMP") ? fopen(getenv("DUMP"), "w") : nullptr;
    for (; pos + B <= (long)x.size(); pos += B) { double dt; H.run(&x[pos], y.data(), B, dt); if (dump) fprintf(dump, "%ld %.6f\n", pos, dt * 1e3); if (pos > fs) { tsum += dt; tmax = std::max(tmax, dt); ts.push_back(dt); } }
    if (dump) fclose(dump);
    std::sort(ts.begin(), ts.end()); double blk = B / fs;
    printf("last_mittel=%.1f%% p99=%.1f%% max=%.1f%% (Block %d bei %.0f Hz = %.2f ms)\n", 100 * tsum / (ts.size() * blk), 100 * ts[ts.size() * 99 / 100] / blk, 100 * tmax / blk, B, fs, blk * 1e3);
    if (getenv("NOSWEEP")) { H.close(); return 0; }
    // jeden Parameter umschalten (Mitte bzw. andere Stufe) und 0,4 s weiterspielen: längster Block und Dauer erhöhter Last
    uint32_t np = H.params->count(H.p); long cyc = 0;
    for (uint32_t i = 0; i < np; i++) { clap_param_info_t inf; H.params->get_info(H.p, i, &inf); double v0; H.params->get_value(H.p, i, &v0);
      bool stepped = (inf.flags & CLAP_PARAM_IS_STEPPED) != 0; double v1 = stepped ? (v0 + 1 > inf.max_value ? inf.min_value : v0 + 1) : inf.min_value + 0.73 * (inf.max_value - inf.min_value); if (v1 == v0) v1 = inf.min_value + 0.27 * (inf.max_value - inf.min_value);
      for (int rep = 0; rep < 2; rep++) {
        addParam(i, rep ? v0 : v1); double m1 = 0; long over = 0; int k = 0;
        for (; k < (int)(0.4 * fs / B); k++) { long q = (pos + (long)k * B) % ((long)x.size() - B); double dt; H.run(&x[q], y.data(), B, dt); m1 = std::max(m1, dt); if (dt > 0.7 * blk) over++; cyc++; }
        pos += (long)k * B;
        printf("param %2u %-44.44s %s %8.3f → %8.3f  längster Block %6.1f%%  Blöcke >70%%: %ld\n", i, inf.name, rep ? "zurück" : "      ", rep ? v1 : v0, rep ? v0 : v1, 100 * m1 / blk, over); } }
    H.close(); return 0; }
  printf("unbekannt: %s\n", cmd.c_str()); return 1;
}
