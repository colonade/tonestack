// Einstieg des Wien-EQ-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/weqplugin.cpp
#include <clap/clap.h>
extern "C" {
bool weq_entry_init(const char *path);
void weq_entry_deinit();
const void *weq_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, weq_entry_init, weq_entry_deinit, weq_entry_factory};
}
