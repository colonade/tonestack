// Einstieg des Cultec-EQ-1A-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/cultecplugin.cpp
#include <clap/clap.h>
extern "C" {
bool cultec_entry_init(const char *path);
void cultec_entry_deinit();
const void *cultec_entry_factory(const char *id);
CLAP_EXPORT extern const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, cultec_entry_init, cultec_entry_deinit, cultec_entry_factory};
}
