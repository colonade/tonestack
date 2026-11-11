# AmpFett 76 – Schaltung, Annahmen, Messungen

Nachbau des FET-Kompressors/Limiters UREI 1176LN nach dem Schaltplan, Fassung mit Op-Amp-Eingang und Klasse-AB-Ausgang
(Plan „1176LN“, Serien mit Suffix K, mit eingetragenen Gleichspannungen). Dazu vier Erweiterungen nach dem Vorbild des
Origin Effects Cali76 (Bass-Ausführung, V2); von diesem gibt es keinen öffentlichen Schaltplan, die Erweiterungen sind ein **eigener Entwurf**.
Der Scan des Plans und das Handbuch liegen nur lokal in `docs/u76/` (nicht im Repo). „UREI“, „1176“, „Universal Audio“,
„Origin Effects“ und „Cali76“ sind Marken ihrer Inhaber.

Code: `engine/src/u76_core.hpp` (Kern), `engine/clap/u76plugin.cpp`, `engine/clap/u76gui.hpp` (Plugin), `engine/wasm/u76_wasm.cpp`,
`web/tonestack/ampfett-76/` (Seite), `engine/tools/u76_panel.py` (Frontplatte, eigene Gestaltung), `engine/tests/u76test.cpp` (Prüfstand),
`engine/tests/u76guitest.cpp`.

## Schaltung (Knotennamen wie im Kern)

- **Eingang**: IC2 als Stufe mit Verstärkung 1 (Aussteuergrenze ±13,5 V als Klemme), C33 6,8 µF, INPUT R5 10 kΩ, R6 27 kΩ auf den Drain des FET.
- **FET Q1** (BF245A) gegen Masse als regelbarer Widerstand. Source: R8 10 kΩ, über C2 100 µF an den Emitterteiler des Vorverstärkers (Bootstrap).
  Gate: R7 2,2 MΩ ∥ C3 200 pF zum Haltepunkt B (C27 0,22 µF); R9 2,2 MΩ ∥ C4 200 pF und C5 0,22 µF zum Gegenkopplungsknoten F
  (halbe Signalspannung am Gate, Verzerrungsausgleich).
- **Vorverstärker** Q2–Q4: C1, R10, C6 27 pF; Gegenkopplung R21 7,5 kΩ, R15 150 Ω, Trimmer R16 100 Ω „DIST“; Ausgang über C9 47 µF an den
  OUTPUT-Regler R27 100 kΩ.
- **Ausgangsverstärker** Q5–Q9: Q5 mit Gegenkopplung R34 4,3 kΩ, Q6/Q7 (PNP), Ruhestromdioden CR1/CR2, Endtransistoren Q8/Q9 mit R40/R41 4,3 Ω,
  Kompensation C12 10 pF + R37 1 kΩ, C29, Bootstrap C14, Auskopplung C15 470 µF auf den Ausgangsübertrager T2.
- **Seitenkette**: Abgriff hinter C9 (Ausgang des Vorverstärkers) → Ratio-Kette B (R26 56 k, R25 68 k, R24 56 k, R2 56 k, R23 47 k gegen Masse;
  Tasten 4/8/12/20 greifen bei V/W/Z/Y ab) → S9 (ATTACK OFF erdet den Eingang) → Regelverstärker Q12–Q15 (Gegenkopplung R68 47 kΩ) →
  C19/C20 6,8 µF → Vollweggleichrichter CR3/CR4 (FD333) → ATTACK R77 25 kΩ (R76 470 Ω ∥ C22 0,022 µF) → Haltekondensator C27.
  RELEASE R78 5 MΩ, R79 270 kΩ parallel zum unteren Teil, gegen den „Q“-Bias-Knoten (R81 2 kΩ, R82 10 kΩ an −10 V, R83 3,9 kΩ).
  Schwellenkette A (R87 150, R42 470, R43 560, R44 1,5 k, R45 2,2 k an −10 V; R46 10 MΩ) legt mit denselben Tasten die Vorspannung der
  Gleichrichterdioden fest (Knoten AC, C21 100 µF).

## Erweiterungen (eigener Entwurf)

- **Stufenlose Ratio**: Tandem-Abgriff, der über beide Ketten gleitet; bei 4, 8, 12 und 20 liegt er auf den Tastenpunkten (Unterschied zur Taste < 0,001 dB).
- **Hochpass der Seitenkette**: Puffer, Sallen-Key-Hochpass 2. Ordnung (Q 0,707, 20–600 Hz, C 47 nF), Puffer, zwischen Vorverstärker-Ausgang und R26.
  Die Ratio-Kette und ihre Belastung durch den Regelverstärker bleiben so wie im Plan (ein erster Entwurf mit dem Filter hinter dem Abgriff hob den
  Pegel der Seitenkette um 2,4 dB). Das Filter ist immer im Netz, der Schalter wählt nur, woher R26 gespeist wird (gleiche Kondensatorliste,
  die Zustände bleiben beim Umschalten erhalten).
- **DRY**: Eingangsspannung zum Ausgang gemischt (Poti log 15 %).
- **STACK**: zweites, gleiches Gerät in Reihe, eigene Regler THRU (INPUT), ATTACK 2, RELEASE 2, OUTPUT 2; Ratio und Hochpass gemeinsam. 20 ms Überblendung.

## Annahmen

- Halbleiter nach Datenblatt: BF245A Idss 4 mA, Abschnürspannung −2 V, λ 0,02; BJT als Ebers-Moll mit Early-Spannung 100 V (β 100–400 je Typ);
  CR1/CR2 (1N4003) als ein Tor mit doppelter Steigungsspannung.
- CR3/CR4 FD333: Durchlasskennlinie wie 1N4148; **Sperrstrom 0,1 nA** (Datenblatt FDH333: höchstens 3 nA bei 125 V; die 1N4148 darf 25 nA bei 20 V).
  Mit den 2,5 nA der 1N4148-Kennlinie fielen am RELEASE-Zweig bis 14 mV ab, und die Ruhedämpfung des FET hinge vom RELEASE-Regler ab.
- Netzteil: +30,3 V hinter 0,5 Ω, −10 V hinter 5 Ω (Gleichspannungen, kein Brumm).
- Ausgangsübertrager T2 ideal 1 : 1, R84 und 10 kΩ Last auf die Primärseite gerechnet. Der Eingang wird unsymmetrisch angesteuert.
- Trimmer DIST in Mittelstellung; das Netzwerk „T+C“ (im Plan „factory selected“, ohne Wert) nicht bestückt.
- „Q“-Bias: so abgeglichen, dass der FET bei ATTACK OFF und schnellem RELEASE 1,0 dB dämpft (Handbuch: Output um 1,0 dB senken; die Stellung der übrigen
  Regler nennt es nicht). Schleifer 0,53813.
- Kein Gerät wurde gemessen.

## Modell und Rechnung

- Zeitbereich, Trapezregel, Knotenanalyse mit Newton-Verfahren (dk.hpp), Halbleiter in fünf Gruppen; ohne Überabtastung, keine Latenz.
- Einschwingen (Ruhezustand): 12 s grob bei 6 kHz, dann bei Zielrate. Dabei C15 (470 µF an der Last) und C9 (47 µF am OUTPUT-Poti), beide τ ≈ 4,7 s,
  für die grobe Stufe auf 1/50; das Haltenetz (C27, C5 an Megaohm) wird nach der halben Zeit auf seinen Ruhewert gesetzt
  („Q“-Bias-Knoten minus Sperrstrom der Dioden mal RELEASE-Zweig). Danach: Ausgang in Ruhe unter 0,1 mV, Haltepunkt auf 0,02 mV.
  Kürzer (6 oder 8 s) reicht nicht, kleinere Siebkondensatoren oder ein kleineres C5 beim Einschwingen verändern das Verhalten.
- Regler gleiten über 15 ms (alle 64 Abtastwerte), wie bei den Verstärkern; Schalter wirken sofort, das Plugin blendet dabei kurz aus und ein.

## Messungen am Modell (u76test, 48 kHz)

| Größe | Plan / Handbuch | Modell |
|---|---|---|
| Gleichspannungen, 25 Punkte | Angaben im Plan | auf 0,1–0,3 V; Ausgangsverstärker +0,45 V (OUT 13,85 statt 13,40 V; die Planwerte sind dort untereinander nicht ganz stimmig) |
| Ruhedämpfung des FET | 1,0 dB | 1,00 dB (Abgleichpunkt); Grundstellung 0,89 dB; RELEASE langsam 0,78 dB |
| größte Verstärkung | 45 dB (Fassungen mit Eingangsübertrager) | 36,7 dB (Eingangsstufe mit Verstärkung 1) |
| Frequenzgang 20 Hz–20 kHz | ±1 dB | +0 / −0,35 dB |
| Verhältnis (Dauerton, zwischen −15 und −5 dBu bei INPUT 7,5) | 4 / 8 / 12 / 20 | 3,4 / 7,1 / 11,4 / 21,2 |
| Schwelle (1 dB Stauchung) | steigt mit der Ratio | −28,0 / −22,8 / −19,9 / −18,8 dBu |
| alle Tasten | – | etwa 12 : 1, Ruhedämpfung 0 dB (FET gesperrt), Schwelle −23,6 dBu |
| Release am Haltekondensator | 50 ms – 1,1 s | 55 ms – 0,88 s |
| Attack am Haltekondensator (63 %) | 20 – 800 µs, ohne Bedingung | schnell 0,27 ms (12 dB) / 0,15 ms (26 dB); langsam 3,2 / 1,8 ms |
| Klirr bei −5 dBu Eingang (etwa 20 dB Dämpfung) | – | 0,06–0,21 % |
| Ruhe | – | Ausgang −77 dBu Spitze (Rechengenauigkeit), keine Schwingung |

- **Gate und Haltepunkt**: Am Gate kommt die Steuerspannung über R7/R9/C5 zur Hälfte sofort an (kapazitiver Teiler C3/C4), der Rest mit
  τ ≈ 1 s (C5 über R7 + R9). Folgen im Modell: Ein Dauerton ist erst nach etwa 4 s statisch (nach 0,6 s liegt der Ausgang bis 0,8 dB höher);
  die Dämpfung erholt sich bei RELEASE 7 nach 0,3 s Ton in etwa 0,2 s, nach 2 s Ton in etwa 0,9 s. So steht es im Plan (Werte am Scan nachgeprüft).
- **Umschalten der Ratio**: Die Vorspannung der Gleichrichter ändert sich, C21 lädt um, der Haltekondensator folgt mit der Release-Zeit: ein bis
  zwei Sekunden veränderte Verstärkung. Das Plugin rechnet im Hintergrund einen eingeschwungenen Zustand für die neue Stellung, der bei einem
  Neustart (Pause, FX aus/an) benutzt wird; die Kennlinie der Webseite schwingt für jede Schalterstellung neu ein.
- **Aliasing** ohne Überabtastung (7 kHz, schnellste Zeiten): gespiegelte Obertöne unter −61 dB, mit allen Tasten −54 dB.
- **Rechenlast**: 15–25 % eines Kerns je Kanal bei 48 kHz, mit STACK das Doppelte; im Browser (WebAssembly) etwa 26 % bzw. 53 %.

## Fehler, die bei der Prüfung gefunden wurden (vor 1.0 behoben)

- R37 fehlte im Netz (stand hinter einem Kommentar): C12 hing in der Luft, die Kompensation des Ausgangsverstärkers fehlte.
- Der Hochpass änderte die Zahl der Kondensatoren beim Umschalten; die Zustände wurden falsch zugeordnet (Haltepunkt sprang auf +9 V).
- Nach dem Einschwingen blieben einige mV Gleichanteil und eine abklingende Schwingung bei halber Abtastrate; der Gleichrichter richtete sie gleich
  (falsche Ruhedämpfung). Behoben durch das oben beschriebene Einschwingen.
