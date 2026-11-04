# Yamaha G100-212II: Schaltung, wie sie gerechnet wird

Quelle (nicht im Repository): Yamaha Service Manual **YAMU-02234** „G50-112II, G100-112, G100-115II, G100-210, G100-212II“ (19 Seiten, '79.10):
Seite 1 Frontplatten, Seite 3 Technische Daten, Seiten 4–6 Einstell- und Prüfanweisungen (Tabellen 1–11), Seite 7 Frequenzgangdiagramme,
Seite 8 Netzteil und Endstufe (DC-/MA-Platine), Seite 9 Vorverstärker (Scan mit 600 dpi – die anderen Seiten 300 dpi), Seiten 11–19 Stücklisten.
Für den G100-212II gilt der **untere** Schaltplan auf Seite 9 („G100-115II, G100-210, G100-212II: JK BOARD, PA1 BOARD, PA2 BOARD“).
Die Netzliste im Code (`engine/src/g100_core.hpp`, Funktion `buildInto`) ist die verbindliche Fassung dieser Liste; `engine/tests/g100test.cpp` prüft sie gegen die Tabellen.

Schreibweise des Plans: „1/25 BP“ = 1 µF/25 V bipolarer Elko, „1/35“ = 1 µF/35 V Tantal, „0.0027“ = µF, „220P“ = pF, „A25K“ = 25 kΩ logarithmisch (Audio),
„B“ linear, „C“ umgekehrt logarithmisch, „G 50K Center Click“ (Stückliste) = LEVEL-Poti mit Mittelrastung. ICs 1, 2, 4, 5, 6, 7, 8: NJM4558D (±15 V), IC3: TA7220P (Halltreiber).

## Eingang (JK-Platine) und IC1 1/2
- Buchse HIGH: Spitze → 15 kΩ → Knoten N0. Buchse LOW: Spitze → 47 kΩ → N0. Der Steckerschalter der HIGH-Buchse legt deren Spitze ohne Stecker an Masse:
  an LOW wirken 47 kΩ gegen 15 kΩ als Teiler (−12 dB, 60 kΩ), an HIGH liegt die LOW-Seite offen (1 MΩ). (Technische Daten: HIGH 1 MΩ, LOW 60 kΩ; Tabelle 3: LOW 12 dB leiser.)
- N0 → 1 kΩ → 0,047 µF → Knoten N1; N1: 47 pF und 1 MΩ gegen Masse; N1 = IC1 Pin 5 (+).
- IC1 1/2: Pin 7 → 10 kΩ → Pin 6 (−); Pin 6 → 1 kΩ → 10 µF/16 V BP → Masse (Verstärkung 11). Zwischen Pin 5 und Pin 6: D101, D102 (1S1555, gegeneinander parallel) und 220 pF.
- Versorgung von IC1 über 560 Ω + D103 (1S1555) nach Pin 8 mit 220 µF/16 V, und D104 + 560 Ω nach −15 V an Pin 4: Betriebsspannung etwa ±12,4 V.
- Ausgang IC1 Pin 7 = „AO/A1“ auf die PA2-Platine: speist **beide** Klangregelungen gleichzeitig (das Relais wählt erst hinter den Kanälen).

## Klangregelung (PA2-Platine, Kanal A; Kanal B auf der PA1-Platine baugleich)
Fender-Anordnung, um den Faktor 10 niederohmiger:
- A1 → 0,0027 µF → oberes Ende TREBLE (A25K); TREBLE-Schleifer = Ausgang.
- A1 → 10 kΩ → Knoten N; N → 1 µF/25 V BP → unteres Ende TREBLE = oberes Ende BASS (A25K); N → zwei 1 µF/35 V Tantal gegeneinander in Reihe (0,5 µF) → unteres Ende BASS = oberes Ende MIDDLE (A1K); MIDDLE unten an Masse.
- BASS und MIDDLE: Schleifer mit dem oberen Bahnende verbunden (Rheostat); Knopf 10 = volle Bahn.
- PULL FAT: A1 → 0,0056 µF → Knoten P; P → 330 kΩ → Masse; Schalter (gezogenes VOLUME- bzw. GAIN-Poti) verbindet P mit dem oberen TREBLE-Ende.
- TREBLE-Schleifer → 1 µF BP → IC8 2/2 Pin 3 (+) [Kanal B: IC2 2/2 Pin 5]; dort 100 kΩ und 560 pF gegen Masse, 100 pF zwischen + und −; Gegenkopplung 3,3 kΩ / 1 kΩ (Verstärkung 4,3).

## Kanal A (PA2-Platine)
- IC8 2/2 Pin 1 → 4,7 µF BP → 220 Ω → oberes Ende VOLUME (A25K, mit Zugschalter FAT-A); unteres Ende Masse.
- PULL BRIGHT-A: 0,0056 µF vom oberen VOLUME-Ende → Schalter → Schleifer; am Kondensator 1 MΩ gegen Masse.
- Schleifer → 1 µF BP → 220 Ω → IC8 1/2 Pin 5 (+); 100 kΩ gegen Masse, 100 pF zwischen + und −; Gegenkopplung 5,6 kΩ / 1 kΩ (Verstärkung 6,6).
- Pin 7 → 4,7 µF BP → Knoten (100 kΩ gegen Masse) → Relaiskontakt A → „AO/A1“ (PA1-Platine) = Eingang des Entzerrers.

## Kanal B (PA1-Platine)
- IC2 2/2 Pin 7 → 220 Ω → 4,7 µF BP → oberes Ende GAIN (A25K, Zugschalter FAT-B); PULL BRIGHT-B wie bei A (0,0056 µF, 1 MΩ).
- Schleifer → 220 Ω → 1 µF BP → IC2 1/2 Pin 3 (+); 100 kΩ, 100 pF; Gegenkopplung 4,7 kΩ / 1 kΩ (Verstärkung 5,7).
- Pin 1 → 0,1 µF → 10 kΩ → Knoten X: 330 pF und 100 kΩ gegen Masse, D105/D106 (1S1555) gegeneinander parallel gegen Masse (Begrenzung auf etwa ±0,6 V),
  TR102 (2SK30A GR) Drain–Source gegen Masse = Stummschalter (leitet, wenn Kanal A gewählt ist; Gate über 100 kΩ aus dem Teiler 10 k / 12 k / 22 k zwischen ±15 V, den TR107 vom Relaistreiber B umschaltet).
- TR101 (2SK30A GR): Gate X, Source → 1 kΩ ∥ 47 µF/6,3 V → Masse, Drain → 5,6 kΩ → Knoten (220 µF/16 V gegen Masse) → 560 Ω → +15 V.
- Drain → 1 µF BP → oberes Ende MASTER VOLUME (A25K); Schleifer → Relaiskontakt B → Eingang des Entzerrers.

## A/B-Umschaltung
Relais FRL644D12/2AS (Kontakte A und B), Treiber TR103–TR106 (2SC2240, 2SC1509), LED-A/LED-B, Tastschalter A/B auf der Frontplatte und Fußschalterbuchse (parallel).
Für die Rechnung: Kontakt A bzw. B geschlossen, der andere offen; TR102 stumm bei Kanal A.

## Nach dem Relais, Hall (ab 1.1 gerechnet)
- Knoten A1: 100 kΩ gegen Masse; Abzweig zum Halltreiber: A1 → 2,2 kΩ → 1 µF BP → IC3 Pin 3 (100 kΩ gegen Masse, 68 pF).
- Halltreiber IC3 (TA7220P, Leistungs-IC an ±20 V): 100 pF zwischen Pin 3 und Pin 4, Gegenkopplung 100 kΩ von Pin 8 (Ausgang) auf Pin 4, Pin 4 → 2,2 nF → 2,2 kΩ → Masse.
  Verstärkung 1 bei tiefen Frequenzen, ab 723 Hz mit +6 dB/Oktave steigend bis 46 (100 kΩ / 2,2 kΩ + 1): der Treiber liefert bei hohen Frequenzen mehr Spannung, damit durch die induktive Treiberspule ein über die Frequenz etwa gleicher Strom fließt (Stromtreiber).
  Pin 8 → 10 µF BP → 56 Ω → Tankeingang D (100 pF D–E, E = Masse). Gerechnet wie die OPVs (spannungsgesteuerte Quelle, Leerlaufverstärkung 10⁴, 100 Ω, Grenze ±17 V geschätzt).
- Federtank: der Plan nennt nur „Reverb Unit“ (Accutronics-Bauart, Impedanzcodes nicht angegeben). Angenommen: Eingang 28 Ω + 24 mH (Code B: 150 Ω bei 1 kHz – passt zum 56-Ω-/723-Hz-Treiber), Ausgang 200 Ω + 0,357 H (Code B: 2,25 kΩ bei 1 kHz), zwei Federn.
  Die Treiberspule liegt im Netz (Strom = Eingang des Federmodells), die Abnehmerspule als Spannungsquelle (Ausgang des Federmodells, ein Abtastwert verzögert) hinter 200 Ω + 0,357 H.
- Rückholverstärker IC7 1/2: Tankausgang P → 1 nF gegen Masse → Pin 3 (100 kΩ gegen Masse), 220 pF zwischen den Eingängen, Gegenkopplung 100 kΩ gegen 1 kΩ + 1 µF: 101-fach ab 160 Hz. Die Abnehmerinduktivität bildet mit dem 1 nF eine Resonanz bei etwa 8 kHz (Güte durch die 100 kΩ begrenzt; die Feder liefert dort schon wenig).
- Hallrückweg: IC7 1/2 → 4,7 µF BP → REVERB (B25K, linear) → Schleifer → 2,2 kΩ → RFS (Fußschalterbuchse) → 47 kΩ → Ausgangsknoten O. Mit REVERB auf 0 liegen so 49,2 kΩ von O gegen Masse. Der Fußschalter ist nicht nachgebildet (immer ein).

## Federmodell (engine/src/spring.hpp)
Physikalisches Modell der Feder statt einer gemessenen Impulsantwort, nach Abel/Berners/Costello/Smith (AES 2006) und Välimäki/Parker/Abel (JAES 2010):
- Unterhalb der Übergangsfrequenz fC laufen Torsionswellen dispersiv – tiefe Frequenzen kommen zuerst an, hohe später –, jedes Echo ist ein aufwärts gleitender Chirp, der mit jedem Umlauf länger wird. Nachgebildet mit einer Kette von 100 gestreckten Allpässen erster Ordnung (Streckung K = fs / 2 fC, Koeffizient 0,6) in einer Rückkopplungsschleife mit der Laufzeit der Feder.
- Je Feder: Laufzeit 33 bzw. 37 ms, Nachhallzeit 2,75 s bei tiefen Frequenzen, Tiefpass 6 kHz und Hochpass 40 Hz in der Schleife; oberhalb fC = 4,2 kHz ein zweiter, kaum dispersiver Weg mit 1,2 s Nachhall und einem Viertel des Pegels. Eingangshochpass 120 Hz (Wandler). Ausgang je Feder nach einem Durchlauf (das erste Echo kommt nach der Laufzeit, kein Direktanteil).
- Übertragung Spulenstrom → Leerlaufspannung der Abnehmerspule 16 V/A als Effektivwert im Mittel über die Frequenz (die Schleife wirkt als Kammfilter; die Zähne sind herausgerechnet).
- Alle Werte geschätzt (kein Tank ausgemessen): Laufzeiten und Nachhall wie bei einem kurzen Zweifedertank (Typ 8), fC nach der Literatur (2 bis 5 kHz), Pegel so, dass REVERB 10 bei einem Akkord etwa Hall = Direktsignal ergibt (engine/tests/g100test.cpp: Treiberverstärkung, erstes Echo 32,6 ms, T60 2,3 s aus Fensterenergien, Hall/Direkt +0,1 dB).

### Nichtlinearer Federtank (ab 1.2, Schalter REVERB TANK; „LINEAR 1.1“ rechnet Bit für Bit wie 1.1)
Anlass (Nutzer): ein Rimklick setzt die Feder in Schwingung, der nächste Anschlag trifft auf eine schon schwingende Feder, und je nach Pause und Nachschwingen
ändert sich das Verhalten. Im linearen Modell überlagern sich zwei Anschläge nur. Drei Ursachen eingebaut, alle mit geschätzten Kennwerten (`springrev::Params`):
1. **Wandler**: Kernsättigung der Treiberspule, Antrieb = isat · tanh(i / isat), isat = 80 mA Spitze; Kopplung jeder Magnetperle 1 / (1 + (m/m₀)²) nach ihrer
   momentanen Auslenkung m (Treiberseite: eingespeiste plus zurückgelaufene Welle; Abnehmerseite: ankommende Welle), m₀ = 19 mA-Äquivalent (ab 1.3: Spitzen-Nennstrom des B-Eingangs; 1.2: 80 mA – damit war der Unterschied bei normalem Spiel unhörbar, Nutzer: „ich höre keinen Unterschied“).
   Anhalt: Typ-B-Eingang (150 Ω) Nennstrom etwa 6,5 mA eff. / 19 mA Spitze; beim 8-Ω-Typ setzte die Sättigung bei etwa dem Zehnfachen des Nennstroms ein
   (sound-au.com, „Reverb Tanks“), auf 150 Ω umgerechnet etwa 58 mA eff. ≈ 80 mA Spitze.
2. **Feder**: Hüllkurve der ankommenden Welle (Anstieg 1 ms, Abfall 20 ms), Aussteuerung q = min(1, (Hüllkurve/m₀)²): Schleifenlaufzeit (1 − 0,03·q) – steifer,
   schneller, linear interpoliert gelesen –, Schleifenverstärkung g·(1 − 0,10·q) – stärker gedämpft –, weiche Grenze der Verdrillung 2·m₀ = 38 mA-Äquivalent (tanh; 1.2: 160 mA).
3. **Rückwirkung**: Spannung rMot · Σ m·(B(m) − 1) in Reihe mit der Treiberspule (Quelle u[3], Knoten Lb zwischen Spule und Masse; rMot = 2 Ω).
   Nur die Abweichung von der Nenn-Kopplung, weil die lineare Rückwirkung schon in den Kennwerten des linearen Tanks steckt (Nachhall, Impedanz).
   Gemessen im Kern: als volle Rückwirkung gerechnet dämpfte rMot = 2 Ω den Nachhall bei 150–600 Hz nach 1–2 s um 3–4 dB (mit dem anderen Vorzeichen
   entsprechend mehr Nachhall – mit 10 Ω liefe das um 30 dB auf, deshalb klein gehalten und nur als Abweichung).
Spulenströme im Modell (96 kHz, 0,5 s): Kanal A VOLUME 5 mit 100 mV-Sinus 5,6 mA Spitze, mit 500 mV 28 mA; VOLUME 10 mit 100 mV 39 mA, Anschlag 1 V 129 mA;
Kanal B 10/10 mit 100 mV 53 mA. Prüfstand (g100test, ab 1.3): leiser Akkord Hallanteil linear +0,10 / physikalisch +0,45 dB (1.2: +0,11); zweiter Anschlag 50/200 ms nach dem ersten
(1-kHz-Burst 0,5 ms, Kanal A VOLUME 10): bei 0,05 V Wechselwirkung −22 / −21 dB (1.2: −45 / −43; linear −128 / −132 dB), bei 0,3 V −4 / −6 dB (1.2: −19 / −18; linear −68 / −117 dB;
bei harten Anschlägen begrenzen auch IC3 und IC7, dann wirkt schon das lineare Modell nicht mehr rein überlagernd); Dauerlast Kanal B 10/10 mit REVERB 10:
kein NaN, keine Newton-Fehlschläge, Abklingen nach dem Ende −35 / −59 dB nach 1–2 / 2–3 s. Rechenzeit praktisch unverändert.
Ab 1.4 Regler REVERB TANK 0–10 (`Tank::setStrength`): 0 linear, 5 wie 1.3 (bitgleich), unter 5 alle Maßstäbe größer (m0·16^−f bis 300 mA, isat bis 320 mA), über 5 Federaussteuerung empfindlicher (mq = m0·16^−f bis 1,2 mA), Laufzeit bis 12 %, Dämpfung bis 30 %, Kopplung/Grenze m0·2^−f. Plugin-Grundstellung (50-mV-Töne), REVERB 10: Unterschied gegen den Hall −77 / −62 / −36 / −8 / +2,5 dB bei 1 / 2,5 / 5 / 7,5 / 10, Hall −1,8 dB bei 10.
Unterschied physikalisch/linear im Plugin bei 5 (gezupfte Töne, REVERB 10, gegen den Hall): Kanal A VOLUME 5 / 100 mV −24 dB, B GAIN 10 MASTER 5 −10 dB, A VOLUME 10 −2 dB (1.2: −48 / −32 / −16 dB).

## Parametrischer Entzerrer (PA1-Platine)
- A1 → 10 kΩ → IC4 1/2 Pin 6 (−), Pin 5 an Masse, 10 kΩ Gegenkopplung (invertierend ×1) → V1. V1 → 10 kΩ → IC4 2/2 Pin 2 (−), 10 kΩ Gegenkopplung → V2 (invertierend ×1; insgesamt nicht invertierend).
- LEVEL (G50K, Mittelrastung) zwischen den beiden Summenpunkten (Pin 6 von IC4 1/2 und Pin 2 von IC4 2/2); Schleifer → 680 Ω → Ausgang des Puffers IC5 2/2.
  Schleifer am Summenpunkt von IC4 2/2 (Knopf +5) = Anhebung, am Summenpunkt von IC4 1/2 (−5) = Absenkung.
- Zustandsvariablenfilter: IC5 1/2 Pin 6 (−): 10 kΩ von V1, 10 kΩ vom Tiefpass-Ausgang (IC6 2/2 Pin 1), 10 kΩ Gegenkopplung von Pin 7; Pin 5 (+) = Schleifer des Q-Potis.
  Pin 7 → 1 kΩ → FREQUENCY (C50K, Tandem, als Rheostat) → IC6 1/2 Pin 6, 0,033 µF Gegenkopplung (Integrator 1, Bandpass an Pin 7).
  Pin 7 → 1 kΩ → FREQUENCY (zweite Bahn) → IC6 2/2 Pin 2, 0,03 µF Gegenkopplung (Integrator 2, Tiefpass an Pin 1).
- Q: Bandpass → 4,7 kΩ → oberes Ende Q (A50K); unteres Ende → 6,8 kΩ → Masse; Schleifer → IC5 1/2 Pin 5 und → IC5 2/2 Pin 3 (Puffer, Pin 2 an Pin 1).
  Knopf links (Symbol breit) = Schleifer am Bandpass-Ende (starke Dämpfung, breit), rechts (Symbol spitz) = am 6,8-kΩ-Ende (schmal).
- Mittenfrequenz: f = 1 / (2π · (1 kΩ + R_Bahn) · √(0,033 µ · 0,03 µ)): 99 Hz bei 51 kΩ, 5,06 kHz bei 1 kΩ (Frontplatte: 100 Hz … 5K, 500 in der Mitte).
  Der Code bildet den Knopf über die aufgedruckte Skala ab (100 / 200 / 500 / 1K / 5K bei 0 / 28 / 50 / 72 / 100 % des Drehwegs, dazwischen logarithmisch) und rechnet daraus den Bahnwiderstand.
- Ausgang: V2 → 1 µF BP → 10 kΩ → Knoten O (100 kΩ gegen Masse, Hallrückweg 49,2 kΩ) → „OUT PUT“ zur Endstufe.

## Endstufe (DC-/MA-Platine, Seite 8)
- Eingang: OUT PUT → 2,2 kΩ → Pin 3 des 6-poligen Steckers (4700 pF gegen Masse; TR206/TR207 2SK30A als Einschalt-Stummschalter, im Betrieb gesperrt).
  MA-Platine: Pin 3 → 10 kΩ gegen Masse, 1 µF/160 V → Gate TR301 (2SK30A Y), 100 kΩ auf die Mittenspannung (100 kΩ / 100 kΩ aus der Betriebsspannung, 1 µF gepuffert).
- TR301/TR302 Differenzverstärker (Sources über B4,7k „Output Waveform“ mit 10 µF BP, 120 kΩ nach Masse), Stromspiegel TR303/TR304 (2SA872A), TR305 (2SA970), Spannungsverstärker TR306 (2SC1624),
  Ruhestrom-Stufe TR307 (2SC1509) mit B470 „Idling Current“ (5 mV über 0,22 Ω, beide Enden), Treiber und Endtransistoren TR308–TR312 (2SA777, 2SC1624, 2SA814, 2SC1586 beim G100; R1 680 Ω), Emitterwiderstände 0,22 Ω.
- Gegenkopplung: Ausgang → 1,8 MΩ → Gate TR302; Gate TR302 → 100 kΩ → Mittenspannung, → 10 kΩ → 1 µF/160 V → Masse. Spannungsverstärkung aus den Werten: 1 + 1,8 M / (100 k ∥ 10 k) = 199 (46 dB) oberhalb 16 Hz.
- Einfache Betriebsspannung: Mittenspannung am Ausgang 45 V ± 1 V (B470k „Center Voltage“, Einstellanweisung Seite 4), Ausgangselko 2200 µF/160 V an die Lautsprecherbuchsen (2 × JA3066, 8 Ω, parallel = 4 Ω).
- DIRECT OUT: Lautsprecherspannung über 820 Ω / 10 Ω (−38 dB) – kein eigenes Signal, daher im Nachbau nicht als Ausgang vorgesehen.
- **Pegeleichung:** Tabelle 3 (Seite 4) misst −70 dB (0,245 mV) an HIGH → 1 dBm (0,87 V) an 4 Ω bei Tabelle-1-Stellung (alle Regler Maximum, LEVEL Mitte, Q und FREQUENCY Minimum, Kanal A): 71 dB.
  Der Vorverstärker ergibt aus den Bauteilwerten 30,6 dB bis zum Gate von TR301 (Klangregelung −10 dB bei 1 kHz, Ausgangsnetzwerk mit 10-kΩ-Eingang der Endstufe −8 dB).
  Mit 199-fach läge das Ganze 5,6 dB über der Tabelle (außerhalb ± 3 dB); der Code eicht die Endstufe deshalb auf 105-fach (40,4 dB) bei 1 kHz und behält die Form der Gegenkopplung (Schulter unter 16 Hz).
  Dazu passt die Einstellanweisung der MA-Platine: −6 dB (0,39 V) am Eingang ergibt „eine gerundete, senkrecht symmetrische Kurve“ an 8 Ω – mit 199-fach wäre das längst ein Rechteck.
- Begrenzung (Tabelle 5): 100 W an 4 Ω (20 V eff) bei weniger als 10 % Klirr, höchstens 140 W bei 10 %; im Code weiches Knie 26 V Spitze, Grenze 31 V Spitze (an 4 Ω), Hochpass 18 Hz durch 2200 µF an 4 Ω.

## Prüfwerte aus dem Service Manual (engine/tests/g100test.cpp)
Tabelle 1 (Stellung für alle Messungen): VOLUME, GAIN, MASTER, TREBLE, MIDDLE, BASS Maximum; LEVEL Mitte; Q, FREQUENCY, REVERB Minimum; FAT/BRIGHT aus; Kanal A. Eingang HIGH, Last 4 Ω.
- Tabelle 3 (−70 dB / 1 kHz): A HIGH 1 ± 3 dBm, A LOW −11 ± 3 dBm, B HIGH 14 ± 3 dBm (dBm hier: dB gegen 0,775 V).
- Tabelle 6 (gegen 1 kHz): 70 Hz −1, 400 Hz −8, 7 kHz +6 (A) / +5 (B), je ± 3 dB. Fig. 1-2: Verlauf 20 Hz … 20 kHz.
- Tabelle 7 (Regler Maximum → Minimum): TREBLE 16 dB bei 7 kHz, MIDDLE 6 dB bei 400 Hz, BASS 11 dB bei 70 Hz, je ± 3 dB.
- Tabelle 8: FAT 9 ± 3 dB bei 400 Hz. Tabelle 9: BRIGHT 11 ± 3 dB bei 7 kHz (VOLUME/GAIN auf 5).
- Tabelle 10 (VOLUME 5, LEVEL Maximum/Minimum): Q min & F min, 650 Hz: ± 7 ± 2 dB; Q min & F max, 650 Hz: ± 7 ± 2 dB; Q max & F min/max, 650 Hz: höchstens ± 1 dB; Q max & F min, 90 Hz: ± 15 ± 2 dB; Q max & F max, 5 kHz: ± 15 ± 2 dB.
- Tabelle 11 (Rauschen): nicht nachgebildet. Die Hall-Prüfung des Manuals (Pegel am Tankausgang) hat keinen Sollwert für diesen Tank; der Treiber wird gegen die gerechnete Verstärkung 1 / 1,7 / 6,9 / 13 bei 200 Hz / 1 / 5 / 10 kHz geprüft.
Ergebnis des Prüfstands: alle 29 Werte innerhalb der Toleranzen (MIDDLE 4,2 dB bei Soll 6 ± 3 am Rand).

## Abgleich Modell ↔ Plan (2026-11-04, Seite 9 bei 600 dpi, Seite 8 bei 300 dpi, Bauteil für Bauteil)
Alle Widerstände, Kondensatoren, Potis und Verbindungen des Vorverstärkers (JK, PA1, PA2: IC1, beide Klangregelungen, Kanal A, Kanal B mit Dioden/TR102/TR101, Relais, Hall IC3/Tank/IC7/REVERB, EQ IC4/IC5/IC6, Ausgang) und des Endstufeneingangs (2,2 k, 4700 p, 10 k, 1 µ, 100 k; Gegenkopplung 1,8 M, 100 k auf den Bezugsknoten, 10 k + 1 µ gegen Masse – die 1 µ hängt nur am 10 k, nicht am Bezugsknoten) stimmen mit `g100_core.hpp` überein. Die Logik der Stummschaltung ist aus dem Plan bestätigt: TR104 (Emitterfolger, Relais B + LED-B) → 100 k → TR107 → Teiler 10 k / 12 k / 22 k (+15 / −15 V) → 100 k → Gate TR102: Kanal A gewählt → Gate etwa +4,4 V → TR102 leitet → Kanal B stumm; Kanal B gewählt → Gate etwa −5 V → gesperrt.
Abweichung gefunden:
- Eingangsbuchsen (JK-Platine): die Kontaktfedern sind im Plan als „V“-Arme gezeichnet, Anker am unteren Stift, Spitze am oberen; Deutung „geschlossen ohne Stecker“ ist die einzige, bei der HIGH nicht kurzgeschlossen wird und LOW die −12 dB aus Tabelle 3 ergibt. Damit: HIGH-Buchse leer → I an Masse (so im Modell); LOW-Buchse leer → J mit I **und** mit K verbunden, K ist der Knoten hinter 15 k und 47 k. Folge: bei gestecktem HIGH und leerer LOW-Buchse sind 15 k und 47 k überbrückt, das Instrument liegt direkt am 1 k / 0,047 µ. Das Modell hatte die 15 k in Reihe. Wirkung: Pegel +0,13 dB (Tabelle 3 A HIGH 1,1 → 1,2 dBm), ein Pol bei etwa 40 kHz entfällt; hörbar nur allenfalls bei Übersteuerung der Eingangsstufe (Quellenbelastung durch D101/D102). Korrektur vorbereitet (HIGH ohne 15 k), noch nicht veröffentlicht.
Nicht entscheidbar aus dem Plan (bleibt Schätzung):
- Lautsprecherimpedanz: Tabelle 5 gibt 100 W an 4 Ω (28,2 dBm) und 100 W an 8 Ω (31,2 dBm); die Impedanz der zwei JA3066 steht nirgends. Modell: 4 Ω (2 × 8 Ω parallel). Wären es 2 × 16 Ω, läge die Begrenzung bei 40 statt 28 V Spitze und der Ausgangselko bei 9 statt 18 Hz.
- Kennlinien: LEVEL „G 50K“ (Mittelrastung) – Kennlinie im Plan nicht definiert, linear angenommen; Q „A50K“ – angenommen: Knopf 0 = Schleifer am Bandpass-Ende (breit), log-Verlauf dazwischen unbelegt (Tabelle 10 prüft nur die Enden); FREQUENCY „C50K“ – nach der Frontplattenskala gerechnet; deren Enden (100 Hz, 5 kHz) stimmen mit 1 k + 50 k, 33 n, 30 n überein (99 Hz, 5,06 kHz) – das stützt die abgelesenen Kondensatorwerte.
- Kanal A, Koppelkondensator vor VOLUME: Druck „4 7/25“ (Dezimalpunkt fehlt); die G50/G100-112-Fassung derselben Seite zeigt 4,7/25 → 4,7 µF angenommen (47 µF wäre klanglich gleich: 1,4 bzw. 0,14 Hz).
- Halbleiter und Grenzen: 2SK30A (Up, Idss), 1S1555, TR102-Einschaltwiderstand, OPV-Leerlaufverstärkung und -Grenzen (IC1 an ±15 V über 560 Ω + Diode), TA7220P-Grenze ±17 V, Endstufenpegel 105 (Tabelle 3) statt 199 (Rechnung), Knie der Begrenzung.
- Federtank: Typ, Spulen, Laufzeiten, Nachhall, Übergangsfrequenz, Pegel (siehe Federmodell).
Bewusst nicht gerechnet: Einschaltstummschaltung TR205/TR206/TR207 (DC-Platine, JFETs am Endstufeneingang), 820 Ω / 10 Ω an den Lautsprecherbuchsen und DIRECT OUT, Gleichspannungsabgleich der Endstufe („Output Waveform“ B4.7K, „Center Voltage“ B470K), Hall-Fußschalter (gesteckter Schalter legt RFS an Masse), Kompensation 27 p / 47 p des TA7220P, Rauschen.

## Geschätzt oder angepasst (nicht im Plan)
- 2SK30A (TR101): Up −2 V, Idss 4 mA (Datenblatt GR: Idss 2,6 … 6,5 mA): so liegt Kanal B 13 dB über Kanal A (Tabelle 3); Ruhestrom 1 mA, Drain 8,5 V.
- 1S1555: Is 2,5 nA, n·Ut 45 mV (0,58 V bei 1 mA). TR102 im leitenden Zustand 500 Ω.
- NJM4558: Leerlaufverstärkung 10⁴, Ausgangswiderstand 100 Ω, Aussteuerungsgrenze ±13,5 V (IC1 ±11 V wegen der Vorwiderstände in der Versorgung); TA7220P (IC3) an ±20 V: ±17 V.
- Federtank: Typ unbekannt, Impedanzcodes B/B angenommen, zwei Federn 33/37 ms, Nachhall 2,75 s, fC 4,2 kHz, 16 V/A (siehe Federmodell).
- Ab 1.2: Nichtlinearität des Tanks (isat, m₀, Steifigkeit 3 %, Dämpfung 10 %, Grenze, Hüllkurve, rMot) – siehe „Nichtlinearer Federtank“.
- Endstufe: Pegel 105-fach (siehe oben), Knie/Grenze 26/31 V. Innenwiderstand der Quelle 1 kΩ.
- Nicht gerechnet: Lautsprecher, Rauschen, Einschaltstummschaltung, Umschaltknacken des Relais, Hall-Fußschalter (immer ein).
