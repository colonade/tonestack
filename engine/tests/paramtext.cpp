// Prüft die Umwandlung Parameterwert ↔ Text aller Parameter eines CLAP-Plugins (ab 2026-11-27), so wie clap-validator (param-conversions):
// Wert → Text → Wert → Text muss denselben Text ergeben, und jeder Text, den das Plugin selbst ausgibt, muss sich zurücklesen lassen.
// Geprüft werden je Parameter Unter- und Obergrenze, Grundstellung, 401 gleichmäßig verteilte und 400 zufällige Werte (gestufte: alle ganzen Zahlen).
//   paramtext PLUGIN.clap        Rückgabe 0 = alles passt, 1 = Abweichungen (je Parameter höchstens drei Beispiele)
#include "clap-1.2.2/include/clap/clap.h"
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

static const void *host_ext(const clap_host_t *, const char *) { return nullptr; }
static void host_nop(const clap_host_t *) {}

int main(int argc, char **argv) {
  if (argc < 2) { std::fprintf(stderr, "paramtext PLUGIN.clap\n"); return 2; }
  void *h = dlopen(argv[1], RTLD_NOW); if (!h) { std::fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
  auto *entry = (const clap_plugin_entry_t*)dlsym(h, "clap_entry"); if (!entry || !entry->init(argv[1])) { std::fprintf(stderr, "clap_entry fehlt\n"); return 2; }
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host{CLAP_VERSION_INIT, nullptr, "paramtext", "tonestack", "", "1", host_ext, host_nop, host_nop, host_nop};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, d->id); if (!p || !p->init(p)) { std::fprintf(stderr, "Plugin lässt sich nicht anlegen\n"); return 2; }
  auto *par = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); if (!par) { std::fprintf(stderr, "keine Parameter\n"); return 2; }
  std::mt19937_64 rng(1234); int bad = 0; uint32_t n = par->count(p); long checks = 0;
  for (uint32_t i = 0; i < n; i++) {
    clap_param_info_t inf{}; par->get_info(p, i, &inf);
    std::vector<double> vs{inf.min_value, inf.max_value, inf.default_value};
    bool stepped = inf.flags & CLAP_PARAM_IS_STEPPED;
    if (stepped) for (double v = std::ceil(inf.min_value); v <= inf.max_value; v += 1) vs.push_back(v);
    else { for (int k = 0; k <= 400; k++) vs.push_back(inf.min_value + (inf.max_value - inf.min_value) * k / 400.0);
      std::uniform_real_distribution<double> u(inf.min_value, inf.max_value); for (int k = 0; k < 400; k++) vs.push_back(u(rng)); }
    int shown = 0;
    for (double v : vs) {
      char t1[256] = {}, t2[256] = {}; double v1 = NAN; checks++;
      if (!par->value_to_text(p, inf.id, v, t1, sizeof t1)) { if (shown++ < 3) std::printf("  %u %s: kein Text für %.17g\n", inf.id, inf.name, v); bad = 1; continue; }
      if (!par->text_to_value(p, inf.id, t1, &v1) || !std::isfinite(v1)) { if (shown++ < 3) std::printf("  %u %s: Text „%s“ (von %.17g) lässt sich nicht zurücklesen\n", inf.id, inf.name, t1, v); bad = 1; continue; }
      if (v1 < inf.min_value - 1e-9 || v1 > inf.max_value + 1e-9) { if (shown++ < 3) std::printf("  %u %s: „%s“ → %.17g liegt außerhalb %g … %g\n", inf.id, inf.name, t1, v1, inf.min_value, inf.max_value); bad = 1; continue; }
      par->value_to_text(p, inf.id, v1, t2, sizeof t2);
      if (std::strcmp(t1, t2)) { if (shown++ < 3) std::printf("  %u %s: %.17g → „%s“ → %.17g → „%s“\n", inf.id, inf.name, v, t1, v1, t2); bad = 1; }
    }
    if (shown > 3) std::printf("  %u %s: insgesamt %d Abweichungen\n", inf.id, inf.name, shown);
  }
  std::printf("%s %s: %u Parameter, %ld Werte – %s\n", d->name, d->version, n, checks, bad ? "ABWEICHUNGEN" : "Text und Wert passen zusammen");
  p->destroy(p); entry->deinit(); return bad;
}
