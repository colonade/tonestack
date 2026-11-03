// Zufällige Sprünge aller Parameter auf einmal (wie ein Preset-Wechsel), danach je 1 s Bass-Signal; streng deterministisch
// (ein Thread, keine Pausen), ein Fund lässt sich also mit demselben Startwert genau wieder herstellen:
//   swrand plugin.clap [Startwert 1] [Grenze in ms je Block, 10] [Runden, 100]
// Meldet jeden Block über der Grenze mit dem Parametersatz davor und dem aktuellen. Oversampling bleibt, wie aktiviert.
// Damit wurde der STANDBY-Fehler des AmpFett gefunden (bis 1.13: Startwert 2, Runde 104, 61 ms für 128 Abtastwerte).
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
static std::vector<clap_event_param_value_t> Q; static uint32_t qs(const clap_input_events_t *) { return Q.size(); } static const clap_event_header_t *qg(const clap_input_events_t *, uint32_t i) { return &Q[i].header; }
static bool push(const clap_output_events_t *, const clap_event_header_t *) { return true; } static const void *hx(const clap_host_t *, const char *) { return nullptr; } static void hn(const clap_host_t *) {}
static unsigned rs = 1; static double ur() { rs = rs * 1664525u + 1013904223u; return (rs >> 8) / 16777216.0; }
int main(int argc, char **argv) { rs = argc > 2 ? atoi(argv[2]) : 1; double LIM = argc > 3 ? atof(argv[3]) : 10; int N = argc > 4 ? atoi(argv[4]) : 100;
  if (argc < 2) { printf("swrand plugin.clap [Startwert] [Grenze ms] [Runden]\n"); return 2; }
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("dlopen: %s\n", dlerror()); return 1; } auto *en = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); en->init(argv[1]); auto *fac = (const clap_plugin_factory_t*)en->get_factory(CLAP_PLUGIN_FACTORY_ID);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "t", "", "", "1", hx, hn, hn, hn}; const clap_plugin_t *p = fac->create_plugin(fac, &host, fac->get_plugin_descriptor(fac, 0)->id); p->init(p);
  auto *par = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); std::vector<clap_param_info_t> info; for (uint32_t i = 0; i < par->count(p); i++) { clap_param_info_t in; par->get_info(p, i, &in); info.push_back(in); }
  double fs = 48000; int B = 128; p->activate(p, fs, 32, 4096); p->start_processing(p); float L[128], OL[128]; float *in[1] = {L}, *out[1] = {OL}; long pos = 0; std::string prev, cur;
  for (int r = 0; r < N; r++) { prev = cur; cur.clear();
    for (auto &pi : info) { if (pi.flags & CLAP_PARAM_IS_HIDDEN) continue; if (std::string(pi.name).find("Oversampling") != std::string::npos) continue; double v = pi.min_value + ur() * (pi.max_value - pi.min_value); if (pi.flags & CLAP_PARAM_IS_STEPPED) v = std::round(v);
      clap_event_param_value_t ev{}; ev.header.size = sizeof ev; ev.header.type = CLAP_EVENT_PARAM_VALUE; ev.param_id = pi.id; ev.value = v; ev.note_id = -1; ev.port_index = -1; ev.channel = -1; ev.key = -1; Q.push_back(ev); char b[48]; snprintf(b, 48, "%u=%.6g,", pi.id, v); cur += b; }
    for (int b = 0; b < (int)(fs / B); b++) { for (int i = 0; i < B; i++) { double t = (pos + i) / fs; L[i] = (float)(0.5 * std::exp(-2.5 * std::fmod(t, 0.8)) * std::sin(2 * M_PI * 55 * t)); }
      clap_audio_buffer_t ib{}; ib.data32 = in; ib.channel_count = 1; clap_audio_buffer_t ob{}; ob.data32 = out; ob.channel_count = 1; clap_input_events_t ie = {nullptr, qs, qg}; clap_output_events_t oe = {nullptr, push};
      clap_process_t pr{}; pr.frames_count = B; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
      auto t0 = std::chrono::steady_clock::now(); p->process(p, &pr); double ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() * 1000; Q.clear(); pos += B;
      if (ms > LIM) { printf("SPIKE round=%d block=%d ms=%.1f\n  prev=%s\n  cur=%s\n", r, b, ms, prev.c_str(), cur.c_str()); fflush(stdout); } } }
  p->stop_processing(p); p->deactivate(p); p->destroy(p); return 0; }
