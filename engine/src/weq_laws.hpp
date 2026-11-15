// Reglerkennlinien und Bereiche des Wien-EQ (gemeinsam für Kern weq_core.hpp und Oberfläche weqgui.hpp, auch für mac/test/render.mm).
// Herleitung in weq_core.hpp und docs/wieneq/ENTWURF.md.
#pragma once
#include <cmath>
#include <algorithm>
namespace weq {
static const double MID_LO[3] = {20, 60, 250}, MID_HI[3] = {2000, 7000, 25000};
static const double RANGE_DB[3] = {6, 12, 20};
constexpr double QLH = 1.0 / 3;   // Glocken von LOW und HIGH (mit Taste)
// monotone kubische Interpolation (Fritsch–Carlson) durch n Stützstellen x[] (aufsteigend), y[]
inline double pchip(const double *x, const double *y, int n, double t) {
  if (t <= x[0]) return y[0];
  if (t >= x[n - 1]) return y[n - 1];
  int k = 0; while (t > x[k + 1]) k++;
  auto del = [&](int i) { return (y[i + 1] - y[i]) / (x[i + 1] - x[i]); };
  auto der = [&](int i) { if (i == 0 || i == n - 1) { int a = i == 0 ? 0 : n - 2, b = i == 0 ? 1 : n - 3; double h0 = x[a + 1] - x[a], h1 = x[b + 1] - x[b], d0 = del(a), d1 = del(b);
        double d = ((2 * h0 + h1) * d0 - h0 * d1) / (h0 + h1); if (d * d0 <= 0) return 0.0; if (d0 * d1 < 0 && std::fabs(d) > 3 * std::fabs(d0)) return 3 * d0; return d; }
    double d0 = del(i - 1), d1 = del(i); if (d0 * d1 <= 0) return 0.0; double h0 = x[i] - x[i - 1], h1 = x[i + 1] - x[i], w1 = 2 * h1 + h0, w2 = h1 + 2 * h0; return (w1 + w2) / (w1 / d0 + w2 / d1); };
  double h = x[k + 1] - x[k], u = (t - x[k]) / h, u2 = u * u, u3 = u2 * u;
  return (2 * u3 - 3 * u2 + 1) * y[k] + (u3 - 2 * u2 + u) * h * der(k) + (-2 * u3 + 3 * u2) * y[k + 1] + (u3 - u2) * h * der(k + 1); }
// Frequenzregler (ADT-Skala): Anteil ln(f/fu)/ln(fo/fu) an den sieben Strichen, Mittel aus MID 1 und MID 2
static const double FX[7] = {0, 1.0 / 6, 2.0 / 6, 3.0 / 6, 4.0 / 6, 5.0 / 6, 1}, FU[7] = {0, 0.097686, 0.195742, 0.393836, 0.527882, 0.737642, 1};
// Pegelregler (ADT-Skala ±12 dB: 4 / 9 / 12 dB am 1. / 2. / 3. Strich), auf den Bereich bezogen, ungerade fortgesetzt
static const double GX[7] = {-1, -2.0 / 3, -1.0 / 3, 0, 1.0 / 3, 2.0 / 3, 1}, GY[7] = {-1, -0.75, -1.0 / 3, 0, 1.0 / 3, 0.75, 1};
constexpr double QMIN = 0.1824791693, QMAX = 4.318473046;   // N Oktaven: Q = 1 / (2 sinh(N ln 2 / 2)); 5 Oktaven bzw. Terz (N = 1/3)
inline double gainDb(double p, int range) { p = std::max(-1.0, std::min(1.0, p)); return RANGE_DB[std::max(0, std::min(2, range))] * pchip(GX, GY, 7, p); }
inline double midFreq(int k, double x) { x = std::max(0.0, std::min(1.0, x)); return MID_LO[k] * std::pow(MID_HI[k] / MID_LO[k], pchip(FX, FU, 7, x)); }
inline double midQ(double x) { x = std::max(0.0, std::min(1.0, x)); return QMIN * std::pow(QMAX / QMIN, x); }
} // namespace weq
