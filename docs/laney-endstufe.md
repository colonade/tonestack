# Laney AOR 50: Endstufe als Schaltung (ab Plugin 3.20, erweitert 3.21)

Quelle: Schaltplan DRG 1054 (AOR 50/12 CIRCUIT, 2/4/85). Code: `engine/src/laney_pa.hpp`, eingebunden als `PaDk` in `engine/src/core.hpp`.
Gerechnet im Röhrenmodus mit „Effektweg + Endstufe“ (ab 3.21 überabgetastet: 2x bei 2x und 4x, 4x bei 8x). Der lineare Modus rechnet ab 3.21 dieselbe Schaltung im Arbeitspunkt linearisiert (`lpa::linearNet`, `PaLin`).

## Netz (Werte aus dem Plan)
| Teil | Plan | Im Modell |
|---|---|---|
| Effektweg-Rückweg | C201, C202 2,2 µF, 39 k ‖ 55 k, Puffer | wie bisher (C201 und C202 in Reihe zusammengefasst) |
| Koppelkondensator zur Phasenumkehr | C18 22 n | C18 |
| Phasenumkehr V104 (ECC83) | Gitter R34/R37 1 M an Knoten n, Kathoden R36 470 → n, R35 10 k → x, R105 4k7 x → Masse, C19 10 n (Gitter B → x), VR108 22 k + C110 100 n (Presence), R38 100 k vom Lautsprecherabgriff an x | alles so; R38 am 8-Ω-Abgriff |
| Anoden | R39 82 k / R40 100 k an 326 V, C20 47 p | so |
| Endröhren-Gitter | C21/C22 22 n, R42/R43 220 k an −36 V, R106/R107 5k6 | so |
| Endröhren V105/V106 | Kathoden an Masse, Schirmgitter R108/R109 470 Ω an 398 V | EL34 (Typ nicht im Plan) |
| Übertrager T102 | Mittelanzapfung an 400 V, Abgriffe Orange/Rot/Braun/Schwarz | 4 kΩ : 8 Ω, Primärinduktivität 40 H, Kopplung 0,9995 (Hälften) / 0,999 (primär–sekundär), Wicklungswiderstand 80 Ω / 0,25 Ω, Wicklungskapazität 400 pF, Eisenverluste 100 kΩ, Kernsättigung φs = 2,0 V·s – alles geschätzt |
| Netzteil | Brücke D5–12 (1N4007), C111+C112 2 × 50 µF (400 V), Drossel T103, C108+C109 (398 V), R41 10 k, C107 50 µF (326 V) | Brücke als zwei gegenphasige Quellen hinter 15 Ω (296 V eff, auf 400 V abgeglichen), glatte Si-Kennlinie; Drossel 5 H / 100 Ω, 100 µF, 100 µF, 50 µF; Vorstufenlast 68 k an 326 V (geschätzt aus dem Spannungsabfall an R41) |
| Vorspannung | Orange-Wicklung → D4 → (−68 V) → R46 22 k → C24 10–22 µF (−48 V) → R45 15 k → −36 V, R44 + P1 nach Masse, C23 | Wicklung 86 V eff (auf −36 V abgeglichen), C24 22 µF, C23 10 µF (Wert unleserlich), R44 + P1 = 45 k |
| Lautsprecher (Last) | – | Eminence CB158 im 78-l-Bassreflex wie der Lautsprecher-Schalter: Re 6,15 Ω, Le 0,33 mH (20 Ω Wirbelstrom), Qms 5 (angenommen), Qes 0,365, Box und Kanal als Reihenkreis (50 Hz, Ql 7) |

## Röhrenmodelle
- 12AX7: mu 100, ex 1,2, kg1 1090,9, kp 287,3, kvb 300 (an RCA-Datenblatt und die Laney-Vorstufen angepasst, `engine/tools/tube_fits.py`).
- EL34: Koren (mu 11, ex 1,35, kp 60, kvb 24), kg1 737,1 und kg2 3674,9 an den Datenblattpunkt 250 V / 250 V / −12,2 V (100 mA, 14,9 mA).
- Gitterstrom Dempwolf/Zölzer wie im übrigen Kern.

## Prüfung
| Knoten | Plan | Modell |
|---|---|---|
| B+ nach Gleichrichter / nach Drossel / Vorstufe | 400 / 398 / 326 V | 404,8 / 403,1 / 328,9 V |
| Anoden V104 A / B | 225 / 209 V | 216,7 / 208,2 V |
| Kathode V104 | 35 V | 38,5 V |
| Gittervorspannung | −36 V (−48 V, −68 V) | −35,9 V (−47,9 V, −65,5 V) |
| Schirmgitter | 395 V | 400,8 V |
| Ruhestrom je EL34 | – | 34,9 mA (14 W von 25 W) |

Gegenkopplung: mit vertauschtem Wicklungssinn schwingt die Endstufe; die gewählte Richtung ist stabil (Brumm am Lautsprecher 0,07 mV).
Klirr der Endstufe allein bei 1 kHz: 8,7 V am Lautsprecher 1,7 % (H3 −36 dB), 17,9 V 3,6 %, 27,9 V 11 %.
Pegel: Ausgang = Lautsprecherspannung × 0,00625534254, damit 1 kHz bei kleinem Signal und Presence 5 so laut ist wie die bisherige Endstufe (3.19).

## Ab 3.21
- Netztrafo als Trafo: Primär 230 V / 50 Hz (Eingang 1), Hauptinduktivität 5 H, Kopplung primär–sekundär 0,998, Hochspannungshälften 0,9995, Vorspannungswicklung 0,997, Primär 1 Ω, je Hälfte 15 Ω, 100 kΩ an jedem Wicklungsende (gegen Schwingen der Trapezregel bei gesperrten Dioden). Hochspannung 297,5 V eff je Hälfte, Vorspannung 86 V eff.
- Vorstufenschienen im selben Netz: B3 → R26 10 k → 260-V-Schiene (C106 50 µF) → R14 10 k → 252 V (C102 50 µF) → R4 10 k → 240 V (47 µF geschätzt); Ruhelast 100 k / 252 k / 400 k (geschätzt; die Planwerte 260/252/240 lassen sich mit dieser Kette nicht gleichzeitig treffen, deshalb wirken nur die Änderungen). Die Vorstufe meldet je Abtastwert die Änderung ihrer Anodenströme (Stromquellen an den Schienen) und rechnet mit Ruhespannung + Änderung.
- Übertragerkern mit Hysterese (wie AmpFett, φs 2 V·s).
- 12AX7 in der Dempwolf-Form (G 2,019059e-3, C 2,02928, mu 100, γ 1,26). Ausgangsmaßstab neu 0,00549522876 (Pegel 1 kHz wie 3.19).
- Ruhe: B+ 400,3 / 398,7 / 327,5 V, V104-Anoden 217,1 / 208,2 V, Vorspannung −35,9 V, EL34 je 33 mA.

## Ab 3.25
- EL34 als Derk-Strahltetrode nach Reefman (EM4 Theory 2016, Gl. 14/15 und 45/46, mit Sekundäremission): Parameter einer mit dem uTracer gemessenen EL34 (ExtractModel, TubeLib.inc 2013: mu 12,5, ex 1,363, kg1 217,7, kp 50,5, kVB 1282,7, kg2 1950,2, A/kg1 1,6e-6, β 0,105, αs 6,09, S 0,022, ap 0,033, w 64, ν 2,91, λ 4,23), Ströme auf den Datenblattpunkt 250/250/−12,2 V skaliert (Ia ×1,1854, Ig2 ×1,2193). Code: `b15::pentodeD`, `lpa::DEL34`; Rechnung: engine/tools/tube_fits.py.
- Gegen das Koren-Modell bis 3.24: Innenwiderstand 20 kΩ statt 39 kΩ (Datenblatt 15–20 kΩ), Knie bei Ug1 0 V / Ug2 250 V: 20 V Anode 193 mA statt 128 mA, 50 V 246 statt 207 mA; Schirmgitterstrom bei 20 V Anode 116 mA (steigt beim Übersteuern stark).
- Ruhe: EL34 je 32,8 mA, Ig2 4,3 mA (vorher 33,0 / 4,6 mA), B+ 400,5 V.
- Ausgangsmaßstab 0,00579406058 (Endstufe allein bei 1 kHz, 20 mV, Presence 5 so laut wie 3.24; das neue Modell war 0,46 dB leiser).
- Klirr der Endstufe allein bei 1 kHz (Eingang 0,3 / 1 / 4 V): 0,52 / 3,5 / 15,1 % (3.24: 0,35 / 2,3 / 15,1 %); höchste Spannung am Lautsprecher 28,7 statt 28,0 V.
