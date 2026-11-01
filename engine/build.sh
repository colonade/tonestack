#!/bin/bash
# Baut CLAP-Plugins und WebAssembly-Kerne aus den Quellen.  bash engine/build.sh [--wasi /pfad/zu/wasi-sdk]
# Mit VST3=1 zusätzlich die VST3-Fassungen (cmake, ninja; holt beim ersten Mal clap-wrapper und VST3-SDK aus GitHub) → engine/build/vst3/*.vst3
# Voraussetzungen: g++ (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; für WebAssembly das wasi-sdk 24
# (https://github.com/WebAssembly/wasi-sdk/releases, Standard /opt/wasi-sdk). Ergebnis in engine/build/.
set -euo pipefail
E="$(cd "$(dirname "$0")" && pwd)"; R="$(dirname "$E")"; WASI="${WASI:-/opt/wasi-sdk}"
[ "${1:-}" = "--wasi" ] && WASI="$2"
cd "$E"; mkdir -p build
node tools/extract.js "$R"; python3 tools/gen_geo.py gen; if python3 -c "import PIL" 2>/dev/null; then python3 tools/b15_panel.py "$R" >/dev/null; else echo "Pillow fehlt: Frontplatte B-15NF wie eingecheckt (engine/gen)"; fi
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/plugin.cpp -o build/Laney_AOR50.clap $(pkg-config --cflags --libs cairo x11)
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/yplugin.cpp -o build/Yamaha_B100II.clap $(pkg-config --cflags --libs cairo x11)
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/b15plugin.cpp -o build/AmpFett_B15NF.clap $(pkg-config --cflags --libs cairo x11) -lpthread
echo "CLAP: build/Laney_AOR50.clap, build/Yamaha_B100II.clap, build/AmpFett_B15NF.clap"
if [ "${VST3:-0}" = 1 ]; then
  cmake -S vst3 -B build/vst3 -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build/vst3 >/dev/null
  echo "VST3: build/vst3/Laney_AOR50.vst3, build/vst3/Yamaha_B100II.vst3, build/vst3/AmpFett_B15NF.vst3"
fi
if [ -x "$WASI/bin/clang++" ]; then
  W="$WASI/bin/clang++ -O3 -std=c++17 -fno-exceptions -fno-rtti --target=wasm32-wasi -mexec-model=reactor -Wl,--export-memory -Wl,--strip-all -Isrc -Igen"
  $W -Wl,--export=aor_buffer -Wl,--export=aor_parts_buffer -Wl,--export=aor_target_buffer -Wl,--export=aor_peaks_buffer -Wl,--export=aor_parts_count \
     -Wl,--export=aor_init -Wl,--export=aor_set_target -Wl,--export=aor_process -Wl,--export=aor_take_peaks -Wl,--export=aor_ss_buffer -Wl,--export=aor_small_signal -Wl,--export=aor_latency -Wl,--initial-memory=16777216 wasm/wasm_api.cpp -o build/laney-core.wasm
  $W -Wl,--export=y_buffer -Wl,--export=y_parts_buffer -Wl,--export=y_target_buffer -Wl,--export=y_parts_count -Wl,--export=y_init \
     -Wl,--export=y_set_target -Wl,--export=y_process -Wl,--export=y_take_peak -Wl,--initial-memory=8388608 wasm/yamaha_wasm.cpp -o build/b100ii-core.wasm
  $W -Wl,--export=bf_buffer -Wl,--export=bf_target_buffer -Wl,--export=bf_peaks_buffer -Wl,--export=bf_freq_buffer -Wl,--export=bf_resp_buffer -Wl,--export=bf_init \
     -Wl,--export=bf_latency -Wl,--export=bf_set_target -Wl,--export=bf_process -Wl,--export=bf_take_peaks -Wl,--export=bf_plot_init -Wl,--export=bf_plot -Wl,--initial-memory=16777216 wasm/b15_wasm.cpp -o build/b15nf-core.wasm
  echo "WebAssembly: build/laney-core.wasm, build/b100ii-core.wasm, build/b15nf-core.wasm"
else echo "wasi-sdk nicht gefunden ($WASI): WebAssembly übersprungen"; fi
g++ -O3 -std=c++17 -Isrc -Igen tests/harness.cpp -o build/harness
g++ -O2 -std=c++17 -Ithird_party tests/hosttest.cpp -ldl -o build/hosttest
g++ -O2 -std=c++17 -Ithird_party tests/plugtest.cpp -ldl -o build/plugtest
g++ -O2 -std=c++17 -Ithird_party tests/defaults.cpp -ldl -o build/defaults
g++ -O2 -std=c++17 -Ithird_party tests/guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/guitest
g++ -O2 -std=c++17 -Ithird_party tests/yguitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/yguitest
g++ -O2 -std=c++17 -Ithird_party tests/b15guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/b15guitest
g++ -O3 -std=c++17 -Isrc -Igen tests/b15test.cpp -o build/b15test; g++ -O3 -std=c++17 -Isrc -Igen tests/b15amp.cpp -o build/b15amp
g++ -O1 -g -std=c++17 -Ithird_party tests/clapstress.cpp -ldl -lpthread $(pkg-config --cflags --libs x11) -o build/clapstress
g++ -O1 -g -fsanitize=address -std=c++17 -Isrc -Igen tests/switchtest.cpp -o build/switchtest
echo "Testwerkzeuge: build/harness, build/hosttest, build/plugtest, build/defaults, build/guitest, build/yguitest, build/b15guitest, build/b15test, build/b15amp, build/clapstress, build/switchtest"
