// Klirr im Loldano (ab 1.7): k2 … k5 an Anode V3B, Kathode V4A und Lautsprecherklemme, NORMAL Clean, Preamp Normal 10, Master Normal 3, Klang 5,
// Sinus 400 Hz bei 96 kHz (240 Werte je Periode, Auswertung über ganze Perioden, daher ohne Fenster), 100 und 140 mV Spitze am Eingang.
// Bezugswerte aus der Messung vom 16.11.2026 (Vergleich mit dem LTspice-Modell des Kritikers, siehe PROJEKTSTAND); Grenzen ±0,3 dB (Grundton ±0,1 dB),
// damit spätere Änderungen am Netz oder an den Röhrenmodellen den Klirr nicht unbemerkt verschieben.
// Dazu der Abgriff FX SEND (ab 1.7) über den ganzen Weg des Plugins (Amp, 2x): er muss den Klirr von V3B tragen (k2/k3 wie an V3B auf ±0,5 dB).
#include "slo_core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace slo;
static void harm(const std::vector<double> &y, double f, double fs, double *h) { int n = (int)y.size(); for (int m = 1; m <= 5; m++) { double re = 0, im = 0;
  for (int i = 0; i < n; i++) { re += y[i] * std::cos(2 * PI * m * f * i / fs); im += y[i] * std::sin(2 * PI * m * f * i / fs); } h[m] = 2 * std::hypot(re, im) / n; } }
int main(int argc, char **argv) { const bool show = argc > 1; int bad = 0; const double fs = 96000, f = 400; Setting s; s.od = false; s.ngain = 1.0; s.nmaster = 0.3;
  std::vector<std::string> nd = {"p4", "f2", "s8"}; const char *nm[] = {"Anode V3B", "Kathode V4A", "Lautsprecher"};
  // Bezug: je Pegel und Knoten Grundton (V Spitze), k2, k3 (dB re Grundton)
  struct Ref { double mv, g[3], k2[3], k3[3]; } R[] = {{100, {60.316, 42.212, 5.754}, {-36.23, -48.45, -47.99}, {-52.25, -41.01, -39.85}},
                                                       {140, {82.881, 56.487, 7.717}, {-32.92, -34.80, -35.19}, {-38.47, -30.62, -28.81}}};
  for (auto &r : R) { Core c; c.dbg = &nd; c.init(fs, Parts(), s); c.settle(SETTLE); const int n0 = (int)fs, n = 240 * 48; std::vector<std::vector<double>> y(3, std::vector<double>(n)); double out[O_NO + 32];
    for (int i = 0; i < n0 + n; i++) { c.step(r.mv / 1000 * std::sin(2 * PI * f * i / fs), out); if (i >= n0) for (int k = 0; k < 3; k++) y[k][i - n0] = out[O_NO + k]; }
    for (int k = 0; k < 3; k++) { double h[6]; harm(y[k], f, fs, h); double k2 = 20 * std::log10(h[2] / h[1]), k3 = 20 * std::log10(h[3] / h[1]), k4 = 20 * std::log10(h[4] / h[1]), k5 = 20 * std::log10(h[5] / h[1]);
      bool ok = std::fabs(20 * std::log10(h[1] / r.g[k])) < 0.1 && std::fabs(k2 - r.k2[k]) < 0.3 && std::fabs(k3 - r.k3[k]) < 0.3; if (!ok) bad++;
      printf("%3.0f mV, %-13s Grundton %7.3f V Spitze, k2 %6.2f dB (%.2f %%), k3 %6.2f, k4 %6.2f, k5 %6.2f dB%s\n", r.mv, nm[k], h[1], k2, 100 * h[2] / h[1], k3, k4, k5, ok ? "" : "   ABWEICHUNG");
      if (show) printf("   Bezug: %.3f V, k2 %.1f, k3 %.1f\n", r.g[k], r.k2[k], r.k3[k]); } }
  { Amp a; a.vin = 0.14; a.pre = a.send = true; a.init(48000, 2, Parts(), s); const int n0 = 48000, n = 120 * 48; std::vector<float> b(n0 + n); std::vector<double> y(n);
    for (int i = 0; i < n0 + n; i++) b[i] = (float)std::sin(2 * PI * f * i / 48000.0); a.process(b.data(), n0 + n); for (int i = 0; i < n; i++) y[i] = b[n0 + i];
    double h[6]; harm(y, f, 48000, h); double k2 = 20 * std::log10(h[2] / h[1]), k3 = 20 * std::log10(h[3] / h[1]); bool ok = std::fabs(k2 - R[1].k2[0]) < 0.5 && std::fabs(k3 - R[1].k3[0]) < 0.5;
    printf("Abgriff FX SEND (Amp, 2x), 140 mV: k2 %.2f dB, k3 %.2f dB (erwartet wie Anode V3B)%s\n", k2, k3, ok ? "" : "   ABWEICHUNG"); if (!ok) bad++; }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
