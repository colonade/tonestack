# Ampeg B-15NF: Schaltung, wie sie gerechnet wird

Quellen (nicht im Repository, Scans mit Wasserzeichen von Vintage Blue, abrufbar über el34world.com):
- Werksschaltplan „Model Nos. B 12 NF & B 15 NF“, Stände **4-65**, **9-65** und **1-66** (R.M.M.)
- „Technical Data, Fourth Edition“ (Ampeg, Eugene B. Andre, 2. Druck 1968): Spannungs- und Widerstandstabellen, Trafo- und Bauteildaten

Grundlage ist der Stand **1-66**. Unterschiede zu den früheren Ständen stehen unten.
Die Netzliste im Code (`engine/src/b15_core.hpp`, Funktion `build`) ist die verbindliche Fassung dieser Liste.

## Röhren
V1, V2 6SL7 (Vorstufe Kanal 1/2), V3 6SL7 (Phasenumkehr), V4, V5 6L6GC (Endstufe), V6 5AR4 (Gleichrichter).

## Eingänge
- Kanal 1, Buchse „Guitar“ (offene Buchse): Spitze → 120 kΩ → Gitter V1A.
- Kanal 1, Buchse „Bass“ (Schaltbuchse): Spitze → 120 kΩ → Gitter V1A; ohne Stecker liegt die Spitze über 100 kΩ an Masse.
  Steckt der Bass in „Guitar“, teilt die leere Bass-Buchse (120 k + 100 k) den Eingang: −3,8 dB und mehr Höhenverlust.
- Kanal 2, Buchse „Inst.“ (Schaltbuchse): Spitze direkt an das Gitter V2A; ohne Stecker an Masse.
- Gitterableitwiderstand: Kanal 1 5,6 MΩ, Kanal 2 2,2 MΩ.

## Vorstufe (je Kanal gleich)
- V1A/V2A: Kathode 5,6 kΩ (ohne Kondensator), Anode 470 kΩ an B4, Koppelkondensator .022 µF auf Volume 1 MΩ (log).
- V1B/V2B: Gitter am Schleifer von Volume, Kathode 2,2 kΩ (ohne Kondensator), Anode 220 kΩ an B4, .022 µF zum Klangregler.

## Klangregler (Leiterplatte, je Kanal)
Eingang I (nach .022 µF):
- Bass: I → 220 kΩ → Bass-Poti 1 MΩ (linear) → 22 kΩ → Masse; .001 µF und .01 µF von den beiden Poti-Enden zum Schleifer;
  Schleifer → 270 kΩ → Masse und → 120 kΩ → Ausgang.
- Treble: I → .00047 µF → Treble-Poti 1 MΩ (linear) → .0047 µF → Masse; Schleifer = Ausgang.
- Ausgang → 270 kΩ → Mischpunkt (Gitter V3A). Beide Kanäle hängen immer am Mischpunkt.
Technical Data bestätigt die Leiterplatte: 220k/.00047 oben, .001/.01 in der Mitte, 22k/.0047 unten.

## Phasenumkehr V3 (6SL7, schwebende Paraphase mit Gegenkopplung)
- V3A: Gitter am Mischpunkt (Ableitung nur über die Klangregler), Anode 120 kΩ an B3, Kathode → 220 Ω → gemeinsamer Punkt N.
- V3B: Kathode an N, Anode 120 kΩ an B3; Gitter 470 kΩ an Masse und über .022 µF an den Mittelpunkt von 470 kΩ (Anode A) / 510 kΩ (Anode B).
- N: 25 µF/25 V an Masse und 1 kΩ an F (Sekundär-Bezugspunkt, über die Brücke 2–3 im Lautsprecherstecker an Masse;
  ohne Lautsprecher bleibt die Phasenumkehr stromlos – Schutz des Übertragers).
- Gegenkopplung: 16-Ω-Abgriff → 10 kΩ → Kathode V3A.
- Ausgänge: je .022 µF, 270 kΩ an die Vorspannung (−50 V), 1 kΩ Gitterstopper an V5 (von V3A) bzw. V4 (von V3B).

## Endstufe
- V4/V5 6L6GC, Kathoden an Masse, feste Gittervorspannung −50 V, Schirmgitter direkt an B2 (425 V), Anoden an den Übertrager.
- Übertrager OT-214: primär 6,5 kΩ (Anode–Anode), sekundär 8 Ω und 16 Ω; Gleichstromwiderstand primär 290 Ω, sekundär 1 Ω / 1,4 Ω.
- Lautsprecher: CTS 15", 8 oder 16 Ω (Technical Data). Gerechnet: 8 Ω am 8-Ω-Abgriff (gleiche Last an der Primärseite wie 16 Ω am 16-Ω-Abgriff).

## Netzteil
- PT-108: 750 V Mittelanzapfung, 160 mA; 5 V für die 5AR4; 6,3 V Heizung.
- B1 (Gleichrichter-Katode, Mittelanzapfung des Übertragers): 30 µF/600 V. B1 → 1 kΩ/10 W → B2 (Schirmgitter): 40 µF.
  B2 → 22 kΩ/1 W → B3 (Phasenumkehr): 40 µF. B2 → 22 kΩ/1 W → B4 (Vorstufen): 40 µF.
- Vorspannung: Ende der HV-Wicklung → 100 kΩ/2 W → Knoten mit 56 kΩ an Masse → 10D6 (Anode am Vorspannungsknoten) → −50 V mit 47 kΩ und 100 µF/100 V.

## Prüfung gegen die Spannungstabelle (Technical Data, ohne Signal, Messgerät 20 kΩ/V, Toleranz ±5 %)

| Messpunkt | Tabelle | Rechnung |
|---|---|---|
| V1 Anode A / Kathode A | 165 V / 2,2 V | 172 V / 2,28 V |
| V1 Anode B / Kathode B | 175 V / 2,0 V | 177 V / 1,86 V |
| V3 Anode A / Kathode A | 235 V / 2,4 V | 225 V / 2,38 V |
| V3 Anode B / Kathode (N) | 225 V / 2,2 V | 216 V / 2,17 V |
| V4/V5 Anode / Schirmgitter / Gitter | 430 / 425 / −50 V | 424 / 420 / −49,6 V |
| Gleichrichter | 430 V | 427 V |

Die Röhrenmodelle sind nur an Datenblätter angepasst, nicht an diese Tabelle; die Übereinstimmung ist also eine echte Prüfung.
Einzig der Innenwiderstand des Netztrafos ist an 430 V angepasst (380 Ω je Hälfte, er fasst Wicklung, Primärseite und Netz zusammen).
Hoch­ohmige Anoden misst ein 20-kΩ/V-Gerät etwas zu niedrig (bei V1A etwa 4 %).

## Geschätzt (nicht im Plan, nicht in der Tabelle)
- Übertrager: Primärinduktivität 40 H, Kopplung 0,9998 (Streuinduktivität), Wicklungskapazität 500 pF (Anode–Anode), Eisenverluste 150 kΩ, keine Kernsättigung (noch).
- Lautsprecher CTS 15" im geschlossenen Gehäuse: Fs 40 Hz, Qms 5, Qes 0,55, Vas 280 l, Gehäuse 85 l; Schwingspule Le 1 mH mit 20 Ω Wirbelstromverlust parallel.
- Schirmgitterstrom-Aufteilung der 6L6GC bei kleiner Anodenspannung.
- Innenwiderstand der Quelle 1 kΩ.

## Unterschiede der Stände
- **4-65 und 9-65**: Übertrager **OT-208** statt OT-214 (keine Daten in Technical Data). Treble-Poti Kanal 1 als „4 M“ eingezeichnet
  (Kanal 2 und Technical Data: 1 M). Bezugspunkt der Sekundärwicklung (Schwarz) fest an Masse statt nur über den Lautsprecherstecker.
- **4-65**: Entstörkondensator am Netzschalter .05 µF/600 V statt 1000 V.
- Sonst Bauteilwerte gleich.

## Offene Punkte
- Ext.-Speaker-Buchse: Die Zeichnung lässt offen, ob der eingebaute Lautsprecher am 8- oder 16-Ω-Abgriff hängt
  (Rückseite des Geräts: „8 OHM EXT SPEAKER“). Für die Rechnung ohne Zusatzbox gleichwertig, siehe oben.
- Frontplatte: nachgezeichnet (`engine/tools/b15_panel.py`), kein Foto in der Simulation.
