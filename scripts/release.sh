#!/bin/bash
# Veröffentlicht die Linux-Plugins (dist/*.zip aus pack-plugins.sh) als GitHub-Release laney-<V>_yamaha-<V>
# und löscht alle älteren Releases samt Tags (Binärdateien lassen sich aus dem jeweiligen Commit neu bauen).
# Das Mac-Installerpaket hängt der Mac-Bau (.github/workflows/mac-plugins.yml) an dasselbe Release.
# Braucht gh (angemeldet, Schreibrechte).   bash scripts/release.sh
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
REPO="${REPO:-colonade/tonestack}"
FILES=(dist/Laney_AOR50_CLAP-"$LANEY".zip dist/Laney_AOR50_VST3-"$LANEY".zip dist/Yamaha_B100II_CLAP-"$YAMAHA".zip dist/Yamaha_B100II_VST3-"$YAMAHA".zip)
for f in "${FILES[@]}"; do [ -f "$f" ] || { echo "fehlt: $f (erst bash scripts/pack-plugins.sh)" >&2; exit 1; }; done
SHA="$(git rev-parse HEAD)"
git fetch -q origin main; [ "$SHA" = "$(git rev-parse origin/main)" ] || { echo "HEAD ist nicht origin/main – erst pushen" >&2; exit 1; }
NOTES="Laney AOR 50 Preamp $LANEY · Yamaha B100-II Bass Amp $YAMAHA

Linux (x86-64): je Verstärker CLAP und VST3 als ZIP mit Anleitung (README.txt).
Mac (Audio Unit für Logic und VST3): Installerpaket tonestack-Plugins-Mac_….pkg, angehängt vom Mac-Bau.
Gebaut aus Commit $SHA. Änderungen: README.txt im ZIP (Abschnitt $LANEY bzw. $YAMAHA)."
if gh release view "$TAG" -R "$REPO" >/dev/null 2>&1; then gh release upload "$TAG" "${FILES[@]}" --clobber -R "$REPO"; gh release edit "$TAG" --notes "$NOTES" -R "$REPO" >/dev/null
else gh release create "$TAG" "${FILES[@]}" --target "$SHA" --title "Laney $LANEY / Yamaha $YAMAHA" --notes "$NOTES" --latest -R "$REPO"; fi
for old in $(gh api "repos/$REPO/releases" --paginate --jq '.[].tag_name'); do
  [ "$old" = "$TAG" ] && continue; echo "lösche altes Release $old"; gh release delete "$old" --yes --cleanup-tag -R "$REPO"; done
echo "Release $TAG: https://github.com/$REPO/releases/tag/$TAG"
