# Loldano SLO-100 (Nachbau Soldano SLO-100): Schaltung, wie sie gerechnet wird

Quellen (nicht im Repository): beschrifteter Gesamtplan „Annotated Soldano SLO-100 Schematic“ von Rob Robinette (robrobinette.com, „How the Soldano SLO-100 Works“, V1.6k) –
eine Nachzeichnung mit Erläuterungen, kein Werksplan; Schaltungsanalyse von Richard Kuehnel (ampbooks.com, „Soldano SLO“ und „SLO Feedback“); Bedienungsanleitung
des Herstellers (Anordnung der Bedienelemente). Alle Bauteilwerte aus dem Plan; was dort fehlt, ist unten unter „Geschätzt“ aufgeführt.
Ein Werksplan, Messungen an einem Gerät oder Herstellerdaten zu Verstärkung und Frequenzgang liegen nicht vor.

Schreibweise: „A“ am Poti = logarithmisch, „B“/„lin.“ = linear (amerikanische Kennzeichnung). Kern: engine/src/slo_core.hpp, Prüfstand engine/tests/slotest.cpp.

## Signalweg
### Eingang, V1B (12AX7)
- Eingangsbuchse: 1 M gegen Masse, 68 k zum Gitter. Anode 220 k an B+5, Kathode 1,8 k ∥ 1 µ. Anode → 0,02 µ → Verteilpunkt n1 (beide Kanäle).
### OVERDRIVE-Kanal
- n1 → 470 k ∥ 0,002 µ → oberes Ende PREAMP OVERDRIVE (500 k lin.), 0,001 µ vom oberen Ende zum Schleifer (fester Höhenweg), Schleifer → 470 k → Gitter V2A.
- V2A: Anode 100 k an B+4, Kathode 1,8 k ∥ 1 µ. Anode → 0,02 µ → 470 k → Gitter V2B, 1 M vom Gitter gegen Masse.
- V2B („kalte“ Stufe): Kathode 39 k ohne Kondensator, Anode 100 k ∥ 0,001 µ an B+4. Anode → 0,02 µ → 2,2 M gegen Masse → Fotowiderstand LDR2 → Mischpunkt M.
- LDR1 legt im NORMAL-Betrieb das Gitter von V2B an Masse.
### NORMAL-Kanal
- CLEAN: n1 → 470 k → Knoten mit 39 k gegen Masse → 470 k (BRIGHT: 470 pF darüber) → oberes Ende PREAMP NORMAL (500 k lin.).
  CRUNCH: n1 → 470 k direkt zum Regler (39 k abgetrennt, zweite 470 k überbrückt; BRIGHT über den 470 k).
- Schleifer → Gitter V1A. V1A: Anode 100 k an B+5, Kathode 2,2 k ohne Kondensator. Anode → 0,02 µ → 2,2 M ∥ 120 pF → Mischpunkt M; M: 330 k gegen Masse.
- Der NORMAL-Weg bleibt im OVERDRIVE-Betrieb angeschlossen (über 2,2 M, gegenphasig), wie im Gerät.
### V3, Effektweg, V4
- M → 220 k → Gitter V3B. V3B: Anode 220 k an B+3, Kathode 1,8 k ∥ 1 µ. Kathodenfolger V3A direkt an der Anode (Anode an B+3), Kathode → 100 k → FX SEND, 1 k gegen Masse (Teiler −40 dB).
- Effektweg durchverbunden: 1 µ → 1 M gegen Masse → Gitter V4B. V4B: Anode 220 k an B+3, Kathode 2,2 k ∥ 25 µ. Kathodenfolger V4A direkt an der Anode, Kathode 100 k gegen Masse.
### Klangregelung, MASTER
- Kathode V4A → 470 pF → TREBLE (250 k lin.) oberes Ende; → 47 k → 0,02 µ (Bass) und 0,02 µ (Mitten). TREBLE unteres Ende am Bass-Kondensator;
  BASS (1 M log.) als Stellwiderstand zwischen Bass-Kondensator und oberem Ende MIDDLE (25 k lin.), Mitten-Kondensator am MIDDLE-Schleifer, unteres Ende Masse.
- TREBLE-Schleifer → beide MASTER (je 1 M log.), Schleifer über LDR3 (OVERDRIVE) bzw. LDR4 (NORMAL) → 0,02 µ → Phasenumkehr.
### Phasenumkehr V5 (12AX7), Gegenkopplung, PRESENCE
- Gitter je 1 M zum Schwanzknoten T, gemeinsame Kathode über 470 Ω an T, T → 10 k → Knoten F, F → 4,7 k → Masse. Zweites Gitter → 0,1 µ → F.
- Anoden 82 k (Eingangsseite) und 100 k an B+3, 47 pF zwischen den Anoden.
- Gegenkopplung: 39 k vom 4-Ω-Abgriff auf F. PRESENCE: F → 0,1 µ → 25 k lin. gegen Masse (Regler auf = Kondensator an Masse, Gegenkopplung für Höhen aufgehoben).
### Endstufe
- Je Anode der Phasenumkehr 0,047 µ → 220 k zur Vorspannung, 2,2 k je Röhre zum Gitter. Vier 6L6GC, je Seite zwei parallel (gerechnet als eine Röhre mit doppeltem Strom,
  Gitter- und Schirmgitterwiderstand halbiert); Schirmgitter 470 Ω je Röhre an B+2; Kathoden an Masse.
- Ausgangsübertrager 2,2 kΩ Anode–Anode auf 8 Ω; 4-Ω-Abgriff = 0,707 × Spannung der 8-Ω-Wicklung (für die Gegenkopplung).
### Netzteil
- Gleichspannung hinter Innenwiderstand (statt Netztrafo und Brückengleichrichter; der Gleichrichter leitet an Gleichspannung immer und steckt als 0,62 V und 0,5 Ω im Innenwiderstand) → 200 µ (B+1, Anoden) → Drossel → 100 µ (B+2, Schirmgitter)
  → 10 k → 40 µ (B+3) → 15 k → 10 µ (B+4) und 15 k → 10 µ (B+5).
- Vorspannung: Gleichspannung hinter 15 k mit 10 µ (statt 50-V-Wicklung, Diode, Teiler und Trimmer).

## Prüfung (engine/tests/slotest.cpp)
| Prüfpunkt | Plan | Rechnung |
|---|---|---|
| B+1 | 497 V | 497,0 V (darauf geeicht) |
| B+2 | 495 V | 495,1 V |
| B+3 | 378 V | 379,7 V |
| B+4 | 350 V | 361,3 V |
| B+5 | 359 V | 352,8 V |
| Leistung bei 5 % Klirr (NORMAL, 1 kHz) | 100 W Nennleistung | 101 W |
| Ruhestrom je 6L6GC | – | 35,0 mA (Annahme, darauf geeicht) |

B+4 und B+5 sind im Plan vermutlich vertauscht: V1 (an B+5) zieht mehr Strom als V2 (an B+4), die Summe passt.
Ruhespannungen der Rechnung (der Plan nennt keine): V1B Anode 174 V / Kathode 1,47 V; V2A 250 V / 2,01 V; V2B 350 V / 4,21 V; V1A 255 V / 2,15 V; V3B 188 V / 1,57 V;
V4B 203 V / 1,77 V; Phasenumkehr Anoden 252 V / 242 V, Kathoden 43,0 V (Robinette nennt etwa 34 V), Knoten F 12,3 V.
Gerechnet: OVERDRIVE, PREAMP 5, MASTER 3, 1 kHz: 1 mV Spitze am Eingang → 9,0 V eff am Lautsprecher (Klirr 24 %), 10 mV → 13,3 V (84 %); MASTER 10: 44,7 V eff (Rechteck).
NORMAL (Clean), PREAMP 5, MASTER 3: Verstärkung 34,4 dB bei 1 kHz; 82 Hz −5,2 dB, 2 kHz +6,6 dB, 5 kHz +9,4 dB bezogen auf 1 kHz.

## Geschätzt (nicht im Plan)
- Ausgangsübertrager: Primärinduktivität 25 H, Kopplung 0,9999 (primär–primär) / 0,9998 (primär–sekundär), Wicklungswiderstände 50 Ω / 0,2 Ω, Wicklungskapazität 400 pF,
  Eisenverluste 100 k. Mit mehr Streuung (50 mH) schwingt die Gegenkopplungsschleife in der Rechnung.
- Netzteil: Leerlaufspannung 510,45 V hinter 80 Ω (so, dass B+1 in Ruhe 497 V hat), Drossel 10 H / 100 Ω.
- Vorspannung −58,1 V (so, dass jede 6L6GC in Ruhe 35 mA zieht; der Plan nennt keinen Ruhestrom).
- Fotowiderstände VTL5C1: leitend 600 Ω, gesperrt 50 MΩ (Datenblatt), ohne ihre Trägheit (Umschalten im Plugin mit kurzem Aus- und Einblenden).
- Röhrenkapazitäten 12AX7 nach Datenblatt: 1,6 pF Gitter–Kathode, 1,7 pF Gitter–Anode, als echte Kapazitäten zwischen den Elektroden – bei den sechs Verstärkerstufen und dem Eingang der Phasenumkehr.
  Bei den Kathodenfolgern V3A/V4A und der zweiten Hälfte der Phasenumkehr weggelassen (ab 1.1): Frequenzgang bis 20 kHz höchstens 0,03 dB anders. Ohne die Gitter–Kathoden-Kapazitäten der Verstärkerstufen wären es 0,2 dB bei 10 kHz, daher bleiben sie.
- Lautsprecher: Schwingspule 6,4 Ω, 0,6 mH mit 30 Ω parallel, Resonanz 110 Hz (Qms 6, Qes 1) – vier 12-Zoll-Lautsprecher in geschlossener Box als 8-Ω-Last.
- Logarithmische Potis: 15 % in Mittelstellung.
- Röhrenmodelle: 12AX7 nach Dempwolf (wie im Laney), Gitterstrom gemessen; 6L6GC Strahltetroden-Modell aus uTracer-Messungen (D. Reefman), Gitterstrom nach RCA-Datenblatt.

## Nicht gerechnet
Rauschen und Brummen, Trägheit der Fotowiderstände, Geräte im Effektweg, SLAVE-Ausgang, Impedanzwahl (immer 8 Ω), DEPTH (das Original hat keinen), STANDBY/POWER.

## Rechnung
Ein Netz für die ganze Schaltung (72 Knoten, 51 Zustände, 26 nichtlineare Tore), Trapezregel, Newton. Gelöst in drei Gruppen: die ganze Vorstufe (acht Trioden) gemeinsam,
Phasenumkehr mit Endstufe, Netzteil. Je Stufe einzeln gelöst brauchte das Verfahren bei Übersteuerung bis zu sechs Durchgänge (das Gitter der nächsten Stufe belastet die
Anode der vorigen), und der OVERDRIVE-Kanal verstärkte den Rest der Rechnung mit: Ruherauschen bei allen Reglern auf 10 −51 dBFS, gemeinsam gelöst −91 dBFS.
Ausgabe: 100 V = 0 dBFS. Überabtastung 1x/2x/4x; Aliasing bei 48 kHz, 1,3-kHz-Ton, OVERDRIVE, PREAMP 6: −18 / −37 / −47 dB.
Rechenlast: in Ruhe etwa wie der AmpFett SVT-CL je Rechentakt, bei kräftiger Übersteuerung bis zum Doppelten (mehr Newton-Schritte); 2x kostet das Doppelte von 1x.

## Rechenlast (1.1, Befehle je Abtastwert bei 96 kHz Rechentakt, gezählt mit callgrind)
| | 1.0 | 1.1 |
|---|---|---|
| OVERDRIVE, kräftige Anschläge (PREAMP 8, MASTER 6) | 185,9 Tsd. | 173,8 Tsd. |
| NORMAL/CRUNCH, kräftige Anschläge | 169,0 Tsd. | 157,5 Tsd. |
| Ruhe | 78,6 Tsd. | 69,8 Tsd. |

Entfallen: sechs Röhrenkapazitäten, ein BRIGHT-Kondensator, das nichtlineare Tor des Gleichrichters. Offene Schalterkontakte 100 GΩ statt 1 GΩ (mit 1 GΩ sprach das ungeteilte
Signal am CLEAN-Abschwächer vorbei um 0,05 dB über). Ohne Wirkung und verworfen: Röhren des nicht gewählten Kanals als eigene Gruppe (±5 %), Zerlegung über Abtastwerte
weiterverwenden (Ruhe −24 %, übersteuert +9 %), dünne Produkte (+12 … +29 %).
Verteilung bei 1.1, übersteuert: Newton-Löser etwa die Hälfte (Aufbau der Matrix, Zerlegung, Rückwärtseinsetzen, Nachführen der Ströme), Röhrenkennlinien ein Viertel
(exp, log, pow), lineare Produkte ein Achtel. Diese Teile (dk.hpp, b15_core.hpp) teilen sich alle Verstärker; sie schneller zu machen ändert alle Plugins.
