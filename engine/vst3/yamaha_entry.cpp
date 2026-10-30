// Einstieg des Yamaha-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/yplugin.cpp
#include <clap/clap.h>
extern "C" {
bool yamaha_entry_init(const char *path);
void yamaha_entry_deinit();
const void *yamaha_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, yamaha_entry_init, yamaha_entry_deinit, yamaha_entry_factory};
}
