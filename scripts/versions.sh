#!/bin/bash
# Gibt die Plugin-Versionen aus den Quellen aus: LANEY=3.18 YAMAHA=1.8 B15=1.1 GK=1.0 G100=1.0 B100III=1.0 SVTCL=1.0 WIENEQ=1.0 AMPFETT76=1.0 TAG=laney-3.18_b100ii-1.8_b15nf-1.1_gk800rb-1.0_g100-1.0_b100iii-1.0_svtcl-1.0_wieneq-1.0_ampfett76-1.0 PKG=tonestack-Plugins-Mac_….pkg  (zum Einlesen mit eval)
R="$(cd "$(dirname "$0")/.." && pwd)"
v() { sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/$1" | head -n1; }   # nur die erste Zeile
L="$(v plugin.cpp)"; Y="$(v yplugin.cpp)"; B="$(v b15plugin.cpp)"; G="$(v gkplugin.cpp)"; G1="$(v g100plugin.cpp)"; B3="$(v b3plugin.cpp)"; SV="$(v svtplugin.cpp)"; WQ="$(v weqplugin.cpp)"; U7="$(v u76plugin.cpp)"
for x in "$L" "$Y" "$B" "$G" "$G1" "$B3" "$SV" "$WQ" "$U7"; do [[ "$x" =~ ^[0-9]+(\.[0-9]+)+$ ]] || { echo "Versionen nicht gefunden oder ungültig: '$L' '$Y' '$B' '$G' '$G1' '$B3' '$SV' '$WQ' '$U7'" >&2; exit 1; }; done
echo "LANEY=$L YAMAHA=$Y B15=$B GK=$G G100=$G1 B100III=$B3 SVTCL=$SV WIENEQ=$WQ AMPFETT76=$U7 TAG=laney-${L}_b100ii-${Y}_b15nf-${B}_gk800rb-${G}_g100-${G1}_b100iii-${B3}_svtcl-${SV}_wieneq-${WQ}_ampfett76-${U7} PKG=tonestack-Plugins-Mac_Laney-${L}_B100II-${Y}_B15NF-${B}_GK800RB-${G}_G100-${G1}_B100III-${B3}_SVTCL-${SV}_WienEQ-${WQ}_AmpFett76-${U7}.pkg"
