#!/bin/bash
# Gibt die Plugin-Versionen aus den Quellen (TS_VERSION in engine/clap/*plugin.cpp) aus, zum Einlesen mit eval: LANEY=… YAMAHA=… … CULTEC=… TAG=laney-…_cultec-…
# PKG=tonestack-Plugins-Mac.pkg (Name fest, unabhängig vom Inhalt) SUMVER=… DEBVER=SUMVER-DEBREV WINEXE=tonestack-Plugins-Windows.exe (Beispiel: bash scripts/versions.sh)
R="$(cd "$(dirname "$0")/.." && pwd)"
v() { sed -n 's/^#define TS_VERSION "\([0-9.]*\)".*/\1/p' "$R/engine/clap/$1" | head -n1; }   # nur die erste Zeile
L="$(v plugin.cpp)"; Y="$(v yplugin.cpp)"; B="$(v b15plugin.cpp)"; G="$(v gkplugin.cpp)"; G1="$(v g100plugin.cpp)"; B3="$(v b3plugin.cpp)"; SV="$(v svtplugin.cpp)"; WQ="$(v weqplugin.cpp)"; U7="$(v u76plugin.cpp)"; LO="$(v sloplugin.cpp)"; KA="$(v kaiplugin.cpp)"; BL="$(v blechplugin.cpp)"; WI="$(v wisentplugin.cpp)"; VR="$(v viereckplugin.cpp)"; CT="$(v cultecplugin.cpp)"
for x in "$L" "$Y" "$B" "$G" "$G1" "$B3" "$SV" "$WQ" "$U7" "$LO" "$KA" "$BL" "$WI" "$VR" "$CT"; do [[ "$x" =~ ^[0-9]+(\.[0-9]+)+$ ]] || { echo "Versionen nicht gefunden oder ungültig: '$L' '$Y' '$B' '$G' '$G1' '$B3' '$SV' '$WQ' '$U7' '$LO' '$KA' '$BL' '$WI' '$VR' '$CT'" >&2; exit 1; }; done
# Gesamtnummer aller Plugins (ab 2026-11-21): 1.<Summe aus 1000·Haupt- + Nebenversion>[.<Summe der dritten Stellen>] – steigt mit jeder Plugin-Version
# (auch bei x.y.z) und bleibt gleich, solange sich keine ändert. Debian-Paket: SUMVER-DEBREV; DEBREV nur erhöhen, wenn sich das Paket ohne neue
# Plugin-Version ändert (Abhängigkeiten, Beschreibung, Dokumentation), bei der nächsten Plugin-Version wieder 1. Windows-Installer: SUMVER als Anzeigeversion.
DEBREV=3
S=0; P=0; for x in "$L" "$Y" "$B" "$G" "$G1" "$B3" "$SV" "$WQ" "$U7" "$LO" "$KA" "$BL" "$WI" "$VR" "$CT"; do
  M="${x%%.*}"; r="${x#*.}"; N="${r%%.*}"; if [ "$r" = "$N" ]; then Q=0; else Q="${r#*.}"; fi
  [[ "$Q" =~ ^[0-9]+$ ]] || { echo "Version mit mehr als drei Stellen: $x" >&2; exit 1; }
  S=$((S + 1000 * 10#$M + 10#$N)); P=$((P + 10#$Q)); done
SUMVER="1.$S"; [ "$P" = 0 ] || SUMVER="$SUMVER.$P"
# Release-Name: bei DEBREV > 1 mit Zusatz _deb-<DEBREV> (ab 2026-11-26) – das geänderte Paket ändert SHA256SUMS-linux.txt, und gleichnamige Dateien
# müssen im selben Release bytegleich bleiben (release.sh); so entsteht ein neues Release, die Plugin-Dateien darin sind dieselben
DEBTAG=""; [ "$DEBREV" = 1 ] || DEBTAG="_deb-$DEBREV"
echo "LANEY=$L YAMAHA=$Y B15=$B GK=$G G100=$G1 B100III=$B3 SVTCL=$SV WIENEQ=$WQ AMPFETT76=$U7 LOLDANO=$LO KAISERIN=$KA BLECH=$BL WISENT=$WI VIERECK=$VR CULTEC=$CT TAG=laney-${L}_b100ii-${Y}_b15nf-${B}_gk800rb-${G}_g100-${G1}_b100iii-${B3}_svtcl-${SV}_wieneq-${WQ}_ampfett76-${U7}_loldano-${LO}_kaiserin-${KA}_blech140-${BL}_wisent-${WI}_viereck34-${VR}_cultec-${CT}${DEBTAG} PKG=tonestack-Plugins-Mac.pkg SUMVER=$SUMVER DEBVER=$SUMVER-$DEBREV WINEXE=tonestack-Plugins-Windows.exe"
