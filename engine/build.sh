#!/bin/bash
# Baut CLAP-Plugins und WebAssembly-Kerne aus den Quellen.  bash engine/build.sh [--wasi /pfad/zu/wasi-sdk]
# Mit VST3=1 zusätzlich die VST3-Fassungen (cmake, ninja; holt beim ersten Mal clap-wrapper und VST3-SDK aus GitHub) → engine/build/vst3/*.vst3
# Voraussetzungen: g++ (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; für WebAssembly das wasi-sdk 24
# (https://github.com/WebAssembly/wasi-sdk/releases, Standard /opt/wasi-sdk). Ergebnis in engine/build/.
set -euo pipefail
E="$(cd "$(dirname "$0")" && pwd)"; R="$(dirname "$E")"; WASI="${WASI:-/opt/wasi-sdk}"
[ "${1:-}" = "--wasi" ] && WASI="$2"
cd "$E"; mkdir -p build
node tools/extract.js "$R"; python3 tools/gen_geo.py gen; 
if [ "${B15PANEL:-0}" = 1 ]; then python3 tools/b15_panel.py "$R" >/dev/null; fi   # Frontplatte AmpFett nur auf Wunsch neu zeichnen (Pillow); sonst wie eingecheckt, damit Linux und Mac dasselbe Bild haben
if [ "${SVTPANEL:-0}" = 1 ]; then python3 tools/svt_panel.py "$R" >/dev/null; fi   # ebenso die Frontplatte AmpFett SVT-CL (Bild, Geometrie, Seitensymbole)
if [ "${WEQPANEL:-0}" = 1 ]; then python3 tools/weq_panel.py "$R" >/dev/null; fi   # ebenso die Frontplatte Wien-EQ
if [ "${U76PANEL:-0}" = 1 ]; then python3 tools/u76_panel.py "$R" >/dev/null; fi   # ebenso die Frontplatte AmpFett 76
if [ "${SLOPANEL:-0}" = 1 ]; then python3 tools/slo_panel.py "$R" >/dev/null; fi   # ebenso die Frontplatte Loldano SLO-100
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/plugin.cpp -o build/Laney_AOR50.clap $(pkg-config --cflags --libs cairo x11)
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/yplugin.cpp -o build/Lametta_B100II.clap $(pkg-config --cflags --libs cairo x11)
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/b15plugin.cpp -o build/AmpFett_B15NF.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/gkplugin.cpp -o build/GK_800RB.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/g100plugin.cpp -o build/Lametta_G100.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/b3plugin.cpp -o build/Lametta_B100III.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/svtplugin.cpp -o build/AmpFett_SVTCL.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/weqplugin.cpp -o build/Wien_EQ.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/u76plugin.cpp -o build/AmpFett_76.clap $(pkg-config --cflags --libs cairo x11) -lpthread
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/sloplugin.cpp -o build/Loldano_SLO100.clap $(pkg-config --cflags --libs cairo x11) -lpthread
echo "CLAP: build/Laney_AOR50.clap, build/Lametta_B100II.clap, build/AmpFett_B15NF.clap, build/GK_800RB.clap, build/Lametta_G100.clap, build/Lametta_B100III.clap, build/AmpFett_SVTCL.clap, build/Wien_EQ.clap, build/AmpFett_76.clap, build/Loldano_SLO100.clap"
if [ "${VST3:-0}" = 1 ]; then
  cmake -S vst3 -B build/vst3 -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build/vst3 >/dev/null
  echo "VST3: build/vst3/Laney_AOR50.vst3, build/vst3/Lametta_B100II.vst3, build/vst3/AmpFett_B15NF.vst3, build/vst3/GK_800RB.vst3, build/vst3/Lametta_G100.vst3, build/vst3/Lametta_B100III.vst3, build/vst3/AmpFett_SVTCL.vst3, build/vst3/Wien_EQ.vst3, build/vst3/AmpFett_76.vst3, build/vst3/Loldano_SLO100.vst3"
fi
if [ -x "$WASI/bin/clang++" ]; then
  W="$WASI/bin/clang++ -O3 -std=c++17 -fno-exceptions -fno-rtti --target=wasm32-wasi -mexec-model=reactor -Wl,--export-memory -Wl,--strip-all -Isrc -Igen"
  $W -Wl,--export=aor_buffer -Wl,--export=aor_parts_buffer -Wl,--export=aor_target_buffer -Wl,--export=aor_peaks_buffer -Wl,--export=aor_parts_count \
     -Wl,--export=aor_init -Wl,--export=aor_set_target -Wl,--export=aor_process -Wl,--export=aor_take_peaks -Wl,--export=aor_ss_buffer -Wl,--export=aor_small_signal -Wl,--export=aor_latency -Wl,--initial-memory=16777216 wasm/wasm_api.cpp -o build/laney-core.wasm
  $W -Wl,--export=y_buffer -Wl,--export=y_parts_buffer -Wl,--export=y_target_buffer -Wl,--export=y_parts_count -Wl,--export=y_init \
     -Wl,--export=y_set_target -Wl,--export=y_process -Wl,--export=y_take_peak -Wl,--initial-memory=8388608 wasm/yamaha_wasm.cpp -o build/b100ii-core.wasm
  $W -Wl,--export=bf_buffer -Wl,--export=bf_target_buffer -Wl,--export=bf_peaks_buffer -Wl,--export=bf_freq_buffer -Wl,--export=bf_resp_buffer -Wl,--export=bf_init \
     -Wl,--export=bf_latency -Wl,--export=bf_set_target -Wl,--export=bf_process -Wl,--export=bf_take_peaks -Wl,--export=bf_plot_init -Wl,--export=bf_plot -Wl,--initial-memory=16777216 wasm/b15_wasm.cpp -o build/b15nf-core.wasm
  $W -Wl,--export=gk_buffer -Wl,--export=gk_target_buffer -Wl,--export=gk_peaks_buffer -Wl,--export=gk_freq_buffer -Wl,--export=gk_resp_buffer -Wl,--export=gk_init \
     -Wl,--export=gk_latency -Wl,--export=gk_set_target -Wl,--export=gk_process -Wl,--export=gk_take_peaks -Wl,--export=gk_plot_init -Wl,--export=gk_plot -Wl,--initial-memory=16777216 wasm/gk_wasm.cpp -o build/gk800rb-core.wasm
  $W -Wl,--export=g100_buffer -Wl,--export=g100_target_buffer -Wl,--export=g100_peaks_buffer -Wl,--export=g100_freq_buffer -Wl,--export=g100_resp_buffer -Wl,--export=g100_init \
     -Wl,--export=g100_latency -Wl,--export=g100_set_target -Wl,--export=g100_process -Wl,--export=g100_take_peaks -Wl,--export=g100_plot_init -Wl,--export=g100_plot -Wl,--initial-memory=16777216 wasm/g100_wasm.cpp -o build/g100-core.wasm
  $W -Wl,--export=b3_buffer -Wl,--export=b3_target_buffer -Wl,--export=b3_peaks_buffer -Wl,--export=b3_freq_buffer -Wl,--export=b3_resp_buffer -Wl,--export=b3_init \
     -Wl,--export=b3_latency -Wl,--export=b3_set_target -Wl,--export=b3_process -Wl,--export=b3_take_peaks -Wl,--export=b3_plot_init -Wl,--export=b3_plot -Wl,--initial-memory=16777216 wasm/b3_wasm.cpp -o build/b100iii-core.wasm
  $W -Wl,--export=sv_buffer -Wl,--export=sv_target_buffer -Wl,--export=sv_peaks_buffer -Wl,--export=sv_freq_buffer -Wl,--export=sv_resp_buffer -Wl,--export=sv_init \
     -Wl,--export=sv_latency -Wl,--export=sv_set_target -Wl,--export=sv_process -Wl,--export=sv_take_peaks -Wl,--export=sv_plot_init -Wl,--export=sv_plot -Wl,--initial-memory=16777216 wasm/svt_wasm.cpp -o build/svtcl-core.wasm
  $W -Wl,--export=weq_buffer -Wl,--export=weq_target_buffer -Wl,--export=weq_freq_buffer -Wl,--export=weq_resp_buffer -Wl,--export=weq_init \
     -Wl,--export=weq_latency -Wl,--export=weq_process -Wl,--export=weq_plot -Wl,--initial-memory=4194304 wasm/weq_wasm.cpp -o build/wieneq-core.wasm
  $W -Wl,--export=u7_buffer -Wl,--export=u7_target_buffer -Wl,--export=u7_peaks_buffer -Wl,--export=u7_level_buffer -Wl,--export=u7_resp_buffer -Wl,--export=u7_init \
     -Wl,--export=u7_latency -Wl,--export=u7_set_target -Wl,--export=u7_process -Wl,--export=u7_take_peaks -Wl,--export=u7_plot_init -Wl,--export=u7_plot -Wl,--initial-memory=16777216 wasm/u76_wasm.cpp -o build/ampfett76-core.wasm
  $W -Wl,--export=lo_buffer -Wl,--export=lo_target_buffer -Wl,--export=lo_peaks_buffer -Wl,--export=lo_freq_buffer -Wl,--export=lo_resp_buffer -Wl,--export=lo_init \
     -Wl,--export=lo_latency -Wl,--export=lo_set_target -Wl,--export=lo_process -Wl,--export=lo_take_peaks -Wl,--export=lo_plot_init -Wl,--export=lo_plot -Wl,--initial-memory=16777216 wasm/slo_wasm.cpp -o build/loldano-core.wasm
  echo "WebAssembly: build/laney-core.wasm, build/b100ii-core.wasm, build/b15nf-core.wasm, build/gk800rb-core.wasm, build/g100-core.wasm, build/b100iii-core.wasm, build/svtcl-core.wasm, build/wieneq-core.wasm, build/ampfett76-core.wasm, build/loldano-core.wasm"
else echo "wasi-sdk nicht gefunden ($WASI): WebAssembly übersprungen"; fi
g++ -O3 -std=c++17 -Isrc -Igen tests/harness.cpp -o build/harness
g++ -O2 -std=c++17 -Ithird_party tests/hosttest.cpp -ldl -o build/hosttest
g++ -O2 -std=c++17 -Ithird_party tests/plugtest.cpp -ldl -o build/plugtest
g++ -O2 -std=c++17 -Ithird_party tests/rtcheck.cpp -ldl -lpthread -o build/rtcheck
g++ -O2 -std=c++17 -Ithird_party tests/swprobe.cpp -ldl -o build/swprobe; g++ -O2 -std=c++17 -Ithird_party tests/swrand.cpp -ldl -o build/swrand
g++ -O2 -std=c++17 -Ithird_party tests/defaults.cpp -ldl -o build/defaults
g++ -O2 -std=c++17 -Ithird_party tests/guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/guitest
g++ -O2 -std=c++17 -Ithird_party tests/yguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/yguitest
g++ -O2 -std=c++17 -Ithird_party tests/b15guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/b15guitest
g++ -O2 -std=c++17 -Ithird_party tests/gkguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/gkguitest
g++ -O2 -std=c++17 -Ithird_party tests/g100guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/g100guitest
g++ -O2 -std=c++17 -Ithird_party tests/b3guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/b3guitest
g++ -O2 -std=c++17 -Ithird_party tests/svtguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/svtguitest
g++ -O2 -std=c++17 -Ithird_party tests/weqguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/weqguitest
g++ -O2 -std=c++17 -Ithird_party tests/u76guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/u76guitest
g++ -O2 -std=c++17 -Ithird_party tests/sloguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/sloguitest
g++ -O3 -std=c++17 -Isrc -Igen tests/b15test.cpp -o build/b15test; g++ -O3 -std=c++17 -Isrc -Igen tests/b15amp.cpp -o build/b15amp
g++ -O3 -std=c++17 -Isrc -Igen tests/gktest.cpp -o build/gktest
g++ -O3 -std=c++17 -Isrc -Igen tests/g100test.cpp -o build/g100test
g++ -O3 -std=c++17 -Isrc -Igen tests/b3test.cpp -o build/b3test
g++ -O3 -std=c++17 -Isrc tests/svttest.cpp -o build/svttest
g++ -O3 -std=c++17 -Isrc tests/weqtest.cpp -o build/weqtest
g++ -O3 -std=c++17 -Isrc -Igen tests/u76test.cpp -o build/u76test -lpthread
g++ -O3 -std=c++17 -Isrc -Igen tests/slotest.cpp -o build/slotest -lpthread
g++ -O1 -g -std=c++17 -Ithird_party tests/clapstress.cpp -ldl -lpthread $(pkg-config --cflags --libs x11) -o build/clapstress
g++ -O1 -g -fsanitize=address -std=c++17 -Isrc -Igen tests/switchtest.cpp -o build/switchtest
echo "Testwerkzeuge: build/harness, build/hosttest, build/plugtest, build/rtcheck, build/swprobe, build/swrand, build/defaults, build/guitest, build/yguitest, build/b15guitest, build/gkguitest, build/g100guitest, build/b3guitest, build/svtguitest, build/weqguitest, build/u76guitest, build/b15test, build/b15amp, build/gktest, build/g100test, build/b3test, build/svttest, build/weqtest, build/u76test, build/clapstress, build/switchtest"
