#!/bin/bash
# Baut aus dem VST3-SDK, das clap-wrapper für den VST3-Bau holt (engine/build/vst3/cpm/vst3sdk, 3.8.0, Commit 9fad977), zwei Werkzeuge (ab 2026-11-27):
#   validator       Steinbergs Prüfprogramm (validator -e <Bundle>: alle Tests, auch die erweiterten)
#   moduleinfotool  erzeugt Contents/Resources/moduleinfo.json im Bundle (Hosts lesen daraus Name, Klassen und Version, ohne die .so zu laden)
#   bash engine/tools/vst3tools.sh [SDK-Ordner] [Bau-Ordner]   → <Bau-Ordner>/bin/Release/validator und moduleinfotool
# Die Beispiel-Hosts editorhost, audiohost und inspectorapp brauchen unter Linux gtkmm bzw. Qt und werden darum aus einer Kopie des SDK
# herausgelassen (das SDK nimmt jeden Unterordner von vst-hosting mit, ohne Schalter); VSTGUI wird nicht gebraucht.
set -euo pipefail
E="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${1:-$E/build/vst3/cpm/vst3sdk}"; OUT="${2:-$E/build/vst3tools}"
[ -f "$SDK/pluginterfaces/vst/ivstcomponent.h" ] || { echo "VST3-SDK nicht gefunden: $SDK (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }
if [ -x "$OUT/bin/Release/validator" ] && [ -x "$OUT/bin/Release/moduleinfotool" ] && [ "$(cat "$OUT/.sdk" 2>/dev/null)" = "$SDK" ]; then exit 0; fi
rm -rf "$OUT"; mkdir -p "$OUT/src"
tar -C "$SDK" --exclude=./public.sdk/samples/vst-hosting/editorhost --exclude=./public.sdk/samples/vst-hosting/audiohost \
    --exclude=./public.sdk/samples/vst-hosting/inspectorapp --exclude=./vstgui4 --exclude=./doc --exclude=./tutorials --exclude=./.git -cf - . | tar -C "$OUT/src" -xf -
# Das SDK linkt unter Linux fest gegen stdc++fs; neuere Übersetzer (z. B. im Flatpak-SDK) haben die Bibliothek nicht mehr, weil std::filesystem in
# libstdc++ selbst steckt – dann eine leere Ersatzbibliothek anbieten
LF=""; if ! echo 'int main(){}' | "${CXX:-c++}" -x c++ - -lstdc++fs -o "$OUT/fs-probe" >/dev/null 2>&1; then
  mkdir -p "$OUT/stub" && ar rc "$OUT/stub/libstdc++fs.a" && LF="-DCMAKE_EXE_LINKER_FLAGS=-L$OUT/stub"; fi; rm -f "$OUT/fs-probe"
cmake -S "$OUT/src" -B "$OUT" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=ON -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF \
    -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF -DSMTG_CREATE_PLUGIN_LINK=OFF -DSMTG_RUN_VST_VALIDATOR=OFF $LF > "$OUT/cfg.log" 2>&1 || { tail -n 30 "$OUT/cfg.log" >&2; exit 1; }
cmake --build "$OUT" --target validator moduleinfotool > "$OUT/build.log" 2>&1 || { tail -n 30 "$OUT/build.log" >&2; exit 1; }
echo "$SDK" > "$OUT/.sdk"
echo "VST3-Werkzeuge: $OUT/bin/Release/validator, $OUT/bin/Release/moduleinfotool"
