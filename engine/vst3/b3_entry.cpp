// Einstieg des Lametta-B100-III-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/b3plugin.cpp
#include <clap/clap.h>
extern "C" {
bool b3_entry_init(const char *path);
void b3_entry_deinit();
const void *b3_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, b3_entry_init, b3_entry_deinit, b3_entry_factory};
}
