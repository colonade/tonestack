# Cultec EQ-1A – Abschrift des Pultec EQP-1A

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
  8. Werksplan Verstärker EQP-1R (DWG E-112,353-7).
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
| Ausgangsübertrager | Peerless S-217-D: Primär 1–6, Mitte 2/4 an +290 V (40 µF/450 V); Sekundär 7–12 (600 Ω: Brücke 9–10), 620 Ω und 3 nF parallel zum Ausgang; Gegenkopplungswicklung 3–5, Abgriff „S“ über 1000 Ω an Masse (+1,1 V) |
| Gegenkopplung | Wicklungsenden 3 und 5 je über 360 Ω an die Kathoden der 12AX7 (überkreuzt laut Plan) |
| Netzteil | Chicago PCC-55, 6X4, +355 V → 470 Ω → +345 V (40 µF) → 1000 Ω → +325 V (40 µF) → 2200 Ω → +290 V; → 47 kΩ → +250 V; Heizung über 270 kΩ/62 kΩ auf +60 V |

„Overall gain with input & output transformers connected for same impedance is 0 dB.“ Größter Ausgangspegel +21 dBm (Datenblatt S. 4),
mittlerer Eingangspegel −15 … +8 dBm.

## Passives Netz

Aufbau (beide Abschriften und der Werksplan des EQP-1R stimmen überein): ein L-Glied aus Längszweig 10 kΩ (HF-Boost-Poti) und
Querzweig 1 kΩ (HF-Atten-Poti) über einem RC-Glied nach Masse; dahinter der Bass-Atten-Zweig in Reihe zum Ausgang.

| Knoten | Bauteile |
|---|---|
| A (Eingang) | Sekundär HS-56, 620 Ω nach Masse |
| A → B | HF BOOST 10 kΩ linear (Allen-Bradley JU-1031): Ende „+“ an A, Ende „0“ an B; Schleifer → C(f) → Spulenabgriff L(f) → BANDWIDTH 2,5 kΩ linear (JU-2521, schmal = 0 Ω) → Schalter IN → B (1 MΩ über dem Schalter) |
| B → D | HF ATTEN 1 kΩ linear (JU-1021): Ende „−“ an B, Ende „0“ an D; Schleifer → 75 Ω → C(ATTEN SEL) → D |
| D → Masse | LF BOOST 10 kΩ log (JA-1031) als Regelwiderstand ∥ C(Bass-Boost, LOW FREQUENCY) |
| B → P | LF ATTEN 100 kΩ log (CA-1041) als Regelwiderstand ∥ C(Bass-Atten, LOW FREQUENCY) |
| P → O (Ausgang) | 1 kΩ |
| O → D | 10 kΩ |
| O | an HS-29 Primär |

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
- Bass: recproaudio (Boost 1,5 µ / 0,82 µ / 0,22 µ / 0,33 µ, Atten 8,2 n / 27 n / 56 n / 12 n laut Teileliste) trifft die Werkskurven nicht
  (Effektivfehler 3,7 dB, bis 7,6 dB; Atten bei 20 Hz zu breit). Gyraf trifft sie mit Nennwerten der Potis (10,8 k / 87 k angepasst)
  auf 0,7 dB (höchstens 2,1 dB); freie Anpassung aller acht Kondensatoren ergäbe 2,31 / 1,19 / 0,67 / 0,43 µ und 77 / 42 / 20 / 11,5 n
  (0,3 dB) – Gyrafs Werte liegen nahe dabei, also Gyraf.
- Höhen-Atten: recproaudio 0,83 dB, Gyraf (47 n / 94 n / 197 n) 1,78 dB – also recproaudio.
- Höhen-Boost: beide treffen die Nennfrequenzen. Gyraf hat keine 8-kHz-Stellung; recproaudio hat die Frequenzen des EQP-1A.
  Spitze schmal/breit der Werkskurven etwa +18,2 / +10,3 dB (alle sieben Frequenzen), Rechnung mit recproaudio +19,1 / +10,8 dB
  bei verlustfreier Spule → Spulenverluste (Güte) werden an die Spitzenhöhe angepasst (geschätzt, im Kern dokumentiert).

**Unsicher:** Das Netz des EQP-1A gibt es nur als Abschrift; die Bass-Kondensatoren stammen aus einer zweiten Abschrift, die zu den
Werkskurven passt. Spulendaten (Gleichstromwiderstand, Kernverluste) und Übertragerdaten sind geschätzt.

## Werkskurven (abgelesen)

`werkskurven.json`: Maximalkurven der Werksdiagramme, je (Frequenz Hz, dB gegen 1 kHz), mit einem Raster aus den Diagrammen abgelesen
(Lineal-Einblendung, ±0,5 dB). Prüfprogramm: `engine/tests/cultec_test.cpp`.

## Bedienung (Frontplatte, Foto des Nutzers)

Obere Reihe: LOW FREQUENCY BOOST, ATTEN (0–10), HIGH FREQUENCY BOOST, ATTEN (0–10), ATTEN SEL (5/10/20 kHz).
Untere Reihe: IN (Kippschalter), LOW FREQUENCY CPS (20/30/60/100), BANDWIDTH (0 SHARP … 10 BROAD), HIGH FREQUENCY KCS
(3/4/5/8/10/12/16), Netzlampe, OFF/ON.
