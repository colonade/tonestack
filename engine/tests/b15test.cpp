// Prüfstand B-15NF-Kern: Arbeitspunkte gegen Technical Data, Rechenzeit, Frequenzgang, Ausgangsleistung
#include "b15_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace b15;
static double v_mains(Core &c) { return c.v.mains_hz * 2; }
static void dc(Core &c) {
  std::vector<double> u = {0, std::sqrt(2.0) * c.v.hv_rms * std::sin(c.t), 0, 0}; u[2] = -u[1]; u[3] = u[1];
  auto v = dk::nodeVoltages(c.net, c.fs, c.run.h, u, c.run.ip);
  auto V = [&](const char *n) { return v[c.net.idx.at(n)]; };
  printf("Knoten        Rechnung  Technical Data\n");
  struct R { const char *n, *a, *b; double ref; } rows[] = {
    {"V1A Anode", "p1a", "0", 165}, {"V1A Kathode", "k1a", "0", 2.2}, {"V1B Anode", "p1b", "0", 175}, {"V1B Kathode", "k1b", "0", 2.0},
    {"V3A Anode", "pA", "0", 235}, {"V3A Kathode", "kA", "0", 2.4}, {"V3B Anode", "pB", "0", 225}, {"V3B Kathode", "N", "0", 2.2},
    {"V4/V5 Anode", "a5", "0", 430}, {"Schirmgitter", "B2", "0", 425}, {"Gitter (Vorsp.)", "g5", "0", -50}, {"Gleichrichter", "B1", "0", 430},
    {"B3 (V3)", "B3", "0", 0}, {"B4 (V1/V2)", "B4", "0", 0}};
  for (auto &r : rows) printf("%-16s %8.2f  %8.1f\n", r.n, V(r.a) - (std::string(r.b) == "0" ? 0 : V(r.b)), r.ref);
  // Ströme
  double ia5 = (V("B1") - V("a5")) / (c.v.ot_rp / 2), ia4 = (V("B1") - V("a4")) / (c.v.ot_rp / 2);
  printf("Anodenstrom V5 %.1f mA, V4 %.1f mA (über den Wicklungswiderstand)\n", ia5 * 1e3, ia4 * 1e3);
}
int main(int argc, char **argv) {
  double fs = argc > 1 ? atof(argv[1]) : 96000; Parts p; Setting s; Core c;
  auto t0 = std::chrono::steady_clock::now(); c.init(fs, p, s);
  printf("Knoten %d, Zustände %d, Tore %d\n", c.run.s.nn, c.run.s.nh, c.run.s.m);
  c.settle(1.0); c.run.fails = 0; long its = 0; auto ta = std::chrono::steady_clock::now(); { double y[O_NO]; for (int i = 0; i < 48000; i++) { c.step(0, y); its += c.run.lastIt; } } printf("Newton-Schritte im Mittel %.2f, %.2f µs je Abtastwert\n", its / 48000.0, std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count() / 48000 * 1e6);
  c.settle(0.0); auto t1 = std::chrono::steady_clock::now();
  double sec = std::chrono::duration<double>(t1 - t0).count();
  printf("2 s Einschwingen: %.2f s Rechenzeit (%.0f %% Echtzeit), Newton-Fehlschläge %d\n", sec, sec / 2 * 100, c.run.fails);
  dc(c);
  // Goertzel-Messung: Amplitude der Grundwelle und Klirrfaktor (H2..H9) am Lautsprecher
  auto meas = [&](Core &cc, double f, double amp, double &a1, double &thd, double &hum) {
    double y[O_NO]; int per = (int)std::lround(fs / f); int n0 = (int)(0.3 * fs) / per * per, n = (int)(0.2 * fs) / per * per; double ph = 0, dph = 2 * PI * f / fs;
    for (int i = 0; i < n0; i++) { cc.step(amp * std::sin(ph), y); ph += dph; }
    std::vector<double> buf(n); for (int i = 0; i < n; i++) { cc.step(amp * std::sin(ph), y); ph += dph; buf[i] = y[O_SPK]; }
    auto G = [&](double fr) { double re = 0, im = 0; for (int i = 0; i < n; i++) { re += buf[i] * std::cos(2 * PI * fr * i / fs); im += buf[i] * std::sin(2 * PI * fr * i / fs); } return 2 * std::hypot(re, im) / n; };
    a1 = G(f); double hs = 0; for (int k = 2; k <= 9; k++) { double a = G(k * f); hs += a * a; } thd = std::sqrt(hs) / a1; hum = G(v_mains(cc)); };
  { double a1, thd, hum; Core &cc = c;
    printf("\nFrequenzgang am Lautsprecher (8-Ω-Abgriff), Kanal 1 Bass-Buchse, alle Regler 5, 10 mV Spitze:\n");
    double ref = 0; for (double f : {1000.0, 30.0, 50.0, 100.0, 200.0, 400.0, 700.0, 2000.0, 4000.0, 8000.0, 12000.0}) { meas(cc, f, 0.01, a1, thd, hum); if (f == 1000) ref = a1; printf("  %6.0f Hz: %7.3f V  %+6.2f dB\n", f, a1, 20 * std::log10(a1 / ref)); }
    printf("Verstärkung 1 kHz: %.1f dB (Spitze %.3f V am Lautsprecher bei 10 mV)\n", 20 * std::log10(ref / 0.01), ref);
    printf("\nAussteuerung 1 kHz, alle Regler 5:\n");
    for (double a : {0.05, 0.1, 0.2, 0.3, 0.5, 1.0, 2.0}) { meas(cc, 1000, a, a1, thd, hum); printf("  Eingang %.2f V Spitze: %6.2f V Spitze am Lautsprecher = %5.1f W an 8 Ω, Klirr %5.2f %%\n", a, a1, a1 * a1 / 2 / 8, thd * 100); }
  }
  return 0;
}
