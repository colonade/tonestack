// Blockzeiten rund um ein einzelnes Umschalten (Mono, 48 kHz, Block 128):
//   swprobe plugin.clap "id=wert,..." "id=wert,..."
// Erster Satz gilt ab Beginn, zweiter ab 2 s. Ausgabe: Rechenzeit der 90 Blöcke nach dem Umschalten (Block:ms) und
// die mittlere Blockzeit davor. Umgebung: DUR=<s> Gesamtdauer (4), SILENT=1 Stille statt Bass-Signal,
// OUT=datei.f32 Ausgang schreiben (Vergleich zweier Fassungen, z. B. auf Knacken), TL=1 Zeitmarken auf stderr.
// Damit wurde 3.33 das Nachrechnen beim Umschalten des Laney gemessen (Röhrenwechsel 5,2 → 2,7-fache Blockzeit).
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
static std::vector<clap_event_param_value_t> Q; static uint32_t qs(const clap_input_events_t *) { return Q.size(); } static const clap_event_header_t *qg(const clap_input_events_t *, uint32_t i) { return &Q[i].header; }
static bool push(const clap_output_events_t *, const clap_event_header_t *) { return true; } static const void *hx(const clap_host_t *, const char *) { return nullptr; } static void hn(const clap_host_t *) {}
static void parse(const char *s) { std::string t(s); size_t p = 0; while (p < t.size()) { size_t e = t.find(',', p); if (e == std::string::npos) e = t.size(); std::string k = t.substr(p, e - p); p = e + 1; size_t q = k.find('='); if (q == std::string::npos) continue;
  clap_event_param_value_t ev{}; ev.header.size = sizeof ev; ev.header.type = CLAP_EVENT_PARAM_VALUE; ev.param_id = atoi(k.substr(0, q).c_str()); ev.value = atof(k.substr(q + 1).c_str()); ev.note_id = -1; ev.port_index = -1; ev.channel = -1; ev.key = -1; Q.push_back(ev); } }
int main(int argc, char **argv) { if (argc < 4) { std::printf("swprobe plugin.clap \"id=wert,...\" \"id=wert,...\"\n"); return 2; }
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { std::printf("dlopen: %s\n", dlerror()); return 1; } auto *en = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); en->init(argv[1]); auto *fac = (const clap_plugin_factory_t*)en->get_factory(CLAP_PLUGIN_FACTORY_ID);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "t", "", "", "1", hx, hn, hn, hn}; const clap_plugin_t *p = fac->create_plugin(fac, &host, fac->get_plugin_descriptor(fac, 0)->id); p->init(p);
  double fs = 48000; int B = 128; p->activate(p, fs, 32, 4096); p->start_processing(p); float L[128], OL[128]; float *in[1] = {L}, *out[1] = {OL}; long pos = 0;
  int silent = getenv("SILENT") ? 1 : 0; double base = 0; int nb = 0;
  double DUR = getenv("DUR") ? atof(getenv("DUR")) : 4; for (int b = 0; b < (int)(DUR * fs / B); b++) { if (b == 0) parse(argv[2]); if (b == (int)(2 * fs / B)) parse(argv[3]);
    for (int i = 0; i < B; i++) { double t = (pos + i) / fs; L[i] = silent ? 0 : (float)(0.5 * std::exp(-2.5 * std::fmod(t, 0.8)) * std::sin(2 * M_PI * 55 * t)); }
    clap_audio_buffer_t ib{}; ib.data32 = in; ib.channel_count = 1; clap_audio_buffer_t ob{}; ob.data32 = out; ob.channel_count = 1; clap_input_events_t ie = {nullptr, qs, qg}; clap_output_events_t oe = {nullptr, push};
    clap_process_t pr{}; pr.frames_count = B; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
    auto t0 = std::chrono::steady_clock::now(); p->process(p, &pr); double ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() * 1000; Q.clear(); pos += B; if (getenv("OUT")) { static FILE *fo = fopen(getenv("OUT"), "wb"); fwrite(OL, 4, B, fo); fflush(fo); }
    int sw = (int)(2 * fs / B); if (b > sw - 100 && b < sw) { base += ms; nb++; }
    if (b >= sw && b < sw + 90) std::printf("%d:%.2f ", b - sw, ms); if (getenv("TL") && b % 375 == 0) std::fprintf(stderr, "t=%.0fs\n", pos / fs); }
  std::printf("\nbase %.3f ms/Block\n", base / nb); return 0; }
