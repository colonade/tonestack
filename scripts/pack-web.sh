#!/bin/bash
# Baut die Webseite: Seiten aus web/tonestack/ plus die Downloads des aktuellen Releases (Linux- und Windows-ZIPs, Debian-Paket,
# Windows-Installer, Mac-Installerpaket, Quelltext von Cairo/pixman für die Windows-Plugins) in download/.
# Ab 2026-11-20 liegt die Seite auf GitHub Pages (https://tonestack.fluxstatus.de/, Workflow .github/workflows/pages.yml):
#   SITE=<ordner> bash scripts/pack-web.sh   schreibt die fertige Seite in <ordner> (ohne .htaccess und LIESMICH.txt, die nur der Apache brauchte)
# Ohne SITE wie bisher web/tonestack-apache.zip (für einen eigenen Apache oder zum Ansehen offline). Die Links in web/tonestack/index.html (Mac) und web/tonestack/plugins/index.html
# (Linux, ab 2026-11-15) müssen zu den Dateinamen passen.
#   bash scripts/pack-web.sh            (Downloads aus dem GitHub-Release, braucht gh)
#   LOCAL=1 bash scripts/pack-web.sh    (Downloads aus dist/)
# Ab 2026-11-15 zusätzlich: download/SHA256SUMS.txt im Paket (vom Installationsskript geprüft) und web/tonestack-apache.zip.sha256
# daneben (das Installationsskript verlangt sie). Aus dem Release geladene Dateien werden gegen dessen SHA256SUMS-*.txt geprüft.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
REPO="${REPO:-colonade/tonestack}"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
cp -r web/tonestack "$T/"; rm -rf "$T/tonestack/download"; mkdir -p "$T/tonestack/download"
if [ "${LOCAL:-0}" = 1 ]; then cp dist/*.zip dist/*.pkg dist/*.exe dist/*.deb dist/*.orig.tar.* dist/*.debian.tar.* dist/*.diff.gz dist/*.dsc "$T/tonestack/download/"
else   # über die REST-Schnittstelle (gh api), nur .zip, .pkg, .exe, .deb
  mkdir -p "$T/sums"
  gh api "repos/$REPO/releases/tags/$TAG" --jq '.assets[] | select(.name | test("\\.(zip|pkg|exe|deb|dsc|diff\\.gz)$|\\.(orig|debian)\\.tar\\.(xz|gz)$|^SHA256SUMS.*\\.txt$")) | "\(.id) \(.name)"' | while read -r id name; do
    case "$name" in SHA256SUMS*) o="$T/sums/$name";; *) o="$T/tonestack/download/$name";; esac
    gh api -H "Accept: application/octet-stream" "repos/$REPO/releases/assets/$id" > "$o"; done
  ls "$T"/sums/SHA256SUMS*.txt >/dev/null 2>&1 || { echo "FEHLER: Release ohne SHA256SUMS-*.txt" >&2; exit 1; }
  ( cd "$T/tonestack/download" && for f in *; do grep -hE "  $f\$" "$T"/sums/SHA256SUMS*.txt | head -n1 | grep . >/dev/null || { echo "FEHLER: $f nicht in den Prüfsummen des Releases" >&2; exit 1; }; done
    cat "$T"/sums/SHA256SUMS*.txt | sha256sum --quiet -c --ignore-missing ) || { echo "FEHLER: Download passt nicht zur Prüfsumme des Releases" >&2; exit 1; }
fi
# Mac-Paket: fester Name (ab 2026-11-12). Releases davor trugen die Versionen im Namen; ein solches Paket wird hier umbenannt.
for p in "$T/tonestack/download"/tonestack-Plugins-Mac_*.pkg; do [ -f "$p" ] && mv "$p" "$T/tonestack/download/tonestack-Plugins-Mac.pkg"; done
for f in $(grep -oh 'href="\(\.\./\)\?download/[^"]*"' web/tonestack/index.html web/tonestack/plugins/index.html | cut -d'"' -f2 | sed 's|^\.\./||'); do
  [ -f "$T/tonestack/$f" ] || { echo "FEHLER: eine Seite verlinkt $f, im Release nicht vorhanden" >&2; exit 1; }; done
( cd "$T/tonestack/download" && sha256sum -- * > SHA256SUMS.txt )
if [ -n "${SITE:-}" ]; then
  rm -f "$T/tonestack/.htaccess" "$T/tonestack/LIESMICH.txt"; mkdir -p "$SITE"; [ -z "$(ls -A "$SITE")" ] || { echo "FEHLER: $SITE ist nicht leer" >&2; exit 1; }
  chmod -R u=rwX,go=rX "$T/tonestack"; cp -r "$T/tonestack/." "$SITE/"; ls "$SITE/download"; echo "Seite in $SITE"; exit 0; fi
chmod -R u=rwX,go=rX "$T/tonestack"; rm -f web/tonestack-apache.zip web/tonestack-apache.zip.sha256; (cd "$T" && zip -qr9 "$R/web/tonestack-apache.zip" tonestack)
( cd web && sha256sum tonestack-apache.zip > tonestack-apache.zip.sha256 )
ls "$T/tonestack/download"; echo "web/tonestack-apache.zip"; cat web/tonestack-apache.zip.sha256
