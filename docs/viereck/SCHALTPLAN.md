# Viereck 34 – Abschrift des Klangteils des Quad 34

Nachbau des Klangteils des Quad 34 (Vorverstärker, 1982–1996): TILT, BASS (Step/Lift), BALANCE und Filter F1/F2/SLOPE.
Verbindlich ist `buildInto()` in `engine/src/viereck_core.hpp`; diese Datei hält fest, woher jeder Wert kommt.

## Quellen

- Schaltplan „QUAD 34 circuit diagram, M12746 iss. 2 onward“ (für Seriennummern 6001–8000, Platine M12730 iss. 4, mit Ergänzungen ab 8001),
  Nachzeichnung mit Bauteilwerten von keith-snook.info (`QUAD-34-Schematic.pdf`). Nicht im Repository.
- Quad 34 Service Data (Bildschirmfotos des Nutzers): Schaltungsbeschreibung (Tilt Fig. 3–7, Bass Fig. 8(a)/(b), Steller Fig. 9(a)/(b),
  Filter Fig. 10–12), Signalfluss Fig. 1, Bauteilliste, Prüfanweisung (Balance), Fehlersuche. Nicht im Repository.
- Messungen am Gerät: kenrockwell.com „Quad 34“ (Bass Lift bei 82,5 / 20,4 / 27,0 / 36,7 Hz; Step 5,5 dB; Ausgang 0,5 V an 830 Ω).

Gerechnet wird der linke Kanal (IC12, RV2 A, RV3 A, RV4 A, IC19); der rechte Kanal ist gleich aufgebaut (IC13, IC20, R77 …).

## Tilt- und Bassstufe IC12 (TL071, „Ambler tone control“)

| Bauteil | Wert | Lage |
|---|---|---|
| C36 | 220n | Eingang (vom Ausgang der Lautstärkestufe IC9) → Knoten A |
| R55 | 2M2 | A → Masse |
| R54 / R66 | 820k / 820k | A → invertierender Eingang N, N → Ausgang O (Gleichspannungs-Gegenkopplung) |
| C51 | 68n | Schleifer von RV2 → N (Fig. 8(a): Ci) |
| C47 | 220n | O → Knoten C (Fig. 8(a): Co) |
| R51 | 91k | A → rechtes Ende P3 von RV2 (gekreuzt) |
| C40 + R52 ∥ C41 + R53 | 3n3 + 91k ∥ 1n + 56k | A → linkes Ende P1 |
| R63 | 91k | C → P1 (gekreuzt) |
| R64 + C45 ∥ R65 + C46 | 91k + 3n3 ∥ 56k + 1n | C → P3 |
| RV2 Tilt | 13k – 9k1 – 9k1 – 9k1 – 9k1 – 9k1 – 9k1 – 13k | Schalter mit 7 Abgriffen (Fig. 9(b)): ±3, ±2, ±1, 0 |
| R47 ∥ C38 | 100k ∥ 220n | A → D (Bass Step) |
| RV3 Bass (Anschluss 1, an D) | 4K3 / +2K7 / +4K7 | Step 300 / 170 / 100 Hz: D über 4,3 / 7,0 / 11,7 kΩ an Masse |
| RV3 Bass (Anschluss 3, an C) | 3K3 / +2K7 / +4K7 | Lift +9 / +6 / +3 dB: C über 3,3 / 6,0 / 10,7 kΩ an Masse |

RV3 hat eine unterbrochene Bahn (Service Data): in 0 dB ist der Schleifer offen, D hängt frei, C ohne Last.
Versorgung +8,6 / −9,4 V.

Prüfung (engine/tests/viereck_test.cpp): Tilt ±1/±2/±3 dB bei 100 Hz und 10 kHz wie Fig. 3 (höchstens 0,12 dB daneben),
Drehpunkt 847 Hz (Fig. 3 knapp unter 1 kHz; Quad nennt heute 700 Hz). Bass Lift gegen die Messung am Gerät:
+3: 82,5 Hz +2,82 (gemessen +3,00), 20,4 Hz +12,34 (+12,27); +6: +5,69 (+6,03), 27 Hz +13,99 (+14,00); +9: +9,85 (+10,26), 36,7 Hz +15,20 (+15,26).
Step bei 10 Hz gegen 0 dB −5,6 / −5,5 / −5,3 dB (300 / 170 / 100 Hz; Messung „5,5 dB“), −3 dB bei 235 / 143 / 84 Hz.
Filter (mit RV4 5 kΩ): F1 −6,15 dB, F1 SLOPE −9,48, F2 −9,59 bei 20 kHz, F2 SLOPE −14,94 bei 18 kHz (Fig. 10: −6,3 / −9 / −10 / −14).
Ohne Filter 20 Hz … 20 kHz höchstens 0,24 dB neben 1 kHz. Balance +2,50 / −64,99 dB.

## Balance RV4 und Filter IC19 (TL071, Fig. 11)

| Bauteil | Wert | Lage |
|---|---|---|
| C48 | 100µ | O → RV4 (Balance) |
| RV4 | 5k (geschätzt, s. u.), Tandem „Balance Pot R34BALB“ | O über C48 an das obere Ende E, Schleifer → Filtereingang F, unteres Ende an Masse |
| R73 | 15k | F → X |
| Schalter A (4066) + R74 / R75 | 8k2 / 18k | F → M (A zu) → X bzw. Y |
| R76 | 39k | F → Y |
| C61 | 680p | X – Y |
| C62 + Schalter B, R86 | 1n, 2M2 | X → K, K → Y über R86; B legt C62 parallel zu C61 |
| R91 / R92 | 10k / 22k | X → Z, Y → Z |
| C70 + Schalter C, R86 | 1n, 2M2 | Z → L, L über R86 an Masse; C legt C70 an Masse |
| C71 | 220p | Z → Masse |
| IC19 | Folger, R111 8k2 ∥ C75 120p | Z → Ausgang |
| C77, Schalter D | 100µ | Ausgang → G; D verbindet G mit Y (Sallen-Key) |
| R117 / R118 / R119 | 2k7 / 2k2 / 1k | G → OUTPUT, 1k gegen Masse: Teiler 1/5,9, Quelle 830 Ω |

Schalterstellungen (Service Data „Filters“): ohne Filter A + D (Sallen-Key oberhalb des Hörbereichs); F1: A, B, C; F2: B, C;
F1 SLOPE: A, B, C, D; F2 SLOPE: B, C, D. SLOPE lässt sich nur mit F1/F2 wählen (Flipflop gesperrt), ohne Filter wird es gelöscht.

### Balance-Regler RV4

Weder Schaltplan noch Bauteilliste (Service Data, Seiten 21–29: „RV4 Balance Pot R34BALB“) nennen Wert oder Kennlinie.

- Wert: die Filterkurven hängen vom Innenwiderstand am Schleifer ab. Angepasst an Fig. 10 (abgelesen; Spalten F1 / F1 SLOPE / F2 / F2 SLOPE):
  2,5 kHz −0,4 / −0,4 / −0,78 / −1,16 dB; 3,5 kHz −0,6 / −0,6 / −1,29 / −1,94; 4,5 kHz −0,83 / −0,83 / −1,86 / −2,81; 6,1 kHz −1,25 / −1,4 / −2,85 / −4,42;
  8,5 kHz −2,11 / −2,54 / −4,34 / −6,51; 12 kHz −3,58 / −4,62 / −6,24 / −9,47; 16 kHz −5,08 / −6,89 / −8,31 / −12,98; 19 kHz −6,14 / −8,58 / −9,76.
  Mittlere Abweichung (Effektivwert): 4,7 kΩ 0,33 dB, 5 kΩ 0,32 dB, 5,5 / 6 kΩ 0,31 dB; der Rest ist ein gleichbleibender Versatz von etwa 0,26 dB
  (Zeichengenauigkeit). Gewählt 5 kΩ.
- Kennlinie: Service-Prüfanweisung 3 – aus der Mitte zum einen Ende +2,5 dB, zum anderen −65 dB. Modell: Mitte = Schleiferanteil, bei dem das laute Ende
  (Anteil 1) 2,5 dB lauter ist (0,750 mit 5 kΩ und Filterlast); zum lauten Ende gleichmäßig in dB, zum leisen Ende quadratisch im Schleiferanteil bis −65 dB
  (der Verlauf zwischen den Prüfpunkten ist nicht angegeben). Linker Kanal RV4 A, rechter RV4 B gegensinnig.

## Erweiterungen

- TILT stufenlos: Schleifer auf derselben Bahn, zwischen den Abgriffen linear (Kippschalter STEPS/SMOOTH). Auch in STEPS läuft der Schleifer beim
  Umschalten in 15 ms über die Zwischenstellungen (wie beim Drehen des Schalters), damit ein Sprung über mehrere Rasten nicht knackt.
- Drehpunkt des Tilt 80 Hz … 4 kHz: die vier Kondensatoren C40, C41, C45, C46 gemeinsam mit 847 Hz / Drehpunkt skaliert; die Kurve verschiebt sich
  etwas anders als die verschobene Originalkurve, weil C51, C47, C36 und R55 bleiben (nachgerechnet 2026-11-24 nach der unabhängigen Prüfung; bis
  dahin stand hier „höchstens 0,18 dB“, das galt nur ab etwa 200 Hz Drehpunkt). TILT +3 im verschobenen Bereich 300 Hz … 3 kHz des Originals:
  Drehpunkt 80 Hz 0,42 dB, 100 Hz 0,35 dB, 200 Hz 0,18 dB, 500 Hz … 4 kHz höchstens 0,14 dB. Am Rand des Hörbereichs mehr (Drehpunkt 4 kHz:
  1,85 dB bei 48 Hz, weil der Bass-Hochpass nicht mitwandert). Nebenwirkung: TILT 0 ist mit verschobenem Drehpunkt nicht ganz das Original,
  unter 20 Hz bis −0,65 dB (Drehpunkt 80 Hz, 8,5 Hz), +0,07 dB (4 kHz).
- LEVEL: 0 dBFS bezogen auf den Nennausgang 0,5 V eff. (Grundstellung −10 dB); OUTPUT.

## Geschätzt

- OPV TL071 als gesteuerte Quelle (Verstärkung 10⁴, Ausgangswiderstand 100 Ω), Aussteuerungsgrenze ±7,5 V (an +8,6/−9,4 V).
- Analogschalter 4066 als 120 Ω (an), 10 GΩ (aus).
- Quelle (Ausgang IC9) 10 Ω.
- RV4 Balance: 5 kΩ und Kennlinie wie oben.
