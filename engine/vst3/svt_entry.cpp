// Einstieg des AmpFett-SVT-CL-Plugins für clap-wrapper (VST3): verweist auf die Funktionen aus engine/clap/svtplugin.cpp
#include <clap/clap.h>
extern "C" {
bool svt_entry_init(const char *path);
void svt_entry_deinit();
const void *svt_entry_factory(const char *id);
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, svt_entry_init, svt_entry_deinit, svt_entry_factory};
}
