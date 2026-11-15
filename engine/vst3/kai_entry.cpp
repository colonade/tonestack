// Einstieg des Kaiserin-Reverb-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/kaiplugin.cpp
#include <clap/clap.h>
extern "C" {
bool kai_entry_init(const char *path);
void kai_entry_deinit();
const void *kai_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, kai_entry_init, kai_entry_deinit, kai_entry_factory};
}
