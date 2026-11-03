#!/bin/bash
# Baut web/tonestack-apache.zip für den Server: Seiten aus web/tonestack/ plus die Downloads des aktuellen Releases
# (Linux-ZIPs und Mac-Installerpaket) in download/. Die Links in web/tonestack/index.html müssen zu den Dateinamen passen.
#   bash scripts/pack-web.sh            (Downloads aus dem GitHub-Release, braucht gh)
#   LOCAL=1 bash scripts/pack-web.sh    (Downloads aus dist/)
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
REPO="${REPO:-colonade/tonestack}"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
cp -r web/tonestack "$T/"; rm -rf "$T/tonestack/download"; mkdir -p "$T/tonestack/download"
if [ "${LOCAL:-0}" = 1 ]; then cp dist/*.zip dist/*.pkg "$T/tonestack/download/"
else   # über die REST-Schnittstelle (gh api), nur .zip und .pkg
  gh api "repos/$REPO/releases/tags/$TAG" --jq '.assets[] | select(.name | test("\\.(zip|pkg)$")) | "\(.id) \(.name)"' | while read -r id name; do
    gh api -H "Accept: application/octet-stream" "repos/$REPO/releases/assets/$id" > "$T/tonestack/download/$name"; done
fi
for f in $(grep -o 'href="download/[^"]*"' web/tonestack/index.html | cut -d'"' -f2); do
  [ -f "$T/tonestack/$f" ] || { echo "FEHLER: index.html verlinkt $f, im Release nicht vorhanden" >&2; exit 1; }; done
chmod -R u=rwX,go=rX "$T/tonestack"; rm -f web/tonestack-apache.zip; (cd "$T" && zip -qr9 "$R/web/tonestack-apache.zip" tonestack)
ls "$T/tonestack/download"; echo "web/tonestack-apache.zip"
