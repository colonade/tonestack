# Lautsprecher des G100-212II im Modell (ab Plugin 1.10)

Schalter SPEAKER (Modellstreifen, Grundstellung aus). Aus: Spannung an den Lautsprecherklemmen wie bis 1.9 (bitgleich).
An: Schalldruck der beiden Lautsprecher in der offenen Box, bei 1 kHz auf die Klemmenspannung normiert (gleicher Pegel bei 1 kHz).
Code: `engine/src/g12t75.hpp`, eingebunden in `engine/src/g100_core.hpp` (Amp::run1, bei der Rechenrate des Kerns, also auch überabgetastet).

## Was im Gerät steckt (Angaben des Nutzers)

- Zwei Celestion G12T-75, 8 Ω, parallel (zusammen 4 Ω; das Service Manual nennt für den G100-212II 2 × JA3066 und 100 W an 4 Ω).
- Etikett „Celestion International Limited, Ipswich, England“ (englische Fertigung).
- Box hinten offen.

## Quellen

| Angabe | Wert | Quelle |
|---|---|---|
| Fs, Re | 85 Hz, 6,77 Ω (8-Ω-Ausführung) | Celestion-Datenblatt G12T-75 (celestion.com/productpdf.php?id=898) |
| Frequenzgang | Diagramm „8 Ω Frequency Response“ | ebenda; aus einem Bildschirmfoto des Nutzers Pixel für Pixel abgelesen (Raster 1/48 Oktave, auf 1/24 Oktave geglättet), Ablesegenauigkeit etwa ±0,5 dB |
| Qms, Qes, Vas, Mms, BL | 10,6 / 0,63 / 61 l / 21,5 g / 11,1 Tm (Re 6,8 Ω) | Datenbank petoindominique.fr, Eintrag „G12T-75 v2026“ (8 Ω); Herkunft dort nicht angegeben, in sich stimmig (Qes, Vas und Mms nachgerechnet) |
| Außenmaße der Box | 687 × 560 (mit Rollen) × 280 mm | Service Manual YAMU-02234, Seite 3 „General Specifications“ |

Ein zweiter Datensatz derselben Datenbank (16 Ω: Qts 0,84, Mms 25,8 g) weicht deutlich ab (Streuung zwischen Exemplaren). Gegen den Bassabfall
der Celestion-Kurve (20–150 Hz) passt der 8-Ω-Satz besser (rms 1,4 dB gegen 1,8 dB). Die Messbedingungen des Datenblatts stehen nicht darin;
der Bassabfall entspricht einem Lautsprecher auf großer Schallwand (ohne Gehäuseeinfluss).

## Kette

1. **Elko und Lautsprecher (exakt nach Ersatzschaltbild):** Endstufe als Spannungsquelle vor dem Ausgangselko 2200 µF, dahinter zwei gleiche
   Lautsprecher parallel: Re, Bewegung als Parallelkreis Res = Re·Qms/Qes, Cmes = Qes/(2π·Fs·Re), Lces = 1/((2π·Fs)²·Cmes).
   Schalldruck auf der Schallwand ∝ Membranbeschleunigung ∝ s · Spannung am Bewegungskreis. Rationale Funktion 3. Ordnung.
   Der Elko bildet mit dem unterhalb der Resonanz induktiven Lautsprecher einen Reihenkreis: +2 dB bei 40 Hz, +1,4 dB bei 30 Hz, −1 dB bei 20 Hz
   gegenüber idealer Spannungsquelle (am Gerät genauso).
2. **Korrektur K:** Celestion-Kurve geteilt durch Stufe 1 ohne Elko (Le weggelassen, steckt in K), angepasst als Verstärkung, zwei Tiefpässe
   2. Ordnung und 10 Glockenfilter (Ausgleichsrechnung auf den Pegel in dB, 20 Hz … 13 kHz, über 7 kHz halbes Gewicht).
   Abweichung des Modells (Schallwand, ohne Elko) von der abgelesenen Kurve: 20–40 Hz rms 0,5 dB (höchstens 0,9), 40–150 Hz 0,4 (0,9),
   150 Hz–7 kHz 0,3 (1,1), 7–13 kHz 1,9 (3,6). Über 13 kHz fällt das Modell stetig weiter (−40 … −50 dB bei 15–19 kHz), das Diagramm zeigt dort
   unruhige −20 … −30 dB. Im Zeitbereich jeder Abschnitt mit bilinearer Transformation, auf seine Frequenz vorverzerrt.
3. **Offene Rückwand (geschätzt):** Die Rückwelle kommt mit dem Umweg D und umgekehrter Polarität beim Hörer an: p = x − e^(−sτ')·LP(x).
   D = 0,6 m: durch die Box schräg zur offenen Rückseite (etwa 0,33 m bei 280 mm Tiefe und 178 mm von der Lautsprechermitte zur Seitenwand,
   Lautsprecher nebeneinander angenommen), außen an der Seitenwand nach vorn (0,28 m), dazu der kleine Mehrweg zum Hörer in 1 m.
   Beugungsverlust der Rückwelle als Tiefpass 1. Ordnung bei 170 Hz (Kantenstufe 115/B für die 687 mm breite Front); dessen Laufzeit 1/(2π·170 Hz)
   wird von der Verzögerungsleitung abgezogen (τ' = D/c − 0,94 ms = 0,81 ms), damit bei tiefen Tönen genau der Umweg D gilt.
   Folge: unter etwa 100 Hz zusätzlich 6 dB je Oktave Abfall, um 200–600 Hz bis +3 dB (Rückwelle gleichphasig).
   Nicht berücksichtigt: Raum und Boden, Hohlraumresonanz der Box, Teilabdeckung der Rückseite, Mikrofonabstand (Datenblatt: auf der Achse).

## Vergleich (dB bezogen auf 1 kHz)

Celestion-Datenblatt und „Modell auf der Schallwand“ unterscheiden sich unter 63 Hz vor allem durch den Elko (Stufe 1, oben), der im Datenblatt nicht vorkommt.

| Hz | Celestion-Datenblatt | Modell auf der Schallwand | Modell in der offenen Box |
|---:|---:|---:|---:|
| 40 | −14,8 | −12,1 | −18,0 |
| 50 | −11,7 | −9,8 | −13,8 |
| 63 | −8,0 | −7,8 | −10,1 |
| 80 | −7,0 | −6,1 | −6,6 |
| 100 | −4,4 | −4,8 | −3,9 |
| 125 | −3,2 | −3,7 | −1,5 |
| 160 | −1,9 | −2,7 | +0,7 |
| 200 | −1,5 | −2,0 | +2,1 |
| 250 | −1,1 | −1,6 | +3,0 |
| 315 | −1,7 | −1,8 | +3,0 |
| 400 | −2,4 | −2,5 | +1,9 |
| 500 | −0,6 | −0,6 | +3,0 |
| 630 | −0,1 | +0,0 | +2,2 |
| 800 | +0,2 | +0,2 | +0,6 |
| 1000 | +0,0 | +0,0 | +0,0 |
| 1250 | −3,3 | −3,5 | −1,9 |
| 1600 | +1,2 | +0,2 | +2,6 |
| 2000 | +3,7 | +3,2 | +4,2 |
| 2500 | +6,9 | +6,6 | +8,2 |
| 3150 | +5,5 | +5,2 | +6,6 |
| 4000 | −2,6 | −2,7 | −0,8 |
| 5000 | −3,5 | −3,1 | −1,4 |
| 6300 | −19,9 | −19,9 | −18,2 |
| 8000 | −18,6 | −20,4 | −18,8 |
| 10000 | −26,5 | −26,9 | −25,2 |

## Unsicher

- Thiele-Small-Werte aus einer Nutzerdatenbank, nicht von Celestion; Exemplare streuen. Eine Impedanzmessung der eigenen Lautsprecher (z. B. REW) würde Fs, Qms, Qes direkt liefern.
- Umweg D und Beugungsecke der offenen Box sind aus den Außenmaßen geschätzt (Größe der Rückwandöffnung unbekannt).
- Das Datenblatt-Diagramm ist geglättet und aus einem Bildschirmfoto abgelesen.
