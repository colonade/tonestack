// Prüfstand Lametta-B100-III-Kern gegen die Spezifikation des Service Manuals (Seite 1): Verstärkung INPUT → PRE AMP OUT 20 dB,
// INPUT → SPEAKER OUT 64 dB, POWER AMP IN → SPEAKER OUT 41 dB; Parametrik LEVEL ±15 dB, Q 0,35 … 3, FREQUENCY 50 Hz … 2,5 kHz, PRESET LEVEL ±10 dB;
// dazu: Mittelstellung linear, Klangregelung, Ein/Aus der Entzerrer, Rechenzeit, Übersteuerung.   b3test [fs]
#include "b3_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace b3;
static double fs = 48000;
static double dB(double x) { return 20 * std::log10(std::max(x, 1e-30)); }
static int bad = 0, known = 0;
// known: bekannte Abweichung vom Datenblatt bei geprüften Planwerten (wird gezählt und benannt, aber nicht als Fehler)
static void chk(const char *name, double val, double soll, double tol, const char *unit = "dB", bool isKnown = false) { bool ok = std::fabs(val - soll) <= tol; if (!ok) { if (isKnown) known++; else bad++; }
  printf("  %-52s %+8.2f %s  (Soll %+.2f ± %.2f) %s\n", name, val, unit, soll, tol, ok ? "ok" : isKnown ? "BEKANNT (Planwerte, siehe SCHALTPLAN.md)" : "ABWEICHUNG"); }
int main(int argc, char **argv) {
  if (argc > 1) fs = atof(argv[1]); Parts p; Setting s; Core c;
  c.init(fs, p, s); c.settle(0.3);
  printf("Knoten %d, Zustände %d, Tore %d, Gruppen %d\n", c.run.s.nn, c.run.s.nh, c.run.s.m, (int)c.run.grpPort.size() - 1);
  { c.run.fails = 0; long its = 0; auto ta = std::chrono::steady_clock::now(); double y[O_NO]; int N = 48000; double ph = 0;
    for (int i = 0; i < N; i++) { c.step(0.05 * std::sin(ph), y); ph += 2 * PI * 110 / fs; its += c.run.lastIt; }
    printf("Newton-Schritte im Mittel %.2f, %.2f µs je Abtastwert (50 mV Sinus), Fehlschläge %d\n", its / (double)N, std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count() / N * 1e6, c.run.fails); }
  auto R = [&](const Setting &st, double f, int o) { c.apply(st); return std::abs(c.response(f, o)); };
  auto pre = [&](const Setting &st, double f) { return R(st, f, O_PR); };
  // Stellung für die Verstärkungswerte: VOLUME Maximum, alles andere in der Mitte (Klang und Entzerrer ohne Wirkung)
  Setting mx; mx.vol = 1;
  printf("Verstärkung (1 kHz, VOLUME Maximum, übrige Regler Mitte):\n");
  chk("INPUT → PRE AMP OUT (Spezifikation 20 dB)", dB(pre(mx, 1000)), 20, 0.5);
  double pi_g = R(mx, 1000, O_G);
  printf("  INPUT → Gate FET101 (Endstufeneingang): %.2f dB; PRE AMP OUT → Gate: %.2f dB\n", dB(pi_g), dB(pi_g / pre(mx, 1000)));
  chk("POWER AMP IN → SPEAKER OUT (Spezifikation 41 dB; Modell = pa_gain)", dB(std::abs(paResponse(1000, p))), 41, 0.5);
  { Parts q = p; q.pa_gain = 1; printf("  Endstufe laut Plan: 1 + 330 k / (1 k ∥ 220 k) = %.1f-fach = %.1f dB\n", 1 + 330e3 / (1e3 * 220e3 / 221e3), dB(1 + 330e3 / (1e3 * 220e3 / 221e3))); (void)q; }
  printf("  INPUT → SPEAKER OUT: Modell %.1f dB (Spezifikation 64 dB; mit der Endstufe laut Plan %.1f dB)\n", dB(pi_g * std::abs(paResponse(1000, p))), dB(pi_g * 332.5));
  // Mittelstellung: Frequenzgang 20 Hz … 20 kHz (gegen 1 kHz)
  { double r = pre(s, 1000), mn = 1e9, mxv = -1e9; for (double f : {20.0, 40.0, 100.0, 300.0, 3000.0, 10000.0, 20000.0}) { double d = dB(pre(s, f) / r); mn = std::min(mn, d); mxv = std::max(mxv, d); printf("  Mittelstellung %6.0f Hz: %+6.2f dB\n", f, d); }
    chk("Mittelstellung 100 Hz … 10 kHz linear (max. Abweichung)", std::max(std::fabs(dB(pre(s, 100) / r)), std::fabs(dB(pre(s, 10000) / r))), 0, 0.5); }
  // Klangregelung: Hub bei Maximum/Minimum; Mitte des Gyrators
  printf("Klangregelung (gegen Mittelstellung):\n");
  for (int k = 0; k < 3; k++) { const char *nm[3] = {"TREBLE", "MIDDLE", "BASS"}; double fq[3] = {10000, 0, 0};
    Setting a = s, b = s; sfield(a, 1 + k) = 1; sfield(b, 1 + k) = 0; double fbest = 0, best = -1e9; for (double f = 20; f < 20000; f *= 1.02) { double d = dB(pre(a, f) / pre(s, f)); if (d > best) { best = d; fbest = f; } }
    double f = fq[k] > 0 ? fq[k] : fbest; printf("  %-7s Maximum %+5.1f dB, Minimum %+5.1f dB bei %5.0f Hz (größter Hub bei %.0f Hz)\n", nm[k], dB(pre(a, f) / pre(s, f)), dB(pre(b, f) / pre(s, f)), f, fbest); }
  // Parametrik: Spitzenfrequenz, Hub, Güte des Bandpasses
  printf("Parametrischer Entzerrer 1 (PEQ 2 entsprechend):\n");
  auto peak = [&](Setting st, double &fpk) { double best = 0; fpk = 0; for (double f = 20; f < 20000; f *= 1.005) { double d = dB(pre(st, f) / pre(s, f)); if (std::fabs(d) > std::fabs(best)) { best = d; fpk = f; } } return best; };
  auto qOf = [&](Setting st, int o) { c.apply(st); double fpk = 0, best = 0; for (double f = 20; f < 20000; f *= 1.003) { double m = std::abs(c.response(f, o)); if (m > best) { best = m; fpk = f; } }
    double lo = fpk, hi = fpk; while (lo > 5 && std::abs(c.response(lo, o)) > best / std::sqrt(2.0)) lo /= 1.001; while (hi < 40000 && std::abs(c.response(hi, o)) > best / std::sqrt(2.0)) hi *= 1.001; return fpk / (hi - lo); };
  for (int eq = 0; eq < 2; eq++) { int L = eq ? 8 : 4, Q = L + 1, F = L + 2, P = L + 3; int ob = eq ? O_B2 : O_B1; char nm[80];
    Setting a = s; sfield(a, L) = 1; double fpk; double up = peak(a, fpk); std::snprintf(nm, 80, "PEQ%d LEVEL +5 (Spitze bei %.0f Hz)", eq + 1, fpk); chk(nm, up, 15, 1.5);
    a = s; sfield(a, L) = 0; double dn = peak(a, fpk); std::snprintf(nm, 80, "PEQ%d LEVEL −5 (Senke bei %.0f Hz)", eq + 1, fpk); chk(nm, dn, -15, 1.5);
    a = s; sfield(a, L) = 1; sfield(a, F) = 0; peak(a, fpk); std::snprintf(nm, 80, "PEQ%d FREQUENCY links (Skala 50 Hz)", eq + 1); chk(nm, fpk, 50, 5, "Hz");
    sfield(a, F) = 1; peak(a, fpk); std::snprintf(nm, 80, "PEQ%d FREQUENCY rechts (Skala 2,5 kHz; Poti bei 0 Ω)", eq + 1); chk(nm, fpk, 2500, 250, "Hz");
    sfield(a, F) = 0.5; peak(a, fpk); std::snprintf(nm, 80, "PEQ%d FREQUENCY Mitte (Skala 250 Hz)", eq + 1); chk(nm, fpk, 250, 25, "Hz");
    // Güte bei LEVEL +5 (Bandpass-Knoten; gleich der Güte der Anhebung, −3 dB unter der Spitze)
    a = s; sfield(a, L) = 1; sfield(a, Q) = 0; double q0 = qOf(a, ob); sfield(a, Q) = 1; double q1 = qOf(a, ob);
    std::snprintf(nm, 80, "PEQ%d Q links (LEVEL +5, f/B −3 dB)", eq + 1); chk(nm, q0, 0.35, 0.05, "  ");
    std::snprintf(nm, 80, "PEQ%d Q rechts (A25K gegen 4,7 k: Verhältnis 6,3)", eq + 1); chk(nm, q1, 3.0, 0.3, "  ", true);
    a = s; sfield(a, P) = 1; std::snprintf(nm, 80, "PEQ%d PRESET LEVEL +5", eq + 1); chk(nm, dB(pre(a, 1000) / pre(s, 1000)), 10, 1);
    a = s; sfield(a, P) = 0; std::snprintf(nm, 80, "PEQ%d PRESET LEVEL −5", eq + 1); chk(nm, dB(pre(a, 1000) / pre(s, 1000)), -10, 1);
    a = s; sfield(a, L) = 1; sfield(a, P) = 1; if (eq) a.eq2 = false; else a.eq1 = false; double off = 0; for (double f = 20; f < 20000; f *= 1.05) off = std::max(off, std::fabs(dB(pre(a, f) / pre(s, f))));
    std::snprintf(nm, 80, "PEQ%d ausgeschaltet bei LEVEL +5, PRESET +5 (Restwirkung)", eq + 1); chk(nm, off, 0, 0.5); }
  // Übersteuerung: Fehlschläge, NaN, Rechenzeit je Überabtastung
  for (int os : {2, 4}) { Setting b = s; b.vol = 1; b.lev1 = 1; b.bass = 1; Amp A; A.vin = 1; A.init(fs, os, p, b); A.setTarget(b); A.snap(); A.c.run.fails = 0; int n = (int)fs; std::vector<float> bf(n); int nan = 0; double mxo = 0;
    for (double amp : {0.01, 0.3, 2.0, 5.0}) { double ph = 0; for (int i = 0; i < n; i++) { bf[i] = (float)(amp * (std::sin(ph) + 0.5 * std::sin(ph * 7.3))); ph += 2 * PI * 41 / fs; }
      auto ta = std::chrono::steady_clock::now(); for (int i = 0; i < n; i += 128) A.process(&bf[i], std::min(128, n - i)); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - ta).count();
      for (int i = 0; i < n; i++) { if (!std::isfinite(bf[i])) nan++; mxo = std::max(mxo, (double)std::fabs(bf[i])); }
      double q[5]; A.takePeaks(q); printf("os %d, %4.2f V: %5.1f %% Echtzeit, Fehlschläge %d, NaN %d, Spitze %.2f, IC101 %.0f %% Vorstufe %.0f %% PEQ1 %.0f %% PEQ2 %.0f %% Endstufe %.0f W\n",
        os, amp, dt * 100, A.c.run.fails, nan, mxo, q[0] * 100, q[1] * 100, q[2] * 100, q[3] * 100, q[4]); if (nan || A.c.run.fails) bad++; } }
  printf("bekannte Abweichungen vom Datenblatt: %d (Q oben bei beiden Entzerrern)\n", known); printf("%d Abweichungen\n", bad); return bad ? 1 : 0; }
