#!/bin/bash
# Startet die Boxenmessung (neo212-messung.py) auf Ubuntu: installiert die Python-Pakete, falls sie fehlen, und ruft das Skript auf.
#   bash neo212-messung.sh            Menü mit allen Schritten
#   bash neo212-messung.sh --geraete  Audiogeräte zeigen
#   bash neo212-messung.sh --test     Rechnung prüfen (ohne Interface)
set -u
HIER="$(cd "$(dirname "$0")" && pwd)"
fehlt=""
for m in numpy scipy matplotlib sounddevice; do python3 -c "import $m" 2>/dev/null || fehlt="$fehlt python3-$m"; done
python3 -c "import sounddevice" 2>/dev/null || fehlt="$fehlt libportaudio2"
if [ -n "$fehlt" ]; then
  echo "Fehlende Pakete:$fehlt – werden mit apt installiert (sudo fragt nach dem Passwort)"
  sudo apt install -y $fehlt || { echo "apt konnte nicht alles installieren; notfalls: pip3 install --user sounddevice"; }
fi
exec python3 "$HIER/neo212-messung.py" "$@"
