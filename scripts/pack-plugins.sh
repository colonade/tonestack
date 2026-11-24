#!/bin/bash
# Packt die Linux-Plugins aus engine/build (vorher: VST3=1 bash engine/build.sh) nach dist/:
#   <Name>_CLAP-<Version>.zip (<Name>.clap) und <Name>_VST3-<Version>.zip (Ordner <Name>.vst3), je mit README.txt, LICENSE.txt (GPL-3.0) und THIRD-PARTY-LICENSES.txt; Liste: scripts/plugins.txt
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"; bash scripts/check-plugins.sh   # Namen in plugins.txt nur mit harmlosen Zeichen
B=engine/build; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
pack() {   # $1 Name, $2 Version, $3 README
  for f in "$B/$1.clap" "$B/vst3/$1.vst3"; do [ -e "$f" ] || { echo "fehlt: $f (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }; done
  rm -f "$D/$1"_CLAP-*.zip "$D/$1"_VST3-*.zip
  mkdir -p "$T/$1-c" "$T/$1-v"; cp "$B/$1.clap" "$T/$1-c/"; cp -r "$B/vst3/$1.vst3" "$T/$1-v/"
  for x in c v; do cp "reaper-clap/$3" "$T/$1-$x/README.txt"; cp reaper-clap/THIRD-PARTY-LICENSES.txt "$T/$1-$x/"; cp LICENSE "$T/$1-$x/LICENSE.txt"; done
  # reproduzierbar (ab 2026-11-15): feste Zeitstempel, sortierte Reihenfolge, keine Zusatzfelder – gleicher Inhalt ergibt in jedem Release-Lauf
  # dieselben ZIP-Bytes (vorher wechselte die Prüfsumme mit jedem Lauf, und update-tonestack.sh lehnte ältere, inhaltsgleiche Pakete ab)
  for x in c v; do find "$T/$1-$x" -exec touch -h -d "2026-01-01 00:00:00 UTC" {} +; done
  (cd "$T/$1-c" && find . -mindepth 1 | LC_ALL=C sort | TZ=UTC zip -q -X -D -9 "$R/$D/$1_CLAP-$2.zip" -@)
  (cd "$T/$1-v" && find . -mindepth 1 | LC_ALL=C sort | TZ=UTC zip -q -X -9 "$R/$D/$1_VST3-$2.zip" -@)
  echo "$D/$1_CLAP-$2.zip, $D/$1_VST3-$2.zip"
}
while IFS='|' read -r NAME VV README SHORT DISP; do case "$NAME" in ''|'#'*) continue;; esac
  pack "$NAME" "${!VV}" "$README"; done < scripts/plugins.txt
