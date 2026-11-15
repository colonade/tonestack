# Blech 140: Entwurf (Plugin 1.0)

Hallplatte nach dem EMT 140 (Elektromesstechnik, ab 1957) als physikalisches Modell. Eigener Entwurf aus veröffentlichten Verfahren und Angaben zum Gerät; keine Messung an einem Gerät, keine Impulsantwort eines Geräts.

## Vom Gerät übernommen
- Stahlplatte etwa 2 m × 1 m × 0,5 mm, in einem Rahmen gespannt; ein Treiber, zwei Abnehmer (stereo).
- Nachhallzeit mit der Dämpfungsplatte einstellbar, etwa 1 bis 4 s bei 500 Hz (Regler DECAY, hier 1 bis 3,8 s).
- Tiefenabsenkung in Stufen 0 bis 3 (BASS CUT).
- Nicht am Gerät, nur im Modell: MIX, PRE-DELAY, OUTPUT (am Gerät mischt das Pult).
- Nicht übernommen: Verstärker von Treiber und Abnehmern (Rauschen, Brummen, Übersteuern), Streuung echter Platten (Spannung, Lagerung, Temperatur), das langsame Fahren der Dämpfungsplatte selbst.

## Modell (engine/src/blech_core.hpp)
- **Platte:** dünne Platte nach Kirchhoff mit Spannung T0, Ränder gelenkig gelagert: ρH·ü = T0·∇²u − D·∇⁴u − 2σ(ω)·ρH·u̇ + f, D = E·H³/(12(1 − ν²)); Stahl E = 200 GPa, ν = 0,3, ρ = 7850 kg/m³. Moden Φ = sin(m₁πx/Lx)·sin(m₂πy/Ly), ω² = (T0/ρH)·k² + (D/ρH)·k⁴. Alle Moden bis 18 kHz: 23 362. T0 = 700 N/m ist angenommen und wirkt nur unter etwa 50 Hz.
- **Anregung und Abnahme:** Treiber als Punkt bei (0,52; 0,53) der Kantenlängen (wie in der Literatur zu Plattenmodellen); Abnehmer angenommen bei (0,30; 0,62) und (0,74; 0,38), verschieden weit vom Treiber. Beitrag jeder Mode = Φ(Treiber)·Φ(Abnehmer); im Mittel eben.
- **Dämpfung je Mode** (σ auf die Energie bezogen: T60 = 6·ln10/σ, die Amplitude klingt mit σ/2 ab):
  - thermoelastisch: σ_te(ω) = ω²·R1·C1 / (2(ω²H² + C1²/H²)), R1 = 9,664·10⁻³, C1 = 0,1855·10⁻³ (Arcas/Chaigne; wie bei Russo 2023);
  - Dämpfungsplatte: s·√(f/500 Hz), s so, dass T60(500 Hz) dem Regler DECAY folgt (1–3,8 s; darüber reicht die offene Platte im Modell nicht);
  - Hochtonverlust: 11,8·(f/10 kHz)² (angenommen: T60 bei 10 kHz etwa 0,9 s bei offener Dämpfung).
- **Impulsantwort (8 s, ab 7 s ausgeblendet):**
  - 0–120 ms exakt im Zeitbereich (Rekursion des gedämpften Kosinus je Mode);
  - danach über das Spektrum: je Mode die Lorentz-Kurve des gedämpften Kosinus mit weichem Einsatz (1 − e^(−t/15 ms))². Exakt als Summe dreier Exponentialfolgen, je DFT (1 − aᴺ)/(1 − a·w). Breite je Mode: 30 Halbwertsbreiten der Mode bzw. 6 des Einsatzes. Eine inverse FFT mit 2¹⁹ Punkten bei 48 kHz;
  - Überblendung beider Teile bei 100–120 ms.

  Mit hartem Einsatz klang ein Fehler mit 1/t nach (−35 dB bei 1 s); mit dem weichen Einsatz fallen die Ausläufer mit 1/f³.
- **Pegel:** Hall bei Rauschen so laut wie trocken bei DECAY 2 s, mit der Nachhallzeit wachsend wie (T/2 s)^¼ (wie Kaiserin).
- **Faltung ohne Latenz:**
  - 256 Abgriffe direkt;
  - Ebene A: Blöcke 256 (FFT 512, 31 Partitionen);
  - Ebene B: Blöcke 4096 (FFT 8192, Partitionen ab Abgriff 8192), die Multiplikationen über 16 Teilblöcke verteilt.

  L und R stecken als L + iR in einem komplexen Spektrum. Das Eingangsspektrum der Ebene B wird an der Blockgrenze gerechnet. Bis zum Entwurfsstand geschah es einen Teilblock zu spät, das ergab −15 dB Abweichung.
- **Neue Platte (DECAY gedreht):**
  - Die Moden rechnet ein Hilfsfaden (Plugin) bzw. ein eigener Worker (Seite, blech-ir.js), etwa 0,7 s, nachdem DECAY 120 ms ruhig stand.
  - Die Übergabe geht über ein Postfach mit getauschten Puffern gleicher Größe; im Audio-Faden gibt es nach dem Einrichten kein malloc/free.
  - Der Audio-Faden zerlegt die Impulsantwort stückweise.
  - Der neue Satz rechnet sofort mit; überblendet wird (100 ms) erst, wenn seine Ebene B vollständig ist (zwei 4096er-Grenzen).
  - Beide Faltungen teilen sich die Eingangsspektren.
  - Lampe MOTOR leuchtet, solange gerechnet, zerlegt oder überblendet wird.
  - Der Zeiger des Instruments zeigt die eingebaute Platte und wandert beim Überblenden.
- **Hohe Abtastraten:** Über etwa 88 kHz rechnet der Hallweg bei fs/2 bzw. fs/4 (44–88 kHz), weil die Platte nur Moden bis 18 kHz hat.
  - Die Wandlung geschieht mit einem linearphasigen FIR (Kaiser-Fenster, 90 dB, Grenzfrequenz fsi/2, Übergang 18 kHz … fsi − 18 kHz).
  - Laufzeit nur im Hallweg: (N−1)/fs, bei 96 und 192 kHz 0,49 ms.
  - Ohne die Wandlung lag die Rechenzeit bei 192 kHz im Mittel bei 61 % mit Blöcken über 100 %; mit ihr bei 3,7 %.
- **Tiefenabsenkung:** Hochpass erster Ordnung am Hallausgang, 20 (aus), 100, 200, 400 Hz (Frequenzen angenommen).

## Prüfung (engine/tests/blechtest.cpp, blechguitest.cpp, blechwasm.js)
- **Nachhallzeit je Terz** (Schroeder, T30, −5 bis −35 dB) gegen die aus dem Modell erwartete Auswertung (Energie je Mode × Terzfilter):
  - 250 Hz–8 kHz auf 1–2 % genau;
  - 125 Hz bei DECAY 3,8 s: 10,57 statt 11,19 s (das Ausblenden ab 7 s schneidet mit);
  - T60 bei 500 Hz: 1,02 / 2,01 / 3,79 s für DECAY 1 / 2 / 3,8 s.
- **Faltung:** gegen direkte Rechnung −129 dB (Blockgröße 97), erste Ausgabe ohne Verzögerung.
- **Impulsantwort** gegen die exakte Summe aller Moden im Zeitbereich: −146 dB (0–20 ms), −58 bis −65 dB (0,1–4 s).
- **Plattenwechsel** (2 → 3,8 s) mit fertig angebotener Impulsantwort: eingebaut nach 0,29 s, kein Pegeleinbruch, längster Block 52 % der Blockdauer.
- **96 und 192 kHz:** Hallpegel ±0,00 dB gegen 48 kHz, T30 wie bei 48 kHz, Anteile über 26 kHz −83 bzw. −80 dB.
- **Rechenzeit bei 48 kHz:** 3 % eines Kerns; Einrichten 0,75 s; neue Platte 0,63 s.
- **Plugin:**
  - rtcheck: 44,1–192 kHz, Blöcke 128, kein Block über 100 % CPU außer dem ersten nach dem Einschalten bei 192 kHz;
  - deactivate 0,2 ms (der Hilfsfaden bricht ab);
  - clapstress 30 Runden;
  - ThreadSanitizer ohne Befund;
  - Oberflächentest 16 Prüfungen (Lampe an und wieder aus, Wahlschalter, Knöpfe).
- **WebAssembly gegen C++:** −135 dB; Platte aus zweiter Instanz übergeben, eingebaut nach 0,3 s.
- **Seite:** Download gegen C++ mit gleichem Eingang auf 1 LSB (24 Bit) gleich. Chrome dekodiert 16-Bit-WAV nicht genau als x/32768 (bis 3·10⁻⁵ relativ), deshalb wird mit dem vom Browser dekodierten Eingang verglichen.

## Vergleich mit Aufnahmen
Fünf öffentliche Videos (zwei EMT-140-Demos, eine EMT-140-Aufnahme mit Sinustönen, zwei Empress-Demos) wurden nur zum Hören und Messen benutzt, nicht ins Repository übernommen.
- Nachhallzeiten je Band lassen sich aus den Musikaufnahmen nicht verlässlich messen: Die Pausen sind zu kurz, das trockene Signal ist beigemischt und das Gerät ist unbekannt eingestellt.
- Die Aufnahme mit Sinustönen ergab bei 1–1,6 kHz etwa 1,4–2 s. Das Modell gibt bei DECAY 2 s 1,6 s (1 kHz) und 1,4 s (1,6 kHz).
- Das Modell blieb unverändert.

## Quellen
- EMT 140: Bedienungsanleitung bzw. Datenblatt (Nachhallzeit, Dämpfungsplatte, Tiefenabsenkung, zwei Abnehmer).
- Arcas, Chaigne: thermoelastische Dämpfung dünner Platten (Konstanten R1, C1).
- Russo, Aalborg Universitet 2023: Modell der Hallplatte EMT 140 (Gleichungen 3.12–3.13, Lage des Treibers).
- Bilbao, Ducceschi/Webb: Modelle von Hallplatten (gespannte Kirchhoff-Platte, modale Rechnung).
