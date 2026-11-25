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
  von Chris Jenrick, Übertragerdaten laut Gyraf geraten; Höhenwahl dort 3/4/5/**6**/10/12/16 kHz statt 8 kHz). **Keine Pultec-Quelle**: ihr
  Autor schreibt auf Gearspace, sie sei „reverse-engineer'ed from a Tube-tech (which is reverse-eng'd from the original back in the 80'es....)“
  (Recherche 2026-11-24). Ab Plugin 1.3 wird darum kein Wert mehr aus ihr genommen.
- **VintageWindings** (Reparatur und Nachbau der Original-Höhenspule, Händlerangabe): Abgriffe 32,0 / 51,1 / 84,3 / 100,2 / 150,5 mH,
  Wicklungswiderstand „about 14 ohms at the first tap to about 34 ohms on the final tap“; frühe EQP-1 mit zwei Ringkernen 100 mH und 50 mH
  (passt zum EQP-1R-Plan). Für 3 kHz nennt VintageWindings 156 mH / 18 nF.
- Ian Bell (GroupDIY) zu den Werkskurven: „those look like what they calculated them to be“ – die Werkskurven sind womöglich gerechnet, nicht
  gemessen. Sie bleiben die Referenz (es gibt keine bessere), aber ein Effektivfehler unter etwa 1 dB ist nicht mehr aussagekräftig.
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
| B → P | LF ATTEN 100 kΩ log (CA-1041) als Regelwiderstand |
| P → O (Ausgang) | 1 kΩ |
| B → O | C(Bass-Atten, LOW FREQUENCY): über Poti und 1 kΩ (ab 1.4; bis 1.3 nur über dem Poti, B–P) |
| O → D | 10 kΩ |
| O | an HS-29 Primär |

**IN/OUT** (zweipolig, recproaudio SW2a/SW2b; Bedienungsanleitung S. 4: „Operating the key switch away from the IN position is equivalent to returning
ALL boost and attenuate controls to their zero positions. The amplifier remains in the circuit.“): IN legt das Ende des Höhenboost-Zweigs an B und den
Fußpunkt der Höhen-Atten-Kondensatoren an D; OUT trennt beide (1 MΩ bleibt über SW2a), schließt das Bass-Atten-Poti (B–P) kurz und legt D an Masse.
Das entspricht genau allen Reglern auf 0 – so bestätigt die Anleitung die Abschrift.

Werte je Stellung (Herkunft und Prüfung siehe unten):

| LOW FREQUENCY | 20 | 30 | 60 | 100 Hz |
|---|---|---|---|---|
| Bass-Boost-C (recproaudio: 330 nF fest + Zuschaltung) | 1,83 µF (0,33 + 1,5) | 1,15 µF (0,33 + 0,82) | 0,55 µF (0,33 + 0,22) | 0,33 µF |
| Bass-Atten-C (recproaudio: 12 nF fest + Zuschaltung) | 68 nF (12 + 56) | 39 nF (12 + 27) | 20,2 nF (12 + 8,2) | 12 nF |

| HIGH FREQUENCY | 3 | 4 | 5 | 8 | 10 | 12 | 16 kHz |
|---|---|---|---|---|---|---|---|
| C (recproaudio) | 18 nF | 18 nF | 15 nF | 8,2 nF | 8,2 nF | 5,6 nF | 3,3 nF |
| L-Abgriff (recproaudio) | 150 mH | 82 mH | 68 mH | 47 mH | 33 mH | 33 mH | 27 mH |
| Resonanz | 3,06 | 4,14 | 4,98 | 8,11 | 9,68 | 11,7 | 16,9 kHz |

| ATTEN SEL | 5 | 10 | 20 kHz |
|---|---|---|---|
| C (recproaudio, 68 nF fest + Zuschaltung) | 288 nF (68 + 220) | 136 nF (68 + 68) | 68 nF |

**Wahl der Werte** (Rechnung des passiven Netzes, Quelle 600 Ω, gegen die Werkskurven, Maximalkurven abgelesen):
- Bass (ab 1.3 recproaudio; bis 1.2 Gyraf): In der recproaudio-Abschrift liegen 330 nF (Boost) und 12 nF (Atten) **fest** parallel, der
  Schalter legt in 20/30/60 Hz 1,5 µ / 0,82 µ / 0,22 µ bzw. 56 n / 27 n / 8,2 n dazu (Kontakt 100 frei). Dieselbe Lesart in der Drip-Stückliste
  (C10 0,22 µ, C11 1,5 µ, C12 0,82 µ, C13 0,33 µ) und auf Gearspace („The 330nF is always there and all but the 100Hz positions add another cap
  in parallel“). Wirksam also Boost 1,83 / 1,15 / 0,55 / 0,33 µF, Atten 68 / 39 / 20,2 / 12 nF. Bis 1.2 stand hier, recproaudio treffe die Kurven
  nicht (3,7 dB): das war die falsche Lesart „je Stellung ein Kondensator“. Gyraf (2,2 / 1 / 0,47 / 0,33 µ, 100 / 47 / 22 / 15 n) ist ein
  Tube-Tech-Abkömmling (Quellen oben) und darum verworfen. Vergleich, jede Lesart mit eigener Anpassung (cultec_test fit, ab 1.4 mit dem
  Verstärker-Datenblatt als Nebenbedingung): Gyraf-Kondensatoren mit Gyraf-Anschluss B–P 0,88 dB über alle 105 Punkte, recproaudio-Kondensatoren
  mit B–P 0,89 dB, recproaudio vollständig (mit B–O, unten) 0,94 dB. Die Werkskurven unterscheiden die Lesarten also nicht (abgelesen auf ±0,5 dB
  und womöglich gerechnet); recproaudio ist die bessere Quelle.
- **Lage des Bass-Atten-Kondensators** (ab 1.4 B–O; Recherche 2026-11-25): Die recproaudio-Abschrift legt die Kondensatoren von B an den
  Ausgang O, also über Poti **und** 1 kΩ: die Oberseite der Kondensatorbank hängt an B, ihr Fußpunkt (Schalter SW3b, 12 nF fest) am Ausgang zum
  HS-29 (Pin 4), dort auch der 10 kΩ nach D; das Poti geht mit dem Schleifer über 1 kΩ an denselben Ausgang. Gyraf legt sie nur über das Poti
  (B–P, vor dem 1 kΩ). Herkunft: die Abschrift „Hosef 7-11-97“ hat Joe Malone (JLM) laut GroupDIY nach drei echten Röhren-Pultecs gezeichnet
  („Joe said he made that schematic from 3 REAL tube Pultecs“); Gyraf ist ein Tube-Tech-Abkömmling (oben). Einen Werksplan des EQP-1A-Netzes gibt
  es nicht (Dan Alexander laut Gearspace: „there never was a "real" (pultec made) schematic from the filter section“). Die Werkspläne des EQP-1
  und des EQP-1R haben keinen 1 kΩ, dort fallen beide Lesarten zusammen. Auf GroupDIY werden beide Lesarten beschrieben („In other schematics,
  the LF frequency selection capacitors bypass the control *and* the 1K“), ein Teilnehmer erinnert B–P „from my memory“, ein anderer kennt
  Messkurven von Geräten mit beiden Verläufen; keine zweite Abschrift von Originalgeräten. → Die einzige von Originalgeräten abgenommene
  Abschrift gibt B–O, darum ab 1.4 `attenBO = true`.
  Folge: bei Reglern 0 überbrückt der Kondensator den 1 kΩ (gegen 10 kΩ nach D), die Höhen steigen je nach LOW FREQUENCY: 20 Hz +0,8 dB bei
  10 kHz, 30 Hz +0,7, 60 Hz +0,4, 100 Hz +0,1 dB (cultec_test flat). Bis 1.3 wurde B–O verworfen, weil das dem Datenblatt „Flat … +0, −1 db“
  widerspräche – das steht dort aber unter „AMPLIFIER“ und gilt für den Verstärker allein; die Prüfung rechnet ab 1.4 den Verstärker mit rein
  ohmschem Netz (B–P) und gibt das Gerät nur aus.
- Höhen-Atten: recproaudio 0,83 dB, Gyraf (47 n / 94 n / 197 n) 1,78 dB (Rechnung mit dem Modell 1.2) – also recproaudio.
- Höhen-Boost: recproaudio hat die Frequenzen des EQP-1A (Gyraf keine 8-kHz-Stellung). 3 und 4 kHz teilen sich in beiden Abschriften die
  18 nF (zwei Kontakte; ebenso laut einem Techniker auf R/E/P: „most have a common cap for 3k & 4k“). Ab 1.3 darum wieder 150 mH / 18 nF
  wie abgeschrieben (bis 1.2 an die Werkskurve angepasst 204,1 mH / 13,22 nF: gleiche Resonanz, im Gerät so nicht möglich). VintageWindings
  nennt 156 mH; 150 mH (Abschrift) bleibt, die Resonanz verschöbe sich nur um 2 %.
  Spitze schmal/breit der Werkskurven etwa +18,2 / +10,3 dB (alle sieben Frequenzen), Rechnung mit verlustfreier Spule +19,1 / +10,8 dB
  → Spulenverluste (ab 1.3 Kupfer und Kern, siehe Modell).

**Unsicher:** Das Netz des EQP-1A gibt es nur als Abschrift (recproaudio); die Werkskurven sind womöglich gerechnet. Kupferwiderstand der
Spule nach VintageWindings (Händlerangabe), Kernverluste und Übertragerdaten sind geschätzt bzw. angepasst.

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
  jede „1“ dieser Handschrift ein dünner Strich; der Nutzer hat 80 kΩ bestätigt (Vergleichsbild 2026-11-24). Bass-Boost damit +20,3 … +20,4 dB
  bei 10 Hz (Modell 1.4; mit 10 kΩ +15,9 … +16,0 dB).
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
  +0,5 dB (EQP-1A-Plan: 0 dB), Gegenkopplung 16,5 dB (EQP-1A 21,3 dB, 12AY7 hat die kleinere Verstärkung).
- **Geschätzt**: E-372-Q und G-212-Q ohne Daten → Werte der Triad-Übertrager aus der Anpassung des EQP-1A; Kopplung E-372-Q 0,99999 und G-212-Q
  0,9999 so, dass der Verstärker wie beim EQP-1A und EQH-2 angegeben flach ist („+0, −1 db“, 20 Hz–20 kHz): −0,96 … +0,15 dB. Kleinere
  Kopplung läge etwas flacher (0,99989: −0,92 dB), verbiegt aber bei 96 kHz Rechenrate die Höhen um mehr als 2 dB (cultec_test unit r).
  Mit den Kopplungen des EQP-1A gäbe das Zobel-Glied 350 pF + 22 kΩ +0,5 dB bei 10 kHz und −1,5 dB bei 20 kHz. Spulenverluste wie EQP-1A.
- Keine Werkskurven des EQP-1R: geprüft werden Lage und Höhe (cultec_test curves r): Bass-Boost +20,3 … +20,4 dB (80 kΩ), Bass-Atten −20,1 … −20,3 dB,
  Höhenboost schmal +18,1 … +18,5 dB, breit +10,5 … +10,6 dB (EQP-1A-Werk +18,2 / +10,3), Höhen-Atten 10 kHz −16,5 dB.
- Klirr +10 dBm: 0,006 % (1 kHz), 0,023 % (50 Hz); +21 dBm 0,035 % bzw. 0,062 % (die 12AU7 laufen an +268 V statt +290 V).

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
- **Gegenkopplungswicklung**: Windungsverhältnis 0,02254 je Hälfte (bezogen auf die ganze Primärseite) aus „Overall gain … 0 dB“ (Ausgang an
  600 Ω ∥ (620 Ω + 0,003 µF) gegen Eingangsklemme, 1 kHz, Regler 0; cultec_test nf); Gegenkopplung damit 21,3 dB. Primärseite : ganze
  Gegenkopplungswicklung also 22,2 : 1 – bestätigt von außen (Recherche 2026-11-24): ein zerlegter S-217-D hat je Spule 1575 Windungen primär und
  71 Gegenkopplung (GroupDIY, „1575/71=22.18:1“), der Sowter-Nachbau 9530 „Feedback winding 25:1 (ref primary)“. (1.2: 0,020932 = 23,9 : 1,
  1.1 mit dem fälschlich parallelen 620 Ω: 0,018747; 1.3: 0,02262; mit jeder neuen Anpassung der Übertrager neu bestimmt.)
  Wicklungssinn so, dass sie gegenkoppelt (cultec_test gain; ab 1.2 zählt „MITKOPPELND“ als Fehler).
- **Ausgangsübertrager**: 12,5 kΩ : 600 Ω (CineMag CM-S217D, Nachbau des Peerless: „12,500/300 … 3125:150/75“); Lp 100 H, Kopplung, Wicklungs-
  widerstände und -kapazität geschätzt.
- **Spulenverluste der Höhenanhebung** (ab 1.3; bis 1.2 feste Güte 20 als Reihenwiderstand): Kupfer je Abgriff nach VintageWindings (14 Ω
  bei 32 mH … 34 Ω bei 150,5 mH, dazwischen als Potenz von L: R = 14 Ω · (L / 32 mH)^0,573), dazu Kernverluste als Widerstand parallel zur
  Spule (158,3 kΩ am 150-mH-Abgriff, mit L / 150 mH auf die anderen Abgriffe übersetzt; angepasst; 1.3: 315,65 kΩ). Spulengüte bei der
  Resonanz damit 33 (3 kHz) … 18 (8 kHz) … 9,5 (16 kHz), Kupfer allein 85 … 225 – die Güte sinkt zu hohen Frequenzen, wie bei Kernverlusten.
  EQP-1R: Kupfer der 50- und 100-mH-Spule wie ein EQP-1A-Abgriff gleicher Induktivität, in Reihe bzw. parallel wie geschaltet (geschätzt).
- **An die Werkskurven angepasst** (cultec_test fit, Nelder-Mead, 13 Kurven und die Spitzen der Höhenanhebung bei 3/8/16 kHz; ab 1.4 mit der
  Nebenbedingung Verstärker −1 … +0,3 dB von 20 Hz bis 20 kHz, sonst glich die Anpassung die Höhen von B–O mit einem Verstärker von −2,2 dB bei
  20 kHz aus): HS-29 Lp 255 H und Eisenverluste 241,5 kΩ parallel zur Primärseite, HS-56 Lp 35,85 H, Kernverluste der Höhenspule (oben); aus
  drei Startpunkten dasselbe Ergebnis, HS-29 Lp danach von 249,7 auf 255 H, damit der Verstärker bei 20 kHz sicher über −1 dB bleibt.
  Effektivfehler aller 105 Punkte 0,94 dB (1.3 mit B–P: 0,89 dB; 1.2: 0,76 dB mit der im Gerät unmöglichen 3-kHz-Stellung und Gyraf-Bässen);
  einzeln Bass-Boost 0,37 … 1,32, Bass-Atten 0,49 … 0,68, Höhen-Atten 0,80 … 1,01, Höhenboost 3 kHz breit 1,15 / schmal 1,96 dB (die
  Werkskurve 3 kHz ist schmaler, als 150 mH / 18 nF ergeben). Spitzen schmal +17,9 … +18,8, breit +10,7 … +11,0 dB (Werk +18,2 / +10,3).
  Wegen der womöglich gerechneten Werkskurven (Quellen oben) wird nicht weiter angepasst.
- **Geschätzt, an das Datenblatt** („Flat, 20 cps to 20 kcs; +0, −1 db“): Kopplung HS-56 0,99995, HS-29 0,99985, Kapazität der HS-29-Sekundärseite
  40 pF, HS-56 100 pF; Ergebnis −0,99 … +0,07 dB (Netz bei Reglern 0 rein ohmsch gerechnet, siehe Lage des Bass-Atten-Kondensators).
- **Klirr** (Datenblatt höchstens 0,15 % bei +10 dBm an 600 Ω): 0,003 % bei 1 kHz, 0,024 % bei 50 Hz; +21 dBm 0,017 % bzw. 0,038 %; +24 dBm
  0,033 % (ab 1.2 mit der richtigen Reihenschaltung am Ausgang und damit stärkerer Gegenkopplung). Das Modell ist ideal symmetrisch (gleiche Röhren); Kernsättigung der Übertrager nicht gerechnet (keine Daten).
- **Rechenrate**: die Trapezregel verbiegt die Frequenzachse; bei 48 kHz ohne Überabtastung lägen 16 kHz bei 12 kHz. Darum wird die Resonanz der
  Höhenspule auf die Rechenrate vorverzerrt (Spule so, dass die digitale Resonanz genau auf der analogen liegt), und das Plugin rechnet bis 4-fach
  (intern höchstens etwa 200 kHz, nur 1-, 2- oder 4-fach; bis 1.1 bei 50–66,7 kHz fälschlich 3-fach mit den Filtern für 4-fach). Abweichung zur analogen Rechnung bei 44,1/48 kHz und 4x höchstens 0,4 dB bis 20 kHz, bei 2x bis 1,9 dB
  (EQP-1R mit HIGH ATTEN 10; cultec_test unit).
- **„Audio“-Kennlinie** der Bass-Potis: 10 % des Widerstands in der Mitte (geschätzt; Allen-Bradley Typ A). Die Werksdiagramme zeigen Zwischen-
  stellungen nur als „typical settings“ ohne Knopfstellung; die Form der Zwischenkurven (gleicher oberer Knick, flachere Absenkung) trifft das
  Modell.
