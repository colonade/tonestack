// Einstieg des GK-800RB-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/gkplugin.cpp
#include <clap/clap.h>
extern "C" {
bool gk_entry_init(const char *path);
void gk_entry_deinit();
const void *gk_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, gk_entry_init, gk_entry_deinit, gk_entry_factory};
}
