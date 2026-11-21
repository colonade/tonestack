// Einstieg des Wisent-Echoreck-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/wisentplugin.cpp
#include <clap/clap.h>
extern "C" {
bool wisent_entry_init(const char *path);
void wisent_entry_deinit();
const void *wisent_entry_factory(const char *id);
CLAP_EXPORT extern const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, wisent_entry_init, wisent_entry_deinit, wisent_entry_factory};
}
