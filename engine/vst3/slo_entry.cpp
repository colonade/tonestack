// Einstieg des Loldano-SLO-100-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/sloplugin.cpp
#include <clap/clap.h>
extern "C" {
bool slo_entry_init(const char *path);
void slo_entry_deinit();
const void *slo_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, slo_entry_init, slo_entry_deinit, slo_entry_factory};
}
