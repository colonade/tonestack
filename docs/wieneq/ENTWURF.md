# Wien EQ – Entwurf

Fünfband-Stereo-Equalizer nach den veröffentlichten Angaben zum adt-audio ToolMod TM205 („Stereo Mastering Equalizer“, Stereofassung des TM105).
Ein Schaltplan ist nicht öffentlich, ein Gerät wurde nicht gemessen. Der Wien EQ ist deshalb ein **eigener Entwurf**, der die Herstellerangaben
erfüllt; er ist kein Nachbau einer Schaltung. „adt-audio“ und „ToolMod“ sind Marken ihres Inhabers.

Code: `engine/src/weq_core.hpp` (Kern), `engine/src/weq_laws.hpp` (Reglerkennlinien, auch für die Oberfläche), `engine/clap/weqplugin.cpp`, `engine/clap/weqgui.hpp` (Plugin), `engine/wasm/weq_wasm.cpp`,
`web/tonestack/wien-eq/` (Seite), `engine/tools/weq_panel.py` (Frontplatte), `engine/tests/weqtest.cpp` (Prüfstand), `engine/tests/weqguitest.cpp`.

## Herstellerangaben (Beschreibung TM205)

| Angabe | adt-audio |
|---|---|
| Mittenbänder | MID 1 20 Hz–2 kHz, MID 2 60 Hz–7 kHz, MID 3 250 Hz–25 kHz; vollparametrisch, Frequenzregler „logarithmisch abgestuft“; bis auf den Bereich gleich |
| Prinzip der Mittenbänder | aktive Wien-Robinson-Brücke, alle Parameter unabhängig |
| Bandbreite | für alle Mittenbänder gleich, von etwa 5 Oktaven bis zur Terz (technische Daten und Bedienelemente: „> 3 Oktaven bis Terz“) |
| Tiefen | aktives RC-Filter, weiche Glocke, 6 dB/Oktave oder weniger, verhindert Anheben im Subsonic-Bereich; 40 Hz, mit zwei Tasten 60, 100, 160 Hz; die Taste ändert auch die Güte |
| Höhen | aktives RC-Filter; Kuhschwanz bei Stellung 20 kHz (keine Taste, „Auslauf weit oberhalb des Hörbereichs“), sonst sehr weiche Glocke bei 12, 8 oder 4 kHz (zwei Tasten), höchstens 6 dB/Oktave; die Bandbreite ändert sich mit |
| Regelbereich | ±12 dB für alle Bänder; Ausführungen ±6 und ±20 dB |
| Frontplatte (Foto, ±12-dB-Ausführung) | sieben Skalenstriche in gleichen Winkeln je Knopf; GAIN 0 / 4 / 9 / 12 dB; MID 1 20/30/50/120/240/600/2000 Hz; MID 2 .06/.1/.15/.4/.7/2/7 kHz; MID 3 .25/.4/.7/1/2/10/25 kHz; LOW-Tasten 60 Hz links, 100 Hz rechts; HIGH-Tasten 8k links, 12k rechts (die Bedienungsseite nennt Taste 14 = 12 kHz, 15 = 8 kHz, also umgekehrt) |

Quellen: Beschreibung, Bedienelemente und technische Daten auf adt-audio.de / adt-audio.com, Frontplattenfotos dort (vom Nutzer am 2026-11-15 auch als Bildschirmfotos geschickt).
Die technischen Daten nennen für MID 2 denselben Bereich wie MID 1 und für MID 3 60 Hz–7 kHz; Frontplatte, Beschreibung und Bedienelemente stimmen dagegen überein (oben), dort also ein Abschreibfehler.

## Annahmen

- **Filterform der Mittenbänder**: wie die ideale Wien-Brücke, ein Bandpass 2. Ordnung mit Spitze 1:
  B(s) = (s/Qω) / (1 + s/Qω + (s/ω)²).
- **Anheben** H = 1 + G·B, **Absenken** H = 1 / (1 + G·B) (Bandpass in der Gegenkopplung), G = 10^(|dB|/20) − 1.
  Damit bleibt die Bandbreite beim Drehen am Pegel gleich (Konstant-Q), und gleiches Anheben und Absenken heben sich genau auf.
- **Bandbreite** zwischen den −3-dB-Punkten des Bandpasses: N Oktaven ↔ Q = 1 / (2·sinh(N·ln2/2)).
  5 Oktaven → Q = 0,1825; Terz → Q = 4,3185. Der Q-Regler verläuft logarithmisch zwischen beiden.
- **LOW/HIGH** (ab 1.4 wie beim Original; 1.2/1.3 auf damaligen Wunsch überall Kuhschwanz, bis 1.1 überall Glocke):
  LOW immer Glocke, HIGH mit Taste Glocke: Bandpass 2. Ordnung wie bei den Mittenbändern mit Q = 1/3 (passives RC-Glied mit gleichen
  Bauteilen, Flanken 6 dB/Oktave). ADT nennt keine Güte; dass die Taste auch die Bandbreite ändert, ist nicht beziffert und nicht nachgebildet.
  HIGH ohne Taste: Kuhschwanz 1. Ordnung, g = 10^(|dB|/20), H = (√g·s + ω)/(s/√g + ω), Absenken = Kehrwert; 20 kHz ist die Mitte der Flanke
  (halbe Anhebung in dB): +12 dB → +6,0 dB bei 20 kHz, +2,7 dB bei 10 kHz, +0,9 dB bei 5 kHz.
  Tastenbelegung: „60“ = 60 Hz, „100“ = 100 Hz, beide = 160 Hz, keine = 40 Hz; „8k“ = 8 kHz, „12k“ = 12 kHz, beide = 4 kHz, keine = 20 kHz.
- **Gain-Regler** (ab 1.4 nach der Skala): monoton kubisch (Fritsch–Carlson) durch 0 / ⅓ / ¾ / 1 des Bereichs an den Strichen 0 / 1 / 2 / 3
  (±12 dB: 0 / 4 / 9 / 12 dB), ungerade fortgesetzt, für ±6/±20 dB im selben Verhältnis; Rastung bei 0 dB. Bis 1.3: dB = R·(|p| + p²)/2.
- **Frequenzregler** (ab 1.4 nach der Skala): Anteil u = ln(f/f_unten)/ln(f_oben/f_unten) an den sieben Strichen = Mittel aus MID 1 und
  MID 2 (0 / 0,0977 / 0,1957 / 0,3938 / 0,5279 / 0,7376 / 1), dazwischen monoton kubisch; beide Skalen liegen damit auf höchstens
  0,08 Oktaven. In der Mitte MID 1 123 Hz, MID 2 391 Hz, MID 3 1,53 kHz (bis 1.3 logarithmisch: 200 / 648 / 2500 Hz). Die MID-3-Skala
  der Frontplatte (Mitte 1 kHz, 2 → 10 kHz in einem Schritt) passt nicht zu den anderen beiden, obwohl ADT die drei Bänder bis auf den Bereich
  gleich nennt; sie weicht bis 0,6 Oktaven von diesem Verlauf ab und ist deshalb nicht übernommen.
- **Q-Regler**: logarithmisch zwischen 5 Oktaven und Terz (die Frontplatte hat dort keine Zahlen).
- **EQ ON**: Durchgang ohne Wirkung, gleich verzögert wie mit EQ.

Wer ein Gerät hat, kann Kurven messen (z. B. mit REW); Abweichungen lassen sich dann in den Annahmen nachziehen.

## Digitale Umsetzung

- Je Band ein Abschnitt in Direktform I (knackfest bei gleitenden Koeffizienten); Glocken 2. Ordnung wie folgt, HIGH-Kuhschwanz 1. Ordnung (siehe unten).
- HIGH hat zwei Abschnitte, die beide laufen: eine Taste stellt den ruhenden auf die neue Stellung und blendet 20 ms über (Kuhschwanz ↔ Glocke
  und Glocke ↔ Glocke). Gleitende Frequenz zwischen den Glocken knackte bei +20 dB mit −63 dB, so höchstens −74 dB (wie 1.3). LOW gleitet.
- Pole exakt wie analog (z = e^{sT}). Zähler über seinen Betrag im Quadrat |B|² = B0·φ0 + B1·φ1 + B2·φ2
  (φ0 = cos², φ1 = sin², φ2 = 4φ0φ1 von ω/2; M. Vicanek, „Matched Second Order Digital Filters“, 2016):
  - Weg A: Betrag bei 0 Hz, Mittenfrequenz und halber Abtastrate wie analog (Vicanek);
  - Weg B: B0 bei 0 Hz exakt, B1 und B2 per gewichteter Ausgleichsrechnung über 48 Frequenzen (fc/30 … 22 kHz, relativer Fehler);
  - genommen wird der Weg mit dem kleineren größten Fehler an 16 Prüffrequenzen (20 Hz … 22 kHz).
    A ist bei tiefen, schmalen Bändern besser, B bei breiten (Q 0,18 wirkt bis in die Höhen).
- Absenken = genauer Kehrwert des angehobenen Abschnitts (Zähler und Nenner getauscht).
- HIGH-Kuhschwanz: |H|² eines Abschnitts 1. Ordnung ist (N0 + N1 φ)/(D0 + D1 φ) mit φ = sin²(ω/2) (der Weg für einen LOW-Kuhschwanz bleibt im Kern,
  ungenutzt). 0 Hz exakt, N1 und D1 per gewichteter Ausgleichsrechnung über 48 Frequenzen (20 Hz … 22 kHz);
  mit Pol und Nullstelle wie analog wären es bei 20 kHz / 20 dB und 88,2 kHz Rechenrate 0,18 dB Fehler, so höchstens 0,011 dB.
- Unter 80 kHz Abtastrate doppelte interne Rate: Kaiser-Halbbandfilter (127 Koeffizienten, β = 9, bis 20 kHz flach auf ±0,005 dB),
  polyphas hin und zurück; Latenz 63 Abtastwerte der Grundrate. Ab 80 kHz keine Überabtastung, Latenz 0.
- Reglerwerte gleiten über 20 ms (ln f, ln Q, dB), neue Koeffizienten alle 16 interne Abtastwerte, nur für Bänder, die sich ändern.
- EQ ON/aus: 20-ms-Überblendung (Smoothstep) mit latenzgleich verzögertem Direktsignal.

## Prüfung

- `engine/tests/weqtest.cpp` (läuft im Bau): Herstellerangaben (Bereiche, 5 Oktaven / Terz, Pegel an den Anschlägen exakt); ab 1.4 Skalen
  (Frequenz MID 1/2 an den Strichen ≤ 0,1 Oktave, gemessen 0,08; Pegel 4/9/12 dB exakt, beide streng steigend), Glocken LOW/HIGH Q 1/3 digital
  gegen analog, Knacken beim Umschalten der Tasten bei ±20 dB (Grenze −70 dB, gemessen −73,9 dB);
  ein Mittenband digital gegen analog über 120 Frequenzen × 9 Güten × 8 Pegel bei 88,2/96/176,4/192 kHz: höchstens 0,016 dB;
  Kuhschwanz LOW/HIGH (40 Frequenzen × 11 Pegel): höchstens 0,011 dB; voller Pegel am Rand, halber an der genannten Frequenz;
  ganzer Kanal (Impulsantwort, 12 Einstellungen) gegen analog bei 44,1/48/96 kHz: höchstens 0,011 dB; gemeldete Latenz.
- Web-Seite: der heruntergeladene WAV-Export stimmt mit dem nativen Kern auf weniger als 1 LSB (24 Bit) überein (Float-Eingang).
- Knacken beim Drehen −77 dB oder leiser, beim Umschalten der Tasten −74 dB oder leiser; Echtzeit (rtcheck), AddressSanitizer, Oberfläche (weqguitest).
- CPU: etwa 1,7 % eines Kerns bei 48 kHz stereo, 0,3 % bei 96 kHz.
