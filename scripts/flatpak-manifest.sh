#!/bin/bash
# Erzeugt die Dateien für eine Flathub-Erweiterung org.freedesktop.LinuxAudio.Plugins.Tonestack (ab 2026-11-27; noch nicht eingereicht, siehe
# packaging/flatpak/LIESMICH.md):   bash scripts/flatpak-manifest.sh <Ordner> [Commit, Standard HEAD] [Datum JJJJ-MM-TT, Standard: Commit-Datum]
# → <Ordner>/org.freedesktop.LinuxAudio.Plugins.Tonestack.json (baut aus den Quellen dieses Commits, ohne Netz: clap-wrapper, CLAP und VST3-SDK als
# feste Quellen), …metainfo.xml (Version = DEBVER), flathub.json. Prüfbau: .github/workflows/flatpak.yml.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; OUT="${1:?Ordner angeben}"; C="${2:-$(git -C "$R" rev-parse HEAD)}"
[[ "$C" =~ ^[0-9a-f]{40}$ ]] || { echo "Commit als 40 Hex-Zeichen angeben: $C" >&2; exit 1; }
D="${3:-$(git -C "$R" log -1 --format=%cs "$C")}"
[ "$C" = "$(git -C "$R" rev-parse HEAD)" ] || { echo "Commit muss der ausgecheckte sein (Versionen kommen aus dem Arbeitsbaum)" >&2; exit 1; }
eval "$(bash "$R/scripts/versions.sh")"
# clap-wrapper-Stand muss zu engine/vst3/CMakeLists.txt passen
W="$(git -C "$R" show "$C:engine/vst3/CMakeLists.txt" | sed -n 's/.*GIT_TAG \([0-9a-f]\{40\}\).*/\1/p' | head -n1)"
grep -q "\"commit\": \"$W\"" "$R/packaging/flatpak/org.freedesktop.LinuxAudio.Plugins.Tonestack.json.in" || { echo "clap-wrapper $W fehlt in der Vorlage" >&2; exit 1; }
mkdir -p "$OUT"
sed "s/@COMMIT@/$C/" "$R/packaging/flatpak/org.freedesktop.LinuxAudio.Plugins.Tonestack.json.in" > "$OUT/org.freedesktop.LinuxAudio.Plugins.Tonestack.json"
sed "s/@DEBVER@/$DEBVER/; s/@DATE@/$D/" "$R/packaging/flatpak/org.freedesktop.LinuxAudio.Plugins.Tonestack.metainfo.xml.in" > "$OUT/org.freedesktop.LinuxAudio.Plugins.Tonestack.metainfo.xml"
printf '{\n    "skip-icons-check": true\n}\n' > "$OUT/flathub.json"
python3 -c "import json,sys; json.load(open(sys.argv[1]))" "$OUT/org.freedesktop.LinuxAudio.Plugins.Tonestack.json"
echo "Flatpak-Erweiterung $DEBVER (Commit $C) in $OUT"
