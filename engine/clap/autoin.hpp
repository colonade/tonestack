// Eingangsbezug der Plugins (ab SVT-CL 1.6 / Laney 3.40 / Yamaha 1.19 / B15NF 1.19 / GK 1.4 / G100 1.7 / B100-III 1.4)
// Bisher bedeutete INPUT „so viele mV am Verstärker entsprechen 0 dBFS“. Liegt die Spur bei −12 dBFS, kam ein Viertel davon an – missverständlich.
// Neu (Grundstellung): INPUT = Spitze deines Spiels in mV. Das Plugin merkt sich die höchste Eingangsspitze (gemerkte Spitze) und
// skaliert so, dass sie am Verstärker den eingestellten Wert hat. Die gemerkte Spitze steigt nur (Spitzenhalter, keine Regelung,
// kein Pumpen); ein Klick in der Oberfläche setzt sie zurück. Sie wird mit dem Projekt gespeichert (Zustand „pk=“), damit ein
// Projekt beim nächsten Öffnen und beim Rendern gleich klingt.
// Beim allerersten lauten Ton (gemerkte Spitze steigt noch) wird die ansteigende Flanke auf den eingestellten Wert begrenzt;
// danach ist die Verstärkung fest. Unter −40 dBFS (FLOOR) wird nicht weiter verstärkt (sonst Rauschen um mehr als 40 dB angehoben).
// Alte Projekte (ohne den Parameter „Input reference“) laden mit dem festen Bezug 0 dBFS und klingen wie bisher.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <algorithm>

namespace tsin {
constexpr double FLOOR = 0.01;   // −40 dBFS
static const char *N_REF[] = {"Peak of your playing (auto)", "0 dBFS (fixed, before this version)"};
struct AutoIn {
  std::atomic<double> pk{0};          // gemerkte Spitze (linear, Vollausschlag = 1; 0 = noch nichts gehört)
  std::atomic<bool> reset{false};     // von der Oberfläche: gemerkte Spitze vergessen (Audio-Faden übernimmt)
  void requestReset() { pk.store(0); reset = true; }   // Oberfläche: sofort sichtbar (Anzeige, Zustand); der Audio-Faden setzt sie beim nächsten Block endgültig zurück
  // m (und m2, falls nicht null): Eingangspuffer; danach so normiert, dass die gemerkte Spitze 1 ist (vin des Kerns = eingestellte mV)
  void apply(float *m, float *m2, uint32_t n) {
    if (reset.exchange(false)) pk.store(0);
    double p = pk.load();
    for (uint32_t i = 0; i < n; i++) { double a = std::fabs((double)m[i]); if (m2) a = std::max(a, std::fabs((double)m2[i]));
      if (std::isfinite(a) && a > p) p = std::min(a, 1e3); double g = 1 / std::max(p, FLOOR); m[i] = (float)(m[i] * g); if (m2) m2[i] = (float)(m2[i] * g); }
    pk.store(p); }
  // nur lernen (Lametta B100-II: dort wirkt der Pegel nur im Endstufenpfad, als vin des Kerns): Spitze des Blocks übernehmen, gemerkte Spitze zurück
  double learn(double blockPeak) { if (reset.exchange(false)) pk.store(0); double p = pk.load(); if (std::isfinite(blockPeak) && blockPeak > p) { p = std::min(blockPeak, 1e3); pk.store(p); } return p; }
};
inline double pkDb(double pk) { return pk > 0 ? 20 * std::log10(pk) : -999; }   // −999: noch nichts gehört
// Zustand: Zeile „999=<Wert>“ hinter den Parametern (Kennung 999 liegt über jeder Parameterzahl: ältere Fassungen übergehen sie)
constexpr int STATE_ID = 999;
inline int saveLine(char *buf, int n, double pk) { return std::snprintf(buf, n, "%d=%.17g\n", STATE_ID, pk); }
inline double clampPk(double v) { return std::isfinite(v) && v > 0 ? std::min(v, 1e3) : 0; }
// Texte für Oberfläche und Host
inline void valueText(char *b, int n, double mv, bool fixed) { (void)fixed; std::snprintf(b, n, "%.0f mV", mv); }   // Bezug steht in der Zeile darunter
inline void extraText(char *b, int n, double pkdb, bool fixed) {
  if (fixed) std::snprintf(b, n, "at 0 dBFS (old) \xc2\xb7 click: auto");
  else if (pkdb < -900) std::snprintf(b, n, "at your peak (not heard yet)");
  else std::snprintf(b, n, "at your peak (%.1f dBFS) \xc2\xb7 reset", pkdb); }
}
