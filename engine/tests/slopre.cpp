// Abgriff nach der Vorstufe und Lautsprecher (Loldano 1.5): Pegelbezug PRE_GAIN, Lautsprecher-Filter gegen die Datenblattkurve (Vintage 30),
// Frequenzgang Zeitbereich gegen Knotenanalyse (1x, 2x, 4x mit Mehrtakt), Ruhe nach dem Start, Umschalten, Anzeigen, Rechenzeit.
#include "slo_core.hpp"
#include <chrono>
#include <cstdio>
using namespace slo;
static double sineGain(Amp &a, double f, double amp, double secs = 1.0) { int fs = (int)a.fsB, n0 = (int)(secs * fs), n = (fs / 5) / (int)(fs / f) * (int)(fs / f); std::vector<float> b(n0 + n);
  for (int i = 0; i < n0 + n; i++) b[i] = (float)(amp * std::sin(2 * PI * f * i / fs)); a.process(b.data(), n0 + n);
  double re = 0, im = 0; int lat = a.latency(); for (int i = n0; i < n0 + n; i++) { re += b[i] * std::cos(2 * PI * f * (i - lat) / fs); im += b[i] * std::sin(2 * PI * f * (i - lat) / fs); }
  return 2 * std::hypot(re, im) / n / amp * VFS / a.vin; }
// Datenblatt Vintage 30 (8 Ω): Schalldruck, abgelesen (dB, bezogen auf 1 kHz = 101 dB)
static const double V30[][2] = {{20, 77}, {25, 80}, {28, 81.5}, {32, 82}, {40, 84.5}, {45, 85}, {50, 86.5}, {60, 90.5}, {70, 91}, {80, 92}, {90, 93.5}, {100, 94.5}, {150, 97}, {200, 98.5},
  {250, 99}, {300, 98}, {400, 98}, {500, 100}, {700, 100.5}, {800, 100}, {1000, 101}, {1200, 99.2}, {1400, 98.3}, {1600, 101}, {1800, 101}, {2000, 103}, {2300, 105.7}, {2600, 102.5},
  {3000, 105.2}, {3500, 104}, {4000, 102}, {4500, 100}, {5000, 96}, {6000, 87}, {7000, 83}};
int main() { int bad = 0; Parts p; Setting s;
  { Amp a; a.vin = 0.01; a.init(48000, 1, p, s); double gf = std::abs(a.c.response(1000)), gp = std::abs(a.cp.response(1000)) / PRE_GAIN;
    printf("Grundstellung, 1 kHz: Lautsprecherklemme %.3f V/V, Gitter der Phasenumkehr %.3f V/V → PRE_GAIN = %.4f (eingetragen %.4f)\n", gf, gp, gf / gp, PRE_GAIN);
    printf("Speisungen der Vorstufe: B2 %.2f V, Strom der Phasenumkehr %.4f mA\n", a.vB2r, a.iPIr * 1e3); if (std::fabs(20 * std::log10(gf / gp / PRE_GAIN)) > 0.05) bad++;
    Setting n = s; n.od = false; a.c.apply(n); a.cp.apply(n); double gf2 = std::abs(a.c.response(1000)), gp2 = std::abs(a.cp.response(1000));
    printf("NORMAL: Klemme %.3f V/V, Vorstufe angeglichen %.3f V/V (%+.2f dB)\n", gf2, gp2, 20 * std::log10(gp2 / gf2)); if (std::fabs(20 * std::log10(gp2 / gf2)) > 0.3) bad++; }
  { Amp a; a.vin = 0.01; a.init(48000, 1, p, s); double gf = std::abs(a.c.response(1000)); a.cp.acSend = true; a.cp.preGain = SEND_GAIN; double gs = std::abs(a.cp.response(1000));   // FX SEND (ab 1.7)
    printf("FX SEND, Grundstellung 1 kHz: Klemme %.3f V/V, Effektweg angeglichen %.3f V/V (%+.3f dB, SEND_GAIN %.4f)\n", gf, gs, 20 * std::log10(gs / gf), SEND_GAIN); if (std::fabs(20 * std::log10(gs / gf)) > 0.05) bad++; }
  { spkv::Filter F; F.design(spkParams(), 48000); double mx = 0; printf("Lautsprecher gegen Datenblatt (Filter bei 48 kHz − Kurve, dB):");
    for (auto &q : V30) { double d = 20 * std::log10(std::abs(F.response(q[0]))) - (q[1] - 101); mx = std::max(mx, std::fabs(d)); if (q[0] == 40 || q[0] == 100 || q[0] == 1400 || q[0] == 2300 || q[0] == 5000 || q[0] == 7000) printf("  %.0f Hz %+.2f", q[0], d); }
    printf("  größte Abweichung bis 7 kHz %.2f dB (erwartet < 1,6)\n", mx); if (mx > 1.6) bad++; }
  for (int os : {1, 2, 4}) { Amp a; a.vin = 0.0005; a.pre = true; a.init(48000, os, p, s); printf("Abgriff nach der Vorstufe, %dx%s: Zeitbereich gegen Knotenanalyse (dB)\n ", os, a.split ? " (Mehrtakt)" : "");
    for (double f : {100.0, 1000.0, 4000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > (os == 1 && f > 2000 ? 0.4 : 0.2)) bad++; }
    a.spk = true; a.cp.spkAc = true; sineGain(a, 1000, 0.5, 0.1); printf("\n  mit Lautsprecher:"); for (double f : {100.0, 1000.0, 2300.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.3) bad++; }
    printf("\n"); }
  for (int os : {2, 4}) { Amp a; a.vin = 0.2; a.pre = true; a.init(48000, os, p, s); std::vector<float> b(48000); a.process(b.data(), (int)b.size()); double mx = 0; for (float y : b) mx = std::max(mx, (double)std::fabs(y));
    printf("Ruhe nach dem Start (Abgriff nach der Vorstufe, %dx): größter Ausgang %.2g (erwartet < 0,001)\n", os, mx); if (!(mx < 1e-3)) bad++; }
  for (int os : {2, 4}) { Amp a, r; a.vin = r.vin = 0.02; a.pre = true; a.init(48000, os, p, s); r.init(48000, os, p, s); std::vector<float> b(24000), c(24000); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    double q[NPEAK], qr[NPEAK]; c = b; a.process(c.data(), (int)c.size()); a.takePeaks(q); a.pre = false; for (int k = 0; k < 3; k++) { c = b; a.process(c.data(), (int)c.size()); a.takePeaks(q); c = b; r.process(c.data(), (int)c.size()); r.takePeaks(qr); }
    double mx = 0; printf("Aussteuerung nach Start im Abgriff und Umschalten (%dx) gegen ohne Umschalten:", os); for (int i = 0; i < NPEAK; i++) { printf(" %.3f/%.3f", q[i], qr[i]); mx = std::max(mx, std::fabs(q[i] - qr[i])); } printf(" (erwartet gleich auf 0,05)\n"); if (!(mx < 0.05)) bad++; }
  for (int os : {2, 4}) { Amp a; a.vin = 0.1; a.init(48000, os, p, s); std::vector<float> b(4800); double t = 0, mx = 0, jump = 0, last = 0; bool fin = true;
    for (int k = 0; k < 40; k++) { a.pre = (k / 3) % 2; a.spk = (k / 5) % 2; a.send = (k / 4) % 2;   /* FX SEND ab 1.7 */ for (auto &x : b) { x = (float)(0.5 * std::sin(t)); t += 2 * PI * 110 / 48000; } a.process(b.data(), (int)b.size());
      for (float y : b) { fin = fin && std::isfinite(y); mx = std::max(mx, (double)std::fabs(y)); jump = std::max(jump, std::fabs(y - last)); last = y; } }
    printf("Umschalten %dx (Abgriff 13-mal, Lautsprecher 8-mal, FX SEND 10-mal in 4 s): endlich %s, Spitze %.3f, größter Sprung je Abtastwert %.4f\n", os, fin ? "ja" : "NEIN", mx, jump); if (!fin || jump > 0.25) bad++; }
  for (int os : {2, 4}) for (int pre : {0, 1}) { Amp a; a.vin = 0.2; a.pre = pre; a.init(48000, os, p, s); std::vector<float> b(48000 * 2); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    auto t0 = std::chrono::steady_clock::now(); a.process(b.data(), (int)b.size()); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("Rechenzeit %dx %s: %.1f %% eines Kerns (48 kHz)\n", os, pre ? "nur Vorstufe" : "ganzer Verstärker", dt / 2 * 100); }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
