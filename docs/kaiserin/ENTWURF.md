# Kaiserin Reverb: Entwurf (Plugin 1.0, Web 2026-11-15-10)

Hall mit der Bedienung des Empress Effects Reverb, wie sie dessen Anleitung beschreibt (RVRB user manual, Firmware 6.41): MODE, DECAY, MIX, OUTPUT, LOW, HI, THING 1, THING 2, unendliches Halten am Fußschalter SELECT. Das Original ist ein Digitalgerät; Abtastrate und Rechenverfahren sind nicht veröffentlicht. Das hier ist ein **eigener Entwurf** mit veröffentlichten Verfahren, kein Nachbau des Klangs. Name nach dem Muster der anderen Simulatoren (Empress → Kaiserin); Frontplatte eigene Gestaltung, kein Logo, kein Foto.

## Aus der Anleitung übernommen
- Alle Regler klingen in der Mitte (12 Uhr) brauchbar.
- MIX: links nur trocken, rechts nur Hall, gleich laut bei 2 Uhr.
- OUTPUT: 12 Uhr = Einheitsverstärkung.
- LOW/HI: Klang des Halls über Klangregelung und Dämpfung.
- THING 1/2 je Modus (Auswahl): Concert Hall (Modulation / frühe Reflexionen), Classic Plate (Vorverzögerung / „bright early decay“), Spring (Ausklang / Pegel des Rasselns), L.A. Studio Room (Vorverzögerung / frühe Reflexionen), Modulated (Tempo / Tiefe), Sparkle (Pegel / Länge).
- Nicht übernommen: Ambient Swell, Delay + Reverb, Reverse, Ghost, Lo-Fi, „Beer“-Modi; Voreinstellungen, Tap, Expression, MIDI.

## Verfahren (engine/src/kaiserin_core.hpp)
- **Netz (HALL, ROOM, MOD, SPARKLE):** 16 Verzögerungen, Längen geometrisch zwischen kürzester und längster (HALL 30–110 ms, ROOM 6–35, MOD 25–95, SPARKLE 40–125), je auf die nächste Primzahl; Rückkopplung über die Hadamard-Matrix (orthogonal, 1/4). Je Leitung Verstärkung aus der Mitten-Nachhallzeit g = 10^(−3·L/(fs·T60)), dazu zwei Kuhschwanzfilter erster Ordnung (bilinear) für Tiefen (Übergang 300 bzw. 400 Hz) und Höhen (3,5/5/5/3 kHz), deren Gleich- bzw. Nyquist-Verstärkung die Nachhallzeit der Tiefen und Höhen ergibt (Jot/Chaigne). Eingang über vier Schroeder-Allpässe (HALL/MOD/SPARKLE: 4,77/3,59/12,73/9,31 ms, g 0,75/0,75/0,625/0,625 wie Dattorro; ROOM 1,9/2,9/4,3/6,1 ms, 0,7), Eingangsverteilung und zwei Ausgänge über verschiedene Zeilen der Hadamard-Matrix (links/rechts unkorreliert). Modulation: Lesezeiger je Leitung sinusförmig (Tempo 0,72 … 1,28 × Grundtempo, verschiedene Phasen), Lagrange-Interpolation 3. Ordnung.
- **Frühe Reflexionen (HALL, ROOM):** 18 Abgriffe je Seite aus der Eingangsleitung hinter der Vorverzögerung, Zeiten verdichtet zum Anfang hin (HALL 8–90 ms, ROOM 2–45 ms), Vorzeichen zufällig, Pegel exponentiell fallend, je Seite auf Energie 1 normiert.
- **PLATE:** Dattorro, „Effect Design Part 1“ (JAES 45/9, 1997), Bild 1 und Tabelle 2: Verzögerungen ×fs/29761, Ausgangsabgriffe wie Tabelle 2. Abklingfaktor aus T60 und der mittleren Zeit zwischen zwei Faktoren (Umlauf/4 = 0,181 s); Dämpfung (Tiefpass im Tank) aus HI (0,9 kHz … offen); THING 2 = Bandbreite am Eingang 2,5–20 kHz; je Hälfte ein Tiefen-Kuhschwanz für LOW. Unter 0,5 s Nachhall nicht erreichbar (Diffusoren).
- **SPRING:** spring.hpp (wie Laney/G100), drei Federn (33, 37, 41 ms), linear; THING 1 = Nachhallzeit des Anteils über 4,2 kHz (0,25–5 s), THING 2 = dessen Pegel (0–60 %); HI setzt den Tiefpass im Umlauf. Links Feder 1 + ½ Feder 2, rechts Feder 3 + ½ Feder 2.
- **SPARKLE:** Oktave nach oben im Rückkopplungsweg: Summe der Leitungen → Hochpass 300 Hz → Tonhöhenverschiebung ×2 (zwei Leseköpfe, Verzögerung nimmt je Abtastwert um 1 ab, Fenster 50 ms, sin/cos-Fenster) → × Pegel (THING 1, 0–0,9) → weich begrenzt (0,5·tanh) → Netzeingang. **Gefundener Fehler:** mit festem Abstand der Köpfe (W/2) löschen sie sich bei f·W/(2·fs) = k + ½ aus (220 Hz → keine 440 Hz, nur ±20-Hz-Seitenbänder); jetzt mit zufälligem Zusatzversatz je Korn (0–0,4 W): bei 220/330/440/1000 Hz liegt die Energie zu 99 % bei 2f. THING 2 verlängert den Ausklang der Höhen (×5^±1).
- **Klangregelung am Hallausgang:** LOW links: Hochpass 2. Ordnung 20 Hz … 600 Hz, rechts: Kuhschwanz bis +6 dB bei 200 Hz; HI links: Tiefpass 20 kHz … 1,25 kHz, rechts: Kuhschwanz bis +6 dB bei 4 kHz. Im Netz zusätzlich LOW: Tiefen-Nachhall ×0,5 … ×2, HI: Höhen-Nachhall 8 % … 28 % (Mitte) … 100 %.
- **Pegel:** je Modus ein Abgleich (CAL), sodass der Hall bei Rauschen und allen Reglern auf 12 Uhr so laut ist wie das trockene Signal; mit DECAY wächst er um (T60/T60(12 Uhr))^¼ (die volle Energie wüchse mit √T60).
- **Moduswechsel** im Audio-Faden: Hall 10 ms aus, nur die Puffer des neuen Modus leeren (bei 192 kHz zusammen etwa 7 MB), 10 ms ein. **HOLD:** Eingang zum Hall in 20 ms stumm, Netz verlustfrei (Verstärkung 1, Filter flach; Feder 0,9995), Oktave aus; das trockene Signal läuft weiter.
- Regler gleiten (40 ms, Koeffizienten alle 16 Abtastwerte), ungültige Eingangswerte zählen als 0, Werte unter 10⁻¹⁵ werden in den Puffern zu 0 (keine Subnormalen im WebAssembly).

## Prüfung (engine/tests/kaitest.cpp, kaiguitest.cpp)
- Nachhallzeit je Oktave (Schroeder, T30) bei DECAY 0/0,5/1 gegen die Kennlinie: Netz-Modi 125–500 Hz innerhalb etwa 5 % (44,1 und 48 kHz); PLATE 2,00/1,89/1,84 s bei Soll 1,73 (vor der Bereichsänderung), oberhalb 0,5 s innerhalb 10 %; SPRING 10–25 % kürzer als eingestellt (Verluste im Tank), angezeigt als „ungefähr“.
- Pegel nach Abgleich je Modus 0,00 ± 0,07 dB. Korrelation L/R bei Rauschen: Netz-Modi und PLATE unter 0,02, ROOM 0,12, SPRING 0,21.
- Stabilität: alle Regler rechts, 2 s Rauschen, 30 s Stille: alle Modi klingen ab (SPARKLE mit Oktave langsamer, −27 dB in 27 s), Spitze höchstens 3,1; HOLD 30 s: Pegel −4 … −6 dB (Interpolation), endlich. 60 s Zufallsstellungen mit Moduswechseln, HOLD und einem NaN am Eingang: keine nicht endliche Ausgabe; ebenso mit AddressSanitizer/UBSan bei 22,05, 48 und 192 kHz.
- Plugin: rtcheck ohne nicht endliche Werte und ohne Block über 100 % (teuerster Block: erster nach dem Start bei 192 kHz, 49 %, Leeren der Puffer); clapstress 30 Runden; Oberflächentest 16 Prüfungen (Ziehen, Rastung, Modustasten und Anzeigen, Mausrad, HOLD, Doppelklick, VALUE POPUP, Ausgang stereo).
- Seite: Download (stereo, 24 Bit, mit Ausklang) gegen den C++-Kern bei 44,1 kHz: Abweichung −94 dB unter dem Signal; deutsche Wertetexte gegen die Texte des Kerns, 2100 Zufallsstellungen, Abweichungen nur bei Rundung genau auf ,5.
- Rechenzeit (ns je Abtastwert, 48 kHz): HALL 240, PLATE 64, SPRING 1000, ROOM 237, MOD 227, SPARKLE 273.

## Offen
- Kein Vergleich mit dem Original: Aufnahmen des Pedals (Impuls/Knacken, Sinus-Burst je Modus) würden zeigen, wie weit Klangfarbe und Ausklang abweichen.
- SPRING-Nachhallzeit nur ungefähr; Aufwand der Federn (etwa 5 % eines Kerns) ließe sich mit weniger Allpass-Stufen senken.
