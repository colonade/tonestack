#!/bin/bash
# Lädt die Plugin-Dateien eines Ordners in das GitHub-Release laney-<V>_b100ii-<V>_b15nf-<V>_gk800rb-<V>_g100-<V>_b100iii-<V>_svtcl-<V>_wieneq-<V>_ampfett76-<V>_loldano-<V>_kaiserin-<V>_blech140-<V>_wisent-<V>_viereck34-<V>_cultec-<V> (legt es an, ersetzt gleichnamige Dateien)
# und löscht alle älteren Releases samt Tags (Binärdateien lassen sich aus dem jeweiligen Commit neu bauen).
# Läuft normalerweise im Bau auf GitHub (.github/workflows/plugins.yml); von Hand mit angemeldetem gh: bash scripts/release.sh dist
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"; bash scripts/check-plugins.sh   # Namen in plugins.txt nur mit harmlosen Zeichen
DIR="${1:?Ordner mit den Dateien angeben}"; REPO="${GITHUB_REPOSITORY:-colonade/tonestack}"; SHA="${GITHUB_SHA:-$(git rev-parse HEAD)}"
# ab 2026-11-27 auch Linux ARM64 (Debian-Paket), die Archive tonestack-plugins-<DEBVER>-linux-<arch>.tar.gz und – wenn der Bau sie signiert hat – die
# Dateien der apt-Paketquelle (scripts/apt-repo.sh)
shopt -s nullglob; FILES=("$DIR"/*.zip "$DIR"/*.pkg "$DIR"/*.exe "$DIR"/*.deb "$DIR"/tonestack-plugins-*-linux-*.tar.gz "$DIR"/*.png "$DIR"/SHA256SUMS*.txt "$DIR"/*.orig.tar.* "$DIR"/*.debian.tar.* "$DIR"/*.diff.gz "$DIR"/*.dsc
  "$DIR"/InRelease "$DIR"/Release "$DIR"/Release.gpg "$DIR"/Packages "$DIR"/Packages.gz "$DIR"/tonestack-archive-keyring.gpg "$DIR"/tonestack-archive-keyring.asc "$DIR"/tonestack.sources); shopt -u nullglob
[ ${#FILES[@]} -gt 0 ] || { echo "keine Dateien in $DIR" >&2; exit 1; }
for f in "${FILES[@]}"; do case "$(basename "$f")" in   # nur Dateien der aktuellen Versionen
  *_CLAP-*.zip|*_VST3-*.zip|*_Windows-*.zip) b="$(basename "$f")"; ok=0   # Name und Version aus scripts/plugins.txt
    while IFS='|' read -r NAME VV REST; do case "$NAME" in ''|'#'*) continue;; esac
      for k in CLAP VST3 Windows; do [ "$b" = "${NAME}_$k-${!VV}.zip" ] && ok=1; done; done < scripts/plugins.txt
    [ "$ok" = 1 ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
  *.pkg) [ "$(basename "$f")" = "$PKG" ] || { echo "Version passt nicht: $f" >&2; exit 1; };;
  *.exe) [ "$(basename "$f")" = "$WINEXE" ] || { echo "unerwartete Datei: $f" >&2; exit 1; };;
  *.deb) case "$(basename "$f")" in "tonestack-plugins_${DEBVER}_amd64.deb"|"tonestack-plugins_${DEBVER}_arm64.deb") ;; *) echo "Version passt nicht: $f" >&2; exit 1;; esac;;
  tonestack-plugins-*-linux-*.tar.gz) case "$(basename "$f")" in "tonestack-plugins-${DEBVER}-linux-x86_64.tar.gz"|"tonestack-plugins-${DEBVER}-linux-aarch64.tar.gz") ;; *) echo "Version passt nicht: $f" >&2; exit 1;; esac;;
  *.orig.tar.*|*.debian.tar.*|*.diff.gz|*.dsc) case "$(basename "$f")" in cairo_1.18.0*|pixman_0.42.2*) ;; *) echo "unerwartete Quelldatei: $f" >&2; exit 1;; esac;;
esac; done
NOTES="Laney AOR 50 Preamp $LANEY · Lametta B100-II Bass Amp $YAMAHA · AmpFett B-15NF Bass Amp $B15 · GK 800RB Bass Amp $GK · Lametta G100-212II Guitar Amp $G100 · Lametta B100-III Bass Amp $B100III · AmpFett SVT-CL Bass Amp $SVTCL · Wien EQ $WIENEQ · AmpFett 76 Limiting Amplifier $AMPFETT76 · Loldano SLO-100 Guitar Amp $LOLDANO · Kaiserin Reverb $KAISERIN · Blech 140 Plate Reverb $BLECH · Wisent Echoreck Drum Echo $WISENT · Viereck 34 Tilt EQ $VIERECK · Cultec EQ-1A Program EQ $CULTEC

- Linux (x86-64): je Plugin CLAP und VST3 als ZIP, mit Anleitung (README.txt); alle zusammen als Debian/Ubuntu-Paket tonestack-plugins_${DEBVER}_amd64.deb (glibc ≥ 2.38: Debian 13, Ubuntu 24.04 und neuer; installieren mit sudo apt install ./tonestack-plugins_${DEBVER}_amd64.deb).
- Linux (ARM64): Debian/Ubuntu-Paket tonestack-plugins_${DEBVER}_arm64.deb.
- Linux ohne Debian-Paket (andere Distributionen, Flatpak-Hosts): Archiv tonestack-plugins-${DEBVER}-linux-x86_64.tar.gz bzw. -aarch64.tar.gz, ./install.sh kopiert nach ~/.clap und ~/.vst3.
- Windows (x86-64, Windows 10/11): Installer tonestack-Plugins-Windows.exe (VST3 und CLAP, Auswahl je Plugin) oder je Plugin ein ZIP. Nicht signiert: SmartScreen → „Weitere Informationen“ → „Trotzdem ausführen“.
- Mac (Audio Unit für Logic, VST3; Apple Silicon und Intel ab macOS 11): Installerpaket tonestack-Plugins-Mac.pkg. Nicht bei Apple beglaubigt: beim ersten Öffnen Systemeinstellungen → Datenschutz & Sicherheit → „Dennoch öffnen“.
- Prüfbilder der Mac- und Windows-Oberfläche (mac-*.png, win-*.png), SHA-256-Summen. Herkunftsnachweis jeder Datei (GitHub-Attestierung): gh attestation verify <Datei> -R colonade/tonestack.
- Quelltext von Cairo 1.18.0 und pixman 0.42.2 (Ubuntu-Quellpakete), die statisch in den Windows-Plugins stecken (LGPL-2.1 bzw. MIT; Bau: engine/win/build-cairo.sh).

Gebaut auf GitHub aus Commit $SHA. Änderungen: README.txt in den ZIPs."
# Gleicher Name, gleicher Inhalt (Regel seit 2026-11-15): liegt eine Datei schon im Release dieser Versionen, muss sie bytegleich sein – sonst hat sich
# etwas geändert, ohne dass eine Version stieg (z. B. neuer Übersetzer auf dem Bau-Rechner). Ausgenommen: Mac-Paket (nicht reproduzierbar, inhaltsgleich
# geprüft), seine Prüfsummen und die Prüfbilder (Schriftdarstellung des Bau-Rechners).
# Ab 2026-11-24 vor dem Anlegen/Ändern des Releases, und nur „nicht gefunden“ (HTTP 404) zählt als neues Release; jeder andere Fehler der Abfrage bricht ab
# (vorher ließ ein Fehler der Abfrage alles durch).
EXISTS=0
if gh api "repos/$REPO/releases/tags/$TAG" --jq '.assets[] | "\(.name) \(.digest)"' > "$DIR/.alt" 2> "$DIR/.err"; then EXISTS=1
elif grep -q 'HTTP 404' "$DIR/.err"; then : > "$DIR/.alt"
else cat "$DIR/.err" >&2; rm -f "$DIR/.alt" "$DIR/.err"; echo "::error::Release $TAG ließ sich nicht abfragen – abgebrochen" >&2; exit 1; fi
rm -f "$DIR/.err"
bad=0; for f in "${FILES[@]}"; do b="$(basename "$f")"; case "$b" in *.pkg|*.png|SHA256SUMS-mac.txt) continue;; esac
  alt="$(awk -v n="$b" '$1 == n { print $2 }' "$DIR/.alt")"; [ -n "$alt" ] || continue
  neu="sha256:$(sha256sum "$f" | cut -d' ' -f1)"; [ "$alt" = "$neu" ] || { echo "::error::$b liegt im Release $TAG schon mit anderem Inhalt ($alt, neu $neu) – Version anheben" >&2; bad=1; }; done
rm -f "$DIR/.alt"; [ "$bad" = 0 ] || exit 1
# Debian-Version darf nicht fallen (apt nähme sonst die ältere als neuer bzw. das neue Paket nicht als Aktualisierung): gegen die Pakete aller
# vorhandenen eigenen Releases prüfen (ab 2026-11-24)
OLDDEB="$(gh api "repos/$REPO/releases" --paginate --jq '.[] | select(.tag_name | startswith("laney-")) | .assets[].name' | sed -n 's/^tonestack-plugins_\(.*\)_amd64\.deb$/\1/p')"
for v in $OLDDEB; do [ "$v" = "$DEBVER" ] && continue
  dpkg --compare-versions "$DEBVER" gt "$v" || { echo "::error::Debian-Version $DEBVER ist nicht größer als $v eines vorhandenen Releases" >&2; exit 1; }; done
if [ "$EXISTS" = 1 ]; then gh release edit "$TAG" --title "Laney $LANEY / B100-II $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ / AmpFett 76 $AMPFETT76 / Loldano $LOLDANO / Kaiserin Reverb $KAISERIN / Blech 140 $BLECH / Wisent Echoreck $WISENT / Viereck 34 $VIERECK / Cultec EQ-1A $CULTEC" --notes "$NOTES" --latest -R "$REPO" >/dev/null
else gh release create "$TAG" --target "$SHA" --title "Laney $LANEY / B100-II $YAMAHA / AmpFett B-15NF $B15 / GK 800RB $GK / G100 $G100 / B100-III $B100III / AmpFett SVT-CL $SVTCL / Wien EQ $WIENEQ / AmpFett 76 $AMPFETT76 / Loldano $LOLDANO / Kaiserin Reverb $KAISERIN / Blech 140 $BLECH / Wisent Echoreck $WISENT / Viereck 34 $VIERECK / Cultec EQ-1A $CULTEC" --notes "$NOTES" --latest -R "$REPO"; fi
gh release upload "$TAG" "${FILES[@]}" --clobber -R "$REPO"
for old in $(gh api "repos/$REPO/releases" --paginate --jq '.[].tag_name'); do
  [ "$old" = "$TAG" ] && continue; [[ "$old" == laney-*_yamaha-* || "$old" == laney-*_b100ii-* ]] || continue   # nur eigene Releases
  echo "lösche altes Release $old"; gh release delete "$old" --yes --cleanup-tag -R "$REPO"; done
echo "Release $TAG: https://github.com/$REPO/releases/tag/$TAG"
