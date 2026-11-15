// Einstieg des Blech-140-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/blechplugin.cpp
#include <clap/clap.h>
extern "C" {
bool blech_entry_init(const char *path);
void blech_entry_deinit();
const void *blech_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, blech_entry_init, blech_entry_deinit, blech_entry_factory};
}
