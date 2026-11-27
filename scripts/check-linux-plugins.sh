#!/bin/bash
# Prüft die Linux-Plugins (ab 2026-11-27) – im Bau-Ordner oder installiert (/usr/lib):
#   bash scripts/check-linux-plugins.sh <CLAP-Ordner> <VST3-Ordner> [glibc-Obergrenze, Standard 2.38]
#     z. B. engine/build engine/build/vst3   oder   /usr/lib/clap /usr/lib/vst3
# Je Plugin aus scripts/plugins.txt: Exporte (CLAP genau clap_entry, VST3 genau GetPluginFactory, ModuleEntry, ModuleExit, clap_entry),
# höchste verlangte glibc-Version, Härtung (GNU_RELRO, BIND_NOW, kein ausführbarer Stack), keine Symboltabelle und kein .comment,
# moduleinfo.json passend zur Version. Mit VALIDATOR=<Pfad> zusätzlich Steinbergs validator -e, mit CLAPVAL=<Pfad> clap-validator,
# mit PLUGINVAL=<Pfad> pluginval (Strenge 5, ohne Oberflächentests; unter xvfb-run, wenn vorhanden). Ende mit Fehlerstatus, wenn etwas nicht passt.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; eval "$(bash "$R/scripts/versions.sh")"
C="${1:?CLAP-Ordner}"; V="${2:?VST3-Ordner}"; GMAX="${3:-2.38}"; bad=0; n=0
fehler() { echo "FEHLER: $*" >&2; bad=1; }
READELF="${READELF:-readelf}"
LOG="$(mktemp -d)"; trap 'rm -rf "$LOG"' EXIT
dyn() { "$READELF" -W --dyn-syms "$1" | awk '$1 ~ /^[0-9]+:$/ && $7 != "UND" && $5 != "LOCAL" && $4 != "SECTION" && $4 != "FILE" && $8 != "" { sub(/@.*/, "", $8); print $8 }' | LC_ALL=C sort -u | tr '\n' ' ' | sed 's/ $//'; }
pruef() {   # $1 Datei, $2 erwartete Exporte
  local f="$1" e; e="$(dyn "$f")"
  [ "$e" = "$2" ] || fehler "$f exportiert „$e“, erwartet „$2“"
  local g; g="$("$READELF" -W --dyn-syms "$f" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -n1)"
  [ -n "$g" ] && [ "$(printf '%s\n%s\n' "$g" "$GMAX" | sort -V | tail -n1)" = "$GMAX" ] || fehler "$f braucht glibc $g (erlaubt bis $GMAX)"
  "$READELF" -lW "$f" | grep -q 'GNU_RELRO' || fehler "$f ohne GNU_RELRO"
  "$READELF" -dW "$f" | grep -qE 'BIND_NOW|FLAGS.*NOW' || fehler "$f ohne BIND_NOW"
  "$READELF" -lW "$f" | awk '$1 == "GNU_STACK" && $7 ~ /E/ { x = 1 } END { exit x }' || fehler "$f mit ausführbarem Stack"
  "$READELF" -SW "$f" | grep -qE '\] \.symtab |\] \.comment |\] \.debug_' && fehler "$f nicht gestrippt (.symtab, .comment oder .debug_*)"
  "$READELF" -dW "$f" | grep -q 'libstdc++' && fehler "$f hängt an libstdc++ (sollte statisch eingebunden sein)"
  echo "  $f: Exporte $e; glibc ≤ $g; RELRO, BIND_NOW, gestrippt"
}
while IFS='|' read -r NAME VV REST; do case "$NAME" in ''|'#'*) continue;; esac
  n=$((n + 1)); ver="${!VV}"; echo "== $NAME $ver"
  [ -f "$C/$NAME.clap" ] || { fehler "$C/$NAME.clap fehlt"; continue; }
  pruef "$C/$NAME.clap" "clap_entry"
  so="$(ls "$V/$NAME.vst3/Contents/"*-linux/"$NAME.so" 2>/dev/null | head -n1)"
  [ -n "$so" ] || { fehler "$V/$NAME.vst3: keine .so in Contents/<arch>-linux"; continue; }
  pruef "$so" "GetPluginFactory ModuleEntry ModuleExit clap_entry"
  mi="$V/$NAME.vst3/Contents/Resources/moduleinfo.json"
  if [ -f "$mi" ]; then grep -q "\"Version\": \"$ver\"" "$mi" && grep -q "\"Name\": \"$NAME\"" "$mi" || fehler "$mi passt nicht zu $NAME $ver"
  else fehler "$mi fehlt"; fi
  if [ -n "${VALIDATOR:-}" ]; then
    if "$VALIDATOR" -e "$V/$NAME.vst3" > "$LOG/v-$NAME.txt" 2>&1 && grep -q '^Result: [0-9]* tests passed, 0 tests failed' "$LOG/v-$NAME.txt"; then
      echo "  VST3-Validator: $(grep '^Result:' "$LOG/v-$NAME.txt")"
    else tail -n 30 "$LOG/v-$NAME.txt" >&2; fehler "VST3-Validator: $NAME"; fi
  fi
  if [ -n "${CLAPVAL:-}" ]; then
    # Auswertung über --json (scripts/clapval-auswerten.py): Fehler und Abstürze zählen; nur das Zeitlimit des Prüfers (45 s je Test) wird gesondert
    # gemeldet, nicht als Fehler – die Fuzz-Tests aktivieren das Plugin 50-mal, und Schaltungsmodelle mit Einschwingen beim Aktivieren (SVT-CL,
    # AmpFett 76, Blech 140: 0,5 … 1 s) brauchen dafür auf langsamen Rechnern länger. Hänger fänden rtcheck und clapstress.
    "$CLAPVAL" validate --json "$C/$NAME.clap" > "$LOG/c-$NAME.json" 2> "$LOG/c-$NAME.err" || true
    python3 "$R/scripts/clapval-auswerten.py" "$LOG/c-$NAME.json" "$NAME" || { tail -n 20 "$LOG/c-$NAME.err" >&2; fehler "clap-validator: $NAME"; }
  fi
  if [ -n "${PLUGINVAL:-}" ]; then
    X=(); command -v xvfb-run >/dev/null && X=(xvfb-run -a)
    if "${X[@]}" "$PLUGINVAL" --strictness-level 5 --skip-gui-tests --validate-in-process --output-dir "$LOG" --validate "$V/$NAME.vst3" > "$LOG/p-$NAME.txt" 2>&1 \
       && grep -q 'SUCCESS' "$LOG/p-$NAME.txt" && ! grep -q '!!! Test .* failed' "$LOG/p-$NAME.txt"; then echo "  pluginval (Strenge 5): SUCCESS"
    else tail -n 40 "$LOG/p-$NAME.txt" >&2; fehler "pluginval: $NAME"; fi
  fi
done < "$R/scripts/plugins.txt"
[ "$n" -gt 0 ] || fehler "keine Plugins in scripts/plugins.txt"
[ "$bad" = 0 ] && echo "Linux-Plugins: alle $n Prüfungen bestanden" || { echo "Linux-Plugins: FEHLER" >&2; exit 1; }
