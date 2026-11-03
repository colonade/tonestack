// Einstieg des Laney-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/plugin.cpp
#include <clap/clap.h>
extern "C" {
bool laney_entry_init(const char *path);
void laney_entry_deinit();
const void *laney_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, laney_entry_init, laney_entry_deinit, laney_entry_factory};
}
