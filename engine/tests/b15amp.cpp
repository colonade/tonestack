// Prüfung der Hülle Amp: Kleinsignal-Frequenzgang (Knotenanalyse) gegen Zeitbereich, Lautsprecher, Überabtastung, Standby, Rechenzeit
#include "b15_core.hpp"
#include <chrono>
#include <cstdio>
using namespace b15;
static double sineGain(Amp &a, double f, double amp) { int fs = (int)a.fsB, n0 = fs / 4, n = (fs / 5) / (int)(fs / f) * (int)(fs / f); std::vector<float> b(n0 + n);
  for (int i = 0; i < n0 + n; i++) b[i] = (float)(amp * std::sin(2 * PI * f * i / fs)); a.process(b.data(), n0 + n);
  double re = 0, im = 0; int lat = a.latency(); for (int i = n0; i < n0 + n; i++) { re += b[i] * std::cos(2 * PI * f * (i - lat) / fs); im += b[i] * std::sin(2 * PI * f * (i - lat) / fs); }
  return 2 * std::hypot(re, im) / n / amp * V_FS / a.vin; }
int main() { Parts p; Setting s; s.vol1 = 0.6; s.bass1 = 0.7; s.treb1 = 0.3;
  for (int os : {1, 2}) { Amp a; a.vin = 0.01; auto t0 = std::chrono::steady_clock::now(); a.init(48000, os, p, s); double ti = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("Überabtastung %dx: Start %.2f s\n", os, ti);
    for (double f : {40.0, 100.0, 300.0, 1000.0, 3000.0, 8000.0, 15000.0}) { double g = sineGain(a, f, 0.5); double ac = std::abs(a.c.response(f));
      printf("  %6.0f Hz: Zeitbereich %7.2f dB, Knotenanalyse %7.2f dB\n", f, 20 * std::log10(g), 20 * std::log10(ac)); }
    std::vector<float> b(48000, 0.f); auto t1 = std::chrono::steady_clock::now(); a.process(b.data(), 48000); double tp = std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count();
    printf("  Rechenlast %.0f %% eines Kerns\n", tp * 100);
  }
  { Amp a; a.vin = 0.01; a.init(48000, 1, p, s); a.spk = true; a.spkMix = 1;
    printf("Mit Lautsprecher (Schalldruck, 1 kHz = Klemme):\n"); for (double f : {40.0, 80.0, 150.0, 1000.0, 3000.0, 6000.0}) printf("  %6.0f Hz: %7.2f dB (Klemme %7.2f dB)\n", f, 20 * std::log10(sineGain(a, f, 0.5)), 20 * std::log10(std::abs(a.c.response(f)))); }
  { Amp a; a.vin = 0.1; a.init(48000, 1, p, s); a.standby = true; std::vector<float> b(48000 * 3); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 200 * i / 48000.0)); a.process(b.data(), (int)b.size());
    double m = 0; for (int i = (int)b.size() - 4800; i < (int)b.size(); i++) m = std::max(m, (double)std::fabs(b[i]));
    printf("Standby nach 3 s: Ausgang %.1f dB unter Vollaussteuerung, B1 %.0f V\n", 20 * std::log10(m + 1e-12), a.c.run.vp[a.c.bl.mon[3]]); }
}
