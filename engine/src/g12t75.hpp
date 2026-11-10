// Lautsprecher des G100-212II (ab G100 1.10): 2 × Celestion G12T-75, 8 Ω, parallel, in der offenen Box des Combos.
// Herleitung und Quellen: docs/g100/LAUTSPRECHER.md. Kette (Eingang: Spannung der Endstufe vor dem Ausgangselko):
//  1. Elektrisch/mechanisch: Ausgangselko 2200 µ → zwei gleiche Lautsprecher parallel (Re, Bewegung als Parallelkreis Res ∥ Lces ∥ Cmes,
//     Thiele-Small des 8-Ω-Satzes aus der Datenbank petoindominique.fr: Fs 85 Hz, Re 6,8 Ω, Qms 10,6, Qes 0,63). Ausgang: Membranbeschleunigung
//     (Schalldruck auf großer Schallwand), oben auf 1 normiert. Rationale Funktion 3. Ordnung, exakt.
//  2. Korrektur K: Celestion-Datenblatt „8 Ω Frequency Response“ (aus dem Bild abgelesen, ±0,5 dB) geteilt durch Stufe 1 ohne Elko,
//     angepasst als Verstärkung, zwei Tiefpässe 2. Ordnung und 10 Glockenfilter (20 Hz … 13 kHz): bis 7 kHz 0,3–0,5 dB rms, höchstens 1,1 dB daneben.
//     Darin stecken Membranaufbruch, Schwingspuleninduktivität, der Abfall über 5 kHz und der Rest des Bassabfalls, den Thiele-Small nicht trifft.
//  3. Offene Rückwand (geschätzt): Dipol, Rückwelle mit Umweg D = 0,6 m (Box 687 × 560 × 280 mm laut Service Manual: durch die Box
//     zur Rückseite, außen an der Seitenwand nach vorn) und Beugungsverlust als Tiefpass 1. Ordnung bei 170 Hz (Kantenstufe der 687 mm
//     breiten Front, 115/B); die Verzögerungsleitung trägt den Rest des Umwegs, so dass bei tiefen Tönen genau D gilt: p = x − e^{−sτ'}·LP(x).
// Ausgang auf die Spannung an den Lautsprecherklemmen (Elko an 4 Ω) bei 1 kHz normiert: gleicher Pegel bei 1 kHz wie ohne Lautsprecher.
#pragma once
#include <cmath>
#include <complex>
#include <vector>
#include "core.hpp"   // aor::IIR, aor::bilinear

namespace g12t {
constexpr double PI_ = 3.14159265358979323846;
struct Params {
  double re = 6.8, fs = 85, qms = 10.6, qes = 0.63; int n = 2;   // je Lautsprecher; n parallel
  double cout = 2200e-6, rload = 4;   // Ausgangselko der Endstufe; Bezug ohne Lautsprecher: Elko an 4 Ω
  double d = 0.6, fa = 170, c = 343;   // offene Rückwand: Umweg (m), Beugungsecke (Hz), Schallgeschwindigkeit
  double g = -0.102669;   // Korrektur K (dB) …
  double lp[2][2] = {{2124.77, 0.852794}, {7260.48, 3.42733}};   // Tiefpässe 2. Ordnung (Hz, Q)
  double pk[10][3] = {{30, 1.24591, 2.41124}, {380.241, 3.02511, -2.35275}, {723.805, 0.3, 1.80535}, {1298.77, 4.64602, -5.08763}, {2177.18, 6.85603, 4.72527},
    {2414.73, 6.27768, 5.998}, {3161.51, 3.06041, 9.78171}, {4885.35, 3.16679, 10.7734}, {6641.8, 3.11875, -15}, {12322.7, 5.29294, 15}};   // Glocken (Hz, Q, dB)
};
// Bauteile des Ersatzschaltbilds (beide Lautsprecher parallel zusammengefasst)
struct Elem { double re, res, lc, cm; };
inline Elem elem(const Params &p) { double ws = 2 * PI_ * p.fs, res = p.re * p.qms / p.qes, cm = p.qes / (ws * p.re), lc = 1 / (ws * ws * cm);
  return {p.re / p.n, res / p.n, lc / p.n, cm * p.n}; }
// Stufe 1 als s-Polynom (aufsteigend): s³·L·Co·Re·C / [1 + s(L/R + Co·Re) + s²(LC + L·Co·Re/R + L·Co) + s³·LC·Co·Re]
inline void stage1(const Params &p, double *b, double *a) { Elem e = elem(p); double L = e.lc, C = e.cm, R = e.res, Re = e.re, Co = p.cout;
  b[0] = b[1] = b[2] = 0; b[3] = L * Co * Re * C; a[0] = 1; a[1] = L / R + Co * Re; a[2] = L * C + L * Co * Re / R + L * Co; a[3] = L * C * Co * Re; }
inline std::complex<double> peakS(std::complex<double> s, double fc, double q, double gdb) { double A = std::pow(10.0, gdb / 40); std::complex<double> x = s / (2 * PI_ * fc); return (x * x + x * (A / q) + 1.0) / (x * x + x / (A * q) + 1.0); }
inline std::complex<double> lpS(std::complex<double> s, double fc, double q) { std::complex<double> x = s / (2 * PI_ * fc); return 1.0 / (x * x + x / q + 1.0); }
inline double tauDl(const Params &p) { return std::max(0.0, p.d / p.c - 1 / (2 * PI_ * p.fa)); }   // Verzögerungsleitung (s)
// Schalldruck je Volt der Endstufe (analog, ohne Normierung)
inline std::complex<double> acoustic(double f, const Params &p) { std::complex<double> s(0, 2 * PI_ * f); double b[4], a[4]; stage1(p, b, a);
  std::complex<double> h = (b[3] * s * s * s) / (a[0] + s * (a[1] + s * (a[2] + s * a[3]))); h *= std::pow(10.0, p.g / 20); h *= lpS(s, p.lp[0][0], p.lp[0][1]) * lpS(s, p.lp[1][0], p.lp[1][1]);
  for (auto &k : p.pk) h *= peakS(s, k[0], k[1], k[2]);
  return h * (1.0 - std::exp(-s * tauDl(p)) / (1.0 + s / (2 * PI_ * p.fa))); }
inline std::complex<double> terminals(double f, const Params &p) { std::complex<double> s(0, 2 * PI_ * f); double t = p.cout * p.rload; return s * t / (1.0 + s * t); }
inline double norm(const Params &p) { return std::abs(terminals(1000, p)) / std::abs(acoustic(1000, p)); }
// Für Diagramme: Schalldruck relativ zur Klemmenspannung ohne Lautsprecher (Elko an 4 Ω), gleiche Zahlen wie Speaker
inline std::complex<double> rel(double f, const Params &p = Params()) { return acoustic(f, p) * norm(p) / terminals(f, p); }

// Zeitbereich (bei der Rechenrate des Kerns, auch überabgetastet)
struct Speaker {
  aor::IIR s1, lp1, lp2, pk[10], dlp; double gain = 1; std::vector<double> dl; int dpos = 0, dn = 0; double frac = 0; bool ready = false;
  // Abschnitt 2. Ordnung mit angepasstem Betrag (M. Vicanek, „Matched Second Order Digital Filters“, 2016): Pole exakt (z = e^{sT}),
  // Zähler so, dass der Betrag bei 0 Hz, bei fs/2 und bei fc dem analogen gleicht. Die bilineare Transformation verzerrte bei 44,1/48 kHz
  // die Glocken über 2 kHz (bis 1 dB unter 5 kHz, 6 dB zwischen 5 und 10 kHz).
  template <class H> static void matched(aor::IIR &f, double fc, double qd, double fs, H h) {
    double w0 = 2 * PI_ * fc / fs, z = 1 / (2 * qd), e = std::exp(-z * w0), a1 = z <= 1 ? -2 * e * std::cos(std::sqrt(1 - z * z) * w0) : -2 * e * std::cosh(std::sqrt(z * z - 1) * w0), a2 = e * e;
    double A0 = (1 + a1 + a2) * (1 + a1 + a2), A1 = (1 - a1 + a2) * (1 - a1 + a2), A2 = -4 * a2, s = std::sin(w0 / 2), p1 = s * s, p0 = 1 - p1, p2 = 4 * p0 * p1;
    auto m2 = [&](double fr) { double m = std::abs(h(fr)); return m * m; };
    double B0 = m2(0) * A0, B1 = m2(fs / 2) * A1, B2 = (m2(fc) * (A0 * p0 + A1 * p1 + A2 * p2) - B0 * p0 - B1 * p1) / p2;
    double r0 = std::sqrt(B0), r1 = std::sqrt(B1), W = (r0 + r1) / 2, b0 = (W + std::sqrt(std::max(0.0, W * W + B2))) / 2;
    f.n = 2; f.b[0] = b0; f.b[1] = (r0 - r1) / 2; f.b[2] = W - b0; f.a[0] = 1; f.a[1] = a1; f.a[2] = a2; }
  void design(double fs, const Params &p = Params()) {
    double b[4], a[4]; stage1(p, b, a); s1.n = 3; aor::bilinear(b, a, 3, fs, s1.b, s1.a);
    for (int i = 0; i < 2; i++) { double fc = p.lp[i][0], q = p.lp[i][1]; matched(i ? lp2 : lp1, fc, q, fs, [&](double fr) { return lpS(std::complex<double>(0, 2 * PI_ * fr), fc, q); }); }
    for (int i = 0; i < 10; i++) { double fc = p.pk[i][0], q = p.pk[i][1], g = p.pk[i][2], A = std::pow(10.0, g / 40);
      matched(pk[i], fc, A * q, fs, [&](double fr) { return peakS(std::complex<double>(0, 2 * PI_ * fr), fc, q, g); }); }
    { double t = 1 / (2 * PI_ * p.fa), b1[2] = {1, 0}, a1[2] = {1, t}; dlp.n = 1; aor::bilinear(b1, a1, 1, fs, dlp.b, dlp.a); }
    double D = tauDl(p) * fs; dn = (int)std::floor(D); frac = D - dn; dl.assign(dn + 2, 0.0); dpos = 0;
    gain = std::pow(10.0, p.g / 20) * norm(p); reset(); ready = true; }
  void reset() { s1.reset(); lp1.reset(); lp2.reset(); for (auto &f : pk) f.reset(); dlp.reset(); std::fill(dl.begin(), dl.end(), 0.0); dpos = 0; }
  double step(double x) {
    double y = lp2.step(lp1.step(s1.step(x))); for (auto &f : pk) y = f.step(y); y *= gain;
    double r = dlp.step(y); const int N = (int)dl.size(); dl[dpos] = r;   // Rückwelle: Beugungstiefpass, dann Laufzeit (linear zwischen zwei Abtastwerten)
    int i0 = dpos - dn; if (i0 < 0) i0 += N; int i1 = i0 - 1; if (i1 < 0) i1 += N; double back = dl[i0] + frac * (dl[i1] - dl[i0]);
    if (++dpos >= N) dpos = 0; return y - back; }
};
} // namespace g12t
