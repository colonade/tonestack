// Minimaler CLAP-Host: hosttest plugin.clap in.f32 out.f32 "idx=val,..." (STEREOTEST=1: rechts 0,3-faches Signal; RIGHT=1: rechten Kanal schreiben)
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include <algorithm>
static std::vector<clap_event_param_value_t> EVS;
static uint32_t ev_size(const clap_input_events_t *) { return (uint32_t)EVS.size(); }
static const clap_event_header_t *ev_get(const clap_input_events_t *, uint32_t i) { return &EVS[i].header; }
static bool out_push(const clap_output_events_t *, const clap_event_header_t *) { return true; }
static const void *host_ext(const clap_host_t *, const char *) { return nullptr; }
static void host_nop(const clap_host_t *) {}
int main(int argc, char **argv) {
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("dlopen: %s\n", dlerror()); return 1; }
  auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0); printf("Plugin: %s %s (%s)\n", d->name, d->version, d->id);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_nop};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, d->id); p->init(p);
  auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); printf("Parameter: %u\n", params->count(p));
  double fs = 48000; p->activate(p, fs, 32, 512); p->start_processing(p);
  FILE *fi = fopen(argv[2], "rb"); fseek(fi, 0, SEEK_END); long n = ftell(fi) / 4; fseek(fi, 0, SEEK_SET); std::vector<float> x(n); if (fread(x.data(), 4, n, fi) != (size_t)n) return 2; fclose(fi);
  std::vector<float> out(n); const int B = 256; std::vector<float> l(B), r(B), ol(B), orr(B);
  if (argc > 4) { char *s = strdup(argv[4]); for (char *t = strtok(s, ","); t; t = strtok(nullptr, ",")) { clap_event_param_value_t e{}; e.header.size = sizeof e; e.header.type = CLAP_EVENT_PARAM_VALUE; e.header.space_id = CLAP_CORE_EVENT_SPACE_ID; e.param_id = atoi(t); e.value = atof(strchr(t, '=') + 1); e.note_id = -1; e.port_index = -1; e.channel = -1; e.key = -1; EVS.push_back(e); } }
  clap_input_events_t in_ev = {nullptr, ev_size, ev_get}; clap_output_events_t out_ev = {nullptr, out_push};
  int cnt[5] = {0,0,0,0,0}; bool asleep = false; long skipped = 0;
  for (long pos = 0; pos < n; pos += B) { int m = (int)std::min<long>(B, n - pos);
    if (asleep && getenv("HONOR")) { bool silent = true; for (int i = 0; i < m; i++) if (x[pos + i] != 0) silent = false; if (silent && EVS.empty()) { for (int i = 0; i < m; i++) out[pos + i] = 0; skipped++; continue; } asleep = false; } for (int i = 0; i < m; i++) { l[i] = x[pos + i]; r[i] = getenv("STEREOTEST") ? 0.3f * x[pos + i] : x[pos + i]; }
    float *ins[2] = {l.data(), r.data()}, *outs[2] = {ol.data(), orr.data()};
    clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
    clap_process_t pr{}; pr.frames_count = m; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &in_ev; pr.out_events = &out_ev;
    clap_process_status rs = p->process(p, &pr); if (rs >= 0 && rs < 5) cnt[rs]++; if (rs == CLAP_PROCESS_SLEEP) asleep = true; EVS.clear(); for (int i = 0; i < m; i++) out[pos + i] = getenv("RIGHT") ? orr[i] : ol[i]; }
  printf("Rückgaben: CONTINUE %d, SLEEP %d | übersprungene Blöcke %ld\n", cnt[1], cnt[4], skipped);
  FILE *fo = fopen(argv[3], "wb"); fwrite(out.data(), 4, n, fo); fclose(fo);
  char txt[64]; for (uint32_t i = 0; i < params->count(p); i++) { double v; params->get_value(p, i, &v); params->value_to_text(p, i, v, txt, 64); clap_param_info_t inf; params->get_info(p, i, &inf); if (i < 3 || i == 16 || i == 18 || i == 19) printf("  %s = %s\n", inf.name, txt); }
  p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); return 0;
}
