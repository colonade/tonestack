# Ampeg SVT-CL: Schaltung, wie sie gerechnet wird

Quellen (nicht im Repository): Ampeg/SLM/LOUD Service Manual SVT-CL (©2007, 61 Seiten, vom Nutzer bereitgestellt):
Vorverstärker „PWA Preamp“ 07S519-12 (Seite 31, Vektorgrafik, Stand 8-21-03; ältere Fassung 07S519-11 Seite 32, neuere HSVTCL 2033753 Seite 3),
Endstufe 07S419-12 Blatt 3 (Seite 57, SVT-CL: PREAMP OUT / POWER AMP IN, SLAVE, BAL OUT), Netzteil Blatt 2/4 (Seiten 56/58, 07S419-04/-12; älter 07S419-11 Seite 12),
Röhrenplatine 07S419-41 (Seite 54) und 2033759 (Seite 8), Lautsprecher-Ausgangsplatine 06A319-14 (Seite 61), Verdrahtung 16W412-01 (Seite 17).
Herstellerdaten: Owner's Guide SVT-CL (1997, 47-509-22) und Owner's Manual SVT-CL (2018, 2042048 Rev. C) von ampeg.com:
300 W RMS min. an 2 oder 4 Ω bei < 3 % Klirr mit 0,4 V RMS Eingang; Gesamtverstärkung 67 dB bei 1 kHz (Regler auf, Klang Mitte), −3 dB bei 40 Hz und 15 kHz;
BASS ±12 dB bei 40 Hz; MIDRANGE +10/−20 dB bei 220/450/800/1,6 k/3 kHz; TREBLE +15/−20 dB bei 4 kHz; ULTRA LO +2 dB bei 40 Hz, −10 dB bei 500 Hz; ULTRA HI +9 dB bei 8 kHz.
Röhren 3× 12AX7, 2× 12AU7, 6× 6550. Ausgangsübertrager 94-602-12: Primär 1750 Ω mit Mittelanzapfung (Mojotone, Ersatz für den SVT-Übertrager);
TubeAmpDoctor (Ersatz 9460212): Sekundär 2/4/8 Ω. Die Kästchen im Plan „RMS signal voltages with full power output“: 372 V an jeder Anode, 30 V an den Gittern →
bei 300 W an 4 Ω (34,6 V) Übersetzung 21,5 : 1 über die ganze Primärwicklung = 1850 Ω Anode–Anode (wie Enzo im Music Electronics Forum für den SVT nennt).

Schreibweise: „.1“ = µF, „470 pF“, „1M A“ = 1 MΩ logarithmisch, „50K L“ = 50 kΩ linear. Spannungen ohne Kästchen = Gleichspannung (Ruhe).

## Vorverstärker 07S519-12
### Eingänge, V1B (12AX7)
- J1 „0 dB INPUT“, J2 „−15 dB INPUT“ mit Schaltkontakten. Nur J1: J1-Spitze → R1 22 k ∥ R3 100 k (R3 über den geschlossenen Schaltkontakt von J2) → Gitter G1.
  Nur J2: J2-Spitze → R3 100 k → G1, R1 22 k → J1-Spitze, die über ihren Schaltkontakt an Masse liegt (Teiler 22/122 = −14,9 dB).
- R4 3,3 M Gitter → Masse. Kathode R8 1,5 k ∥ C2 22 µ. Anode R7 100 k → Knoten (R6 4,7 k von +345 V, C1 10 µ gegen Masse); 235 V.
- Anode → C3 .1 → Knoten X (C4 56 pF gegen Masse).
### ULTRA LOW (SW1:B) und GAIN
- X → R19 220 k → L. X → R11 100 k → C7 470 pF → T. T: R22 3,3 M gegen Masse, Schalterkontakt 9. U: R23 3,3 M gegen Masse, C8 .0047 von L nach U, Kontakt 12.
- L → R21 220 k → M (oberes Ende GAIN P1 1 M A, CW); L = Kontakt 7; L → R20 330 k → Kontakt 10. Gemeinsame Kontakte: 8 = M, 11 = Masse.
- Aus (8–7, 11–10): R21 überbrückt, R20 330 k gegen Masse; X → 220 k → M. Ein (8–9, 11–12): M = T, U an Masse (C8 .0047 von L gegen Masse),
  X → 220 k → L → 220 k → M, Höhen über R11 100 k + C7 470 pF auf M: Mitten abgesenkt (Rechnung siehe Prüfung).
  Zuordnung ein/aus aus der Wirkung (nur so ergibt sich „+2 dB bei 40 Hz, −10 dB bei 500 Hz“); im Plan beide Schalter in derselben Stellung gezeichnet.
  Bestätigt durch 07S519-11 (Seite 32): Kontakte 7/10 „OUT“, 9/12 „IN“ (gedrückt = ein). Der Plan nennt SW1:A/SW1:B Teile eines Schalters; gerechnet
  (wie im Owner's Manual) als zwei getrennte Tasten.
- GAIN-Schleifer → C9 .1 → Gitter V1A.
### V1A (12AX7), ULTRA HIGH (SW1:A)
- Gitter: R26 1 M zum Knoten zwischen R28 1,5 k und R29 1,5 k (Kathode → R28 → R29 → Masse; Gitter-Vorspannung über R28).
- Anode R27 100 k von +345 V; 240 V.
- Kathode → C10 .033 → N; N → R31 100 k → Masse; N → Kontakt 5 (gemeinsam), Kontakt 6 Masse, Kontakt 4 frei. ULTRA HIGH ein: C10 überbrückt die Kathode (3 k) für Höhen.
### Klangregelung (passiv)
- Anode V1A → C11 .1 → P (7 V Signal). P → R48 220 k → B1 (BASS P2 1 M A, CW-Ende), P → C23 470 pF → TREBLE P4 1 M A (CW-Ende).
- BASS: B1–Schleifer C20 .001, Schleifer–B2 C21 .01, B2 = unteres Ende, R37 22 k gegen Masse. Schleifer → R30 100 k → Q.
- TREBLE: unteres Ende → C27 .0047 → Masse, Schleifer = Q.
- Q → R32 100 k → C16 .1 → Gitter V2B.
### V2B (12AX7), MIDDLE mit FREQUENCY, MASTER
- Gitter R40 470 k zum Knoten R44 1,5 k / R45 2,2 k (Kathode → R44 → R45 → Masse). Anode R43 100 k von +345 V; 240 V.
- Anode → C5 .1 → R12 100 k → K; K → R13 8,2 k → Masse (Teiler, etwa 1/13). K → MASTER P5 50 k linear (CW); Schleifer → C6 .1 → V2A.
- MIDDLE P3 50 k linear: CW-Ende ← C12 10 µ ← Kathode V2B; anderes Ende → R36 470 → K. Schleifer → Spule L1 Anschluss 4 („CMN“).
  Gegen den Uhrzeigersinn: Reihenschwingkreis an K (Absenkung), im Uhrzeigersinn: an der Kathode (weniger Gegenkopplung → Anhebung).
- L1: angezapfte Spule, Anschluss 3 = 100 mH, 2 = 300 mH, 1 = 800 mH (ab „CMN“).
- FREQUENCY S2 (5 Stellungen, gemeinsamer Kontakt an Masse; nicht gewählte Rückleiter über je 470 k gegen Masse):
  CW (3 kHz): Anschl. 3 → C17 .033 → Masse (R39). 4 (1,6 kHz): Anschl. 2 → C19 .033 (R41). 3 (800 Hz): Anschl. 2 → C22 .1 (R42).
  2 (450 Hz): Anschl. 1 → C15 .15 (R46). CCW (220 Hz): Anschl. 1 → C13 .68 → Masse (R17 470 k). R47 470 k von Anschl. 1 gegen Masse.
  Resonanzen ohne Verluste: 2771 / 1600 / 919 / 459 / 216 Hz.
### V2A (12AX7, Kathodenfolger)
- Gitter R14 220 k zum Knoten R15 4,7 k / R16 220 k (Kathode → R15 → R16 → Masse); Anode +345 V; Kathode 100 V.
- Kathode → C26 .68 → Ausgang (R33 1 M gegen Masse) → J3-3 (0,4 V) → Endstufe. (07S519-11, Seite 32: C26 .1 µ/200 V; gerechnet .68 wie -12 und HSVTCL.)
- Weg zur Endstufe (Abgleich 2026-11-07): J3-3 → J1-3 „FX SEND“ → Schaltkontakt von J5B „POWER AMP IN“ (nichts gesteckt) → J1-14 „FX RETURN“ → auf der
  Vorstufenplatine J3-14 = J3-12 = J3-9 → J1-12 → R92 (Eingang Endstufe) und J1-9 „POST“ → R91 220 → Primärwicklung T1 (94-025-11, BAL OUT) gegen Masse.
  An FX SEND außerdem R80 1 M gegen Masse und der Eingang von IC2A (Puffer für PREAMP OUT). **Im Modell nicht enthalten:** R80 und der Zweig R91/T1;
  T1-Daten (Induktivität, Widerstände, Übersetzung) stehen in keinem Plan.

## Endstufe 07S419-12
- J1-12 (Vorstufe über FX RETURN, siehe oben) → R92 220 → Spitze der Buchse SLAVE J2 (Schaltkontakt offen) → R7 1 k → Gitter V1A (12AX7); D55/D56 1N3070 antiparallel und R8 470 k gegen Masse.
- V1A: Anode R12 100 k von +345 V (200 V); Kathode → R13 1 k → F → R14 220 → Masse. F = Gegenkopplungseinspeisung (J10).
- Gegenkopplung (Lautsprecherplatine 06A319-14): R1 68 k ∥ C1 100 pF von J6-1 (Übertrager-Abgriff) nach J5-1 → J10 → F. Die Buchsen hängen über S1 (2/4 Ω) an J6-1 oder J6-3;
  die Gegenkopplung bleibt immer an J6-1. Welcher Abgriff J6-1 ist, steht nicht im Plan → geschätzt (4 Ω), geeicht an 300 W bei 0,4 V.
- V1B (12AX7, Kathodyn-Phasenumkehr), direkt gekoppelt: Anode V1A → C6 .1 → Gitter V1B (5,7 V Signal). Gitter R16 1 M zum Knoten R17 820 / R18 68 k
  (Kathode → R17 → R18 → Masse). Anode R15 68 k von +345 V (250 V). C7 50 pF Anode → Gitter.
- Treiber V2/V3 (12AU7, je zwei Systeme): Anode V1B → C8 .1 → Gitter V2-1 (R19 470 k gegen Masse), Kathode R20 1,8 k, Anode R21 47 k von +365 V (180 V).
  Kathode V1B → C9 .1 → Gitter V3-1 (R27 470 k), R28 1,8 k, R29 47 k.
  Anode → C10/C11 .047 → R22/R31 150 k → Gitter des Kathodenfolgers V2-2/V3-2 (Anode +380 V, Kathode R26/R34 47 k nach −180 V).
  Gitter-Vorspannung über R23/R30 150 k vom Schleifer BIAS P2/P1 50 k L im Teiler −180 V → R24/R33 120 k → Poti → R25/R32 68 k → Masse.
  Kathodenfolger-Ausgang (etwa −45 V, 30 V Signal) direkt an die drei Gitterwiderstände einer Seite.
- Röhrenplatine: je 6550 Gitter über R1/R3/R5 47 k, Schirmgitter über 22 Ω (07S419-41; 220 Ω in 2033759) an die 345-V-Leitung, Anode über Ferritperle an den
  Übertrager (660 V). Kathoden über J7/J11 zu R35–R40 10 Ω 2 W 1 % auf der Endstufenplatine gegen Masse (Messwiderstände der BIAS-Anzeige; 0,24 V = 24 mA Ruhestrom;
  am Knoten außerdem R89 2,2 k mit C36 10 µ, R72 1 k und D39 für die Anzeige/Fehlererkennung – nicht gerechnet, < 0,5 %).
  Auf der Röhrenplatine außerdem D1–D3/D7–D9 1N4007 parallel zu den Schirmgitterwiderständen, Ferritperlen L1–L6 und D4–D6/D10–D12 gegen negative Anodenspitzen: nicht gerechnet.
## Netzteil
- Hochspannung: Brückengleichrichter (je zwei 1N4007) → +660 V, C15/C16 100 µ in Reihe (50 µF), R52/R53 150 k; → Übertrager-Mittelanzapfung.
  R54 15 k 5 W → +470 V (C17/C18 47 µ in Reihe, R56/R57 220 k); daraus R58 47 k 1 W → +345 V (C20 100 µ) für V1A/V1B der Endstufe und
  R55 47 k 5 W → +343 V (C19 47 µ) über J20-5 zur Vorstufe (07S419-04, Seite 56: R55 18 k).
- Zweite Wicklung: Brücke D14–D17 → +380 V (C21 47 µ) für die Kathodenfolger; R59 2,2 k → +365 V (C22 47 µ) für die Treiberanoden; D26 → J18 → Schirmgitter (345 V in Ruhe).
- −180 V aus einer eigenen Wicklung, Brücke D18–D21, C23 100 µ (Gittervorspannung); gerechnet als ideale Quelle hinter 1 Ω.
- Innenwiderstände der Wicklungen und die Wechselspannungen stehen nicht im Plan → geschätzt (siehe Code).

## Entscheidungen bei Widersprüchen im Plan
- Schirmgitter: Verdrahtungsplan 16W412-01 führt die Röhrenplatine J13 (Schirmgitterleitung) an die Endstufe J18 = +380 V hinter D26. Auf der Röhrenplatine
  (07S419-41 und 2033759) steht an dieser Leitung „345V“, an den Treiberanoden „180V“ (aus +365 V über 47 k). Gerechnet: Schirmgitterwicklung so, dass die
  Schirmgitter in Ruhe 345 V haben (gemessener Wert auf der Platine, an der die Röhren sitzen); die Treiberanoden liegen dann bei 156 V statt 180 V.
- Vorstufen-Anodenspannung 345 V als ideale Quelle (Plan: +345 V an J4; die Siebkette R54/R55/R58 lässt sich mit den Strömen nicht ganz in Einklang bringen).
- Ruhestrom: BIAS-Schleifer so, dass jede 6550 24 mA zieht (0,24 V an 10 Ω, Plan); Gittervorspannung dann −39,9 V („approx. −45 V“ im Plan).
- Gleichrichterbrücken als immer leitend (Gleichspannung hinter Innenwiderstand, ohne Brumm), D26 als 1 Ω.

## Geschätzt (in keinem Plan)
- Übertrager: Primärinduktivität 30 H, Kopplung primär–sekundär 0,9995 (Streuung), Primärhälften 0,9998, Wicklungskapazität 1 nF, Eisenverluste 50 kΩ,
  Wicklungswiderstände 40 Ω primär / 0,02–0,03 Ω sekundär, Sättigung 6 V·s mit Hysterese wie AmpFett B-15NF. Streuung und Kapazität so gewählt, dass der
  ganze Verstärker bei 15 kHz −3 dB hat wie in der Spezifikation (mit 0,9985 Kopplung: Überhöhung +5 dB bei 24 kHz, mit 0,9999 numerisch singulär).
- Gegenkopplungs-Abgriff J6-1 = 4 Ω (nicht im Plan).
- Spule L1: Wicklungswiderstand 190 Ω je Henry-Wurzel (anteilig je Abschnitt), Kopplung 0,995.
- Netzteil: Hochspannung 663,9 V hinter 20 Ω (660 V in Ruhe), Schirmgitter-/Treiberwicklung 347,4 V hinter 60 Ω.
- 6550: Derk-Form (wie 6L6GC im AmpFett B-15NF), an die Datenblattwerte 250/250/−14 V (140 mA, Ig2 12 mA, 11 mA/V, 15 kΩ) und 400/225/−16,5 V (85–87 mA,
  Ig2 4–9 mA) angepasst; Knie-Form (ex, β, αs) wie 6L6GC. Die SED-6550C-Gegentaktdaten passen nicht zu den Tung-Sol/GE-Werten und wurden nicht verwendet.
- 1N3070 (D55/D56): Shockley, Is 2 nA, n 1,9.
- Quelle (Instrument) 1 kΩ.
- Gitter-Anoden-Kapazität 1,7 pF je 12AX7 (2,0 pF 12AU7), Röhreneigenschaft, nicht im Plan.
- Kennlinie der „A“-Potis (GAIN, BASS, TREBLE): Plan nur „A“; gerechnet log 15 % in Mittelstellung (umschaltbar 10 % / linear).

## Prüfung (engine/tests/svttest.cpp; Rückgabe 1 bei Abweichung)
| Prüfpunkt | Plan / Spezifikation | Rechnung |
|---|---|---|
| Anoden V1B / V1A / V2B, Kathode V2A | 235 / 240 / 240 / 100 V | 224 / 228 / 228 / 110 V |
| Endstufe V1A / Phasenumkehr V1B | 200 / 250 V | 205 / 250 V |
| Signal nach V1B / V1A / V2B-Anode / Ausgang (15 mV, GAIN und MASTER max.) | 0,9 / 7 / 8 / 0,4 V | 0,84 / 6,84 / 7,67 / 0,38 V |
| BASS bei 40 Hz | ±12 dB | ±13,1 dB |
| TREBLE bei 4 kHz | +15 / −20 dB | +14,4 / −17,6 dB |
| MIDRANGE bei 220 / 450 / 800 / 1600 / 3000 Hz | +10 / −20 dB | +10,7…+9,9 / −19,8…−18,3 dB |
| ULTRA LO 40 Hz / 500 Hz | +2 / −10 dB | +1,8 / −10,8 dB |
| ULTRA HI 8 kHz | +9 dB | +9,3 dB |
| −15-dB-Eingang | −15 dB | −14,95 dB |
| Frequenzgang 15 kHz | −3 dB | −2,9 dB (Übertrager-Streuung geschätzt) |
| Leistung bei 3 % Klirr, 4 Ω / 2 Ω | ≥ 300 W | etwa 360 W (Netzteil geschätzt) |
| Gesamtverstärkung 1 kHz | 67 dB | 72,6 dB (**bekannte Abweichung**) |
| Endstufen-Empfindlichkeit für 300 W | 0,4 V | etwa 0,25 V (**bekannte Abweichung**) |
| Frequenzgang 40 Hz | −3 dB | +0,5 dB (**bekannte Abweichung**) |

- Verstärkung: der Vorverstärker trifft die Kästchen des Plans (0,4 V am Ausgang bei 15 mV), die Endstufe verstärkt 5 dB mehr als die Spezifikation.
  Die Kästchen der Endstufe (5,5 V am Treibergitter → 30 V an den 6550-Gittern, Faktor 5,5) passen nicht zu den Bauteilen des Treibers (12AU7, 47 k / 1,8 k
  ohne Überbrückung, 150 k Last: Faktor etwa 8). Nicht angepasst – jede Anpassung bräuchte einen Wert, der nicht im Plan steht.
- 40 Hz: alle Koppelglieder des Plans liegen unter 20 Hz, die Gegenkopplung gleicht den Übertrager aus; woher die −3 dB der Spezifikation kommen, ist offen.

## Rechenweg
Nichtlineare Zustandsraum-Methode (DK, Trapezregel, Newton) über die ganze Schaltung: 94 Knoten, 51 Zustände, 29 nichtlineare Tore
(4 + 6 Triodensysteme, 2 × 6550-Gruppe (je drei Röhren parallel, Ströme ×3), Begrenzerdioden, Übertragerkern) in vier Lösungsgruppen
(V1B, V1A, V2B, Rest: V2A und die ganze Endstufe in einer Gruppe – getrennt pendelte die Block-Gauß-Seidel-Iteration bei Übersteuerung).
Rechenlast etwa 1,5-mal AmpFett B-15NF (volles Netzteil).
