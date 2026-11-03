#!/bin/bash
# Mac-Pakete aus dem CMake-Ergebnis: Bundles ad hoc signieren (ohne Apple-Konto; der Nutzer hebt die Quarantäne einmal auf),
# Architektur prüfen, Audio Units mit auval prüfen, je Plugin ein ZIP mit Anleitung.   mac/package.sh <build-Ordner> <Ausgabe-Ordner>
set -euo pipefail
B="$1"; OUT="$2"; HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/.."
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"   # absoluter Pfad (auch wenn ein absoluter oder relativer übergeben wird)
ver() { grep -o '#define TS_VERSION "[^"]*"' "$ROOT/engine/clap/$1" | cut -d'"' -f2; }
for spec in "Laney_AOR50 plugin.cpp Lany" "Yamaha_B100II yplugin.cpp B100" "AmpFett_B15NF b15plugin.cpp AFB1"; do
  set -- $spec; NAME="$1"; V="$(ver "$2")"; SUB="$3"
  VST="$(find "$B" -maxdepth 3 -name "$NAME.vst3" -type d | head -1)"; AU="$(find "$B" -maxdepth 3 -name "$NAME.component" -type d | head -1)"
  [ -n "$VST" ] && [ -n "$AU" ] || { echo "FEHLER: $NAME.vst3 oder $NAME.component fehlt"; find "$B" -maxdepth 3 -name "*.vst3" -o -maxdepth 3 -name "*.component"; exit 1; }
  for b in "$VST" "$AU"; do codesign --force --deep --sign - "$b"; codesign --verify --deep "$b"; lipo -info "$b"/Contents/MacOS/* ; done
  # Audio Unit wie Logic prüfen
  mkdir -p ~/Library/Audio/Plug-Ins/Components && rm -rf ~/Library/Audio/Plug-Ins/Components/"$NAME.component" && cp -R "$AU" ~/Library/Audio/Plug-Ins/Components/
  killall -9 AudioComponentRegistrar 2>/dev/null || true; sleep 2
  auval -v aufx "$SUB" Tnst | tail -25
  auval -v aufx "$SUB" Tnst | grep -q "AU VALIDATION SUCCEEDED" || { echo "FEHLER: auval für $NAME"; exit 1; }
  T="$(mktemp -d)"; mkdir -p "$T/$NAME"; cp -R "$AU" "$VST" "$T/$NAME/"; cp "$HERE/LIESMICH-Mac.txt" "$T/$NAME/LIESMICH.txt"
  cp "$ROOT/reaper-clap/THIRD-PARTY-LICENSES.txt" "$T/$NAME/"
  (cd "$T" && ditto -c -k --sequesterRsrc --keepParent "$NAME" "$OUT/${NAME}_Mac-$V.zip")
  echo "fertig: $OUT/${NAME}_Mac-$V.zip"
done
