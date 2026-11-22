#!/bin/bash
# Lädt die Plugin-Dateien eines Ordners in das GitHub-Release laney-<V>_b100ii-<V>_b15nf-<V>_gk800rb-<V>_g100-<V>_b100iii-<V>_svtcl-<V>_wieneq-<V>_ampfett76-<V>_loldano-<V>_kaiserin-<V>_blech140-<V>_wisent-<V>_viereck34-<V>_cultec-<V> (legt es an, ersetzt gleichnamige Dateien)
# und löscht alle älteren Releases samt Tags (Binärdateien lassen sich aus dem jeweiligen Commit neu bauen).
# Läuft normalerweise im Bau auf GitHub (.github/workflows/plugins.yml); von Hand mit angemeldetem gh: bash scripts/release.sh dist
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
DIR="${1:?Ordner mit den Dateien angeben}"; REPO="${GITHUB_REPOSITORY:-colonade/tonestack}"; SHA="${GITHUB_SHA:-$(git rev-parse HEAD)}"
shopt -s nullglob; FILES=("$DIR"/*.zip "$DIR"/*.pkg "$DIR"/*.exe "$DIR"/*.deb "$DIR"/*.png "$DIR"/SHA256SUMS*.txt "$DIR"/*.orig.tar.* "$DIR"/*.debian.tar.* "$DIR"/*.diff.gz "$DIR"/*.dsc); shopt -u nullglob
[ ${#FILES[@]} -gt 0 ] || { echo "keine Dateien in $DIR" >&2; exit 1; }
for f in "${FILES[@]}"; do case "$(basename "$f")" in   # nur Dateien der aktuellen Versionen
  *_CLAP-*.zip|*_VST3-*.zip|*_Windows-*.zip) b="$(basename "$f")"; ok=0   # Name und Version aus scripts/plugins.txt
    while IFS='|' read -r NAME VV REST; do case "$NAME" in ''|'#'*) continue;; esac
      for k in CLAP VST3 Windows; do [ "$b" = "${NAME}_$k-${!VV}.zip" ] && ok=1; done; done < scripts/plugins.txt
    [ "$ok" = 1 ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
  *.pkg) [ "$(basename "$f")" = "$PKG" ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
  *.exe) [ "$(basename "$f")" = "$WINEXE" ] || { echo "unerwartete Datei: $f" >&2; exit 1; };;
  *.deb) [ "$(basename "$f")" = "tonestack-plugins_${DEBVER}_amd64.deb" ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
  *.orig.tar.*|*.debian.tar.*|*.diff.gz|*.dsc) case "$(basename "$f")" in cairo_1.18.0*|pixman_0.42.2*) ;; *) echo "unerwartete Quelldatei: $f" >&2; exit 1;; esac;;
esac; done
NOTES="Laney AOR 50 Preamp $LANEY · Lametta B100-II Bass Amp $YAMAHA · AmpFett B-15NF Bass Amp $B15 · GK 800RB Bass Amp $GK · Lametta G100-212II Guitar Amp $G100 · Lametta B100-III Bass Amp $B100III · AmpFett SVT-CL Bass Amp $SVTCL · Wien EQ $WIENEQ · AmpFett 76 Limiting Amplifier $AMPFETT76 · Loldano SLO-100 Guitar Amp $LOLDANO · Kaiserin Reverb $KAISERIN · Blech 140 Plate Reverb $BLECH · Wisent Echoreck Drum Echo $WISENT · Viereck 34 Tilt EQ $VIERECK · Cultec EQ-1A Program EQ $CULTEC

- Linux (x86-64): je Plugin CLAP und VST3 als ZIP, mit Anleitung (README.txt); alle zusammen als Debian/Ubuntu-Paket tonestack-plugins_${DEBVER}_amd64.deb (glibc ≥ 2.38: Debian 13, Ubuntu 24.04 und neuer).
- Windows (x86-64, Windows 10/11): Installer tonestack-Plugins-Windows.exe (VST3 und CLAP, Auswahl je Plugin) oder je Plugin ein ZIP. Nicht signiert: SmartScreen → „Weitere Informationen“ → „Trotzdem ausführen“.
- Mac (Audio Unit für Logic, VST3; Apple Silicon und Intel ab macOS 11): Installerpaket tonestack-Plugins-Mac.pkg. Nicht bei Apple beglaubigt: beim ersten Öffnen Systemeinstellungen → Datenschutz & Sicherheit → „Dennoch öffnen“.
- Prüfbilder der Mac- und Windows-Oberfläche (mac-*.png, win-*.png), SHA-256-Summen.
- Quelltext von Cairo 1.18.0 und pixman 0.42.2 (Ubuntu-Quellpakete), die statisch in den Windows-Plugins stecken (LGPL-2.1 bzw. MIT; Bau: engine/win/build-cairo.sh).

Gebaut auf GitHub aus Commit $SHA. Änderungen: README.txt in den ZIPs."
if gh release view "$TAG" -R "$REPO" >/dev/null 2>&1; then gh release edit "$TAG" --title "Laney $LANEY / B100-II $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ / AmpFett 76 $AMPFETT76 / Loldano $LOLDANO / Kaiserin Reverb $KAISERIN / Blech 140 $BLECH / Wisent Echoreck $WISENT / Viereck 34 $VIERECK / Cultec EQ-1A $CULTEC" --notes "$NOTES" --latest -R "$REPO" >/dev/null
else gh release create "$TAG" --target "$SHA" --title "Laney $LANEY / B100-II $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ / AmpFett 76 $AMPFETT76 / Loldano $LOLDANO / Kaiserin Reverb $KAISERIN / Blech 140 $BLECH / Wisent Echoreck $WISENT / Viereck 34 $VIERECK / Cultec EQ-1A $CULTEC" --notes "$NOTES" --latest -R "$REPO"; fi
# Gleicher Name, gleicher Inhalt (Regel seit 2026-11-15): liegt eine Datei schon im Release dieser Versionen, muss sie bytegleich sein – sonst hat sich
# etwas geändert, ohne dass eine Version stieg (z. B. neuer Übersetzer auf dem Bau-Rechner). Ausgenommen: Mac-Paket (nicht reproduzierbar, inhaltsgleich
# geprüft), seine Prüfsummen und die Prüfbilder (Schriftdarstellung des Bau-Rechners).
gh api "repos/$REPO/releases/tags/$TAG" --jq '.assets[] | "\(.name) \(.digest)"' > "$DIR/.alt" 2>/dev/null || : > "$DIR/.alt"
bad=0; for f in "${FILES[@]}"; do b="$(basename "$f")"; case "$b" in *.pkg|*.png|SHA256SUMS-mac.txt) continue;; esac
  alt="$(awk -v n="$b" '$1 == n { print $2 }' "$DIR/.alt")"; [ -n "$alt" ] || continue
  neu="sha256:$(sha256sum "$f" | cut -d' ' -f1)"; [ "$alt" = "$neu" ] || { echo "::error::$b liegt im Release $TAG schon mit anderem Inhalt ($alt, neu $neu) – Version anheben" >&2; bad=1; }; done
rm -f "$DIR/.alt"; [ "$bad" = 0 ] || exit 1
gh release upload "$TAG" "${FILES[@]}" --clobber -R "$REPO"
for old in $(gh api "repos/$REPO/releases" --paginate --jq '.[].tag_name'); do
  [ "$old" = "$TAG" ] && continue; [[ "$old" == laney-*_yamaha-* || "$old" == laney-*_b100ii-* ]] || continue   # nur eigene Releases
  echo "lösche altes Release $old"; gh release delete "$old" --yes --cleanup-tag -R "$REPO"; done
echo "Release $TAG: https://github.com/$REPO/releases/tag/$TAG"
