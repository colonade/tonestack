# Laney AOR 50: Federhall (ab Plugin 3.35, Web 2026-11-04-5)

Quelle: Schaltplan DRG 1054 (AOR 50/12 CIRCUIT, 2/4/85), rechte obere Ecke (Effektweg, IC201–IC203) und Netzteil der OPVs (R219/R220, D201/D202).
Code: `engine/src/laney_rev.hpp` (Namensraum `lrev`), eingebunden in `engine/src/core.hpp` an der Stelle, an der der Vorverstärker die Endstufe speist
(`Amp::paSub` im Röhrenmodus, `LinCore::step` im linearen Modus) – also nur mit „Effektweg + Endstufe“.

## Netz (Werte aus dem Plan)
| Teil | Plan | Im Modell |
|---|---|---|
| Effektweg-Hinweg | W3 (Schleifer PREAMP 1 LEVEL) → R201 1 M → IC203B (R202 330 k, −0,33) → C201 2,2 µ → R203 1 k → SEND/RETURN → C202 2,2 µ → RN | Quelle −0,33·W3 hinter 1 k, C201/C202 in Reihe; RN mit R211 39 k (virtuelle Masse) |
| Summierer | RN → R211 39 k → IC202A (R212 150 k ∥ C209 47 p) → R213 1 k → „B“ (≈ 1,7 V) → C18 der Endstufe | Trockenweg wie bisher im Endstufennetz (Puffer mit Verstärkung 1); Hall mit demselben Maßstab addiert |
| Treiber | RN → R204 33 k / R205 22 k → IC202B (+); (−) über R208 150 k ∥ C204 270 p vom Tankeingang, R206 1 k, R207 39 k ∥ C203 22 n; IC202B → R209 100 Ω, Folger IC203A → R210 100 Ω, beide auf den Tankeingang | so (Gegenkopplung hinter den 100-Ω-Widerständen) |
| Tank | Laney-Teil 5564 („REVERB UNIT“), Typ nicht angegeben | angenommen Eingang B (28 Ω + 24 mH), Ausgang B (200 Ω + 0,357 H), Federmodell `spring.hpp` wie G100 1.2, Übertragung 1 V/A |
| Rückholverstärker | R215 100 k ∥ C205 47 p → IC201 (LF351), R217 220 k ∥ C207 270 p, R216 3k3 + C206 220 n | so |
| REVERB | C208 2,2 µ → 1 k (im Plan „R28“; die Bezeichnung ist im Plan doppelt vergeben) → VR201 22 kA → Schleifer → R214 220 k → IC202A | so; Kennlinie wie Middle/Presence (POT TAPER) |
| Versorgung | „+12 V“/„−12 V“ hinter R219/R220 82 Ω, Z-Dioden D201/D202 15 V | Aussteuerung der OPVs ±10,5 V (geschätzt) |

## Annahmen und Begründung
- **Tanktyp**: Zwei LF353 parallel (je 100 Ω) liefern zusammen einige zehn mA – genug für einen 150-Ω-Eingang (Typ B), nicht für eine 8-Ω-Spule (Typ A).
  Der Rückholverstärker mit 100 kΩ Eingangswiderstand passt zu einem hochohmigen Ausgang. Typ B/B wie beim G100 (dort ebenfalls angenommen).
- **Übertragung 1 V/A**: Typ-4-Tank mit Ausgang B gibt typisch 6,5 mV bei Nennantrieb des B-Eingangs (6,5 mA eff.; sound-au.com, „Reverb Tanks“, Tabellen 1 und 3).
  Mit den 16 V/A des G100 begrenzte IC201 (×68) schon beim Nennpegel des Effektwegs (≈ 500 mV); mit 1 V/A bleibt er darunter
  (Prüfprogramm: 1 kHz, W3 1,3 V → Spulenstrom 22 mA, IC201 0,99 V; bei 4 V 67 mA, 1,2 V).
- **Federmodell**: dasselbe wie G100 1.2, nichtlinear mit Schalter REVERB TANK (Plugin: Taste in der Zeile TUBES; Web: Kippschalter), linear rechnet es wie G100 1.1.

## Prüfung
- REVERB 0 (Grundstellung): der Hallblock rechnet nicht; Ausgabe Bit für Bit gleich 3.34 in allen Betriebsarten (linear, Röhre 1x/2x/4x, eco, ohne Endstufe; Prüfprogramm gegen den Kern aus Git).
- Hallanteil bei REVERB 10, Rauschen 300 Hz–3 kHz am Eingang des Hallblocks: −14,5 dB gegen den Trockenweg, unabhängig von der Rechenrate (48/96/192 kHz: −14,5 / −14,5 / −14,3 dB).
- Plugin (rtcheck, guitest): keine NaN; REVERB-Knopf und Tank-Taste bedienbar; Rechenzeit mit Hall bei 2x etwa ein Drittel mehr.
- Web (Playwright): REVERB-Knopf aktiv, Hallfahne in den Pausen (−20 dB statt −60 dB), Tank-Schalter wirkt, keine Konsolenfehler.

## Offen
- Tanktyp und Kennwerte geschätzt; kein Tank gemessen.
- G100: dort ist die Tankübertragung 16 V/A (auf „Hall bei REVERB 10 etwa so laut wie direkt“ gesetzt, nicht gemessen); nach denselben Daten wären es eher 1 V/A.
