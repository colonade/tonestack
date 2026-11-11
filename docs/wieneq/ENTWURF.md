# Wien EQ – Entwurf

Fünfband-Stereo-Equalizer nach den veröffentlichten Angaben zum adt-audio ToolMod TM205 („Stereo Mastering Equalizer“, Stereofassung des TM105).
Ein Schaltplan ist nicht öffentlich, ein Gerät wurde nicht gemessen. Der Wien EQ ist deshalb ein **eigener Entwurf**, der die Herstellerangaben
erfüllt; er ist kein Nachbau einer Schaltung. „adt-audio“ und „ToolMod“ sind Marken ihres Inhabers.

Code: `engine/src/weq_core.hpp` (Kern), `engine/clap/weqplugin.cpp`, `engine/clap/weqgui.hpp` (Plugin), `engine/wasm/weq_wasm.cpp`,
`web/tonestack/wien-eq/` (Seite), `engine/tools/weq_panel.py` (Frontplatte), `engine/tests/weqtest.cpp` (Prüfstand), `engine/tests/weqguitest.cpp`.

## Herstellerangaben (Beschreibung TM205)

| Angabe | adt-audio |
|---|---|
| Mittenbänder | MID 1 20 Hz–2 kHz, MID 2 60 Hz–7 kHz, MID 3 250 Hz–25 kHz; vollparametrisch, Frequenzregler logarithmisch |
| Prinzip der Mittenbänder | aktive Wien-Robinson-Brücke, alle Parameter unabhängig |
| Bandbreite | für alle Mittenbänder gleich, von etwa 5 Oktaven bis zur Terz |
| Tiefen | weiches RC-Filter, 6 dB/Oktave; 40 Hz, mit zwei Tasten 60, 100, 160 Hz |
| Höhen | weiches RC-Filter; ohne Taste Kuhschwanz, sonst schwach ausgeprägtes Maximum bei 12, 8 oder 4 kHz (zwei Tasten) |
| Regelbereich | ±12 dB für alle Bänder; Ausführungen ±6 und ±20 dB |

## Annahmen

- **Filterform der Mittenbänder**: wie die ideale Wien-Brücke, ein Bandpass 2. Ordnung mit Spitze 1:
  B(s) = (s/Qω) / (1 + s/Qω + (s/ω)²).
- **Anheben** H = 1 + G·B, **Absenken** H = 1 / (1 + G·B) (Bandpass in der Gegenkopplung), G = 10^(|dB|/20) − 1.
  Damit bleibt die Bandbreite beim Drehen am Pegel gleich (Konstant-Q), und gleiches Anheben und Absenken heben sich genau auf.
- **Bandbreite** zwischen den −3-dB-Punkten des Bandpasses: N Oktaven ↔ Q = 1 / (2·sinh(N·ln2/2)).
  5 Oktaven → Q = 0,1825; Terz → Q = 4,3185. Der Q-Regler verläuft logarithmisch zwischen beiden.
- **LOW/HIGH** (seit 1.2, auf Wunsch des Nutzers): Kuhschwanzfilter 1. Ordnung (6 dB/Oktave) in allen Tastenstellungen; für HIGH weicht das
  von der Herstellerangabe („schwach ausgeprägtes Maximum“ bei 12/8/4 kHz) ab. Bis 1.1 waren es Glocken mit Q = 1/3.
  g = 10^(|dB|/20); Tiefen H = (s + ω√g)/(s + ω/√g), Höhen H = (√g·s + ω)/(s/√g + ω); Absenken = Kehrwert. Die genannte Frequenz ist die
  Mitte der Flanke (halbe Anhebung in dB); HIGH 20 kHz mit +12 dB: +6,0 dB bei 20 kHz, +2,7 dB bei 10 kHz, +0,9 dB bei 5 kHz.
  Tastenbelegung: „60“ = 60 Hz, „100“ = 100 Hz, beide = 160 Hz, keine = 40 Hz; „12k“ = 12 kHz, „8k“ = 8 kHz, beide = 4 kHz, keine = 20 kHz.
- **Gain-Regler**: dB = R·sgn(p)·(|p| + p²)/2 mit p = −1 … +1 und R = 6/12/20 dB; um die Mitte gespreizt, Rastung bei 0 dB.
- **EQ ON**: Durchgang ohne Wirkung, gleich verzögert wie mit EQ.

Wer ein Gerät hat, kann Kurven messen (z. B. mit REW); Abweichungen lassen sich dann in den Annahmen nachziehen.

## Digitale Umsetzung

- Je Band ein Abschnitt in Direktform I (knackfest bei gleitenden Koeffizienten); Mittenbänder 2. Ordnung wie folgt, LOW/HIGH 1. Ordnung (siehe unten).
- Pole exakt wie analog (z = e^{sT}). Zähler über seinen Betrag im Quadrat |B|² = B0·φ0 + B1·φ1 + B2·φ2
  (φ0 = cos², φ1 = sin², φ2 = 4φ0φ1 von ω/2; M. Vicanek, „Matched Second Order Digital Filters“, 2016):
  - Weg A: Betrag bei 0 Hz, Mittenfrequenz und halber Abtastrate wie analog (Vicanek);
  - Weg B: B0 bei 0 Hz exakt, B1 und B2 per gewichteter Ausgleichsrechnung über 48 Frequenzen (fc/30 … 22 kHz, relativer Fehler);
  - genommen wird der Weg mit dem kleineren größten Fehler an 16 Prüffrequenzen (20 Hz … 22 kHz).
    A ist bei tiefen, schmalen Bändern besser, B bei breiten (Q 0,18 wirkt bis in die Höhen).
- Absenken = genauer Kehrwert des angehobenen Abschnitts (Zähler und Nenner getauscht).
- LOW/HIGH: |H|² eines Abschnitts 1. Ordnung ist (N0 + N1 φ)/(D0 + D1 φ) mit φ = sin²(ω/2). LOW: Pol und Nullstelle wie analog (z = e^{−ωT}),
  0 Hz exakt; Fehler unter 0,001 dB. HIGH: 0 Hz exakt, N1 und D1 per gewichteter Ausgleichsrechnung über 48 Frequenzen (20 Hz … 22 kHz);
  mit Pol und Nullstelle wie analog wären es bei 20 kHz / 20 dB und 88,2 kHz Rechenrate 0,18 dB Fehler, so höchstens 0,011 dB.
- Unter 80 kHz Abtastrate doppelte interne Rate: Kaiser-Halbbandfilter (127 Koeffizienten, β = 9, bis 20 kHz flach auf ±0,005 dB),
  polyphas hin und zurück; Latenz 63 Abtastwerte der Grundrate. Ab 80 kHz keine Überabtastung, Latenz 0.
- Reglerwerte gleiten über 20 ms (ln f, ln Q, dB), neue Koeffizienten alle 16 interne Abtastwerte, nur für Bänder, die sich ändern.
- EQ ON/aus: 20-ms-Überblendung (Smoothstep) mit latenzgleich verzögertem Direktsignal.

## Prüfung

- `engine/tests/weqtest.cpp` (läuft im Bau): Herstellerangaben (Bereiche, 5 Oktaven / Terz, Pegel an den Anschlägen exakt);
  ein Mittenband digital gegen analog über 120 Frequenzen × 9 Güten × 8 Pegel bei 88,2/96/176,4/192 kHz: höchstens 0,016 dB;
  Kuhschwanz LOW/HIGH (40 Frequenzen × 11 Pegel): höchstens 0,011 dB; voller Pegel am Rand, halber an der genannten Frequenz;
  ganzer Kanal (Impulsantwort, 12 Einstellungen) gegen analog bei 44,1/48/96 kHz: höchstens 0,011 dB; gemeldete Latenz.
- Web-Seite: der heruntergeladene WAV-Export stimmt mit dem nativen Kern auf weniger als 1 LSB (24 Bit) überein (Float-Eingang).
- Knacken beim Schalten und Drehen −77 dB oder leiser; Echtzeit (rtcheck), AddressSanitizer, Oberfläche (weqguitest).
- CPU: etwa 1,7 % eines Kerns bei 48 kHz stereo, 0,3 % bei 96 kHz.
