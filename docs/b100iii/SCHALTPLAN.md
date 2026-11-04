# Yamaha B100-III: Schaltung, wie sie gerechnet wird

Quellen (nicht im Repository): Yamaha Service Manual „B100-112III / B100-115III“ (1984): Seite 1 Technische Daten (Spezifikation),
Seiten 13/14 Schaltplan (Scan mit 600 dpi ausgewertet), Bestückungspläne; dazu die Bedienungsanleitung (englisch) mit Frontplatte und Grundeinstellungen.
Die Netzliste im Code (`engine/src/b3_core.hpp`, Funktion `buildInto`) ist die verbindliche Fassung dieser Liste; `engine/tests/b3test.cpp` prüft sie gegen die Spezifikation.

Schreibweise des Plans: „10/16“ = 10 µF/16 V Elko, „0.068“ = µF, „220P“ = pF, „A25K“ = 25 kΩ logarithmisch, „G20K“ = 20 kΩ mit Mittelrastung
(hier linear angenommen), „C50K“ = 50 kΩ Tandem (umgekehrt logarithmisch, als FREQUENCY-Poti), „CW“ = Drehrichtung im Uhrzeigersinn.
OPVs: NJM2041 bzw. NJM4558 an ±15 V. Schalttransistoren 2SC2878, Gyratoren 2SC2320.

## Eingang und IC101
- Buchse INPUT → 560 Ω → Knoten (47 pF gegen Masse) → 560 Ω → 10 µF → IC101 1/2 + (1 MΩ gegen Masse).
- IC101 1/2: 10 kΩ Gegenkopplung, 1 kΩ + 10 µF gegen Masse (11-fach); zwischen + und −: 47 pF und die Doppeldiode D101.
- Ausgang → 10 µF → oberes Ende VOLUME (A25K, unten Masse); Schleifer → 10 µF → 560 Ω → IC101 2/2 + (100 kΩ und 220 pF gegen Masse, 100 pF zwischen + und −);
  1 kΩ / 1 kΩ + 10 µF (2-fach).

## Klangregelung IC103 1/2
- IC101 2/2 → 3,3 kΩ → + Eingang (A); Ausgang → 3,3 kΩ → − Eingang (Bn); 47 pF zwischen A und Bn.
- TREBLE, MIDDLE, BASS: je G20K zwischen A und Bn, CW zu Bn = Anhebung, Mitte = ohne Wirkung. Schleifer auf je einen Reihenkreis gegen Masse:
  - TREBLE: 1 kΩ + 0,22 µF.
  - MIDDLE: 0,47 µF → Gyrator Tr103: Knoten g → 0,015 µF → Basis (56 kΩ gegen Masse) → 560 Ω → Tr103 (Emitterfolger, Emitter 3,3 kΩ nach −15 V) → 680 Ω → g. Resonanz etwa 314 Hz.
  - BASS: 2,2 µF → Gyrator Tr104 wie oben mit 0,033 µF. Resonanz etwa 92 Hz.
- Transistoren als Kleinsignal-Ersatzschaltung (gm 0,168 S bei etwa 4,4 mA, rπ 1,8 kΩ; β 300 geschätzt).

## Parametrischer Entzerrer 1 (IC103 2/2, IC104, IC105, IC106)
- IC103 1/2 → 10 kΩ → Summenpunkt M = + Eingang von IC103 2/2 (Y; − über 1 kΩ an Masse, 220 pF vom Ausgang Y).
- Umkehrer IC104 1/2 (+ Masse, − = S): M → 10 kΩ → Ausgang X; X → 10 kΩ → S; Y → 10 kΩ → S.
- LEVEL (G20K) zwischen M und S (CW = S). Schleifer → 1 kΩ → 22 µF → Knoten (10 kΩ gegen Masse; Tr105 gegen Masse) → 22 µF → 1 kΩ → Ausgang des Bandpass-Verstärkers IC106 1/2.
  Tr105 leitet bei **ausgeschaltetem** Entzerrer (Fußschalter, angesteuert über Tr106): dann bleibt LEVEL ohne Wirkung.
- Zustandsvariablenfilter: Summierer IC104 2/2 (− : 10 kΩ von X, 10 kΩ ∥ 100 pF vom eigenen Ausgang, 10 kΩ vom Tiefpass; + = Knoten qp),
  Integratoren IC105 2/2 und IC105 1/2 je 1 kΩ + C50K (Tandem, FREQUENCY) und 0,068 µF.
- Q: Bandpass → A25K (als Regelwiderstand) → qn; qn → 4,7 kΩ → Masse; qn → 560 Ω → qp.
- Bandpass-Verstärker IC106 1/2: + = qp, 2,7 kΩ / 5,6 kΩ (3,1-fach).
- PRESET LEVEL: Y → 3,3 kΩ → + Eingang IC106 2/2 (220 pF gegen Masse); 3,3 kΩ Gegenkopplung auf −, 47 pF zwischen − und +; G20K zwischen − (CW) und +;
  Schleifer → 1,5 kΩ → 4,7 µF → Knoten (100 kΩ gegen Masse; Tr107 gegen Masse). Tr107 leitet bei **eingeschaltetem** Entzerrer (angesteuert über Tr108).
- IC106 2/2 → 10 kΩ → Summenpunkt des zweiten Entzerrers.

## Parametrischer Entzerrer 2 (IC107–IC110)
Wie Entzerrer 1 (Umkehrer IC107 2/2, Summierer IC108 2/2, Integratoren IC109, Bandpass-Verstärker IC108 1/2, PRESET IC110 1/2, Schalter Tr109/Tr110 und Tr111/Tr112),
mit einem Unterschied: von Y auf den Summenpunkt des Umkehrers **4,7 kΩ** statt 10 kΩ (im Plan deutlich lesbar; ändert den Hub von LEVEL kaum, siehe Prüfung).

## Ausgang
- IC110 1/2 → 10 µF → PO → 560 Ω → PRE AMP OUT (100 kΩ, 220 pF gegen Masse) → Schaltkontakt der Buchse POWER AMP IN → 560 Ω → PI (220 pF gegen Masse).
- Lasten an PI: BAL OUTPUT LEVEL (A25K gegen Masse, Schleifer in der Mitte wie in der Bedienungsanleitung) → 560 Ω → 10 µF → 100 kΩ ∥ 47 pF;
  Kopfhörer- und Weichenzweig 10 µF → 560 Ω → 100 kΩ ∥ 47 pF.
- PI → Schaltkontakte LPF/HPF OUTPUT → 560 Ω → 10 µF → Gate FET101 (100 kΩ, 47 pF gegen Masse).

## Endstufe (als Filter gerechnet)
- Differenzpaar FET101/FET102 (2SK246), Gegenkopplung 330 kΩ gegen 1 kΩ + 10 µF; Gleichspannungs-Servo IC113 (Integrator 15 kΩ / 10 µF, Umkehrer) über 220 kΩ.
- Übertragungsfunktion H(s) = k · (a·s + s²·C·(a·rg + rf)) / (w0 + s·(1 + w0·C·rg) + s²·C·rg) mit a = 1 + rf/rs, w0 = rf/(rs·ri·ci); −3 dB bei 16 Hz.
- Verstärkung im Band laut Plan 1 + 330 k / 1 k = 332,5 (50,4 dB). Spezifikation POWER AMP IN → SPEAKER OUT: 41 dB; die Empfindlichkeit (245 mV für 100 W) passt ebenfalls zu 41 dB.
  **Entscheidung**: gerechnet mit 41 dB (115-fach), wie beim G100 (Messwerte des Herstellers vor der Ablesung aus dem Plan); die Form der Gegenkopplung bleibt.
- Begrenzung: 100 W an 8 Ω bei 10 % Klirr = 40 V Spitze; weiches Knie 37 V, Grenze 44 V (geschätzt). Kein Ausgangselko (symmetrische Versorgung).

## Prüfung gegen die Spezifikation (engine/tests/b3test.cpp; VOLUME Maximum, sonst Mitte)
| Prüfpunkt | Spezifikation | Rechnung |
|---|---|---|
| INPUT → PRE AMP OUT (1 kHz) | 20 dB | 19,9 dB |
| LEVEL Maximum / Minimum | ±15 dB | +15,4 / −15,4 dB (beide Entzerrer) |
| FREQUENCY Anschläge | 50 Hz … 2,5 kHz | 52 Hz … 2,3 kHz (Mitte 251 Hz) |
| Q (bei LEVEL Maximum) | 0,35 … 3,0 | 0,34 … 2,1 (**bekannte Abweichung**) |
| PRESET LEVEL | ±10 dB | ±10,1 dB |
| POWER AMP IN → SPEAKER OUT | 41 dB | 41 dB (geeicht) |
| INPUT → SPEAKER OUT | 64 dB | 60,8 dB (mit Plan-Endstufe 70 dB) |

- Q: Das Verhältnis von breitestem zu schmalstem Q legen A25K und 4,7 kΩ auf (25 k + 4,7 k) / 4,7 k = 6,3 fest; mit den Planwerten ist 0,35 … 3,0 (Verhältnis 8,6) nicht erreichbar.
  Das breite Ende stimmt. Nicht angepasst.
- FREQUENCY: Skala der Frontplatte 50 / 100 / 250 / 500 / 2.5K bei 0 / 2,9 / 5 / 7,1 / 10; das Poti erreicht bei 0 Ω nur 2,34 kHz (1 kΩ + 0,068 µF).
- Ausgeschalteter Entzerrer: Restwirkung höchstens 0,4 dB (Einschaltwiderstand 1 Ω der 2SC2878 laut Datenblatt, offener PRESET-Schleifer).
- Gesamtverstärkung: die Spezifikation ist in sich nicht stimmig (20 dB + 41 dB ≠ 64 dB); der Vorverstärker trifft 20 dB ohne Anpassung.

## Geschätzt (in keinem Plan)
Doppeldiode D101 (Is 2,5 nA, n·UT 45 mV), β der Gyrator-Transistoren, OPV-Grenzen (±13,5 V), Leerlaufverstärkung (10⁴) und Ausgangswiderstand (100 Ω) der OPVs,
Knie und Grenze der Endstufe, Kennlinie der A-Potis (15 % in Mittelstellung), G-Potis linear, Stellung des BAL-Pegelreglers (Mitte), Innenwiderstand der Quelle (1 kΩ),
Grundstellung der Fußschalter (beide Entzerrer ein).

## Nicht gerechnet
Lautsprecher (für den JA3812 sind keine Thiele-Small-Daten zu finden), Frequenzweiche (LPF/HPF OUTPUT, auf der Frontplatte abgeblendet), symmetrischer Ausgang,
Kopfhörerverstärker (nur seine Last), Schutzschaltung, Netzteil, Rauschen.

## Rechenweg
Nichtlineare Zustandsraum-Methode (DK, Trapezregel, Newton) über den ganzen Vorverstärker: 89 Knoten, 42 Zustände, 18 nichtlineare Tore (17 OPV-Begrenzungen, D101)
in fünf Gruppen (Eingang, Klangregelung, Entzerrer 1, Entzerrer 2, Ausgang). Rechenrate mindestens 88 kHz (Überabtastung 2x/4x, kein 1x: bei 44,1/48 kHz findet der
Löser bei harter Übersteuerung der Entzerrer vereinzelt keine Lösung). Endstufe als Filter dahinter.
