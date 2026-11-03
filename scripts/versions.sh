#!/bin/bash
# Gibt die Plugin-Versionen aus den Quellen aus: LANEY=3.18 YAMAHA=1.8 B15=1.1 GK=1.0 TAG=laney-3.18_yamaha-1.8_b15nf-1.1_gk800rb-1.0 PKG=tonestack-Plugins-Mac_….pkg  (zum Einlesen mit eval)
R="$(cd "$(dirname "$0")/.." && pwd)"
v() { sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/$1" | head -n1; }   # nur die erste Zeile
L="$(v plugin.cpp)"; Y="$(v yplugin.cpp)"; B="$(v b15plugin.cpp)"; G="$(v gkplugin.cpp)"
for x in "$L" "$Y" "$B" "$G"; do [[ "$x" =~ ^[0-9]+(\.[0-9]+)+$ ]] || { echo "Versionen nicht gefunden oder ungültig: '$L' '$Y' '$B' '$G'" >&2; exit 1; }; done
echo "LANEY=$L YAMAHA=$Y B15=$B GK=$G TAG=laney-${L}_yamaha-${Y}_b15nf-${B}_gk800rb-${G} PKG=tonestack-Plugins-Mac_Laney-${L}_Yamaha-${Y}_B15NF-${B}_GK800RB-${G}.pkg"
