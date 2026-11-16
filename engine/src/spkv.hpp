// Lautsprecher als Filter auf die Klemmenspannung (Spannungsquelle an den Klemmen): Schalldruck auf großer Schallwand ∝ Membranbeschleunigung,
// geschlossene Box. Gebraucht, wo keine Endstufe mitgerechnet wird (Abgriff nach der Vorstufe) oder die Last schon im Netz steckt und nur der
// Schall fehlt (ab 2026-11-16: AmpFett B-15NF bei Vorstufen-Abgriff, AmpFett SVT-CL, Loldano).
// Ersatzschaltbild wie im AmpFett-Kern: Re, Le ∥ R2 (Wirbelstromverlust), Bewegung als Parallelkreis Res ∥ Lces ∥ Cmes (Box über α = Vas/Vb
// in Fc, Qec, Qmc eingerechnet). Beschleunigung ∝ s · Cmes · (Spannung am Bewegungskreis). Rationale Funktion 3. Ordnung, exakt;
// dazu optional ein Kegel-Tiefpass 2. Ordnung (Membranaufbruch, Schätzung) und eine Korrektur (Glockenfilter, z. B. aus einem Datenblatt).
// Auf 1 kHz normiert: gleicher Pegel bei 1 kHz wie die Klemmenspannung. n Lautsprecher gleicher Art in Reihe/parallel ändern nur die Impedanz,
// nicht die Form: hier je Lautsprecher gerechnet.
#pragma once
#include <cmath>
#include <complex>
#include <vector>

namespace spkv {
constexpr double PI_ = 3.14159265358979323846;
// Bilineare Transformation und IIR wie bilinear/IIR im Laney-Kern (eigene Kopie: dieser Kopf wird auch von Kernen eingebunden, die core.hpp nicht kennen)
inline void bilinear(const double *bs, const double *as, int n, double fs, double *bz, double *az) {
  double K = 2 * fs; double num[5] = {0,0,0,0,0}, den[5] = {0,0,0,0,0};
  for (int k = 0; k <= n; k++) { double poly[5] = {1,0,0,0,0}; int deg = 0;
    for (int i = 0; i < k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] -= poly[j-1]; deg++; }
    for (int i = 0; i < n - k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] += poly[j-1]; deg++; }
    double pk = std::pow(K, k); for (int j = 0; j <= n; j++) { num[j] += bs[k] * pk * poly[j]; den[j] += as[k] * pk * poly[j]; } }
  for (int j = 0; j <= n; j++) { bz[j] = num[j] / den[0]; az[j] = den[j] / den[0]; } }
struct IIR { int n = 0; double b[5] = {0,0,0,0,0}, a[5] = {1,0,0,0,0}, z[5] = {0,0,0,0,0};
  double step(double x) { if (n == 0) return x; double y = b[0] * x + z[0]; for (int i = 1; i < n; i++) z[i-1] = b[i] * x + z[i] - a[i] * y; z[n-1] = b[n] * x - a[n] * y; return y; }
  void reset() { for (double &q : z) q = 0; } };
struct Params { double re = 6.3, le = 1.0e-3, r2 = 20, fs = 40, qms = 5, qes = 0.55, vas = 280, vb = 85;   // vb ≤ 0: ohne Box (große Schallwand)
  double fc = 3000, qc = 0.7;   // Kegel-Tiefpass (fc ≤ 0: keiner)
  std::vector<std::vector<double>> pk;   // Glocken {Hz, Q, dB}
  double hp = 0, qhp = 0.7; };           // Hochpass 2. Ordnung (Hz, 0 = keiner), z. B. Tiefenabfall offener Boxen
// Analoge Stufe 1 (Spannung → Beschleunigung) als Polynome in s (aufsteigend)
inline void stage1(const Params &p, double *b, double *a) {
  double al = p.vb > 0 ? p.vas / p.vb : 0, fc = p.fs * std::sqrt(1 + al), qec = p.qes * std::sqrt(1 + al), qmc = p.qms * std::sqrt(1 + al);
  double res = p.re * qmc / qec, cm = qec / (2 * PI_ * fc * p.re), lm = 1 / ((2 * PI_ * fc) * (2 * PI_ * fc) * cm);
  // Zm = s·lm·res / (res + s·lm + s²·lm·cm·res); Zl = s·le·r2 / (r2 + s·le); Vm/V = Nm·Dl / (re·Dm·Dl + Nl·Dm + Nm·Dl)
  double Nm[3] = {0, lm * res, 0}, Dm[3] = {res, lm, lm * cm * res}, Nl[2] = {0, p.le * p.r2}, Dl[2] = {p.r2, p.le};
  double den[4] = {0, 0, 0, 0}, num[4] = {0, 0, 0, 0};
  for (int i = 0; i < 3; i++) for (int j = 0; j < 2; j++) { den[i + j] += p.re * Dm[i] * Dl[j] + Nm[i] * Dl[j] + Dm[i] * Nl[j]; num[i + j] += Nm[i] * Dl[j]; }
  // mal s·cm
  b[0] = 0; for (int k = 0; k < 3; k++) b[k + 1] = cm * num[k]; for (int k = 0; k < 4; k++) a[k] = den[k]; }
inline std::complex<double> poly(const double *c, int n, std::complex<double> s) { std::complex<double> r = 0; for (int k = n; k >= 0; k--) r = r * s + c[k]; return r; }
// analoger Frequenzgang (ohne Normierung)
inline std::complex<double> analog(const Params &p, double f) { std::complex<double> s(0, 2 * PI_ * f); double b[4], a[4]; stage1(p, b, a); std::complex<double> h = poly(b, 3, s) / poly(a, 3, s);
  if (p.fc > 0) { double w = 2 * PI_ * p.fc; h *= w * w / (w * w + s * (w / p.qc) + s * s); }
  if (p.hp > 0) { double w = 2 * PI_ * p.hp; h *= s * s / (w * w + s * (w / p.qhp) + s * s); }
  for (auto &q : p.pk) { double w = 2 * PI_ * q[0], A = std::pow(10, q[2] / 40); h *= (s * s + s * (A * w / q[1]) + w * w) / (s * s + s * (w / (A * q[1])) + w * w); }
  return h; }
struct Filter { IIR s1, lp, hp; std::vector<IIR> pk; double g = 1; Params p; double fsR = 48000;
  void design(const Params &p_, double fs) { p = p_; fsR = fs; double b[4], a[4]; stage1(p, b, a);
    // vorverzerrt auf 1 kHz: s → s·(ω0/K)/tan(ω0/(2fs))·…; einfacher: bilinear mit K = 2fs, die Abweichung oben deckt der Kegel-Tiefpass (Schätzung) ab
    s1.n = 3; bilinear(b, a, 3, fs, s1.b, s1.a);
    lp.n = 0; if (p.fc > 0) { double w = 2 * PI_ * p.fc, k = w / std::tan(w / (2 * fs)) / (2 * fs); w /= k; double b2[3] = {w * w, 0, 0}, a2[3] = {w * w, w / p.qc, 1}; lp.n = 2; bilinear(b2, a2, 2, fs, lp.b, lp.a); }
    hp.n = 0; if (p.hp > 0) { double w = 2 * PI_ * p.hp; double b2[3] = {0, 0, 1}, a2[3] = {w * w, w / p.qhp, 1}; hp.n = 2; bilinear(b2, a2, 2, fs, hp.b, hp.a); }
    pk.clear(); for (auto &q : p.pk) { double w = 2 * PI_ * q[0], k = w / std::tan(std::min(1.5, w / (2 * fs))) / (2 * fs); w /= k; double A = std::pow(10, q[2] / 40);
      double b2[3] = {w * w, A * w / q[1], 1}, a2[3] = {w * w, w / (A * q[1]), 1}; IIR f; f.n = 2; bilinear(b2, a2, 2, fs, f.b, f.a); pk.push_back(f); }
    g = 1; g = 1 / std::abs(response(1000)); reset(); }
  // Frequenzgang des digitalen Filters (mit Normierung)
  std::complex<double> response(double f) const { std::complex<double> z = std::polar(1.0, -2 * PI_ * f / fsR), h = g;
    auto one = [&](const IIR &q) { if (q.n == 0) return; std::complex<double> nb = 0, na = 0, zk = 1; for (int k = 0; k <= q.n; k++) { nb += q.b[k] * zk; na += q.a[k] * zk; zk *= z; } h *= nb / na; };
    one(s1); one(lp); one(hp); for (auto &q : pk) one(q); return h; }
  void reset() { s1.reset(); lp.reset(); hp.reset(); for (auto &q : pk) q.reset(); }
  double step(double x) { double y = s1.step(x); y = lp.step(y); y = hp.step(y); for (auto &q : pk) y = q.step(y); return g * y; } };
}  // namespace spkv
