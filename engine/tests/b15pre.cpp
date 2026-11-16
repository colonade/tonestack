// Abgriff nach der Vorstufe (AmpFett 1.27): Pegelbezug PRE_GAIN, Frequenzgang Zeitbereich gegen Knotenanalyse, Rechenzeit, Umschalten, Lautsprecher-Filter
#define B15_V127   // AmpFett ab 1.27 (Abgriff nach der Vorstufe) in b15_core.hpp; andere Plugins sehen den Stand 1.26 (bytegleich)
#include "b15_core.hpp"
#include <chrono>
#include <cstdio>
using namespace b15;
static double sineGain(Amp &a, double f, double amp) { int fs = (int)a.fsB, n0 = fs / 4, n = (fs / 5) / (int)(fs / f) * (int)(fs / f); std::vector<float> b(n0 + n);
  for (int i = 0; i < n0 + n; i++) b[i] = (float)(amp * std::sin(2 * PI * f * i / fs)); a.process(b.data(), n0 + n);
  double re = 0, im = 0; int lat = a.latency(); for (int i = n0; i < n0 + n; i++) { re += b[i] * std::cos(2 * PI * f * (i - lat) / fs); im += b[i] * std::sin(2 * PI * f * (i - lat) / fs); }
  return 2 * std::hypot(re, im) / n / amp * V_FS / a.vin; }
int main() { Parts p; Setting s; int bad = 0;
  { Amp a; a.vin = 0.01; a.init(48000, 1, p, s); double gf = std::abs(a.c.response(1000)), gp = std::abs(a.cp.response(1000)) / PRE_GAIN;
    printf("Grundstellung, 1 kHz: Lautsprecherklemme %.3f V/V, Gitter V3A %.3f V/V → PRE_GAIN = %.4f (eingetragen %.4f)\n", gf, gp, gf / gp, PRE_GAIN);
    printf("Speisungen für die Vorstufe: B3 %.1f V, B4 %.1f V, Vorspannung %.1f V\n", a.cp.v.b3pre, a.cp.v.b4pre, a.cp.v.biaspre);
    if (std::fabs(20 * std::log10(gf / gp / PRE_GAIN)) > 0.05) bad++; }
  { Amp a; a.vin = 0.01; a.pre = true; a.init(48000, 1, p, s); printf("Abgriff nach der Vorstufe: Zeitbereich gegen Knotenanalyse (dB)\n ");
    for (double f : {40.0, 100.0, 1000.0, 5000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f));   /* bis 5 kHz: darüber weicht die Trapezregel bei 1x ab (wie im ganzen Verstärker) */ printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.2) bad++; }
    printf("\n"); a.spk = true; a.spkMix = 1; a.cp.spkAc = true; printf("  mit Lautsprecher-Filter (dB):"); for (double f : {40.0, 100.0, 1000.0, 3000.0, 6000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.3) bad++; }
    printf("\n"); }
  for (int psu : {0, 1}) { Amp a; a.vin = 0.2; a.pre = true; Parts pp = p; Setting ss = s; pp.psu = psu; a.init(48000, 1, pp, ss); std::vector<float> b(48000); a.process(b.data(), (int)b.size()); double mx = 0; for (float y : b) mx = std::max(mx, (double)std::fabs(y));
    printf("Ruhe nach dem Start (Abgriff nach der Vorstufe, Netzteil %d): größter Ausgang %.2g (erwartet < 0,001)\n", psu, mx); if (!(mx < 1e-3)) bad++; }
  { Amp a; a.vin = 0.05; a.pre = true; a.init(48000, 1, p, s); std::vector<float> b(24000); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    a.process(b.data(), (int)b.size()); double q[8]; a.takePeaks(q); a.pre = false; a.process(b.data(), (int)b.size()); a.takePeaks(q); a.process(b.data(), (int)b.size()); a.takePeaks(q);
    double mx = 0; printf("Aussteuerung nach Start im Abgriff und Umschalten auf den ganzen Verstärker (leise):"); for (int i = 0; i < 5; i++) { printf(" %.3f", q[i]); mx = std::max(mx, q[i]); } printf(" (erwartet alle < 0,5)\n"); if (!(mx < 0.5)) bad++; }
  for (int pre : {0, 1}) { Amp a; a.vin = 0.2; a.pre = pre; a.init(48000, 1, p, s); std::vector<float> b(48000 * 2); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    auto t0 = std::chrono::steady_clock::now(); a.process(b.data(), (int)b.size()); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("Rechenzeit %s: %.1f %% eines Kerns (48 kHz, 1x)\n", pre ? "nur Vorstufe" : "ganzer Verstärker", dt / 2 * 100); }
  { Amp a; a.vin = 0.2; a.init(48000, 1, p, s); std::vector<float> b(4800); double t = 0, mx = 0, jump = 0, last = 0; bool fin = true;
    for (int k = 0; k < 40; k++) { a.pre = (k / 3) % 2; for (auto &x : b) { x = (float)(0.5 * std::sin(t)); t += 2 * PI * 110 / 48000; } a.process(b.data(), (int)b.size());
      for (float y : b) { fin = fin && std::isfinite(y); mx = std::max(mx, (double)std::fabs(y)); jump = std::max(jump, std::fabs(y - last)); last = y; } }
    printf("Umschalten 13-mal in 4 s: endlich %s, Spitze %.3f, größter Sprung je Abtastwert %.4f\n", fin ? "ja" : "NEIN", mx, jump); if (!fin || jump > 0.2) bad++; }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
