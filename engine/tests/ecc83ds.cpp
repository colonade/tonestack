// 12AX7/ECC83 gegen das Datenblatt (ab 2026-11-17, Anlass: Vergleich mit dem LTspice-Modell des Kritikers, siehe PROJEKTSTAND).
// Werte aus dem Amperex-Datenblatt „12AX7/ECC83“ (Tabellen „Class A resistance-coupled amplifier“, Seite 3): Ebb, Rp, Rg, Rk, Ip, Eo (eff., „at grid current
// starting point“), Verstärkung, % Distortion; Fußnote 7: Klirr etwa proportional zur Ausgangsspannung. Dazu RCA-Arbeitspunkte (250 V/−2 V, 100 V/−1 V).
// Schaltung wie im Datenblatt: Gitter angesteuert, Rk mit 50 µF überbrückt, Ausgang über 0,01 µF an Rg, 1 kHz. Gerechnet quasistatisch: Kathode und
// Koppelkondensator halten ihre Gleichspannung, die sich selbstkonsistent einstellt (Uk = Rk·Mittel(Ia), Uc = Mittel(Ua)); gegen eine Zeitrechnung
// (Trapezregel, Newton, 192 kHz) geprüft: Klirr auf 0,06 Prozentpunkte gleich.
// Prüft:
//  1. Alle Kerne mit 12AX7 haben dieselben Parameter (Laney core.hpp und laney_pa.hpp, Loldano, SVT-CL, Wisent) – eine Neuanpassung muss alle ändern.
//  2. Bezug (Stand 2026-11-17): Ruhestrom, Verstärkung, Klirr bei Eo und bei 70 % von Eo auf ±0,1 % (relativ; Rechnung ist deterministisch) – keine unbemerkte
//     Änderung der Röhre (γ 1,26 → 1,27 verschiebt den Klirr um etwa 0,2 %). Nach einer gewollten Neuanpassung hier neu eintragen.
//  3. Abstand zum Datenblatt in Grenzen: Klirr −25 … +35 %, Ruhestrom ±10 %, Verstärkung ±12 %, RCA-Strom ±10 %, Steilheit ±30 %.
// Aufruf: ecc83ds [-v] (mit -v je Zeile alle Werte)
#include "core.hpp"
#include "laney_pa.hpp"
#include "slo_core.hpp"
#include "svt_core.hpp"
#include "wisent_core.hpp"
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace {
const b15::TriodeD &TUBE = slo::T12AX7;
double ia(double vgk, double vak) { double o[3]; b15::dempwolf(TUBE, vgk, vak, o); return o[0]; }
struct Q { double ip, eo, gain, thd; };
// quasistatisch, Gitterspitze A
Q stage(double Ebb, double Rp, double Rk, double Rg, double A) {
  const int N = 192; double Vk0; { double lo = 0, hi = Ebb / Rp; for (int i = 0; i < 100; i++) { double m = 0.5 * (lo + hi); if (m - ia(-m * Rk, Ebb - m * (Rp + Rk)) > 0) hi = m; else lo = m; } Vk0 = 0.5 * (lo + hi) * Rk; }
  std::vector<double> va(N); double Vc = Ebb - Vk0 / Rk * Rp, mi = 0;
  auto sweep = [&](double Vk) { for (int pass = 0; pass < 40; pass++) { double si = 0, sv = 0;
      for (int n = 0; n < N; n++) { double vg = A * std::sin(2 * M_PI * n / N), lo = Vk, hi = Ebb;   // (Ebb−Ua)/Rp − (Ua−Uc)/Rg − Ia fällt monoton in Ua
        for (int i = 0; i < 48; i++) { double m = 0.5 * (lo + hi); if ((Ebb - m) / Rp - (m - Vc) / Rg - ia(vg - Vk, m - Vk) > 0) lo = m; else hi = m; }
        va[n] = 0.5 * (lo + hi); si += ia(vg - Vk, va[n] - Vk); sv += va[n]; }
      double nVc = sv / N, d = std::fabs(nVc - Vc); Vc = nVc; mi = si / N; if (d < 1e-7) break; } };
  double lo = 0, hi = 3 * Vk0 + 1; for (int it = 0; it < 40 && hi - lo > 1e-6; it++) { double m = 0.5 * (lo + hi); sweep(m); if (Rk * mi - m > 0) lo = m; else hi = m; }
  sweep(0.5 * (lo + hi));
  double re[11] = {}, im[11] = {}, ss = 0; for (int n = 0; n < N; n++) { double v = va[n] - Vc; ss += v * v; for (int m = 1; m <= 10; m++) { re[m] += v * std::cos(2 * M_PI * m * n / N); im[m] += v * std::sin(2 * M_PI * m * n / N); } }
  double h1 = 2 * std::hypot(re[1], im[1]) / N, t = 0; for (int m = 2; m <= 10; m++) { double hm = 2 * std::hypot(re[m], im[m]) / N; t += hm * hm; }
  return {Vk0 / Rk * 1e3, std::sqrt(ss / N), h1 / A, 100 * std::sqrt(t) / h1}; }
Q atEo(double Ebb, double Rp, double Rk, double Rg, double Eo) { double lo = 0.02, hi = 3; for (int i = 0; i < 26; i++) { double m = std::sqrt(lo * hi); if (stage(Ebb, Rp, Rk, Rg, m).eo < Eo) lo = m; else hi = m; } return stage(Ebb, Rp, Rk, Rg, std::sqrt(lo * hi)); }
struct Row { double Rp, Rg, Ebb, Rk, Ip, Eo, Gain, D; double bIp, bGain, bThd, bThd70; };
// Datenblatt (Amperex: Rp, Rg, Ebb, Rk, Ip mA, Eo V eff., Verstärkung, Klirr %) | Bezug unseres Modells (Stand 2026-11-17: Ruhestrom mA, Verstärkung, Klirr % bei Eo und bei 70 % von Eo)
Row R[15] = {
    {47e3, 150e3, 200, 1500, 0.86, 18, 34, 8.5, 0.860863, 31.3644, 10.326, 6.79421}, {47e3, 150e3, 250, 1200, 1.18, 23, 37.5, 7.0, 1.18639, 36.0441, 8.05206, 5.24865}, {47e3, 150e3, 300, 1000, 1.55, 26, 40, 5.0, 1.55046, 39.9473, 5.74603, 3.76759},
    {47e3, 150e3, 350, 820, 1.98, 33, 42.5, 4.4, 1.98955, 42.9685, 4.81658, 3.12517}, {47e3, 150e3, 400, 680, 2.45, 37, 44, 3.6, 2.48081, 45.5603, 3.58825, 2.34469},
    {100e3, 330e3, 200, 1800, 0.65, 20, 50, 4.8, 0.662325, 45.902, 5.6457, 3.81813}, {100e3, 330e3, 250, 1500, 0.86, 26, 54.5, 3.9, 0.874366, 50.6685, 4.71527, 3.16812}, {100e3, 330e3, 300, 1200, 1.11, 30, 57, 2.7, 1.14065, 55.0313, 3.43518, 2.3161},
    {100e3, 330e3, 350, 1000, 1.40, 36, 61, 2.2, 1.42359, 58.1968, 2.79775, 1.88334}, {100e3, 330e3, 400, 820, 1.72, 38, 63, 1.7, 1.74821, 60.9205, 2.00669, 1.36113},
    {220e3, 680e3, 200, 3300, 0.36, 24, 56, 4.6, 0.389219, 55.061, 4.94253, 3.34444}, {220e3, 680e3, 250, 2700, 0.48, 28, 66.5, 3.4, 0.509898, 60.3871, 3.67161, 2.48991}, {220e3, 680e3, 300, 2200, 0.63, 36, 72, 2.6, 0.650557, 64.5901, 3.14296, 2.12068},
    {220e3, 680e3, 350, 1500, 0.85, 37, 75.5, 1.6, 0.863273, 69.1957, 1.91691, 1.30949}, {220e3, 680e3, 400, 1200, 1.02, 38, 76.5, 1.1, 1.04931, 71.8958, 1.36846, 0.940261}};
}  // namespace

int main(int argc, char **argv) { const bool verbose = argc > 1 && !std::strcmp(argv[1], "-v"); int bad = 0;
  // 1. gleiche Parameter in allen Kernen
  { const b15::TriodeD *T[] = {&lpa::T12AX7, &slo::T12AX7, &svt::T12AX7, &wisent::T12AX7}; const char *nm[] = {"laney_pa", "slo", "svt", "wisent"}; const aor::TubeModel &L = aor::TUBE_MODELS[0];
    bool same = L.form == 1 && L.G == TUBE.G && L.C == TUBE.C && L.ga == TUBE.ga && L.mu == TUBE.mu;
    for (int k = 0; k < 4; k++) if (T[k]->G != TUBE.G || T[k]->C != TUBE.C || T[k]->mu != TUBE.mu || T[k]->ga != TUBE.ga) { same = false; printf("12AX7 in %s weicht ab\n", nm[k]); }
    printf("12AX7 (G %.6e, C %.5f, mu %g, gamma %g) in core.hpp, laney_pa, slo, svt, wisent: %s\n", TUBE.G, TUBE.C, TUBE.mu, TUBE.ga, same ? "gleich" : "VERSCHIEDEN"); if (!same) bad++; }
  // 2./3. Tabelle
  Q q[15], q7[15]; { std::vector<std::thread> th; for (int k = 0; k < 15; k++) th.emplace_back([k, &q, &q7] { const Row &r = R[k]; q[k] = atEo(r.Ebb, r.Rp, r.Rk, r.Rg, r.Eo); q7[k] = atEo(r.Ebb, r.Rp, r.Rk, r.Rg, 0.7 * r.Eo); }); for (auto &t : th) t.join(); }
  double sumDev = 0, maxDev = -1e9, minDev = 1e9; int badDs = 0, badRef = 0;
  for (int k = 0; k < 15; k++) { const Row &r = R[k]; double dT = q[k].thd / r.D - 1, dT7 = q7[k].thd / (0.7 * r.D) - 1, dI = q[k].ip / r.Ip - 1, dG = q[k].gain / r.Gain - 1;
    sumDev += dT; maxDev = std::max(maxDev, dT); minDev = std::min(minDev, dT);
    bool ds = dT > -0.25 && dT < 0.35 && dT7 > -0.25 && dT7 < 0.35 && std::fabs(dI) < 0.10 && std::fabs(dG) < 0.12;
    bool ref = r.bIp == 0 || (std::fabs(q[k].ip / r.bIp - 1) < 1e-3 && std::fabs(q[k].gain / r.bGain - 1) < 1e-3 && std::fabs(q[k].thd / r.bThd - 1) < 1e-3 && std::fabs(q7[k].thd / r.bThd70 - 1) < 1e-3);
    if (!ds) badDs++; if (!ref) badRef++;
    if (verbose || !ds || !ref) printf("  Rp %3.0fk Rg %3.0fk %3.0f V Rk %4.0f: Ip %.3f mA (%+4.1f %%), Verst. %.2f (%+4.1f %%), Klirr bei Eo %.3f %% (%+4.1f %%), bei 70 %% %.3f %% (%+4.1f %%)%s%s\n",
                                       r.Rp / 1e3, r.Rg / 1e3, r.Ebb, r.Rk, q[k].ip, 100 * dI, q[k].gain, 100 * dG, q[k].thd, 100 * dT, q7[k].thd, 100 * dT7, ds ? "" : "  DATENBLATT", ref ? "" : "  BEZUG"); }
  printf("Amperex-Tabelle, 15 Zeilen: Klirr bei Eo im Mittel %+.1f %% (%+.1f … %+.1f %%) gegen das Datenblatt; außerhalb der Grenzen %d, Bezug verletzt %d\n", 100 * sumDev / 15, 100 * minDev, 100 * maxDev, badDs, badRef);
  if (badDs || badRef) bad++;
  // RCA-Arbeitspunkte
  { struct P { double va, vg, ia, gm; } P2[] = {{250, -2, 1.2e-3, 1.6e-3}, {100, -1, 0.5e-3, 1.25e-3}};
    for (auto &p : P2) { double i = ia(p.vg, p.va), g = (ia(p.vg + 1e-4, p.va) - ia(p.vg - 1e-4, p.va)) / 2e-4; bool ok = std::fabs(i / p.ia - 1) < 0.10 && std::fabs(g / p.gm - 1) < 0.30;
      printf("RCA %3.0f V / %2.0f V: Ia %.3f mA (%+.0f %%), gm %.2f mA/V (%+.0f %%)%s\n", p.va, p.vg, i * 1e3, 100 * (i / p.ia - 1), g * 1e3, 100 * (g / p.gm - 1), ok ? "" : "  ABWEICHUNG"); if (!ok) bad++; } }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
