#!/bin/bash
# Gegenrechnung des AmpFett B-15NF mit ngspice (apt install ngspice): Netzliste aus dem Kern erzeugen, beide rechnen lassen, vergleichen.
#   bash engine/tools/spiceref/lauf.sh AUS-ORDNER eingang.wav VIN SEKUNDEN [vol bass treble jack]
#     eingang.wav: beliebige Audiodatei (wird mit ffmpeg zu mono, 44,1 kHz); VIN = Volt bei Vollaussteuerung; Regler 0 … 1
# Ergebnis: Tabelle je Knoten (Ruhespannung, Wechselanteil, Abweichung). Dateien im AUS-ORDNER: b15.cir (auch in LTspice lesbar), spice.txt, kern.txt.
set -euo pipefail
E="$(cd "$(dirname "$0")/../.." && pwd)"; O="$1"; mkdir -p "$O"; O="$(cd "$O" && pwd)"
command -v ngspice >/dev/null || { echo "ngspice fehlt (apt install ngspice)"; exit 1; }
ffmpeg -v error -y -i "$2" -ac 1 -ar 44100 -f f32le "$O/in.f32"
g++ -O3 -std=c++17 -I"$E/src" "$E/tools/spiceref/b15_spice.cpp" -o "$O/b15_spice"
"$O/b15_spice" "$O" "$O/in.f32" 44100 "$3" "$4" "${5:-0.5}" "${6:-0.5}" "${7:-0.5}" "${8:-0}"
(cd "$O" && ngspice -b b15.cir > ngspice.log 2>&1) || true
if grep -q "too small\|aborted" "$O/ngspice.log"; then echo "ngspice hat abgebrochen:"; grep -o "Timestep too small.*" "$O/ngspice.log" | head -2; exit 1; fi
python3 "$E/tools/spiceref/vergleich.py" "$O"
