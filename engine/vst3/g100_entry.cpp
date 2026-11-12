// Einstieg des Lametta-G100-212II-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/g100plugin.cpp
#include <clap/clap.h>
extern "C" {
bool g100_entry_init(const char *path);
void g100_entry_deinit();
const void *g100_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, g100_entry_init, g100_entry_deinit, g100_entry_factory};
}
