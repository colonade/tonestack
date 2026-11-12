// Prüfstand Loldano (Nachbau Soldano SLO-100): Gleichspannungen gegen den Plan, Ruhestrom, Verstärkung, Frequenzgang, Leistung.
#include "slo_core.hpp"
#include <cstdio>
#include <cmath>
using namespace slo;
static const double FS = 96000;
static int bad = 0;
static void chk(const char *n, double v, double soll, double tol, const char *u) { bool ok = std::fabs(v - soll) <= tol; if (!ok) bad++; std::printf("  %-52s %9.2f %-3s (Soll %.2f ± %.2f) %s\n", n, v, u, soll, tol, ok ? "ok" : "ABWEICHUNG"); }
static Core mk(const Setting &s, const std::vector<std::string> *dbg, double settle = 3.0, const Parts *pv = nullptr) { Core c; c.dbg = dbg; c.init(FS, pv ? *pv : Parts(), s); c.settle(settle); return c; }
int main() {
  std::vector<std::string> N = {"B1", "B2", "B3", "B4", "B5", "p1", "k1", "p2", "k2", "p3", "k3", "pc", "kc", "p4", "k4", "f1", "p5", "k5", "f2", "pa", "pb", "kp", "F", "BIAS", "a5", "s5"};
  Setting s; Core c = mk(s, &N, 8.0); double y[O_NO + 32]; c.step(0, y); auto V = [&](const char *n) { for (size_t i = 0; i < N.size(); i++) if (N[i] == n) return y[O_NO + i]; return 0.0; };
  std::printf("Knoten %d, Zustände %d, nichtlineare Tore %d\n", c.net.nn, c.run.s.nh, c.run.s.m);
  std::printf("Versorgung (Plan)\n"); chk("B+1 Anoden (497 V)", V("B1"), 497, 3, "V"); chk("B+2 Schirmgitter (495 V)", V("B2"), 495, 3, "V"); chk("B+3 (378 V)", V("B3"), 378, 10, "V"); chk("B+4 (350 V)", V("B4"), 350, 12, "V"); chk("B+5 (359 V)", V("B5"), 359, 12, "V");   /* Summe passt; im Plan sind die beiden Werte vermutlich vertauscht (V1 zieht mehr Strom als V2) */
  double ia = (V("B1") - V("a5")) / (Parts().ot_rp / 2) / 2 * 1000; chk("Ruhestrom je 6L6GC (35 mA, Annahme)", ia, 35, 1, "mA");
  std::printf("Arbeitspunkte (der Plan nennt keine; zur Auskunft)\n");
  for (auto *p : {"p1", "k1", "p2", "k2", "p3", "k3", "pc", "kc", "p4", "k4", "f1", "p5", "k5", "f2", "pa", "pb", "kp", "F", "BIAS"}) std::printf("  %-5s %8.2f V\n", p, V(p));
  std::printf("  Phasenumkehr: Kathoden %.1f V (Robinette: etwa 34 V), Knoten F %.1f V\n", V("kp"), V("F"));
  // Verstärkung und Frequenzgang (kleines Signal), Leistung
  auto sine = [&](Core &k, double amp, double f, double secs, double &rms, double &thd, int out = O_SPK) { long n = (long)(secs * FS), n0 = n / 2; double a = 0, b = 0, sq = 0, yy[O_NO + 32];
    for (long i = 0; i < n; i++) { k.step(amp * std::sin(2 * PI * f * i / FS), yy); if (i >= n0) { double w = 2 * PI * f * i / FS; a += yy[out] * std::sin(w); b += yy[out] * std::cos(w); sq += yy[out] * yy[out]; } }
    long m = n - n0; double f1 = 2 * std::sqrt(a * a + b * b) / m; rms = std::sqrt(sq / m); double h = sq / m - f1 * f1 / 2; thd = h > 0 ? std::sqrt(h) / (f1 / std::sqrt(2.0)) : 0; };
  std::printf("NORMAL-Kanal (Clean), GAIN 5, MASTER 3: Frequenzgang bei 1 mV\n");
  { Setting q; q.od = false; Core k = mk(q, nullptr); double r0, t; sine(k, 1e-3, 1000, 0.2, r0, t); for (double f : {82.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0}) { double r; sine(k, 1e-3, f, 0.3, r, t); std::printf("  %6.0f Hz %+6.2f dB\n", f, 20 * std::log10(r / r0)); }
    std::printf("  Verstärkung 1 kHz: %.1f dB\n", 20 * std::log10(r0 / (1e-3 / std::sqrt(2.0)))); }
  std::printf("OVERDRIVE, GAIN 5, MASTER 3: Eingang → Lautsprecher (1 kHz)\n");
  for (double a : {0.001, 0.01, 0.05, 0.2}) { Setting q; Core k = mk(q, nullptr); double r, t; sine(k, a, 1000, 0.4, r, t); double rs, ts; Core k2 = mk(q, nullptr); sine(k2, a, 1000, 0.4, rs, ts, O_SEND);
    std::printf("  %5.0f mV Spitze: %6.2f V eff am Lautsprecher (%5.1f W), Klirr %5.1f %% | FX SEND %.3f V eff, Klirr %.1f %%\n", a * 1000, r, r * r / 8, 100 * t, rs, 100 * ts); }
  std::printf("Leistung: OVERDRIVE, MASTER 10, 1 kHz, 50 mV\n");
  { Setting q; q.master = 1; Core k = mk(q, nullptr); double r, t; sine(k, 0.05, 1000, 0.5, r, t); std::printf("  %.1f V eff = %.0f W an 8 Ω, Klirr %.0f %%\n", r, r * r / 8, 100 * t); }
  { Setting q; q.od = false; q.nmaster = 1; q.ngain = 1; double w3 = 0; for (double a = 0.01; a < 1; a *= 1.15) { Core k = mk(q, nullptr, 1.0); double r, t; sine(k, a, 1000, 0.3, r, t); if (t > 0.05) break; w3 = r * r / 8; } chk("NORMAL: Leistung bei 5 % Klirr (100-W-Verstärker)", w3, 100, 25, "W"); }
  std::printf("\n%d Abweichungen\n", bad); return bad ? 1 : 0; }
