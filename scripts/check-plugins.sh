#!/bin/bash
# Prüft scripts/plugins.txt (ab 2026-11-24): die Spalten landen ungeprüft in Dateinamen, im postinst des Debian-Pakets und im NSIS-Installer;
# darum nur harmlose Zeichen (keine Anführungszeichen, kein $, kein `, kein \, keine Pfadtrenner im Dateinamen).
#   bash scripts/check-plugins.sh   (bricht mit Fehler ab, wenn eine Zeile nicht passt)
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; n=0
while IFS= read -r line || [ -n "$line" ]; do case "$line" in ''|'#'*) continue;; esac
  IFS='|' read -r NAME VV README SHORT DISP EXTRA <<< "$line"
  [[ "$NAME" =~ ^[A-Za-z0-9_]+$ ]] && [[ "$VV" =~ ^[A-Z0-9]+$ ]] && [[ "$README" =~ ^README[A-Za-z0-9_-]*\.txt$ ]] && [[ "$SHORT" =~ ^[a-z0-9]+$ ]] \
    && [[ "$DISP" =~ ^[A-Za-z0-9][A-Za-z0-9\ .()+-]*$ ]] && [ -z "${EXTRA:-}" ] || { echo "scripts/plugins.txt: ungültige Zeile: $line" >&2; exit 1; }
  n=$((n + 1)); done < "$R/scripts/plugins.txt"
[ "$n" -gt 0 ] || { echo "scripts/plugins.txt: keine Plugins" >&2; exit 1; }
