// Prüfstand Yamaha-G100-212II-Kern gegen das Service Manual (Seiten 4–7): Tabelle 3 (Verstärkung A HIGH / A LOW / B HIGH bei Tabelle-1-Stellung),
// Tabelle 6 (Frequenzgang 70 / 400 / 7 kHz), Tabelle 7 (Regelumfang TREBLE/MIDDLE/BASS), Tabellen 8/9 (FAT, BRIGHT), Tabelle 10 (parametrischer EQ),
// Fig. 1-2 (Frequenzgang), Begrenzung (Tabelle 5), Arbeitspunkt TR101, Rechenzeit, Übersteuerung.   g100test [fs]
#include "g100_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace g100;
static double fs = 48000;
static double dB(double x) { return 20 * std::log10(std::max(x, 1e-30)); }
static int bad = 0;
static void chk(const char *name, double val, double soll, double tol) { bool ok = std::fabs(val - soll) <= tol; if (!ok) bad++; printf("  %-44s %+7.1f dB  (Soll %+.0f ± %.0f) %s\n", name, val, soll, tol, ok ? "ok" : "ABWEICHUNG"); }
// Tabelle 1: alle Regler Maximum, LEVEL Mitte, Q und FREQUENCY Minimum, FAT/BRIGHT aus, Kanal A
static Setting table1() { Setting s; s.vola = s.gainb = s.masterb = s.treba = s.mida = s.bassa = s.trebb = s.midb = s.bassb = 1; s.level = 0.5; s.q = 0; s.freq = 0; return s; }
int main(int argc, char **argv) {
  if (argc > 1) fs = atof(argv[1]); Parts p; Setting s; Core c;
  auto t0 = std::chrono::steady_clock::now(); c.init(fs, p, s); c.settle(0.5);
  printf("Knoten %d, Zustände %d, Tore %d, Gruppen %d\n", c.run.s.nn, c.run.s.nh, c.run.s.m, (int)c.run.grpPort.size() - 1);
  { c.run.fails = 0; long its = 0, pas = 0; auto ta = std::chrono::steady_clock::now(); double y[O_NO]; int N = 48000; double ph = 0;
    for (int i = 0; i < N; i++) { c.step(0.05 * std::sin(ph), y); ph += 2 * PI * 110 / fs; its += c.run.lastIt; pas += c.run.lastPass; }
    printf("Newton-Schritte im Mittel %.2f, Durchgänge %.2f, %.2f µs je Abtastwert (50 mV Sinus), Fehlschläge %d\n", its / (double)N, pas / (double)N, std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count() / N * 1e6, c.run.fails); }
  printf("Einschwingen + 1 s: %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
  { std::vector<double> u = {0, p.vcc, 0}; auto V = dk::nodeVoltages(c.net, c.fs, c.run.h, u, c.run.ip); auto n = [&](const char *k) { return V[c.net.idx.at(k)]; };
    printf("TR101: Gate %.3f V, Source %.3f V, Drain %.3f V, Siebung %.3f V (Ruhestrom %.2f mA, Rechnung %.2f mA)\n", n("X"), n("S"), n("D"), n("vdd"), (p.vcc - n("vdd")) / p.r_dd * 1e3, c.idQ() * 1e3); }
  // Kleinsignal: Verstärkung Eingang → Lautsprecher (Vorstufe bis Gate · Endstufe), dB gegen 0,775 V bei −70 dB (245 µV) Eingang
  auto g = [&](const Setting &st, double f) { c.apply(st); return std::abs(c.response(f, O_G)) * std::abs(paResponse(f, p)); };
  auto dbm = [&](const Setting &st, double f) { return dB(g(st, f) * 0.775e-3 * std::pow(10, -70.0 / 20) / 0.775 * 1e3); };   // −70 dB = 0,245 mV
  Setting a = table1();
  printf("Tabelle 3 (−70 dB / 1 kHz, Ausgang in dBm = dB gegen 0,775 V):\n");
  chk("A ch HIGH", dbm(a, 1000), 1, 3);
  { Setting b = a; b.low = true; chk("A ch LOW", dbm(b, 1000), -11, 3); }
  { Setting b = a; b.chanB = true; chk("B ch HIGH", dbm(b, 1000), 14, 3); }
  printf("  (Vorstufe bis Endstufeneingang bei Kanal A: %.1f dB, Endstufe %.1f dB bei 1 kHz)\n", dB(std::abs((c.apply(a), c.response(1000, O_G)))), dB(std::abs(paResponse(1000, p))));
  printf("Tabelle 6 (Frequenzgang gegen 1 kHz):\n");
  for (int ch = 0; ch < 2; ch++) { Setting b = a; b.chanB = ch; double r = g(b, 1000); char nm[40];
    double soll[3] = {-1, -8, ch ? 5.0 : 6.0}; double f[3] = {70, 400, 7000}; for (int k = 0; k < 3; k++) { std::snprintf(nm, 40, "%c ch %.0f Hz", ch ? 'B' : 'A', f[k]); chk(nm, dB(g(b, f[k]) / r), soll[k], 3); } }
  printf("Tabelle 7 (Regler Maximum → Minimum):\n");
  { Setting b = a; b.treba = 0; chk("TREBLE 7 kHz", dB(g(a, 7000) / g(b, 7000)), 16, 3); b = a; b.mida = 0; chk("MIDDLE 400 Hz", dB(g(a, 400) / g(b, 400)), 6, 3); b = a; b.bassa = 0; chk("BASS 70 Hz", dB(g(a, 70) / g(b, 70)), 11, 3); }
  printf("Tabelle 8/9 (FAT bei 400 Hz; BRIGHT bei 7 kHz, VOLUME 5):\n");
  { Setting b = a; b.fata = true; chk("FAT A", dB(g(b, 400) / g(a, 400)), 9, 3); b = a; b.chanB = true; Setting d = b; d.fatb = true; chk("FAT B", dB(g(d, 400) / g(b, 400)), 9, 3);
    b = a; b.vola = 0.5; d = b; d.brighta = true; chk("BRIGHT A", dB(g(d, 7000) / g(b, 7000)), 11, 3); b = a; b.chanB = true; b.gainb = 0.5; d = b; d.brightb = true; chk("BRIGHT B", dB(g(d, 7000) / g(b, 7000)), 11, 3); }
  printf("Tabelle 10 (parametrischer EQ, VOLUME 5, LEVEL Maximum / Minimum gegen Mitte):\n");
  { struct T { double q, f, fr, soll, tol; const char *nm; } tt[] = {{0, 0, 650, 7, 2, "Q min F min 650 Hz"}, {0, 1, 650, 7, 2, "Q min F max 650 Hz"}, {1, 0, 650, 0, 1, "Q max F min 650 Hz"}, {1, 1, 650, 0, 1, "Q max F max 650 Hz"}, {1, 0, 90, 15, 2, "Q max F min 90 Hz"}, {1, 1, 5000, 15, 2, "Q max F max 5 kHz"}};
    for (auto &t : tt) { Setting b = a; b.vola = 0.5; b.q = t.q; b.freq = t.f; double m = g(b, t.fr); b.level = 1; double up = dB(g(b, t.fr) / m); b.level = 0; double dn = dB(g(b, t.fr) / m);
      char nm[60]; std::snprintf(nm, 60, "%s +", t.nm); chk(nm, up, t.soll, t.tol); std::snprintf(nm, 60, "%s −", t.nm); chk(nm, dn, -t.soll, t.tol); } }
  printf("FREQUENCY-Skala: "); for (double x : {0.0, 0.28, 0.5, 0.72, 1.0}) printf("%.2f → %.0f Hz (R %.1f k)  ", x, freqOfKnob(x), (p.r_f + freqPotR(p, x)) / 1e3); printf("\n");
  // Fig. 1-2: Frequenzgang Kanal A bei Tabelle-1-Stellung, gegen 1 kHz
  { printf("Fig. 1-2 (A ch, gegen 1 kHz; Zeichnung: 40 Hz +0,5, 100 Hz −3, 300 Hz −10, 2 kHz +4, 4 kHz +7, 10 kHz +4, 20 kHz 0):\n  "); double r = g(a, 1000);
    for (double f : {20.0, 40.0, 70.0, 100.0, 200.0, 300.0, 400.0, 700.0, 1000.0, 2000.0, 4000.0, 7000.0, 10000.0, 20000.0}) printf("%.0f:%+.1f ", f, dB(g(a, f) / r)); printf("\n"); }
  // Hall: Treiberverstärkung (Kleinsignal A1 → IC3-Ausgang), Federmodell (Echo-Laufzeit, Nachhallzeit), Hallanteil bei REVERB 10 (1 kHz, Regler 5)
  { Setting b; c.apply(b); auto gd = [&](double f) { return std::abs(c.response(f, O_RDRV)) / std::abs(c.response(f, O_EQ)); };   // IC4-Ausgang ≈ A1 (Pegel gleich bei LEVEL Mitte)
    printf("Halltreiber IC3 gegen Summenpunkt: 200 Hz %.2f, 1 kHz %.2f, 5 kHz %.2f, 10 kHz %.2f (Rechnung aus 100 k / (2,2 k + 2,2 n): 1,02 / 1,70 / 7,0 / 13,9)\n", gd(200), gd(1000), gd(5000), gd(10000));
    springrev::Tank tk; tk.design(fs, p.spring); int n = (int)(4 * fs); std::vector<double> h(n); for (int i = 0; i < n; i++) h[i] = tk.step(i == 0 ? 1 : 0);
    double e = 0; for (double x : h) e += x * x; double cum = 0; int i1 = -1, i50 = -1; for (int i = 0; i < n; i++) { cum += h[i] * h[i]; if (i1 < 0 && cum > 1e-4 * e) i1 = i; if (i50 < 0 && cum > 0.5 * e) i50 = i; }
    auto ewin = [&](double t0, double t1) { double q = 0; for (int i = (int)(t0 * fs); i < (int)(t1 * fs); i++) q += h[i] * h[i]; return q; };
    double t60 = 60 * 1.0 / (10 * std::log10(ewin(0.5, 1.0) / ewin(1.5, 2.0)));   // Abklingen aus dem Energieverhältnis zweier Fenster im Abstand 1 s
    printf("Federmodell (Impuls): erste Energie nach %.1f ms, halbe Energie nach %.2f s, Nachhallzeit aus dem Abklingen %.2f s (Vorgabe: Laufzeiten %.0f/%.0f ms, T60 %.2f s)\n", i1 * 1e3 / fs, i50 / fs, t60, p.spring.td[0] * 1e3, p.spring.td[1] * 1e3, p.spring.t60);
    auto out = [&](double rv) { Setting w; w.reverb = rv; Amp A; A.vin = 1; A.init(fs, 1, p, w); A.setTarget(w); A.snap(); int N = (int)(3 * fs); std::vector<float> buf(N); for (int i = 0; i < N; i++) { double q = 0; for (double f : {523.3, 659.3, 784.0, 987.8, 1174.7, 1568.0, 1975.5}) q += std::sin(2 * PI * f * i / fs + f); buf[i] = (float)(0.01 * q); } A.process(buf.data(), N); return buf; };   // Akkord 500 Hz … 2 kHz statt eines Tons (Kammfilter des Halls)
    std::vector<float> w0 = out(0), w1 = out(1); double r0 = 0, rw = 0; int N = (int)w0.size(); for (int i = N / 2; i < N; i++) { r0 += (double)w0[i] * w0[i]; rw += ((double)w1[i] - w0[i]) * ((double)w1[i] - w0[i]); }
    printf("Akkord 500 Hz … 2 kHz, Regler 5: Hallanteil bei REVERB 10 gegen das Direktsignal %+.1f dB (Modell-Vorgabe etwa 0 dB)\n", dB(std::sqrt(rw / r0))); }
  // Federtank nichtlinear (ab 1.2): leise wie linear; zweiter Anschlag auf schwingender Feder (Wechselwirkung = Antwort(A+B) − Antwort(A) − Antwort(B) am Rückholverstärker);
  // linear (Schalter, wie 1.1) bei kleinen Anschlägen Überlagerung; Dauerlast ohne NaN, Abklingen nach dem Ende
  { auto ret = [&](bool lin, double amp, std::vector<double> at, int N) { Setting s; s.vola = 1; s.reverb = 0.5; s.tankLin = lin; Core k; k.init(fs, p, s); k.settle(0.3); double y[O_NO]; std::vector<double> r(N);
      for (int i = 0; i < N; i++) { double x = 0; for (double t : at) { int j = i - (int)(t * fs); if (j >= 0 && j < (int)(0.0005 * fs)) x += amp * std::sin(2 * PI * 1000 * j / fs); } k.step(x, y); r[i] = y[O_RRET]; } return r; };
    auto inter = [&](bool lin, double amp, double dt) { int N = (int)(fs * 0.8); auto ab = ret(lin, amp, {0.05, 0.05 + dt}, N), a1 = ret(lin, amp, {0.05}, N), b1 = ret(lin, amp, {0.05 + dt}, N);
      int i0 = (int)((0.05 + dt) * fs), i1 = std::min(N, i0 + (int)(0.5 * fs)); double rr = 0, rb = 0; for (int i = i0; i < i1; i++) { double d = ab[i] - a1[i] - b1[i]; rr += d * d; rb += b1[i] * b1[i]; } return 10 * std::log10(rr / rb + 1e-30); };
    printf("Federtank: zweiter Anschlag (1 kHz, 0,5 ms) auf schwingender Feder, Kanal A VOLUME 10, Wechselwirkung gegen den Anschlag allein:\n");
    for (double amp : {0.05, 0.3}) for (double dt : {0.05, 0.2}) { double wl = inter(true, amp, dt), wp = inter(false, amp, dt);
      bool ok = wl < -60 && (amp < 0.1 ? wp < -30 : wp > -25); if (!ok) bad++;
      printf("  %.2f V, Abstand %3.0f ms: linear (1.1) %+6.1f dB, physikalisch %+6.1f dB  (Soll linear < −60; physikalisch %s) %s\n", amp, dt * 1e3, wl, wp, amp < 0.1 ? "< −30 (leise)" : "> −25", ok ? "ok" : "ABWEICHUNG"); }
    for (int lin = 1; lin >= 0; lin--) { Setting t; t.chanB = true; t.gainb = 1; t.masterb = 1; t.reverb = 1; t.tankLin = lin; Core k; k.init(fs, p, t); k.settle(0.3); double y[O_NO]; int nan = 0; double e[3] = {0, 0, 0};
      for (int i = 0; i < (int)(fs * 6); i++) { double x = i < fs * 3 ? 0.5 * std::sin(2 * PI * 82.4 * i / fs) + 0.3 * std::sin(2 * PI * 110 * i / fs) : 0; k.step(x, y); double v = y[O_RRET]; if (!std::isfinite(v)) nan++;
        double tt = i / fs; if (tt >= 2 && tt < 3) e[0] += v * v; if (tt >= 4 && tt < 5) e[1] += v * v; if (tt >= 5 && tt < 6) e[2] += v * v; }
      double d1 = 10 * std::log10(e[1] / e[0]), d2 = 10 * std::log10(e[2] / e[0]); bool ok = !nan && k.run.fails == 0 && d1 < -20 && d2 < d1 - 15; if (!ok) bad++;
      printf("  Dauerlast Kanal B 10/10, REVERB 10, %s: NaN %d, Fehlschläge %d, Abklingen 1–2 s / 2–3 s nach dem Ende %.1f / %.1f dB  %s\n", lin ? "linear" : "physikalisch", nan, k.run.fails, d1, d2, ok ? "ok" : "ABWEICHUNG"); }
    auto chord = [&](bool lin) { auto out = [&](double rv) { Setting w; w.reverb = rv; w.tankLin = lin; Amp A; A.vin = 1; A.init(fs, 1, p, w); A.setTarget(w); A.snap(); int N = (int)(3 * fs); std::vector<float> buf(N); for (int i = 0; i < N; i++) { double q = 0; for (double f : {523.3, 659.3, 784.0, 987.8, 1174.7, 1568.0, 1975.5}) q += std::sin(2 * PI * f * i / fs + f); buf[i] = (float)(0.01 * q); } A.process(buf.data(), N); return buf; };
      std::vector<float> w0 = out(0), w1 = out(1); double r0 = 0, rw = 0; int N = (int)w0.size(); for (int i = N / 2; i < N; i++) { r0 += (double)w0[i] * w0[i]; rw += ((double)w1[i] - w0[i]) * ((double)w1[i] - w0[i]); } return dB(std::sqrt(rw / r0)); };
    double cl = chord(true), cp = chord(false); bool ok = std::fabs(cp - cl) < 0.1; if (!ok) bad++;
    printf("  leiser Akkord: Hallanteil linear %+.2f dB, physikalisch %+.2f dB (Soll gleich ± 0,1 dB) %s\n", cl, cp, ok ? "ok" : "ABWEICHUNG"); }
  // Begrenzung: Eingang erhöhen (Amp mit Endstufenmodell), Tabelle-1-Stellung, 1 kHz
  for (int ch = 0; ch < 2; ch++) { Setting b = a; b.chanB = ch; Amp A; A.vin = 1; A.init(fs, 1, p, b); A.setTarget(b); A.snap(); std::vector<float> buf;
    printf("Begrenzung Kanal %c (1 kHz, Tabelle-1-Stellung):\n", ch ? 'B' : 'A');
    for (double mv : {0.245, 1.0, 3.0, 10.0, 30.0, 100.0, 300.0}) { int per = (int)std::lround(fs / 1000), n = per * 300; buf.assign(n, 0); for (int i = 0; i < n; i++) buf[i] = (float)(mv * 1e-3 * std::sqrt(2.0) * std::sin(2 * PI * 1000 * i / fs));
      A.process(buf.data(), n); double q[6]; A.takePeaks(q); double rms = 0; for (int i = n / 2; i < n; i++) rms += buf[i] * buf[i]; rms = std::sqrt(rms / (n / 2)) * V_FS;
      printf("  %7.3f mV: Lautsprecher %5.1f V rms (%5.1f W an 4 Ω), IC1 %.0f %%, Kanal A %.0f %%, Dioden B %.0f %%, TR101 %.0f %%, EQ %.0f %%\n", mv, rms, rms * rms / 4, q[0] * 100, q[1] * 100, q[2] * 100, q[3] * 100, q[4] * 100); } }
  // Übersteuerung: Fehlschläge, NaN, Rechenzeit je Überabtastung
  for (int os : {1, 2, 4}) { Setting b = a; b.chanB = true; Amp A; A.vin = 1; A.init(fs, os, p, b); A.setTarget(b); A.snap(); A.c.run.fails = 0; int n = (int)fs; std::vector<float> bf(n); int nan = 0; double mx = 0;
    for (double amp : {0.01, 0.3, 2.0, 5.0}) { double ph = 0; for (int i = 0; i < n; i++) { bf[i] = (float)(amp * (std::sin(ph) + 0.5 * std::sin(ph * 7.3))); ph += 2 * PI * 82 / fs; }
      auto ta = std::chrono::steady_clock::now(); for (int i = 0; i < n; i += 128) A.process(&bf[i], std::min(128, n - i)); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count();
      for (int i = 0; i < n; i++) { if (!std::isfinite(bf[i])) nan++; mx = std::max(mx, (double)std::fabs(bf[i])); }
      double q[6]; A.takePeaks(q); printf("os %d, %4.2f V (Kanal B): %5.1f %% Echtzeit, Fehlschläge %d, NaN %d, Spitze %.2f, Dioden %.0f %% TR101 %.0f %% Endstufe %.0f W\n", os, amp, dt * 100, A.c.run.fails, nan, mx, q[2] * 100, q[3] * 100, q[5]); } }
  printf("%d Abweichungen\n", bad); return bad ? 1 : 0;
}
