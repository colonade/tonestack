// Einstieg des AmpFett-76-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/u76plugin.cpp
#include <clap/clap.h>
extern "C" {
bool u76_entry_init(const char *path);
void u76_entry_deinit();
const void *u76_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, u76_entry_init, u76_entry_deinit, u76_entry_factory};
}
