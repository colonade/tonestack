# Wisent Echoreck: Schaltplan und Modell (Nachbau des Binson Echorec PE 603 T)

Röhren-Echogerät mit Magnettrommel: ein Aufnahmekopf, vier Wiedergabeköpfe, Löschmagnet. Vorlage ist der PE 603 T (Wahl des Nutzers).

Quellen (nicht im Repository, docs/wisent/):
- „Echorec PE603T“, Nachzeichnung von H.-J. Duchert, Stand 12.10.97, „Alle Angaben ohne Gewähr“: Werte und Spannungen.
- „Wiring diagram of the P.E. 603-T Echorec“: Werksplan, dieselbe Schaltung, schwer lesbar; zum Abgleich der Nachzeichnung.
- Zum Vergleich: Echorec 2 (Plan 4-60/1-61), Guild T6F-A, PE 603, PE 603 TU (1969).

Trommel (Effectrode, „Binson Echorec memory system“, gemessen an einem T7E):
- Trommel 4,7″, 71 U/min, Bandgeschwindigkeit 18 ips = 457 mm/s bei 50 Hz.
- Kopfabstand 34 mm, daraus 74/148/222/296 ms.
- Speicher: etwa 100 Windungen Stahldraht 0,1 mm, außen flach geschliffen.
- Spalt Aufnahme 10 µm, Wiedergabe 5 µm; Aufnahmekopf 400–500 Ω.
- Löschen mit Alnico-Dauermagneten. Spaltpolmotor, Drehzahl folgt der Netzfrequenz.

Bei 60 Hz ergeben sich 62/123/185/247 ms. Diese Werte nennt auch T-Rex (61/122/182/243 ms für die Lage des Original-Aufnahmekopfs).

## Signalweg (Werte aus der Nachzeichnung)
- **Eingang:** Buchse mit 47 k gegen Masse, 47 nF, 1 M → V4A (½ ECC83).
  - Anode 100 k an B+3 (160 V), Kathode an Masse (Anlaufvorspannung über 1 M). Plan: Anode 80 V; Modell 68 V.
- **Hinter V4A** (47 nF):
  - TD 1 M (Trocken-Trimmer) → 330 k → Mischpunkt (47 k gegen Masse) → 10 nF → V4B.
  - 100 k ∥ 820 pF → Knoten 6 → INPUT 0,5 M → 10 nF → V5A. Die 4,7 k an Knoten 6 führen zur Buchse 7 und sind im Modell offen.
  - Plan: 1 mV → 0,06 V; TD auf 0,04 V. Modell: TD 0,67.
- **Mischer:** V5A + V5B (ECC83), gemeinsame Anode, 47 k an B+2 (220 V), Kathoden je 560 Ω. Plan: Anode 130 V, Kathode 0,7 V; Modell 113 V und 0,64 V.
  - Gitter V5B: 10 nF vom Abgriff LENGTH OF SWELL (0,25 M; darunter 47 k gegen Masse), 470 pF, 1 M.
  - Oberes Ende von LENGTH OF SWELL am Abgriff von TF (0,25 M, Trimmer) an F.
- **Aufnahme:** Mischer → 47 nF → R1 → 330 k ∥ 150 pF → Gitter V6A.
  - R1 hat TM 0,5 M gegen Masse, Abgriff zur Anzeige. Vom Gitter V6A geht es über 120 k nach Y; Y hat 560 k ∥ 10 nF gegen Masse.
  - V6A (½ ECC82): Anode 47 k an B+1 (280 V), Kathode 1,5 k ∥ 22 nF. Plan: 130 V, Modell 130,5 V; Kathode Modell 4,8 V.
  - Ausgang über 47 nF und 27 k zum Aufnahmekopf TR. Vormagnetisierung 0,6 mA bei 50 kHz aus V6B (Oszillator).
  - Plan bei 1 kHz: 0,8 V am Mischer, 0,2 V am Gitter, 1 V an der Anode. Modell mit INPUT ganz auf: 1 mV am Eingang ergibt 1,2 V an der Anode.
- **Wiedergabe:** je Kopf ½ ECC83 (V1A, V1B, V2A, V2B).
  - Dass jede Hälfte zu einem Kopf gehört, sagt die „Echorec Bible“ (audioexmachina): Köpfe 1/2 an Röhre 2, 3/4 an Röhre 1. Der Plan zeichnet die Stufe einmal („1-2 AB“).
  - Kopf ∥ 220 pF → 2,2 nF → Gitter (10 M), Kathode an Masse. Anode 47 k an B+3 (Plan 120 V).
  - An der Anode: 20 nF + TSC (10 k, Trimmer „shunt caps“), 330 pF, 10 nF → H.
  - An H: TSW 0,5 M (Trimmer Swell-Pegel), Abgriff → 330 k → Swell-Schiene W. Plan: 0,5 V an H bei 1 kHz.
- **Schaltermatrix:** je Kopf zwei Tasten.
  - Obere Reihe: H → 100 k → Schiene T → 220 k → N (270 k gegen Masse).
  - Untere Reihe: H → 100 k → Schiene B → 220 k → X (270 k gegen Masse).
  - Echo-Gitter G mit 470 k gegen Masse.
- **Betriebstasten** (verriegelt; Swell mit drei Kontaktpaaren, Repeat und Echo mit je einem):
  - SWELL: G–W, F–N (zwei Paare).
  - REPEAT: G–X.
  - ECHO: G–X.
- **Echo-Ausgang:** G → V3A (½ ECC83, Anode 47 k an B+2, Kathode 1,5 k ∥ 22 nF; Plan 1,4 V) → 22 nF → passive Baxandall-Klangregelung → VOLUME 0,5 M → 330 k → Mischpunkt.
  - BASS: 47 k, 1 M mit 2,2 nF/22 nF, 10 k; Abgriff über 100 k.
  - TREBLE: 220 pF, 1 M, 2,2 nF.
- **Ausgangsstufe:** V4B (½ ECC83) als Kathodenfolger: Kathode 1,5 k + 47 k, Gitterableitung 1 M vom Abgriff dazwischen (gebootstrappt), 100 pF; Ausgang 270 nF / 47 k.
  - Der Plan nennt 150 V an der Kathode. Mit 160 V an der Anode ist das als Kathodenfolger nicht möglich; das Modell rechnet etwa 35 V.
- **Anzeige:** R1 → TM → V3B → OA81 → EM84 (Gleichrichtung 470 k / 1 M / 100 nF).

## Deutung der Schaltermatrix
Die Nachzeichnung zeigt die Kontaktpaare offen und ohne Ruhe-/Arbeitskontakt. F ist nur über Swell an N angeschlossen; Repeat und Echo verbinden beide G–X. Wörtlich genommen hätte REPEAT dann keine Rückkopplung.

Die Beschreibungen des Geräts sagen dagegen:
- Soundgas: vier Kopfwahltasten, dazu vier Rückkopplungstasten für Repeat und Swell.
- T-Rex: im Swell-Betrieb spielen alle vier Köpfe; sind die Kopftasten aus, gibt es im Swell keine Wiederholungen.

Das Modell deutet daher:
- **Untere Reihe** = Köpfe zum Ausgang (Schiene B → X).
- **Obere Reihe** = Köpfe in die Rückkopplung (Schiene T → N).
- **ECHO:** G–X, keine Rückkopplung.
- **REPEAT:** G–X und F–N.
- **SWELL:** G–W (alle Köpfe über TSW) und F–N.

LENGTH OF SWELL läuft nicht bis null, weil unter dem Poti 47 k liegen. In REPEAT bleibt daher immer eine zweite Wiederholung bei etwa −15 dB.

## Modell (engine/src/wisent_core.hpp)
- **Elektronik:** Netz im Zeitbereich (dk.hpp), Röhren nach Dempwolf (12AX7) bzw. Koren-Form mit Messwerten (12AU7), Röhrenkapazitäten.
  - Zwei Teilnetze: Aufnahme (V4A, V5A, V5B, V6A; nichtlinear, doppelte Abtastrate) und Wiedergabe (linear: Kopfstufen, Matrix, V3A, Klangregelung, V4B; einfache Rate).
  - Übergabe zwischen den Teilnetzen: Abgriff TD und Gitter V5B.
- **Überabtastung:** IIR-Halbbänder (elliptisch, 8 Koeffizienten). Sperrdämpfung etwa 100 dB, Laufzeit 3 Abtastwerte.
- **Anodenspannungen:** ideal (Netzteil nicht gerechnet, Annahme).
- **Trommel:**
  - Aufnahme: Tiefpass 12 kHz (Selbstlöschung durch die Vormagnetisierung, angenommen), dann tanh-Sättigung. Die Sättigung setzt bei 3 V an der Anode V6A ein (9,5 dB über dem Planpegel, angenommen).
  - Löschrest des Dauermagneten −50 dB von der vorigen Umdrehung (angenommen); Rauschen des Drahts −99 dB relativ zur Sättigung (weiß, vor der Ableitung der Wiedergabe; angenommen).
  - Wiedergabe: d/dt mal Abstandsverlust e^(−2π·d·f/v) mal Spaltverlust (5 µm).
  - d = 31 µm wirksam (Draht 0,1 mm, Abstand, Dicke). Damit fällt die Schleifenverstärkung über 2,5 kHz ab, und Wiederholungen werden dunkler. Mit 8 µm schwang die Schleife bei 4–10 kHz.
  - Gleichlauf: Exzentrizität 0,05 % einmal je Umdrehung (Effectrode: bei gewartetem Gerät unhörbar).
  - Kopfspannung so abgeglichen, dass 1 V an der Anode V6A (1 kHz) 0,5 V an H ergibt (beides Planangaben).
- **Kopf-Induktivitäten** 0,1 H (angenommen); Wiedergabekopf 650 Ω (Effectrode: 600–700 Ω).
- **Trimmer:**
  - TF = 0,35: in REPEAT mit einem Kopf schwingt die Schleife ab LENGTH OF SWELL 0,9 (Abgleich, wisenttest loop).
  - TSC = 5 k (Mitte), TSW = Mitte.
- **Ausgang:** so geeicht, dass trocken = Eingang (1 kHz).

## Prüfung (engine/tests/wisenttest.cpp)
- Ruhespannungen siehe oben.
- Echozeiten bei 50 Hz: 74,9/149,2/223,7/298,2 ms (Soll 74,4/148,8/223,2/297,6 ms + 3 Abtastwerte Laufzeit); bei 60 Hz 62,5/124,5/186,4/248,5 ms.
- Rückkopplung (REPEAT, ein Kopf), Verhältnis 2./1. Wiederholung: −15 dB (LENGTH OF SWELL 0), −4 dB (0,5), 0 dB (0,9), +1 dB (1,0).
- Zwei Rückkopplungsköpfe bei 0,5 bleiben stabil; bei 1,0 wächst der Pegel bis in die Sättigung.
- Schleifenverstärkung über der Frequenz (LENGTH OF SWELL 0,5): höchstens −7,8 dB bei 400–2500 Hz, 10 kHz −25 dB, 100 Hz −16 dB.
- Aliasing (Sinus 2,9 kHz, ohne Gleichlauf und Rauschen, nicht harmonische Anteile gegen Grundton): bis −10 dBFS bei INPUT 0,4 und bis −20 dBFS bei INPUT 0,8 unter −68 dB; nur bei starker Übersteuerung (INPUT 0,8 und −10 bzw. 0 dBFS, Aufnahme weit über der Sättigung) −40 bzw. −30 dB (ohne Überabtastung −21 bzw. −24 dB).
