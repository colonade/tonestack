// Ein- und Ausgang wahlweise mono oder stereo (CLAP configurable-audio-ports). clap-wrapper meldet daraus die Kanalbelegungen
// 1→1, 1→2, 2→1, 2→2 an Audio Unit (Logic zeigt Plugins auf Mono-Spuren nur mit Mono-Eingang) und VST3. Grundstellung stereo.
// Die Rechnung selbst ist mono (bzw. zwei Kanäle nur bei verschiedenen Signalen); process liest und schreibt so viele Kanäle, wie da sind.
#pragma once
#include <atomic>
#include <cstring>
struct PortCfg { std::atomic<uint32_t> in{2}, out{2}; };
inline bool portReqOk(const clap_audio_port_configuration_request *r, uint32_t n) {
  for (uint32_t k = 0; k < n; k++) { const auto &q = r[k]; if (q.port_index != 0 || q.channel_count < 1 || q.channel_count > 2) return false;
    if (q.port_type && !((q.channel_count == 1 && !std::strcmp(q.port_type, CLAP_PORT_MONO)) || (q.channel_count == 2 && !std::strcmp(q.port_type, CLAP_PORT_STEREO)))) return false; }
  return true; }
inline void portApply(PortCfg &c, const clap_audio_port_configuration_request *r, uint32_t n) { for (uint32_t k = 0; k < n; k++) (r[k].is_input ? c.in : c.out) = r[k].channel_count; }
inline bool portInfo(const PortCfg &c, uint32_t i, bool is_input, clap_audio_port_info_t *info) {
  if (i != 0) return false; std::memset(info, 0, sizeof *info); info->id = 0; std::snprintf(info->name, sizeof info->name, "%s", is_input ? "Input" : "Output");
  uint32_t ch = is_input ? c.in.load() : c.out.load(); info->channel_count = ch; info->flags = CLAP_AUDIO_PORT_IS_MAIN; info->port_type = ch == 1 ? CLAP_PORT_MONO : CLAP_PORT_STEREO;
  info->in_place_pair = CLAP_INVALID_ID; return true; }
