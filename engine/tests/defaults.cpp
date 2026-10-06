#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
static const void *he(const clap_host_t *, const char *) { return nullptr; } static void hn(const clap_host_t *) {}
int main(int, char **argv) { void *lib = dlopen(argv[1], RTLD_NOW); auto *e = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); e->init(argv[1]);
  auto *f = (const clap_plugin_factory_t*)e->get_factory(CLAP_PLUGIN_FACTORY_ID); clap_host_t h = {CLAP_VERSION_INIT, nullptr, "t", "", "", "1", he, hn, hn, hn};
  const clap_plugin_t *p = f->create_plugin(f, &h, f->get_plugin_descriptor(f, 0)->id); p->init(p); auto *pa = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  char t[64]; for (uint32_t i = 0; i < pa->count(p); i++) { double v; pa->get_value(p, i, &v); clap_param_info_t inf; pa->get_info(p, i, &inf); pa->value_to_text(p, i, v, t, 64); printf("  %-38s %s\n", inf.name, t); } p->destroy(p); return 0; }
