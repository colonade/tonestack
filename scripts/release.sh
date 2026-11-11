#!/bin/bash
# Lädt die Plugin-Dateien eines Ordners in das GitHub-Release laney-<V>_yamaha-<V>_b15nf-<V>_gk800rb-<V>_g100-<V>_b100iii-<V>_svtcl-<V>_wieneq-<V> (legt es an, ersetzt gleichnamige Dateien)
# und löscht alle älteren Releases samt Tags (Binärdateien lassen sich aus dem jeweiligen Commit neu bauen).
# Läuft normalerweise im Bau auf GitHub (.github/workflows/plugins.yml); von Hand mit angemeldetem gh: bash scripts/release.sh dist
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
DIR="${1:?Ordner mit den Dateien angeben}"; REPO="${GITHUB_REPOSITORY:-colonade/tonestack}"; SHA="${GITHUB_SHA:-$(git rev-parse HEAD)}"
shopt -s nullglob; FILES=("$DIR"/*.zip "$DIR"/*.pkg "$DIR"/*.png "$DIR"/SHA256SUMS*.txt); shopt -u nullglob
[ ${#FILES[@]} -gt 0 ] || { echo "keine Dateien in $DIR" >&2; exit 1; }
for f in "${FILES[@]}"; do case "$(basename "$f")" in   # nur Dateien der aktuellen Versionen
  *_CLAP-*.zip|*_VST3-*.zip) case "$(basename "$f")" in Laney_AOR50_*-"$LANEY".zip|Yamaha_B100II_*-"$YAMAHA".zip|AmpFett_B15NF_*-"$B15".zip|GK_800RB_*-"$GK".zip|Yamaha_G100_*-"$G100".zip|Yamaha_B100III_*-"$B100III".zip|AmpFett_SVTCL_*-"$SVTCL".zip|Wien_EQ_*-"$WIENEQ".zip) ;; *) echo "Version passt nicht: $f" >&2; exit 1;; esac;;
  *.pkg) [ "$(basename "$f")" = "$PKG" ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
esac; done
NOTES="Laney AOR 50 Preamp $LANEY · Yamaha B100-II Bass Amp $YAMAHA · AmpFett B-15NF Bass Amp $B15 · GK 800RB Bass Amp $GK · Yamaha G100-212II Guitar Amp $G100 · Yamaha B100-III Bass Amp $B100III · AmpFett SVT-CL Bass Amp $SVTCL · Wien EQ $WIENEQ

- Linux (x86-64): je Plugin CLAP und VST3 als ZIP, mit Anleitung (README.txt).
- Mac (Audio Unit für Logic, VST3; Apple Silicon und Intel ab macOS 11): Installerpaket tonestack-Plugins-Mac_….pkg. Nicht bei Apple beglaubigt: beim ersten Öffnen Systemeinstellungen → Datenschutz & Sicherheit → „Dennoch öffnen“.
- Prüfbilder der Mac-Oberfläche (mac-*.png), SHA-256-Summen.

Gebaut auf GitHub aus Commit $SHA. Änderungen: README.txt in den ZIPs."
if gh release view "$TAG" -R "$REPO" >/dev/null 2>&1; then gh release edit "$TAG" --title "Laney $LANEY / Yamaha $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ" --notes "$NOTES" --latest -R "$REPO" >/dev/null
else gh release create "$TAG" --target "$SHA" --title "Laney $LANEY / Yamaha $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ" --notes "$NOTES" --latest -R "$REPO"; fi
gh release upload "$TAG" "${FILES[@]}" --clobber -R "$REPO"
for old in $(gh api "repos/$REPO/releases" --paginate --jq '.[].tag_name'); do
  [ "$old" = "$TAG" ] && continue; [[ "$old" == laney-*_yamaha-* ]] || continue   # nur eigene Releases
  echo "lösche altes Release $old"; gh release delete "$old" --yes --cleanup-tag -R "$REPO"; done
echo "Release $TAG: https://github.com/$REPO/releases/tag/$TAG"
