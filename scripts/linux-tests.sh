#!/bin/bash
# Rechen-, Schaltungs- und Echtzeitprüfungen der Linux-Fassung (ab 2026-11-27 aus .github/workflows/plugins.yml hierher, damit die Jobs für x86-64 und
# ARM64 dieselbe Liste laufen lassen). Vorher: VST3=1 bash engine/build.sh.   bash scripts/linux-tests.sh
set -euo pipefail   # Ausgaben mit sed -n kürzen, nicht mit head: head schlösse die Leitung vorzeitig, das Prüfprogramm bekäme SIGPIPE (Status 141)
cd "$(dirname "$0")/.."
for p in Laney_AOR50 Lametta_B100II AmpFett_B15NF GK_800RB Lametta_G100 Lametta_B100III AmpFett_SVTCL Wien_EQ AmpFett_76 Loldano_SLO100 Kaiserin_Reverb Blech_140 Wisent_Echoreck Viereck_34 Cultec_EQ1A; do ./engine/build/plugtest engine/build/$p.clap params | sed -n 1,1p; done
./engine/build/b15test | tail -3
./engine/build/gktest | sed -n 1,8p
./engine/build/g100test | grep -E "Abweichung|ABWEICHUNG|Knoten"
./engine/build/b3test | grep -E "Abweichung|ABWEICHUNG|BEKANNT|Knoten"
./engine/build/svttest | grep -E "Abweichung|ABWEICHUNG|BEKANNT|Knoten"
./engine/build/weqtest
./engine/build/u76test | grep -E "ABWEICHUNG|bestanden|Verhältnis|Schwelle|Dämpfung, |Attack|Release"
./engine/build/slotest | grep -E "Abweichung|ABWEICHUNG|Leistung|Ruhestrom"
./engine/build/switchtest | tail -1
./engine/build/kaitest rt | tail -3
./engine/build/blechtest conv | tail -3
./engine/build/blechtest rate | tail -4
./engine/build/wisenttest dc
./engine/build/wisenttest time | tail -3
./engine/build/wisenttest pattern | tail -2
./engine/build/wisenttest tone
./engine/build/wisenttest lat | tail -2
./engine/build/wisenttest wob | tail -3
./engine/build/wisenttest heads | tail -5
./engine/build/wisenttest wobpitch | tail -3
./engine/build/viereck_test | tail -8
./engine/build/cultec_test | grep -E "\\[|ABWEICHUNG|fertig|FEHLER"
./engine/build/cultec_test all r | grep -E "\\[|ABWEICHUNG|fertig|FEHLER"
./engine/build/b15pre | tail -1
./engine/build/svtpre | tail -1
./engine/build/slopre | tail -1
./engine/build/slothd | tail -2
./engine/build/slobassck | tail -1
./engine/build/ecc83ds | tail -4
for p in Laney_AOR50 Lametta_B100II AmpFett_B15NF GK_800RB Lametta_G100 Lametta_B100III AmpFett_SVTCL Wien_EQ AmpFett_76 Loldano_SLO100 Kaiserin_Reverb Blech_140 Wisent_Echoreck Viereck_34 Cultec_EQ1A; do ./engine/build/rtcheck engine/build/$p.clap 2 256 > rt-$p.log; done; cat rt-*.log > rt.log; python3 mac/rtnotice.py Echtzeit-Linux rt.log
