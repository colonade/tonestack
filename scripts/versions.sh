#!/bin/bash
# Gibt die Plugin-Versionen aus den Quellen aus: LANEY=3.18 YAMAHA=1.8 B15=1.0 TAG=laney-3.18_yamaha-1.8_b15nf-1.0 PKG=tonestack-Plugins-Mac_….pkg  (zum Einlesen mit eval)
R="$(cd "$(dirname "$0")/.." && pwd)"
L="$(sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/plugin.cpp")"
Y="$(sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/yplugin.cpp")"
B="$(sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/b15plugin.cpp")"
[ -n "$L" ] && [ -n "$Y" ] && [ -n "$B" ] || { echo "Versionen nicht gefunden" >&2; exit 1; }
echo "LANEY=$L YAMAHA=$Y B15=$B TAG=laney-${L}_yamaha-${Y}_b15nf-${B} PKG=tonestack-Plugins-Mac_Laney-${L}_Yamaha-${Y}_B15NF-${B}.pkg"
