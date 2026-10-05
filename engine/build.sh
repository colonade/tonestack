#!/bin/bash
# Baut CLAP-Plugin und WebAssembly-Kerne aus den Quellen.  bash engine/build.sh [--wasi /pfad/zu/wasi-sdk]
# Voraussetzungen: g++ (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; für WebAssembly das wasi-sdk 24
# (https://github.com/WebAssembly/wasi-sdk/releases, Standard /opt/wasi-sdk). Ergebnis in engine/build/.
set -euo pipefail
E="$(cd "$(dirname "$0")" && pwd)"; R="$(dirname "$E")"; WASI="${WASI:-/opt/wasi-sdk}"
[ "${1:-}" = "--wasi" ] && WASI="$2"
cd "$E"; mkdir -p build
node tools/extract.js "$R"; python3 tools/gen_geo.py gen
g++ -O3 -std=c++17 -fPIC -shared -fvisibility=hidden -static-libgcc -static-libstdc++ -fmacro-prefix-map=third_party/= \
    -Isrc -Igen -Ithird_party clap/plugin.cpp -o build/Laney_AOR50.clap $(pkg-config --cflags --libs cairo x11)
echo "CLAP: build/Laney_AOR50.clap"
if [ -x "$WASI/bin/clang++" ]; then
  W="$WASI/bin/clang++ -O3 -std=c++17 -fno-exceptions -fno-rtti --target=wasm32-wasi -mexec-model=reactor -Wl,--export-memory -Wl,--strip-all -Isrc -Igen"
  $W -Wl,--export=aor_buffer -Wl,--export=aor_parts_buffer -Wl,--export=aor_target_buffer -Wl,--export=aor_peaks_buffer -Wl,--export=aor_parts_count \
     -Wl,--export=aor_init -Wl,--export=aor_set_target -Wl,--export=aor_process -Wl,--export=aor_take_peaks -Wl,--initial-memory=16777216 wasm/wasm_api.cpp -o build/laney-core.wasm
  $W -Wl,--export=y_buffer -Wl,--export=y_parts_buffer -Wl,--export=y_target_buffer -Wl,--export=y_parts_count -Wl,--export=y_init \
     -Wl,--export=y_set_target -Wl,--export=y_process -Wl,--initial-memory=8388608 wasm/yamaha_wasm.cpp -o build/b100ii-core.wasm
  echo "WebAssembly: build/laney-core.wasm, build/b100ii-core.wasm"
else echo "wasi-sdk nicht gefunden ($WASI): WebAssembly übersprungen"; fi
g++ -O3 -std=c++17 -Isrc -Igen tests/harness.cpp -o build/harness
g++ -O2 -std=c++17 -Ithird_party tests/hosttest.cpp -ldl -o build/hosttest
g++ -O2 -std=c++17 -Ithird_party tests/guitest.cpp -ldl $(pkg-config --cflags --libs cairo x11) -o build/guitest
echo "Testwerkzeuge: build/harness, build/hosttest, build/guitest"
