#!/bin/bash
# Packt die Linux-Plugins aus engine/build (vorher: VST3=1 bash engine/build.sh) nach dist/:
#   <Name>_CLAP-<Version>.zip (<Name>.clap) und <Name>_VST3-<Version>.zip (Ordner <Name>.vst3), je mit README.txt und THIRD-PARTY-LICENSES.txt
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
B=engine/build; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
pack() {   # $1 Name, $2 Version, $3 README
  for f in "$B/$1.clap" "$B/vst3/$1.vst3"; do [ -e "$f" ] || { echo "fehlt: $f (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }; done
  rm -f "$D/$1"_CLAP-*.zip "$D/$1"_VST3-*.zip
  mkdir -p "$T/$1-c" "$T/$1-v"; cp "$B/$1.clap" "$T/$1-c/"; cp -r "$B/vst3/$1.vst3" "$T/$1-v/"
  for x in c v; do cp "reaper-clap/$3" "$T/$1-$x/README.txt"; cp reaper-clap/THIRD-PARTY-LICENSES.txt "$T/$1-$x/"; done
  # reproduzierbar (ab 2026-11-15): feste Zeitstempel, sortierte Reihenfolge, keine Zusatzfelder – gleicher Inhalt ergibt in jedem Release-Lauf
  # dieselben ZIP-Bytes (vorher wechselte die Prüfsumme mit jedem Lauf, und update-tonestack.sh lehnte ältere, inhaltsgleiche Pakete ab)
  for x in c v; do find "$T/$1-$x" -exec touch -h -d "2026-01-01 00:00:00 UTC" {} +; done
  (cd "$T/$1-c" && find . -mindepth 1 | LC_ALL=C sort | TZ=UTC zip -q -X -D -9 "$R/$D/$1_CLAP-$2.zip" -@)
  (cd "$T/$1-v" && find . -mindepth 1 | LC_ALL=C sort | TZ=UTC zip -q -X -9 "$R/$D/$1_VST3-$2.zip" -@)
  echo "$D/$1_CLAP-$2.zip, $D/$1_VST3-$2.zip"
}
pack Laney_AOR50 "$LANEY" README.txt
pack Lametta_B100II "$YAMAHA" README-B100II.txt
pack AmpFett_B15NF "$B15" README-B15NF.txt
pack GK_800RB "$GK" README-GK.txt
pack Lametta_G100 "$G100" README-G100.txt
pack Lametta_B100III "$B100III" README-B100III.txt
pack AmpFett_SVTCL "$SVTCL" README-SVTCL.txt
pack Wien_EQ "$WIENEQ" README-WienEQ.txt
pack AmpFett_76 "$AMPFETT76" README-AmpFett76.txt
pack Loldano_SLO100 "$LOLDANO" README-Loldano.txt
pack Kaiserin_Reverb "$KAISERIN" README-Kaiserin.txt
pack Blech_140 "$BLECH" README-Blech140.txt
