// Prüfstand GK-800RB-Kern: Arbeitspunkt der J113-Stufe, Pegel der Stufen bei den Prüfbedingungen des Schaltplans (2 mV, 200 Hz, alles 10),
// Prüfanleitung 8/19/91 (−10 dB, Boost 0, Begrenzung), Frequenzgänge der Regler und Filter, Frequenzweiche, Rechenzeit.   gktest [fs]
#include "gk_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace gk;
static double fs = 48000;
// Effektivwert eines Sinus (f, Amplitude Spitze) an Ausgang o nach dem Einschwingen
static double rmsOf(Core &c, double f, double amp, int o, double *all = nullptr) {
  double y[O_NO]; int per = (int)std::lround(fs / f); int n0 = (int)(0.4 * fs) / per * per, n = (int)(0.2 * fs) / per * per; double ph = 0, dph = 2 * PI * f / fs; double acc[O_NO] = {0};
  for (int i = 0; i < n0; i++) { c.step(amp * std::sin(ph), y); ph += dph; }
  for (int i = 0; i < n; i++) { c.step(amp * std::sin(ph), y); ph += dph; for (int k = 0; k < O_NO; k++) acc[k] += y[k] * y[k]; }
  if (all) for (int k = 0; k < O_NO; k++) all[k] = std::sqrt(acc[k] / n);
  return std::sqrt(acc[o] / n); }
int main2();
static double dB(double x) { return 20 * std::log10(std::max(x, 1e-30)); }
int main(int argc, char **argv) {
  if (argc > 1) fs = atof(argv[1]); Parts p; Setting s; Core c;
  auto t0 = std::chrono::steady_clock::now(); c.init(fs, p, s); c.settle(0.5);
  printf("Knoten %d, Zustände %d, Tore %d, Gruppen %d\n", c.run.s.nn, c.run.s.nh, c.run.s.m, (int)c.run.grpPort.size() - 1);
  { c.run.fails = 0; long its = 0, pas = 0; auto ta = std::chrono::steady_clock::now(); double y[O_NO]; int N = 48000; double ph = 0;
    for (int i = 0; i < N; i++) { c.step(0.05 * std::sin(ph), y); ph += 2 * PI * 110 / fs; its += c.run.lastIt; pas += c.run.lastPass; }
    printf("Newton-Schritte im Mittel %.2f, Durchgänge %.2f, %.2f µs je Abtastwert (50 mV Sinus), Fehlschläge %d\n", its / (double)N, pas / (double)N, std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count() / N * 1e6, c.run.fails); }
  printf("Einschwingen + 1 s: %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
  // Arbeitspunkt
  { std::vector<double> u = {0, p.vcc, -p.vcc}; auto V = dk::nodeVoltages(c.net, c.fs, c.run.h, u, c.run.ip); auto n = [&](const char *k) { return V[c.net.idx.at(k)]; };
    printf("J113: Gate %.3f V, Source %.3f V, Drain %.3f V, Siebung %.3f V (Ruhestrom %.2f mA)\n", n("g"), n("js"), n("jd"), n("c92"), (p.vcc - n("c92")) / p.r90 * 1e3); }
  // Prüfbedingungen des Plans: 2 mV rms 200 Hz, alle Regler 10, Boost 10, Filter aus → Pegel je Stufe (Plan: 19 / 107 / 115 / 115 / 150 / 93 (alt) / 1,1 V)
  Setting a; a.vol = a.treb = a.himid = a.lomid = a.bass = a.boost = a.hima = a.loma = 1; a.freq = 1; c.apply(a);
  { double all[O_NO]; rmsOf(c, 200, 0.002 * std::sqrt(2.0), O_PA300, all);
    printf("2 mV 200 Hz, alles 10: U1a %.1f mV, U2a %.0f mV, U2b %.0f mV, U3a %.0f mV, U3b %.0f mV, Z %.0f mV, J113-Drain %.2f V, Endstufeneingang 300 W %.2f V (×61,7 = %.1f V)\n",
      all[O_U1] * 1e3, all[O_U2A] * 1e3, all[O_U2B] * 1e3, all[O_U3A] * 1e3, all[O_U3B] * 1e3, all[O_DIRECT] * 1e3, all[O_JD], all[O_PA300], all[O_PA300] * 61.7); }
  // Prüfanleitung: Verstärkung des Ganzen, −10 dB, Boost 0 (Verhältnisse)
  { double ref = rmsOf(c, 200, 0.002 * std::sqrt(2.0), O_PA300); Setting b = a; b.pad = true; c.apply(b); double pad = rmsOf(c, 200, 0.002 * std::sqrt(2.0), O_PA300);
    b = a; b.boost = 0; c.apply(b); double bo = rmsOf(c, 200, 0.002 * std::sqrt(2.0), O_PA300); b = a; b.boostOn = false; c.apply(b); double bsw = rmsOf(c, 200, 0.002 * std::sqrt(2.0), O_PA300);
    printf("−10 dB: %.1f dB (Prüfanleitung 28 → 6,0 V = −13,4 dB), Boost 0: %.1f dB (28 → 3,1 V = −19,1 dB), Boost-Schalter aus: %.1f dB\n", dB(pad / ref), dB(bo / ref), dB(bsw / ref)); }
  // Begrenzung: Eingang erhöhen, bis die Endstufen begrenzen (Amp mit Endstufenmodell)
  { Amp A; A.vin = 1; A.init(fs, 1, p, a); A.setTarget(a); A.snap(); std::vector<float> buf;
    for (double mv : {2.0, 5.0, 10.0, 20.0, 50.0, 100.0}) { int per = (int)std::lround(fs / 200), n = per * 60; buf.assign(n, 0); for (int i = 0; i < n; i++) buf[i] = (float)(mv * 1e-3 * std::sqrt(2.0) * std::sin(2 * PI * 200 * i / fs));
      A.process(buf.data(), n); double q[5]; A.takePeaks(q); double rms = 0; for (int i = n / 2; i < n; i++) rms += buf[i] * buf[i]; rms = std::sqrt(rms / (n / 2)) * V_FS;
      printf("  %5.1f mV: 300 W %5.1f W, 100 W %5.1f W, Ausgang %5.1f V rms (beide Endstufen), U1a %.0f %%, EQ %.0f %%, J113 %.0f %%\n", mv, q[3], q[4], rms, q[0] * 100, q[1] * 100, q[2] * 100); } }
  // Frequenzgänge (Kleinsignal) am Endstufeneingang, bezogen auf 1 kHz bei allen Reglern 5
  auto resp = [&](const Setting &st, double f, int o = O_PA300) { c.apply(st); return std::abs(c.response(f, o)); };
  Setting m; m.vol = 1; auto fr = [&](const Setting &st, const char *name) { double r1 = resp(m, 1000); printf("%-28s", name); for (double f : {40.0, 60.0, 100.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 15000.0}) printf(" %6.0f:%+6.1f", f, dB(resp(st, f) / r1)); printf("\n"); };
  printf("Frequenzgang gegen 1 kHz bei allen Reglern 5 (Vol 10), dB:\n"); fr(m, "alle 5");
  for (int k = 1; k <= 4; k++) { const char *nm[] = {"", "TREBLE", "HI-MID", "LO-MID", "BASS"}; for (double x : {0.0, 1.0}) { Setting t = m; sfield(t, k) = x; char b[40]; std::snprintf(b, 40, "%s %.0f", nm[k], x * 10); fr(t, b); } }
  { Setting t = m; t.locut = true; fr(t, "LO CUT"); t = m; t.contour = true; fr(t, "MID CONTOUR"); t = m; t.hiboost = true; fr(t, "HI BOOST (Vol 10)"); t = m; t.hiboost = true; t.vol = 0.5; fr(t, "HI BOOST (Vol 5)"); t = m; t.vol = 0.5; fr(t, "Vol 5"); }
  // Frequenzweiche: Tiefpass (300 W) und Hochpass (100 W) gegenüber FULL mit gleichen Reglern; Trennfrequenz = gleiche Dämpfung beider Wege, bei FREQ 0 / 5 / 10
  for (double x : {0.0, 0.5, 1.0}) { Setting f = m; f.freq = x; Setting t = f; t.biamp = true; auto g = [&](double fr, int o) { c.apply(t); double b = std::abs(c.response(fr, o)); c.apply(f); return b / std::abs(c.response(fr, o)); };
    double fx = 0, dl = 0, dh = 0; for (double fr = 20; fr < 15000; fr *= 1.01) { double lo = g(fr, O_PA300), hi = g(fr, O_PA100); if (lo <= hi) { fx = fr; dl = dB(lo); dh = dB(hi); break; } }
    printf("Bi-Amp FREQ %2.0f: Trennfrequenz %.0f Hz (beide Wege %+.1f / %+.1f dB gegen FULL; Anleitung 100 Hz … 1 kHz)\n", x * 10, fx, dl, dh); }
  main2(); return 0;
}
// Übersteuerung: Eingang bis 5 V Spitze, alles 10, Boost 10 – Fehlschläge des Lösers, NaN, Rechenzeit je Überabtastung
int main2() { Parts p; Setting a; a.vol = a.treb = a.himid = a.lomid = a.bass = a.boost = a.hima = a.loma = 1;
  for (int os : {1, 2, 4}) { Amp A; A.vin = 1; A.init(fs, os, p, a); A.setTarget(a); A.snap(); A.c.run.fails = 0; int n = (int)fs; std::vector<float> b(n); int nan = 0; double mx = 0; long its = 0;
    for (double amp : {0.01, 0.3, 2.0, 5.0}) { double ph = 0; for (int i = 0; i < n; i++) { b[i] = (float)(amp * (std::sin(ph) + 0.5 * std::sin(ph * 7.3))); ph += 2 * PI * 82 / fs; }
      auto t0 = std::chrono::steady_clock::now(); for (int i = 0; i < n; i += 128) A.process(&b[i], std::min(128, n - i)); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      for (int i = 0; i < n; i++) { if (!std::isfinite(b[i])) nan++; mx = std::max(mx, (double)std::fabs(b[i])); } its += A.c.run.lastIt;
      double q[5]; A.takePeaks(q); printf("os %d, %4.2f V: %5.1f %% Echtzeit, Fehlschläge %d, NaN %d, Spitze %.2f, U1a %.0f %% EQ %.0f %% J113 %.0f %% 300 W %.0f W\n", os, amp, dt / 1.0 * 100, A.c.run.fails, nan, mx, q[0] * 100, q[1] * 100, q[2] * 100, q[3]); } }
  return 0; }
