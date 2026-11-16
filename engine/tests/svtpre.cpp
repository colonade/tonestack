// Abgriff nach der Vorstufe und Lautsprecher (AmpFett SVT-CL 1.12): Pegelbezug PRE_GAIN, Frequenzgang Zeitbereich gegen Knotenanalyse, Lautsprecher-Filter
// gegen die Herstellerangaben, Netzteil nach Plan (Speisung der Vorstufe), Rechenzeit, Umschalten.
#include "svt_core.hpp"
#include <chrono>
#include <cstdio>
using namespace svt;
static double sineGain(Amp &a, double f, double amp) { int fs = (int)a.fsB, n0 = 3 * fs,   /* 3 s Vorlauf: Zeitkonstanten bis 0,7 s (C12 mit MIDDLE, C26) */ n = (fs / 5) / (int)(fs / f) * (int)(fs / f); std::vector<float> b(n0 + n);
  for (int i = 0; i < n0 + n; i++) b[i] = (float)(amp * std::sin(2 * PI * f * i / fs)); a.process(b.data(), n0 + n);
  double re = 0, im = 0; int lat = a.latency(); for (int i = n0; i < n0 + n; i++) { re += b[i] * std::cos(2 * PI * f * (i - lat) / fs); im += b[i] * std::sin(2 * PI * f * (i - lat) / fs); }
  return 2 * std::hypot(re, im) / n / amp * a.vfs() / a.vin; }
int main() { Parts p; Setting s; int bad = 0;
  { Amp a; a.vin = 0.01; a.init(48000, 1, p, s); double gf = std::abs(a.c.response(1000)); Core q = a.cp; q.preGain = 1; double gp = std::abs(q.response(1000));
    printf("Grundstellung, 1 kHz: 4-Ohm-Klemme %.3f V/V, Kathodenfolger V2A %.4f V/V → PRE_GAIN = %.4f (eingetragen %.4f)\n", gf, gp, gf / gp, PRE_GAIN);
    if (std::fabs(20 * std::log10(gf / gp / PRE_GAIN)) > 0.05) bad++; }
  { Setting s2 = s; s2.tap = 1; Amp a; a.vin = 0.01; a.init(48000, 1, p, s2); double gf = std::abs(a.c.response(1000)), gp = std::abs(a.cp.response(1000));
    printf("2-Ohm-Abgriff: Klemme %.3f V/V, Vorstufe angeglichen %.3f V/V (%+.2f dB)\n", gf, gp, 20 * std::log10(gp / gf)); if (std::fabs(20 * std::log10(gp / gf)) > 0.3) bad++; }
  { Setting s2 = s; s2.psu = 1; Amp a; a.vin = 0.01; a.init(48000, 1, p, s2); double gf = std::abs(a.c.response(1000)), gp = std::abs(a.cp.response(1000));
    printf("Netzteil nach Plan: Vorstufen-Speisung %.2f V; Klemme %.3f V/V, Vorstufe angeglichen %.3f V/V (%+.2f dB)\n", a.cp.v.bqpre, gf, gp, 20 * std::log10(gp / gf)); if (std::fabs(20 * std::log10(gp / gf)) > 0.3) bad++; }
  { spkv::Filter F; F.design(spkParams(), 48000); double ref = 0; for (double f : {150.0, 300.0, 600.0, 1000.0}) ref += 20 * std::log10(std::abs(spkv::analog(spkParams(), f)) * F.g) / 4;
    printf("Lautsprecher (analog, je 150 Hz … 1 kHz):"); for (double f : {40.0, 58.0, 100.0, 5000.0}) { double d = 20 * std::log10(std::abs(spkv::analog(spkParams(), f)) * F.g) - ref; printf("  %.0f Hz %+.2f dB", f, d); }
    printf("  (Ziel −10 / −3 / – / −3)\n"); }
  { Amp a; a.vin = 0.01; a.pre = true; a.init(48000, 1, p, s); printf("Abgriff nach der Vorstufe: Zeitbereich gegen Knotenanalyse (dB)\n ");
    for (double f : {40.0, 100.0, 1000.0, 5000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.2) bad++; }
    printf("\n"); a.spk = true; a.spkMix = 1; a.cp.spkAc = true; printf("  mit Lautsprecher-Filter (dB):"); for (double f : {40.0, 100.0, 1000.0, 3000.0, 6000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.cp.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.4) bad++; }
    printf("\n"); }
  { Amp a; a.vin = 0.01; a.spk = true; a.init(48000, 1, p, s); a.c.spkAc = true; printf("Ganzer Verstärker mit Lautsprecher (dB):"); for (double f : {40.0, 100.0, 1000.0, 3000.0}) { double g = sineGain(a, f, 0.5), ac = std::abs(a.c.response(f)); printf("  %.0f Hz: %+.2f / %+.2f", f, 20 * std::log10(g), 20 * std::log10(ac)); if (std::fabs(20 * std::log10(g / ac)) > 0.4) bad++; } printf("\n"); }
  for (int psu : {0, 1}) { Amp a; a.vin = 0.2; a.pre = true; Parts pp = p; Setting ss = s; ss.psu = psu; a.init(48000, 1, pp, ss); std::vector<float> b(48000); a.process(b.data(), (int)b.size()); double mx = 0; for (float y : b) mx = std::max(mx, (double)std::fabs(y));
    printf("Ruhe nach dem Start (Abgriff nach der Vorstufe, Netzteil %d): größter Ausgang %.2g (erwartet < 0,001)\n", psu, mx); if (!(mx < 1e-3)) bad++; }
  { Amp a; a.vin = 0.05; a.pre = true; a.init(48000, 1, p, s); std::vector<float> b(24000); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    a.process(b.data(), (int)b.size()); double q[8]; a.takePeaks(q); a.pre = false; a.process(b.data(), (int)b.size()); a.takePeaks(q); a.process(b.data(), (int)b.size()); a.takePeaks(q);
    double mx = 0; printf("Aussteuerung nach Start im Abgriff und Umschalten auf den ganzen Verstärker (leise):"); for (int i = 0; i < 6; i++) { printf(" %.3f", q[i]); mx = std::max(mx, q[i]); } printf(" (erwartet alle < 0,5)\n"); if (!(mx < 0.5)) bad++; }
  for (int pre : {0, 1}) { Amp a; a.vin = 0.2; a.pre = pre; a.init(48000, 1, p, s); std::vector<float> b(48000 * 2); for (size_t i = 0; i < b.size(); i++) b[i] = (float)(0.5 * std::sin(2 * PI * 110 * i / 48000.0));
    auto t0 = std::chrono::steady_clock::now(); a.process(b.data(), (int)b.size()); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("Rechenzeit %s: %.1f %% eines Kerns (48 kHz, 1x)\n", pre ? "nur Vorstufe" : "ganzer Verstärker", dt / 2 * 100); }
  { Amp a; a.vin = 0.2; a.init(48000, 1, p, s); std::vector<float> b(4800); double t = 0, mx = 0, jump = 0, last = 0; bool fin = true;
    for (int k = 0; k < 40; k++) { a.pre = (k / 3) % 2; a.spk = (k / 5) % 2; for (auto &x : b) { x = (float)(0.5 * std::sin(t)); t += 2 * PI * 110 / 48000; } a.process(b.data(), (int)b.size());
      for (float y : b) { fin = fin && std::isfinite(y); mx = std::max(mx, (double)std::fabs(y)); jump = std::max(jump, std::fabs(y - last)); last = y; } }
    printf("Umschalten (Abgriff 13-mal, Lautsprecher 8-mal in 4 s): endlich %s, Spitze %.3f, größter Sprung je Abtastwert %.4f\n", fin ? "ja" : "NEIN", mx, jump); if (!fin || jump > 0.2) bad++; }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
