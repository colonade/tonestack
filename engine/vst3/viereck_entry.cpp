// Einstieg des Viereck-34-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/viereckplugin.cpp
#include <clap/clap.h>
extern "C" {
bool viereck_entry_init(const char *path);
void viereck_entry_deinit();
const void *viereck_entry_factory(const char *id);
CLAP_EXPORT extern const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, viereck_entry_init, viereck_entry_deinit, viereck_entry_factory};
}
