// Einstieg des B-15NF-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/b15plugin.cpp
#include <clap/clap.h>
extern "C" {
bool b15_entry_init(const char *path);
void b15_entry_deinit();
const void *b15_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, b15_entry_init, b15_entry_deinit, b15_entry_factory};
}
