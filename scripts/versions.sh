#!/bin/bash
# Gibt die Plugin-Versionen aus den Quellen aus: LANEY=3.18 YAMAHA=1.8 TAG=laney-3.18_yamaha-1.8  (zum Einlesen mit eval)
R="$(cd "$(dirname "$0")/.." && pwd)"
L="$(sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/plugin.cpp")"
Y="$(sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/yplugin.cpp")"
[ -n "$L" ] && [ -n "$Y" ] || { echo "Versionen nicht gefunden" >&2; exit 1; }
echo "LANEY=$L YAMAHA=$Y TAG=laney-${L}_yamaha-${Y}"
