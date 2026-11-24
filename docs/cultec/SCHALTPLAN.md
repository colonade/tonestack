# Cultec EQ-1A – Abschrift des Pultec EQP-1A (und ab 1.1 des EQP-1R)

Nachbau des Pultec EQP-1A „Program Equalizer“ (Pulse Techniques, West Englewood N. J.): passives LC-Netz, Röhrenverstärker
12AX7/12AU7 im Gegentakt mit Gegenkopplung über den Ausgangsübertrager. Name des Plugins „Cultec EQ-1A“ (Wahl des Nutzers, 2026-11-22).
Verbindlich ist der Kern `engine/src/cultec_core.hpp`; diese Datei hält fest, woher jeder Wert kommt und was geschätzt ist.

## Quellen (nicht im Repository)

- **Werksunterlagen** „PultecEQP.pdf“ (8 Seiten, 300 dpi, vom Nutzer; gleicher Inhalt wie der elektrotanya-Service-Plan
  „pultec_pulse-techniques_eqp-1a_12au7_valves_program_equalizer_amplifier_sm.pdf“ in 72 dpi):
  1. Datenblatt EQP-1A (Frequenzen, Regelbereiche), 2. Datenblatt EQP-1A3 (Halbleiterfassung),
  3. **Werkskurven EQP-1A** für jede Stellung (36 Diagramme), 4. Einbau- und Bedienungsanleitung,
  5. „Tubes & Potentiometers in Pultec Equalizers“ (Potiwerte und Kennlinien je Modell),
  6. Werksplan des passiven Netzes des **EQP-1R** (nicht EQP-1A), 7. **Werksplan Verstärker EQP-1A, DWG E-72,658-2**,
  8. Werksplan Verstärker EQP-1R (DWG E-112,353-7). Seiten 6 und 8 sind ab Plugin 1.1 das zweite Modell (Abschnitt „Zweites Modell: EQP-1R“).
- **recproaudio.com** (Nutzer): Abschrift des passiven Netzes EQP-1A („Hosef, 7-11-97“), Teileliste eqp1a_parts_list.xls,
  Verstärkerplan (gleich DWG E-72,658-2).
- **Gyraf** „pultech.gif“: Nachzeichnung „Pultec/Tubetec PE-1a program equalizer, November 2001“ (Filter nach den „real filter schematics“
  von Chris Jenrick, Übertragerdaten laut Gyraf geraten; Höhenwahl dort 3/4/5/**6**/10/12/16 kHz statt 8 kHz).
- Manley „Enhanced Pultec EQP1-A“ Bedienungsanleitung (Regelbereiche), SMC 2024 „Modeling the Pultec EQP-1A with Wave Digital Filters“
  (gleiche Quellenlage: kein Werksplan des EQP-1A-Netzes öffentlich).
- Frontplattenfoto (Nutzer) für die Anordnung; die Frontplatte des Plugins wird selbst gezeichnet.

## Verstärker (Werksplan DWG E-72,658-2), Spannungen laut Plan

| Teil | Wert / Anschluss |
|---|---|
| Eingangsübertrager | Triad HS-56, 600 Ω (1-6, Brücke 3-4 = Mittelanzapfung), Sekundär 7–12 mit 620 Ω abgeschlossen (9–10 verbunden) |
| Zwischenübertrager | Triad HS-29 (1:2, 20 k : 80 k bzw. 5 k : 20 k; Primär 1–4 mit 2–3 verbunden, Sekundär 5–8 mit Mittelanzapfung 6–7 an Masse), über 5–8: 270 pF **in Reihe mit** 43 kΩ (Zobel-Glied; bei 4-facher Vergrößerung des Werksplans geprüft – nicht parallel, wie zuerst abgeschrieben) |
| V1 12AX7 (Gegentakt) | Gitter 2 ← HS-29/5, Gitter 7 ← HS-29/8; Anoden 1, 6 je 200 kΩ (1 W) an +250 V (+140 V an der Anode), 20 pF zwischen den Anoden; Kathoden 3, 8 (+1,3 V) mit 820 Ω untereinander, je 360 Ω zur Gegenkopplungswicklung |
| Kopplung | je 0,25 µF auf die Gitter 2, 7 der 12AU7; Gitterableitung je 1 MΩ an +31 V (Teiler 470 kΩ von +250 V, 62 kΩ an Masse, 40 µF/450 V) |
| V2 12AU7 (Gegentakt) | Kathoden 3, 8 je 4700 Ω (1 W, „selected alike“) ∥ 50 µF/50 V an Masse (+40 V); Anoden 1, 6 an Peerless S-217-D 1 und 6 |
| Ausgangsübertrager | Peerless S-217-D: Primär 1–6, Mitte 2/4 an +290 V (40 µF/450 V); Sekundär 7–12 (600 Ω: Brücke 9–10), 620 Ω und 0,003 µF **in Reihe** über dem Ausgang (Zobel-Glied; bis Plugin 1.1 im Modell fälschlich parallel); Gegenkopplungswicklung 3–5, Abgriff „S“ über 1000 Ω an Masse (+1,1 V) |
| Gegenkopplung | Wicklungsenden 3 und 5 je über 360 Ω an die Kathoden der 12AX7 (überkreuzt laut Plan) |
| Netzteil | Chicago PCC-55, 6X4, +355 V → 470 Ω → +345 V (40 µF) → 1000 Ω → +325 V (40 µF) → 2200 Ω → +290 V; → 47 kΩ → +250 V; Heizung über 270 kΩ/62 kΩ auf +60 V, der 270 kΩ am +325-V-Knoten (seine Leitung springt mit einem Bogen über die +345-V-Leitung; bis 1.1 im Modell an +345 V) |

„Overall gain with input & output transformers connected for same impedance is 0 dB.“ Größter Ausgangspegel +21 dBm (Datenblatt S. 4),
mittlerer Eingangspegel −15 … +8 dBm.

## Passives Netz

Aufbau (beide Abschriften und der Werksplan des EQP-1R stimmen überein): ein L-Glied aus Längszweig 10 kΩ (HF-Boost-Poti) und
Querzweig 1 kΩ (HF-Atten-Poti) über einem RC-Glied nach Masse; dahinter der Bass-Atten-Zweig in Reihe zum Ausgang.

| Knoten | Bauteile |
|---|---|
| A (Eingang) | Sekundär HS-56, 620 Ω nach Masse |
| A → B | HF BOOST 10 kΩ linear (Allen-Bradley JU-1031): Ende „+“ an A, Ende „0“ an B; Schleifer → C(f) → Spulenabgriff L(f) → BANDWIDTH 2,5 kΩ linear (JU-2521, schmal = 0 Ω) → Schalter IN (SW2a) → B (1 MΩ über dem Schalter) |
| B → D | HF ATTEN 1 kΩ linear (JU-1021): Ende „−“ an B, Ende „0“ an D; Schleifer → 75 Ω → C(ATTEN SEL) → D |
| D → Masse | LF BOOST 10 kΩ log (JA-1031) als Regelwiderstand ∥ C(Bass-Boost, LOW FREQUENCY) |
| B → P | LF ATTEN 100 kΩ log (CA-1041) als Regelwiderstand ∥ C(Bass-Atten, LOW FREQUENCY) |
| P → O (Ausgang) | 1 kΩ |
| O → D | 10 kΩ |
| O | an HS-29 Primär |

**IN/OUT** (zweipolig, recproaudio SW2a/SW2b; Bedienungsanleitung S. 4: „Operating the key switch away from the IN position is equivalent to returning
ALL boost and attenuate controls to their zero positions. The amplifier remains in the circuit.“): IN legt das Ende des Höhenboost-Zweigs an B und den
Fußpunkt der Höhen-Atten-Kondensatoren an D; OUT trennt beide (1 MΩ bleibt über SW2a), schließt den Bass-Atten-Zweig B–P kurz und legt D an Masse.
Das entspricht genau allen Reglern auf 0 – so bestätigt die Anleitung die Abschrift.

Werte je Stellung (Herkunft und Prüfung siehe unten):

| LOW FREQUENCY | 20 | 30 | 60 | 100 Hz |
|---|---|---|---|---|
| Bass-Boost-C (Gyraf) | 2,2 µF | 1 µF | 0,47 µF | 0,33 µF |
| Bass-Atten-C (Gyraf) | 100 nF | 47 nF | 22 nF | 15 nF |

| HIGH FREQUENCY | 3 | 4 | 5 | 8 | 10 | 12 | 16 kHz |
|---|---|---|---|---|---|---|---|
| C (recproaudio) | 18 nF | 18 nF | 15 nF | 8,2 nF | 8,2 nF | 5,6 nF | 3,3 nF |
| L-Abgriff (recproaudio) | 150 mH | 82 mH | 68 mH | 47 mH | 33 mH | 33 mH | 27 mH |
| Resonanz | 3,06 | 4,14 | 4,98 | 8,11 | 9,68 | 11,7 | 16,9 kHz |

| ATTEN SEL | 5 | 10 | 20 kHz |
|---|---|---|---|
| C (recproaudio, 68 nF fest + Zuschaltung) | 288 nF (68 + 220) | 136 nF (68 + 68) | 68 nF |

**Wahl der Werte** (Rechnung des passiven Netzes, Quelle 600 Ω, gegen die Werkskurven, Maximalkurven abgelesen):
- Bass: In der recproaudio-Abschrift liegen 330 nF (Boost) und 12 nF (Atten) **fest** parallel, der Schalter legt in 20/30/60 Hz 1,5 µ /
  0,82 µ / 0,22 µ bzw. 56 n / 27 n / 8,2 n dazu (Kontakt 100 frei). Wirksam also Boost 1,83 / 1,15 / 0,55 / 0,33 µF, Atten 68 / 39 / 20,2 / 12 nF:
  Effektivfehler gegen die Werkskurven 0,95 dB (cultec_test curves cLB0=… cLA0=…). Bis 1.2 stand hier, recproaudio treffe die Kurven nicht
  (3,7 dB): das war die falsche Lesart „je Stellung ein Kondensator“ (unabhängige Prüfung 2026-11-24). Gyraf (2,2 / 1 / 0,47 / 0,33 µ,
  100 / 47 / 22 / 15 n) trifft sie auf 0,76 dB; freie Anpassung aller acht Kondensatoren ergäbe 2,31 / 1,19 / 0,67 / 0,43 µ und 77 / 42 / 20 /
  11,5 n (0,3 dB). Gyraf bleibt etwas besser, also Gyraf; beide Lesarten liegen nahe beieinander.
- Lage des Bass-Atten-Kondensators: Gyraf parallel zum Poti allein (B–P, vor dem 1 kΩ), die recproaudio-Abschrift parallel zu Poti und 1 kΩ
  (B–O). Modell nach Gyraf (B–P): 0,76 dB; mit B–O 0,85 dB (gerechnet).
- Höhen-Atten: recproaudio 0,83 dB, Gyraf (47 n / 94 n / 197 n) 1,78 dB – also recproaudio.
- Höhen-Boost: beide treffen die Nennfrequenzen. Gyraf hat keine 8-kHz-Stellung; recproaudio hat die Frequenzen des EQP-1A.
  Spitze schmal/breit der Werkskurven etwa +18,2 / +10,3 dB (alle sieben Frequenzen), Rechnung mit recproaudio +19,1 / +10,8 dB
  bei verlustfreier Spule → Spulenverluste (Güte) werden an die Spitzenhöhe angepasst (geschätzt, im Kern dokumentiert).

**Unsicher:** Das Netz des EQP-1A gibt es nur als Abschrift; die Bass-Kondensatoren stammen aus einer zweiten Abschrift, die zu den
Werkskurven passt. Spulendaten (Gleichstromwiderstand, Kernverluste) und Übertragerdaten sind geschätzt.

## Zweites Modell: EQP-1R (ab Plugin 1.1)

Das ältere Gerät (Nachfolger des EQP-1 von 1956; Tiefen 30/60/100 Hz, Höhen 3/5/8/10/12 kHz, Höhen-Atten fest) hat dieselbe Schaltungsform. Für
beide Teile gibt es Werkspläne (PultecEQP.pdf S. 6 „PULTEC EQP-1R EQUALIZER“, S. 8 DWG E-112,353-7), anders als für das Netz des EQP-1A. Gelesen bei
300 dpi, Ausschnitte 4- bis 8-fach vergrößert. Modell: `partsR()` in `engine/src/cultec_core.hpp`, Prüfung `build/cultec_test all r`.

### Passives Netz (Werksplan S. 6)

| Teil | Wert / Anschluss |
|---|---|
| Eingang | 620 Ω über der Sekundärseite des Eingangsübertragers |
| HF+ | 10 kΩ linear, „+“ am Eingang, „0“ an B; Schleifer → Kondensatorschalter |
| Kondensatorschalter | 3 kc → 0,02 µF, 5 kc → 0,01 µF (beide zum unteren Ende der 100-mH-Spule), 8 kc und 10 kc → 0,0062 µF, 12 kc → 0,005 µF (zum Abgriff zwischen 50 mH und 100 mH) |
| Spulen | 50 mH (oben, am BANDWIDTH-Poti) und 100 mH (unten), je mit eigenem Kernsymbol gezeichnet |
| Spulenschalter | Schleifer am oberen Ende der 50 mH; 3 kc und 8 kc offen, 5 kc → Abgriff (50 mH kurzgeschlossen), 10 kc und 12 kc → unteres Ende |
| BANDWIDTH | 2,5 kΩ linear als Regelwiderstand (SHARP = 0 Ω) zwischen oberem Spulenende und dem IN-Kontakt W |
| IN/OUT, Pol 1 | Pol an B, IN → W (1 MΩ zwischen B und W), OUT → Ausgang O (überbrückt den Bass-Atten-Zweig) |
| HF− | 1 kΩ linear zwischen B und D, Schleifer → 0,13 µF → D (kein Wahlschalter, kein 75 Ω) |
| IN/OUT, Pol 2 | Pol an D (Pfeilspitze des Schaltarms am IN-Kontakt, wie bei den Wafern), IN-Kontakt über 820 Ω an D, OUT-Kontakt an Masse |
| LF+ | Regelwiderstand „80 KΩ LOG“ (erste Ziffer fleckig, Form wie die „8“ in „·28 µf“) zwischen D und Masse (Schleifer an Masse), 30/60/100 cps: 1 µF, 0,5 µF (Ziffer teils verdeckt), 0,28 µF („·28 µf“) |
| LF− | 100 kΩ log als Regelwiderstand zwischen B und O, ∥ 0,0362 / 0,02 / 0,01 µF (30/60/100) |
| O → D | 12 kΩ (kein 1 kΩ zwischen Bass-Atten und Ausgang: gleichlautend die groupdiy-Diskussion „The one from the EQP1R manual has no 1K resistor at all“) |
| Ausgang | O an den Zwischenübertrager |

Gewählt, wo der Plan nicht eindeutig ist:
- **LF+ 80 kΩ** (ab 1.2; 1.1 hatte 10 kΩ wie das EQP-1A): Die fleckige erste Ziffer ist breit und in der Mitte eingeschnürt wie die „8“ in „·28“,
  jede „1“ dieser Handschrift ein dünner Strich; der Nutzer hat 80 kΩ bestätigt (Vergleichsbild 2026-11-24). Bass-Boost damit +19,4 … +19,6 dB
  bei 10 Hz (mit 10 kΩ +15,4 dB).
- **0,28 µF und 0,5 µF**: passen zum Frequenzverhältnis 30 : 60 : 100 (1 : 0,5 : 0,3); „0,128 µF“ läge bei etwa 230 Hz.
- **Spulen ungekoppelt** (zwei Kernsymbole): Gesamt 150 mH, damit 3 kc bei 2,9 kHz. Folge laut Plan: 8 kc und 10 kc liegen bei 9,1 und 11,1 kHz
  (Resonanz 50 mH / 6,2 nF bzw. 33 mH / 6,2 nF), 12 kc bei 12,3 kHz; so übernommen, die Beschriftung bleibt die des Geräts.
- **820 Ω**: liegen laut Plan mit beiden Enden an D (Pol und IN-Kontakt), also ohne Wirkung; in OUT liegt D unmittelbar an Masse.
- **Höhen-Atten in OUT**: der Kondensator hängt ohne Schalter an D; in OUT bleibt die Höhen-Atten also wirksam (BOOST und Bass-Atten nicht).

### Verstärker (DWG E-112,353-7)

| Teil | Wert / Anschluss |
|---|---|
| Eingangsübertrager | Peerless E-372-Q, 600 Ω (Brücke 3–4), Sekundär 7–12 (9–10 verbunden), Abschluss 620 Ω (im Verstärkerplan „820“ oder „620“ schwer lesbar; Netzplan 620 Ω) |
| Zwischenübertrager | Peerless G-212-Q, Primär 4–1 (2–3 verbunden), Sekundär 5–10 mit Mittelanzapfung an Masse; über 5–10: 0,00035 µF in Reihe mit 22 kΩ |
| V1 12AY7 (Gegentakt) | Gitter 2/7 an 5/10; Anoden 1/6 je 180 kΩ an +195 V (+85 V an der Anode), 0,000015 µF zwischen den Anoden; Kathoden 3/8 (+1,5 V) über 750 Ω verbunden, je 360 Ω zur Gegenkopplungswicklung |
| Kopplung | 0,25 µF auf die Gitter der 12AU7; 1 MΩ an +23 V (470 kΩ von +195 V, 62 kΩ an Masse; 40 µF an +195 V) |
| V2 12AU7 (Gegentakt) | Kathoden je 4700 Ω (1 W) ∥ 50 µF an Masse (+33 V); Anoden an S-217-D 1 und 6 |
| Ausgangsübertrager | Peerless S-217-D, Mitte 2/4 an +268 V (40 µF); Sekundär 7–12, Kondensator und 620 Ω in Reihe über dem Ausgang (Wert des Kondensators unleserlich, wie EQP-1A 0,003 µF angenommen; bis 1.1 im Modell parallel); Gegenkopplungswicklung 3–S–5, S über 1000 Ω an Masse |
| Netzteil | Chicago PCC-55, 6X4 → 1000 Ω (10 W) → 40 µF → 1000 Ω (1 W) → +290 V (40 µF); +290 V → 2,2 kΩ (1 W) → +268 V; → 47 kΩ → +195 V; Heizungsvorspannung 270 kΩ / 62 kΩ ∥ 0,25 µF aus +290 V (+54 V) |

Gleiche Form wie DWG E-72,658-2 (EQP-1A); die Knoten des Kerns heißen nach den Spannungen des EQP-1A (B325 = +290 V usw., siehe Kern).

**Plan in sich nicht ganz stimmig** (gerechnet): 12AU7 mit +33 V an 4,7 kΩ ziehen zusammen 14 mA, damit fielen an 2,2 kΩ 31 V statt 22 V (+290 → +268 V);
die 12AY7 mit +85 V an 180 kΩ ziehen zusammen 1,2 mA, mit dem Teiler 1,6 mA, an 47 kΩ also 75 V statt 95 V (+290 → +195 V). Die Spannungen sind
„typical“ (Messung mit 11-MΩ-Voltmeter); Abweichungen um 10 % sind daher zu erwarten.

### Modell EQP-1R und Prüfung

- Gleichrichterspannung (nicht im Plan) 322,5 V, so dass der +290-V-Knoten 289,6 V hat. Ohne weitere Anpassung (cultec_test dc r): +268 → 259,2 V,
  +195 → 206,3 V, +23 → 24,0 V, Heizung 54,1 V, Anode 12AY7 81,8 V (85), Kathode 1,63 V (1,5), Kathode 12AU7 32,4 V (33).
- 12AY7: Koren-Form mit Messparametern (b15::K12AY7, TubeLib), Kapazitäten 1,3 pF / 1,3 pF (Datenblatt).
- S-217-D samt Gegenkopplungswicklung: dasselbe Bauteil wie im EQP-1A, darum dieselben Daten. Ergebnis ohne Anpassung: Gesamtverstärkung bei 1 kHz
  +0,6 dB (EQP-1A-Plan: 0 dB), Gegenkopplung 15,9 dB (EQP-1A 20,7 dB, 12AY7 hat die kleinere Verstärkung).
- **Geschätzt**: E-372-Q und G-212-Q ohne Daten → Werte der Triad-Übertrager aus der Anpassung des EQP-1A; Kopplung E-372-Q 0,99999 und G-212-Q
  0,99992 so, dass der Verstärker wie beim EQP-1A und EQH-2 angegeben flach ist („+0, −1 db“, 20 Hz–20 kHz): −0,81 … +0,18 dB.
  Mit den Kopplungen des EQP-1A gäbe das Zobel-Glied 350 pF + 22 kΩ +1,1 dB bei 10 kHz und −2,1 dB bei 20 kHz. Spulengüte 20 wie EQP-1A.
- Keine Werkskurven des EQP-1R: geprüft werden Lage und Höhe (cultec_test curves r): Bass-Boost +19,4 … +19,6 dB (80 kΩ), Bass-Atten −20,4 … −20,7 dB,
  Höhenboost schmal +18,3 … +18,7 dB, breit +10,6 … +10,7 dB (EQP-1A-Werk +18,2 / +10,3), Höhen-Atten 10 kHz −16,3 dB.
- Klirr +10 dBm: 0,006 % (1 kHz), 0,021 % (50 Hz); +21 dBm 0,036 % bzw. 0,064 % (die 12AU7 laufen an +268 V statt +290 V).

## Werkskurven (abgelesen)

`werkskurven.json`: Maximalkurven der Werksdiagramme, je (Frequenz Hz, dB gegen 1 kHz), mit einem Raster aus den Diagrammen abgelesen
(Lineal-Einblendung, ±0,5 dB). Prüfprogramm: `engine/tests/cultec_test.cpp`.

## Bedienung (Frontplatte, Foto des Nutzers)

Obere Reihe: LOW FREQUENCY BOOST, ATTEN (0–10), HIGH FREQUENCY BOOST, ATTEN (0–10), ATTEN SEL (5/10/20 kHz).
Untere Reihe: IN (Kippschalter), LOW FREQUENCY CPS (20/30/60/100), BANDWIDTH (0 SHARP … 10 BROAD), HIGH FREQUENCY KCS
(3/4/5/8/10/12/16), Netzlampe, OFF/ON.

## Modell (engine/src/cultec_core.hpp) und Anpassung

Ein DK-Netz (48 Knoten, 37 Zustände, 4 Trioden mit 8 Toren in einer Gruppe, weil die Gegenkopplung alle vier verbindet): Quelle 600 Ω, HS-56 als
gekoppelte Spulen mit den Gleichstromwiderständen laut Triad, das passive Netz wie oben, HS-29 (Primärseite und zwei Sekundärhälften mit je der
Windungszahl der Primärseite, Gleichstromwiderstände laut Triad, Zobel-Glied), 12AX7 (Dempwolf, wie Laney/Loldano), 12AU7 (Koren-Form aus einer
Kennlinienmessung, TubeLib), Röhrenkapazitäten aus den Datenblättern, S-217-D als fünf gekoppelte Wicklungen (zwei Primärhälften, Sekundär,
zwei Hälften der Gegenkopplungswicklung), Netzteil wie im Plan (355 V ideal hinter 1 Ω), Last 600 Ω.

- **Ruhespannungen ohne Anpassung** (cultec_test dc): +346 / +327 / +291 / +252 V (Plan +345 / +325 / +290 / +250), +31-V-Knoten 29,3 V,
  Anode 12AX7 138,9 V (140), Kathode 1,33 V (1,3), Abgriff S 1,13 V (1,1), Kathode 12AU7 38,6 V (40).
- **Gegenkopplungswicklung**: Windungsverhältnis 0,020932 je Hälfte (bezogen auf die ganze Primärseite) aus „Overall gain … 0 dB“ (Ausgang an
  600 Ω ∥ (620 Ω + 0,003 µF) gegen Eingangsklemme, 1 kHz, Regler 0; cultec_test nf); Gegenkopplung damit 20,7 dB (bis 1.1 mit dem fälschlich
  parallelen 620 Ω: 0,018747, 16,7 dB). Wicklungssinn so, dass sie gegenkoppelt (cultec_test gain; ab 1.2 zählt „MITKOPPELND“ als Fehler).
- **Ausgangsübertrager**: 12,5 kΩ : 600 Ω (CineMag CM-S217D, Nachbau des Peerless: „12,500/300 … 3125:150/75“); Lp 100 H, Kopplung, Wicklungs-
  widerstände und -kapazität geschätzt.
- **An die Werkskurven angepasst** (cultec_test fit, Nelder-Mead, 13 Kurven und die Spitzen der Höhenanhebung bei 3/8/16 kHz): HS-29 Lp 317 H und
  Eisenverluste 41,5 kΩ parallel zur Primärseite, HS-56 Lp 38 H, Spulengüte 20 (alle Stellungen), 3-kHz-Stellung 204,1 mH / 13,22 nF (Abschrift
  150 mH / 18 nF; gleiche Resonanz, höhere Kennimpedanz: die Werkskurve 3 kHz ist schmaler, als die Abschrift ergibt; andere Stellungen ohne
  Breitenangabe, daher wie abgeschrieben). Effektivfehler aller 105 Punkte 0,76 dB; Spitzen schmal +18,0 … +18,9, breit +10,5 … +10,9 dB.
  **Widerspruch**: in beiden Abschriften teilen sich 3 kHz und 4 kHz denselben Kondensator (18 nF, zwei Kontakte); ein eigener Wert für 3 kHz
  ist im Gerät so nicht möglich. Die Anpassung bildet nur die schmalere Werkskurve nach (andere Spule oder andere Abgriffe wären die
  wirkliche Ursache); 4 kHz behält 18 nF.
- **Geschätzt, an das Datenblatt** („Flat, 20 cps to 20 kcs; +0, −1 db“): Kopplung HS-56 0,99995, HS-29 0,99985, Kapazität der HS-29-Sekundärseite
  40 pF, HS-56 100 pF; Ergebnis −0,86 … +0,25 dB (der kleine Buckel bei 10 kHz bleibt).
- **Klirr** (Datenblatt höchstens 0,15 % bei +10 dBm an 600 Ω): 0,003 % bei 1 kHz, 0,024 % bei 50 Hz; +21 dBm 0,017 % bzw. 0,039 %; +24 dBm
  0,035 % (ab 1.2 mit der richtigen Reihenschaltung am Ausgang und damit stärkerer Gegenkopplung). Das Modell ist ideal symmetrisch (gleiche Röhren); Kernsättigung der Übertrager nicht gerechnet (keine Daten).
- **Rechenrate**: die Trapezregel verbiegt die Frequenzachse; bei 48 kHz ohne Überabtastung lägen 16 kHz bei 12 kHz. Darum wird die Resonanz der
  Höhenspule auf die Rechenrate vorverzerrt (Spule so, dass die digitale Resonanz genau auf der analogen liegt), und das Plugin rechnet bis 4-fach
  (intern höchstens etwa 200 kHz, nur 1-, 2- oder 4-fach; bis 1.1 bei 50–66,7 kHz fälschlich 3-fach mit den Filtern für 4-fach). Abweichung zur analogen Rechnung bei 44,1/48 kHz und 4x höchstens 0,4 dB bis 20 kHz, bei 2x bis 1,8 dB
  (cultec_test unit).
- **„Audio“-Kennlinie** der Bass-Potis: 10 % des Widerstands in der Mitte (geschätzt; Allen-Bradley Typ A). Die Werksdiagramme zeigen Zwischen-
  stellungen nur als „typical settings“ ohne Knopfstellung; die Form der Zwischenkurven (gleicher oberer Knick, flachere Absenkung) trifft das
  Modell.
