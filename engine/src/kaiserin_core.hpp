// Kaiserin Reverb: Hall nach den veröffentlichten Angaben zum Empress Effects Reverb (Bedienung: MODE, DECAY, MIX, OUTPUT, LOW, HI, THING 1, THING 2,
// Halten über den Fußschalter). Das Original ist ein Digitalgerät mit unveröffentlichten Algorithmen; dies ist ein eigener Entwurf mit veröffentlichten
// Verfahren (docs/kaiserin/ENTWURF.md):
//   HALL, ROOM, MODULATED, SPARKLE: Rückkopplungsnetz aus 16 Verzögerungen (FDN nach Jot/Chaigne 1991, Hadamard-Matrix), Dämpfung je Leitung als
//     Tiefen-, Mitten- und Höhennachhallzeit (zwei Kuhschwanzfilter erster Ordnung), Eingangsdiffusion (Allpässe), frühe Reflexionen aus Abgriffen;
//     SPARKLE mit Oktave nach oben in der Rückkopplung (Tonhöhenverschiebung mit zwei überblendeten Leseköpfen).
//   PLATE: Plattenhall nach Dattorro (J. Audio Eng. Soc. 45/9, 1997, „Effect Design Part 1“), Verzögerungen auf die Abtastrate umgerechnet.
//   SPRING: Federhall aus spring.hpp (dispersive Allpass-Ketten, wie im Laney und G100), drei Federn, linear.
// Eingang mono (bei Stereo die Summe), Hall stereo; der trockene Anteil bleibt je Kanal. Regler 0 … 1 (12 Uhr = 0,5, „klingt in der Mitte gut“ wie beim Vorbild).
#pragma once
#include "spring.hpp"
#include <cmath>
#include <cstdint>
#include <complex>
#include <vector>
#include <algorithm>
namespace kai {
constexpr double PI = 3.14159265358979323846;
enum { M_HALL, M_PLATE, M_SPRING, M_ROOM, M_MOD, M_SPARKLE, NMODE };
static const char *const MODE_NAME[NMODE] = {"Hall", "Plate", "Spring", "Room", "Modulated", "Sparkle"};
// Bedeutung von THING 1 / THING 2 je Modus (Oberfläche, Seite)
static const char *const THING1[NMODE] = {"Modulation", "Pre-delay", "Rattle decay", "Pre-delay", "Mod rate", "Sparkle level"};
static const char *const THING2[NMODE] = {"Early reflections", "Bright early decay", "Rattle level", "Early reflections", "Mod depth", "Sparkle length"};
struct Setting { int mode = M_HALL; double decay = 0.5, mix = 0.5, out = 0.5, low = 0.5, hi = 0.5, t1 = 0.5, t2 = 0.5; bool hold = false; };

// ---- Kennlinien der Regler (Kern, Plugin-Anzeige und Seite rechnen damit) ----
inline double clamp01(double x) { return std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
inline double expMap(double x, double lo, double hi) { return lo * std::pow(hi / lo, clamp01(x)); }
// Nachhallzeit der Mitten (s): je Modus von … bis, 12 Uhr = geometrisches Mittel
static const double T60LO[NMODE] = {0.6, 0.5, 0.8, 0.15, 0.6, 1.0}, T60HI[NMODE] = {20, 10, 8, 4, 20, 30};   // PLATE kürzer als 0,5 s geht mit den Diffusoren nicht; SPRING misst etwa 10–25 % kürzer (Verluste im Tank)
inline double t60Mid(int m, double decay) { return expMap(decay, T60LO[m], T60HI[m]); }
inline double t60Ref(int m) { return std::sqrt(T60LO[m] * T60HI[m]); }
inline double lowRatio(double low) { return std::pow(2.0, (clamp01(low) - 0.5) * 2); }   // Tiefen: halbe bis doppelte Nachhallzeit
inline double hiRatio(double hi) { return 0.08 * std::pow(12.5, clamp01(hi)); }            // Höhen: 8 % (dunkel) … 28 % (Mitte) … 100 % (hell)
static const double XLOW[NMODE] = {300, 300, 300, 400, 300, 300}, XHIGH[NMODE] = {3500, 3500, 3500, 5000, 5000, 3000};   // Übergänge der Dämpfung (Hz)
inline double sparkleLenK(double t2) { return std::pow(5.0, (clamp01(t2) - 0.5) * 2); }
inline double hiRatioMode(int m, const Setting &s) { double r = hiRatio(s.hi); if (m == M_SPARKLE) r *= sparkleLenK(s.t2); return std::min(1.0, r); }
inline double preDelay(int m, double t1) { double x = clamp01(t1); return m == M_PLATE ? 0.25 * x * x : m == M_ROOM ? 0.12 * x * x : m == M_HALL || m == M_SPARKLE ? 0.020 : m == M_MOD ? 0.010 : 0.0; }   // s
inline double erLevel(int m, double t2) { return (m == M_HALL || m == M_ROOM) ? 2 * clamp01(t2) : 0.0; }
inline double modRate(double t1) { return expMap(t1, 0.1, 6.0); }      // MODULATED: Hz
inline double modDepth(double t2) { double x = clamp01(t2); return 0.004 * x * std::sqrt(x); }   // MODULATED: s (bis 4 ms)
inline double hallMod(double t1) { double x = clamp01(t1); return 0.0008 * x * x; }   // HALL: Tiefe bis 0,8 ms
inline double rattleT60(double t1) { return expMap(t1, 0.25, 5.0); }
inline double rattleLevel(double t2) { double x = clamp01(t2); return 0.6 * x * std::sqrt(x); }
inline double sparkleLevel(double t1) { return 0.9 * clamp01(t1); }
inline double outDb(double o) { return (clamp01(o) - 0.5) * 40; }   // OUTPUT: 12 Uhr = 0 dB, −20 … +20 dB
// MIX: links nur trocken, rechts nur Hall, gleich laut bei 2 Uhr (0,7) wie beim Vorbild; bis dahin bleibt der trockene Anteil voll
inline void mixGains(double m, double &dry, double &wet) { m = clamp01(m); if (m <= 0.7) { dry = 1; double r = m / 0.7; wet = r * r; } else { wet = 1; dry = (1 - m) / 0.3; } }

// ---- Bausteine ----
struct Line {   // Ringpuffer, tap(0) = jüngster Wert
  std::vector<float> b; uint32_t mask = 0, w = 0;
  void init(double maxLen) { uint32_t n = 16; while (n < (uint32_t)(maxLen + 8)) n <<= 1; b.assign(n, 0.f); mask = n - 1; w = 0; }
  void clear() { std::fill(b.begin(), b.end(), 0.f); w = 0; }
  inline void push(double x) { if (!(std::fabs(x) > 1e-15)) x = 0; b[w] = (float)x; w = (w + 1) & mask; }   // kleine Werte auf 0 (keine Subnormalen im wasm), NaN ebenso
  inline double tap(int d) const { return b[(w - 1 - (uint32_t)d) & mask]; }
  inline double read(int D) const { return tap(D - 1); }   // vor push gelesen: Verzögerung D ≥ 1
  inline double readF(double D) const {   // gebrochene Verzögerung D ≥ 2, Lagrange 3. Ordnung
    double d = D - 1; int i = (int)d; double p = d - i; double ym = tap(i - 1), y0 = tap(i), y1 = tap(i + 1), y2 = tap(i + 2);
    return -p * (p - 1) * (p - 2) / 6 * ym + (p + 1) * (p - 1) * (p - 2) / 2 * y0 - (p + 1) * p * (p - 2) / 2 * y1 + (p + 1) * p * (p - 1) / 6 * y2; }
};
struct AP {   // Schroeder-Allpass (z^−D − g)/(1 − g z^−D)
  Line l; int D = 1; double g = 0.5;
  void init(int D_, double g_, double extra = 0) { D = std::max(2, D_); g = g_; l.init(D + extra + 4); }
  inline double step(double x) { double d = l.read(D); double w = x + g * d; l.push(w); return d - g * w; }
  inline double stepMod(double x, double Df) { double d = l.readF(Df); double w = x + g * d; l.push(w); return d - g * w; }
};
struct Shelf1 {   // Kuhschwanz erster Ordnung (bilinear): lo = Gleichanteil-Verstärkung (Höhen 1) oder hi = Nyquist-Verstärkung (Tiefen 1)
  double b0 = 1, b1 = 0, a1 = 0, z = 0;
  void low(double A, double fc, double fs) { double K = std::tan(PI * std::min(fc, 0.45 * fs) / fs), n = 1 + K; b0 = (1 + A * K) / n; b1 = (A * K - 1) / n; a1 = (K - 1) / n; }
  void high(double B, double fc, double fs) { double K = std::tan(PI * std::min(fc, 0.45 * fs) / fs), n = 1 + K; b0 = (B + K) / n; b1 = (K - B) / n; a1 = (K - 1) / n; }
  void flat() { b0 = 1; b1 = 0; a1 = 0; }
  inline double step(double x) { double y = b0 * x + z; z = b1 * x - a1 * y; return y; }
  std::complex<double> at(double f, double fs) const { std::complex<double> e = std::polar(1.0, -2 * PI * f / fs); return (b0 + b1 * e) / (1.0 + a1 * e); }
};
struct Biquad {   // RBJ-Kochbuch
  double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void set(int type, double f, double fs, double dB = 0, double Q = 0.7071) {
    f = std::min(f, 0.45 * fs); double w = 2 * PI * f / fs, cw = std::cos(w), sw = std::sin(w), A = std::pow(10, dB / 40), al = sw / (2 * Q), B0, B1, B2, A0, A1, A2;
    if (type == 0) { B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }                 // Hochpass
    else if (type == 1) { B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }               // Tiefpass
    else if (type == 2) { double sa = 2 * std::sqrt(A) * al; B0 = A * ((A + 1) - (A - 1) * cw + sa); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sa);   // Tiefen-Kuhschwanz
      A0 = (A + 1) + (A - 1) * cw + sa; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - sa; }
    else { double sa = 2 * std::sqrt(A) * al; B0 = A * ((A + 1) + (A - 1) * cw + sa); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sa);   // Höhen-Kuhschwanz
      A0 = (A + 1) - (A - 1) * cw + sa; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sa; }
    b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
  inline double step(double x) { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
  void clear() { z1 = z2 = 0; }
  std::complex<double> at(double f, double fs) const { std::complex<double> e = std::polar(1.0, -2 * PI * f / fs), e2 = e * e; return (b0 + b1 * e + b2 * e2) / (1.0 + a1 * e + a2 * e2); }
};
struct Osc {   // Sinus über einen sich drehenden Zeiger (alle 256 Schritte nachnormiert)
  double c = 1, s = 0, cw = 1, sw = 0; int k = 0;
  void set(double f, double fs) { cw = std::cos(2 * PI * f / fs); sw = std::sin(2 * PI * f / fs); }
  void phase(double ph) { c = std::cos(ph); s = std::sin(ph); }
  inline double step() { double c2 = c * cw - s * sw; s = s * cw + c * sw; c = c2; if (++k >= 256) { k = 0; double r = 1 / std::sqrt(c * c + s * s); c *= r; s *= r; } return s; }
};
inline bool isPrime(int n) { if (n < 2) return false; for (int d = 2; d * d <= n; d++) if (n % d == 0) return false; return true; }
inline int nearPrime(int n) { for (int k = 0; k < 1000; k++) { if (isPrime(n + k)) return n + k; if (n - k > 2 && isPrime(n - k)) return n - k; } return n; }
struct Rng { uint32_t s; explicit Rng(uint32_t x) : s(x) {} double next() { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.0; } };

// ---- Rückkopplungsnetz (HALL, ROOM, MODULATED, SPARKLE) ----
// 16 Leitungslängen geometrisch zwischen kürzester und längster, je auf die nächste Primzahl (Abtastwerte)
static const double FDN_MS[NMODE][2] = {{30, 110}, {0, 0}, {0, 0}, {6, 35}, {25, 95}, {40, 125}};
inline int fdnLen(int m, int k, double fs) { double ms = FDN_MS[m][0] * std::pow(FDN_MS[m][1] / FDN_MS[m][0], k / 15.0); return nearPrime((int)std::lround(ms * 1e-3 * fs)); }
struct FDN {
  static const int N = 16;
  Line ln[N]; double L[N] = {}; Shelf1 lo[N], hs[N]; double g[N] = {}; Osc osc[N]; double depth = 0, depthT = 0;
  double y[N] = {};
  void init(double fs, int mode, double modMaxS) {
    for (int k = 0; k < N; k++) { L[k] = fdnLen(mode, k, fs); ln[k].init(L[k] + modMaxS * fs + 8); }
    for (int k = 0; k < N; k++) { osc[k].phase(2 * PI * k / N * 3.7); } }
  void clear() { for (int k = 0; k < N; k++) { ln[k].clear(); lo[k].z = 0; hs[k].z = 0; y[k] = 0; } }
  void setMod(double rate, double depthS, double fs) { for (int k = 0; k < N; k++) osc[k].set(rate * (0.72 + 0.56 * k / (N - 1.0)), fs); depthT = depthS * fs; }
  // Nachhallzeiten tiefen/mittlere/hohe Frequenzen; hold: verlustfrei
  void setDecay(double tLo, double tMid, double tHi, double fLo, double fHi, double fs, bool hold) {
    for (int k = 0; k < N; k++) { if (hold) { g[k] = 1; lo[k].flat(); hs[k].flat(); continue; }
      double gm = std::pow(10.0, -3 * L[k] / (fs * tMid)), gl = std::pow(10.0, -3 * L[k] / (fs * tLo)), gh = std::pow(10.0, -3 * L[k] / (fs * tHi));
      g[k] = gm; lo[k].low(gl / gm, fLo, fs); hs[k].high(gh / gm, fHi, fs); } }
  static inline void hadamard(double *x) { for (int h = 1; h < N; h <<= 1) for (int i = 0; i < N; i += 2 * h) for (int j = i; j < i + h; j++) { double a = x[j], b = x[j + h]; x[j] = a + b; x[j + h] = a - b; }
    const double s = 0.25; for (int i = 0; i < N; i++) x[i] *= s; }   // 1/√16
  // u: Eingang (mono); yL/yR: Ausgang; m: Summe für die Oktave (SPARKLE)
  inline void step(double u, double &yL, double &yR, double &m) {
    depth += (depthT - depth) * 0.0005;
    double v[N];
    for (int k = 0; k < N; k++) { double D = L[k] + depth * (1 + osc[k].step()); y[k] = depth > 0.01 ? ln[k].readF(std::max(2.0, D)) : ln[k].read((int)L[k]); v[k] = g[k] * hs[k].step(lo[k].step(y[k])); }
    hadamard(v);
    double sl = 0, sr = 0, sm = 0; for (int k = 0; k < N; k++) { double bL = (k & 1) ? -1 : 1, bR = (k & 2) ? -1 : 1, bI = (k & 4) ? -1 : 1; ln[k].push(v[k] + 0.25 * bI * u); sl += bL * y[k]; sr += bR * y[k]; sm += y[k]; }
    yL = 0.25 * sl; yR = 0.25 * sr; m = 0.25 * sm; }
  // mittlere Verstärkung je Umlauf bei f (für die Tonkurve)
  double gainAt(double f, double fs) const { double s = 0; for (int k = 0; k < N; k++) s += g[k] * std::abs(lo[k].at(f, fs) * hs[k].at(f, fs)); return s / N; }
  double meanLen() const { double s = 0; for (int k = 0; k < N; k++) s += L[k]; return s / N; }
};

// ---- Plattenhall nach Dattorro ----
struct Plate {
  double sc = 1; AP in[4]; AP m1, m2; Line d1, d2, d3, d4; AP a1, a2; double damp = 0.0005, bw = 0.9995, dec = 0.5, lpS = 0, lpA = 0, lpB = 0; Shelf1 sA, sB; Osc oA, oB; double exc = 16;
  int D1 = 4453, D2 = 3720, D3 = 4217, D4 = 3163, M1 = 672, M2 = 908; double fbA = 0, fbB = 0;
  int T[14];
  void init(double fs) { sc = fs / 29761.0; auto S = [&](double n) { return std::max(2, (int)std::lround(n * sc)); };
    in[0].init(S(142), 0.75); in[1].init(S(107), 0.75); in[2].init(S(379), 0.625); in[3].init(S(277), 0.625);
    exc = 16 * sc; m1.init(S(672), -0.70, 2 * exc + 4); m2.init(S(908), -0.70, 2 * exc + 4); a1.init(S(1800), 0.50); a2.init(S(2656), 0.50);
    D1 = S(4453); D2 = S(3720); D3 = S(4217); D4 = S(3163); M1 = S(672); M2 = S(908); d1.init(D1); d2.init(D2); d3.init(D3); d4.init(D4);
    const int t[14] = {266, 2974, 1913, 1996, 1990, 187, 1066, 353, 3627, 1228, 2673, 2111, 335, 121}; for (int i = 0; i < 14; i++) T[i] = S(t[i]);
    oA.set(0.53, fs); oB.set(0.71, fs); oB.phase(1.3); }
  void clear() { for (auto &a : in) a.l.clear(); m1.l.clear(); m2.l.clear(); a1.l.clear(); a2.l.clear(); d1.clear(); d2.clear(); d3.clear(); d4.clear(); lpS = lpA = lpB = 0; sA.z = sB.z = 0; fbA = fbB = 0; }
  static double loopSeg() { return (672 + 4453 + 1800 + 3720 + 908 + 4217 + 2656 + 3163) / 29761.0 / 4; }   // Zeit zwischen zwei Abklingfaktoren (s)
  // tMid: Nachhallzeit; low: Verhältnis der Tiefen; hi: Dämpfung (Tiefpass im Umlauf); t2: Bandbreite am Eingang (helle frühe Anteile)
  void set(double tMid, double lowR, double hiR, double t2, double fs, bool hold) {
    dec = hold ? 1.0 : std::pow(10.0, -3 * loopSeg() / tMid);
    double fd = hold ? 0.49 * fs : std::min(0.49 * fs, 900 * std::pow(hiR / 0.08, 1.3));   // Dämpfung: hiR 0,08 → 0,9 kHz, 0,28 → 4,5 kHz, 1 → 23 kHz
    damp = std::exp(-2 * PI * fd / fs); if (fd >= 0.49 * fs - 1) damp = 0;
    double fb = 2500 * std::pow(8.0, clamp01(t2)); bw = 1 - std::exp(-2 * PI * std::min(fb, 0.49 * fs) / fs); if (fb >= 0.45 * fs) bw = 1;   // 2,5 … 20 kHz
    double gl = std::pow(10.0, -3 * loopSeg() / (tMid * lowR)), gm = dec; if (hold) sA.flat(), sB.flat(); else { sA.low(gl * gl / (gm * gm), 300, fs); sB.low(gl * gl / (gm * gm), 300, fs); } }
  inline void step(double u, double &yL, double &yR) {
    lpS += bw * (u - lpS); double x = lpS; for (auto &a : in) x = a.step(x);
    double cA = oA.step(), cB = oB.step();
    double a = x + dec * fbB; a = m1.stepMod(a, M1 + exc * (1 + cA)); double ta = d1.read(D1); d1.push(a);
    lpA = (1 - damp) * ta + damp * lpA; double b = sA.step(dec * lpA); b = a1.step(b); double tb = d2.read(D2); d2.push(b); fbA = tb;
    double c = x + dec * fbA; c = m2.stepMod(c, M2 + exc * (1 + cB)); double tc = d3.read(D3); d3.push(c);
    lpB = (1 - damp) * tc + damp * lpB; double d = sB.step(dec * lpB); d = a2.step(d); double td = d4.read(D4); d4.push(d); fbB = td;
    // Abgriffe nach Dattorro, Tabelle 2 (Knoten 24_30 = d1, 31_33 = a1, 33_39 = d2, 48_54 = d3, 55_59 = a2, 59_63 = d4)
    yL = 0.6 * (d3.tap(T[0]) + d3.tap(T[1]) - a2.l.tap(T[2]) + d4.tap(T[3]) - d1.tap(T[4]) - a1.l.tap(T[5]) - d2.tap(T[6]));
    yR = 0.6 * (d1.tap(T[7]) + d1.tap(T[8]) - a1.l.tap(T[9]) + d2.tap(T[10]) - d3.tap(T[11]) - a2.l.tap(T[12]) - d4.tap(T[13])); }
  double gainAt(double f, double fs) const { std::complex<double> e = std::polar(1.0, -2 * PI * f / fs); double lp = std::abs((1 - damp) / (1.0 - damp * e)); return dec * lp * std::abs(sA.at(f, fs)); }
};

// ---- Federhall (spring.hpp), drei Federn ----
struct SpringRev {
  springrev::Tank t; double norm = 1;
  void init(double fs) { springrev::Params p; p.nSprings = 3; p.td[0] = 0.033; p.td[1] = 0.037; p.td[2] = 0.041; p.gain = 1; t.design(fs, p); t.nl = false; }
  void clear() { t.reset(); }
  void set(double tMid, double hiR, double rT60, double rLev, double fs, bool hold) {
    double sum = 0; for (size_t k = 0; k < t.sp.size(); k++) { auto &s = t.sp[k]; double td = t.p.td[k];
      s.g = hold ? 0.9995 : std::pow(10.0, -3 * td / tMid); s.gh = hold ? 0.9995 : std::pow(10.0, -3 * td / rT60); s.hfLevel = rLev;
      double fl = hold ? 0.45 * fs : std::min(0.45 * fs, 6000 * std::pow(hiR / 0.28, 0.6)); s.lp.lp(fl, fs); sum += 1 / std::sqrt(1 - s.g * s.g); }
    norm = 1 / sum; }
  inline void step(double u, double &yL, double &yR) { double a = t.sp[0].step(u), b = t.sp[1].step(u), c = t.sp[2].step(u); yL = norm * (a + 0.5 * b) * 1.6; yR = norm * (c + 0.5 * b) * 1.6; }
};

// ---- Oktave nach oben (SPARKLE): zwei Leseköpfe, Verzögerung nimmt je Abtastwert um 1 ab (doppelte Lesegeschwindigkeit) ----
// Mit festem Abstand W/2 zwischen den Köpfen löschen sie sich bei Frequenzen mit f·W/2/fs = k + ½ gegenseitig aus (bei W = 50 ms z. B. 220 Hz → 440 Hz fehlt,
// nur ±20-Hz-Seitenbänder bleiben; gemessen). Darum beginnt jedes Korn mit einem zufälligen Zusatzversatz (0 … 0,4 W): die Phasenlage der Köpfe wechselt
// je Korn, im Mittel addieren sie sich nach Leistung; Fenster sin/cos (leistungskomplementär), so bleibt der Pegel im Mittel gleich.
struct Octave {
  Line l; int W = 2400, k = 0, ja = 0, jb = 0; Biquad hp; Rng rng{0x0c7a5u};
  void init(double fs) { W = 2 * std::max(32, (int)std::lround(0.025 * fs)); l.init(1.45 * W + 8); k = 0; ja = 0; jb = W / 5; hp.set(0, 300, fs); }
  void clear() { l.clear(); k = 0; ja = 0; jb = W / 5; hp.clear(); }
  inline double step(double x) { l.push(hp.step(x));
    if (k == 0) ja = (int)(rng.next() * 0.4 * W); if (k == W / 2) jb = (int)(rng.next() * 0.4 * W);
    int kb = k + W / 2; if (kb >= W) kb -= W;
    double wa = std::sin(PI * k / W), wb = std::sin(PI * kb / W);
    double y = wa * l.tap(ja + W - k - 1) + wb * l.tap(jb + W - kb - 1); if (++k >= W) k = 0; return y; }
};

// ---- frühe Reflexionen: Abgriffe aus der Eingangsleitung (hinter der Vorverzögerung) ----
struct ER { static const int K = 18; int dL[K], dR[K]; double gL[K], gR[K];
  void init(double fs, double t0, double t1, uint32_t seed) { Rng r(seed); double sl = 0, sr = 0;
    for (int i = 0; i < K; i++) { double ta = t0 + (t1 - t0) * std::pow((i + r.next()) / K, 1.4), tb = t0 + (t1 - t0) * std::pow((i + r.next()) / K, 1.4);
      dL[i] = std::max(1, (int)std::lround(ta * fs)); dR[i] = std::max(1, (int)std::lround(tb * fs));
      gL[i] = (r.next() < 0.5 ? -1 : 1) * std::exp(-2.2 * (ta - t0) / (t1 - t0)); gR[i] = (r.next() < 0.5 ? -1 : 1) * std::exp(-2.2 * (tb - t0) / (t1 - t0)); sl += gL[i] * gL[i]; sr += gR[i] * gR[i]; }
    for (int i = 0; i < K; i++) { gL[i] /= std::sqrt(sl); gR[i] /= std::sqrt(sr); } }
};

// ---- ganzes Gerät ----
struct Reverb {
  double fs = 48000; int cur = M_HALL; Setting sm, tgt; bool havesm = false;
  FDN hall, room, modu, spark; Plate plate; SpringRev spring; Octave oct; AP difH[4], difR[4]; Line pre; ER erH, erR;
  Biquad eqHp[2], eqLs[2], eqLp[2], eqHs[2]; double preS = 0, wetFade = 1, wetDir = 0, holdIn = 1, dryG = 1, wetG = 0, outG = 1, cal = 1; int blk = 0; bool dirtyCoef = true, holdNow = false;
  double erg = 0, sparkL = 0, octIn = 0;
  // Pegelabgleich je Modus: Hall bei Rauschen am Eingang etwa so laut wie trocken (12 Uhr), gemessen mit tests/kaitest.cpp
  static constexpr double CAL[NMODE] = {1.834, 1.363, 4.63, 1.834, 2.966, 3.04};
  void setup(double fs_) { fs = fs_;
    hall.init(fs, M_HALL, 0.0016); room.init(fs, M_ROOM, 0.0002); modu.init(fs, M_MOD, 0.0085); spark.init(fs, M_SPARKLE, 0.0005);
    plate.init(fs); spring.init(fs); oct.init(fs);
    const double dm[4] = {4.77, 3.59, 12.73, 9.31}, dg[4] = {0.75, 0.75, 0.625, 0.625}, rm[4] = {1.9, 2.9, 4.3, 6.1};
    for (int i = 0; i < 4; i++) { difH[i].init((int)std::lround(dm[i] * 1e-3 * fs), dg[i]); difR[i].init((int)std::lround(rm[i] * 1e-3 * fs), 0.7); }
    pre.init(0.30 * fs + 0.10 * fs); erH.init(fs, 0.008, 0.090, 0x5eed1u); erR.init(fs, 0.002, 0.045, 0xca11u);
    reset(); }
  // Moduswechsel im Audio-Faden: nur den neuen Modus leeren (bei 192 kHz liegen alle Puffer zusammen bei etwa 7 MB); die Eingangsleitung bleibt
  void clearMode(int m) { if (FDN *f = fdn(m)) f->clear(); else if (m == M_PLATE) plate.clear(); else spring.clear(); oct.clear(); for (int i = 0; i < 4; i++) { difH[i].l.clear(); difR[i].l.clear(); }
    for (int c = 0; c < 2; c++) { eqHp[c].clear(); eqLs[c].clear(); eqLp[c].clear(); eqHs[c].clear(); } octIn = 0; }
  void clearAll() { hall.clear(); room.clear(); modu.clear(); spark.clear(); plate.clear(); spring.clear(); pre.clear(); clearMode(cur); }
  void reset() { clearAll(); havesm = false; wetFade = 1; wetDir = 0; holdIn = 1; blk = 0; dirtyCoef = true; }
  FDN *fdn(int m) { return m == M_HALL ? &hall : m == M_ROOM ? &room : m == M_MOD ? &modu : m == M_SPARKLE ? &spark : nullptr; }
  // Koeffizienten aus den (geglätteten) Stellungen
  void coef(const Setting &s, bool snap) { const int m = cur; double tm = t60Mid(m, s.decay), lr = lowRatio(s.low), hr = hiRatioMode(m, s); bool hold = s.hold;
    if (FDN *f = fdn(m)) { f->setDecay(tm * lr, tm, tm * hr, XLOW[m], XHIGH[m], fs, hold);
      if (m == M_HALL) f->setMod(0.45, hallMod(s.t1), fs); else if (m == M_MOD) f->setMod(modRate(s.t1), modDepth(s.t2), fs); else if (m == M_ROOM) f->setMod(0.8, 0.00005, fs); else f->setMod(0.3, 0.0002, fs);
      if (snap) f->depth = f->depthT; }
    else if (m == M_PLATE) plate.set(tm, lr, hr, s.t2, fs, hold);
    else spring.set(tm, hr, rattleT60(s.t1), rattleLevel(s.t2), fs, hold);
    double fh = s.low < 0.5 ? 20 * std::pow(30.0, (0.5 - s.low) * 2) : 20, lsDb = s.low > 0.5 ? (s.low - 0.5) * 12 : 0;
    double fl = s.hi < 0.5 ? 20000 * std::pow(1 / 16.0, (0.5 - s.hi) * 2) : 20000, hsDb = s.hi > 0.5 ? (s.hi - 0.5) * 12 : 0;
    for (int c = 0; c < 2; c++) { eqHp[c].set(0, fh, fs); eqLs[c].set(2, 200, fs, lsDb); eqLp[c].set(1, fl, fs); eqHs[c].set(3, 4000, fs, hsDb); }
    erg = erLevel(m, s.t2); sparkL = m == M_SPARKLE && !hold ? sparkleLevel(s.t1) : 0;
    mixGains(s.mix, dryG, wetG); outG = std::pow(10.0, outDb(s.out) / 20);
    cal = CAL[m] * std::pow(t60Ref(m) / tm, 0.25); }
  // Eingangsdämpfung „Halten“ und Pegel gleiten je Abtastwert; Regler alle 16 Abtastwerte (Glättung 40 ms)
  void process(const float *inL, const float *inR, float *outL, float *outR, int n, const Setting &s) {
    tgt = s; tgt.mode = std::max(0, std::min(NMODE - 1, s.mode));
    if (!havesm) { sm = tgt; havesm = true; cur = tgt.mode; holdNow = tgt.hold; holdIn = tgt.hold ? 0 : 1; coef(sm, true); preS = preDelay(cur, sm.t1) * fs; }
    const double kB = 1 - std::exp(-16 / (0.04 * fs)), kFade = 1 / (0.010 * fs), kHold = 1 / (0.020 * fs);
    for (int i = 0; i < n; i++) {
      if (++blk >= 16) { blk = 0; bool ch = false;
        auto sl = [&](double &a, double b) { if (a != b) { a += (b - a) * kB; if (std::fabs(a - b) < 1e-4) a = b; ch = true; } };
        sl(sm.decay, tgt.decay); sl(sm.mix, tgt.mix); sl(sm.out, tgt.out); sl(sm.low, tgt.low); sl(sm.hi, tgt.hi); sl(sm.t1, tgt.t1); sl(sm.t2, tgt.t2);
        if (tgt.hold != sm.hold) { sm.hold = tgt.hold; ch = true; }
        if (tgt.mode != cur && wetDir == 0) wetDir = -1;   // Moduswechsel: Hall 10 ms ausblenden, leeren, umschalten, einblenden
        if (ch || dirtyCoef) { dirtyCoef = false; coef(sm, false); } }
      if (wetDir < 0 && wetFade <= 0) { cur = tgt.mode; clearMode(cur); coef(sm, true); preS = preDelay(cur, sm.t1) * fs; wetDir = 1; }
      if (wetDir != 0) { wetFade += wetDir * kFade; if (wetFade >= 1) { wetFade = 1; wetDir = 0; } if (wetFade < 0) wetFade = 0; }
      { double ht = sm.hold ? 0.0 : 1.0; if (holdIn < ht) holdIn = std::min(ht, holdIn + kHold); else if (holdIn > ht) holdIn = std::max(ht, holdIn - kHold); }
      double xl = inL[i], xr = inR ? inR[i] : xl; if (!std::isfinite(xl)) xl = 0; if (!std::isfinite(xr)) xr = 0; double u = 0.5 * (xl + xr) * holdIn;   // ungültige Eingangswerte als 0
      double ptgt = preDelay(cur, sm.t1) * fs; preS += (ptgt - preS) * 0.0005; pre.push(u);
      double yl = 0, yr = 0;
      if (cur == M_SPRING) spring.step(u, yl, yr);
      else if (cur == M_PLATE) { double p = preS > 2 ? pre.readF(preS) : u; plate.step(p, yl, yr); }
      else { FDN &f = *fdn(cur); double p = preS > 2 ? pre.readF(preS) : u; double x = p;
        AP *dif = cur == M_ROOM ? difR : difH; for (int k = 0; k < 4; k++) x = dif[k].step(x);
        if (cur == M_SPARKLE) { double o = oct.step(octIn); double inj = sparkL * o; inj = 0.5 * std::tanh(inj / 0.5); x += inj; }
        double m; f.step(x, yl, yr, m); if (cur == M_SPARKLE) octIn = m;
        if (erg > 0) { const ER &e = cur == M_ROOM ? erR : erH; double el = 0, er = 0, base = std::max(1.0, preS);
          for (int k = 0; k < ER::K; k++) { el += e.gL[k] * pre.tap((int)base + e.dL[k] - 1); er += e.gR[k] * pre.tap((int)base + e.dR[k] - 1); }
          yl += 0.5 * erg * el; yr += 0.5 * erg * er; } }
      double g = cal * wetFade, w[2] = {yl * g, yr * g};
      for (int c = 0; c < 2; c++) w[c] = eqHs[c].step(eqLp[c].step(eqLs[c].step(eqHp[c].step(w[c]))));
      outL[i] = (float)(outG * (dryG * xl + wetG * w[0])); outR[i] = (float)(outG * (dryG * xr + wetG * w[1])); } }
};

// ---- Anzeige (Plugin-Oberfläche und Seite) ----
// Tonkurve des Halls in dB, auf 1 kHz bezogen: Energie des Abklingens je Frequenz (1/(1 − g²) mit der mittleren Verstärkung je Umlauf) × Klangregelung LOW/HI.
// SPRING: Übertragung des Tanks genähert (Wandler-Hochpass 120 Hz, Übergangsfrequenz 4,2 kHz).
inline void toneCurve(const Setting &s0, double fs, const double *fr, int n, double *out) {
  Setting s = s0; int m = std::max(0, std::min(NMODE - 1, s.mode)); double tm = t60Mid(m, s.decay), lr = lowRatio(s.low), hr = hiRatioMode(m, s);
  Biquad eq[4]; double fh = s.low < 0.5 ? 20 * std::pow(30.0, (0.5 - s.low) * 2) : 20, lsDb = s.low > 0.5 ? (s.low - 0.5) * 12 : 0;
  double fl = s.hi < 0.5 ? 20000 * std::pow(1 / 16.0, (0.5 - s.hi) * 2) : 20000, hsDb = s.hi > 0.5 ? (s.hi - 0.5) * 12 : 0;
  eq[0].set(0, fh, fs); eq[1].set(2, 200, fs, lsDb); eq[2].set(1, fl, fs); eq[3].set(3, 4000, fs, hsDb);
  int L[FDN::N] = {}; Shelf1 lo[FDN::N], hs[FDN::N]; double g[FDN::N] = {};
  if (FDN_MS[m][0] > 0) for (int k = 0; k < FDN::N; k++) { L[k] = fdnLen(m, k, fs); double gm = std::pow(10.0, -3 * L[k] / (fs * tm)), gl = std::pow(10.0, -3 * L[k] / (fs * tm * lr)), gh = std::pow(10.0, -3 * L[k] / (fs * tm * hr));
    g[k] = gm; lo[k].low(gl / gm, XLOW[m], fs); hs[k].high(gh / gm, XHIGH[m], fs); }
  Plate pl; if (m == M_PLATE) { pl.set(tm, lr, hr, s.t2, fs, false); }
  auto e = [&](double f) { double en;
    if (m == M_SPRING) { double a = f / 120, b = f / 4200; en = (a * a / (1 + a * a)) / (1 + b * b * b * b); en = 10 * std::log10(en); }
    else { double gg = 0; if (m == M_PLATE) gg = pl.gainAt(f, fs); else { for (int k = 0; k < FDN::N; k++) gg += g[k] * std::abs(lo[k].at(f, fs) * hs[k].at(f, fs)); gg /= FDN::N; }
      gg = std::min(gg, 0.999999); en = 10 * std::log10(1 / (1 - gg * gg)); }
    std::complex<double> h = eq[0].at(f, fs) * eq[1].at(f, fs) * eq[2].at(f, fs) * eq[3].at(f, fs); return en + 20 * std::log10(std::abs(h) + 1e-30); };
  double r = e(1000); for (int i = 0; i < n; i++) out[i] = std::isfinite(fr[i]) && fr[i] > 0 ? e(std::min(fr[i], 0.49 * fs)) - r : -600; }
// Texte zu den Stellungen (knob: 0 DECAY, 1 MIX, 2 OUTPUT, 3 LOW, 4 HI, 5 THING 1, 6 THING 2)
#include <cstdio>
inline void describe(const Setting &s, int knob, char *b, int cap) { int m = std::max(0, std::min(NMODE - 1, s.mode));
  switch (knob) {
    case 0: { double tm = t60Mid(m, s.decay); std::snprintf(b, cap, "DECAY %.2f s mid (low %.2f s, high %.2f s)%s", tm, tm * lowRatio(s.low), tm * hiRatioMode(m, s), m == M_SPRING ? ", approx." : ""); break; }
    case 1: { double d, w; mixGains(s.mix, d, w); std::snprintf(b, cap, "MIX dry %.0f %%, reverb %.0f %%%s", d * 100, w * 100, std::fabs(s.mix - 0.7) < 0.005 ? " (equal)" : ""); break; }
    case 2: std::snprintf(b, cap, "OUTPUT %+.1f dB", outDb(s.out)); break;
    case 3: { double fh = s.low < 0.5 ? 20 * std::pow(30.0, (0.5 - s.low) * 2) : 20; if (s.low < 0.5) std::snprintf(b, cap, "LOW low cut %.0f Hz, low decay x%.2f", fh, lowRatio(s.low));
      else std::snprintf(b, cap, "LOW %+.1f dB at 200 Hz, low decay x%.2f", (s.low - 0.5) * 12, lowRatio(s.low)); break; }
    case 4: { if (s.hi < 0.5) std::snprintf(b, cap, "HI high cut %.1f kHz, high decay x%.2f", 20 * std::pow(1 / 16.0, (0.5 - s.hi) * 2), hiRatioMode(m, s));
      else std::snprintf(b, cap, "HI %+.1f dB at 4 kHz, high decay x%.2f", (s.hi - 0.5) * 12, hiRatioMode(m, s)); break; }
    case 5: switch (m) {
      case M_HALL: std::snprintf(b, cap, "MODULATION %.2f ms", hallMod(s.t1) * 1e3); break;
      case M_PLATE: case M_ROOM: std::snprintf(b, cap, "PRE-DELAY %.0f ms", preDelay(m, s.t1) * 1e3); break;
      case M_SPRING: std::snprintf(b, cap, "RATTLE DECAY %.2f s", rattleT60(s.t1)); break;
      case M_MOD: std::snprintf(b, cap, "MOD RATE %.2f Hz", modRate(s.t1)); break;
      default: std::snprintf(b, cap, "SPARKLE LEVEL %.0f %%", sparkleLevel(s.t1) / 0.9 * 100); } break;
    default: switch (m) {
      case M_HALL: case M_ROOM: std::snprintf(b, cap, "EARLY REFLECTIONS x%.2f", erLevel(m, s.t2)); break;
      case M_PLATE: std::snprintf(b, cap, "BRIGHT EARLY DECAY: input band %.1f kHz", std::min(20.0, 2.5 * std::pow(8.0, clamp01(s.t2)))); break;
      case M_SPRING: std::snprintf(b, cap, "RATTLE LEVEL %.0f %%", rattleLevel(s.t2) * 100); break;
      case M_MOD: std::snprintf(b, cap, "MOD DEPTH %.2f ms", modDepth(s.t2) * 1e3); break;
      default: std::snprintf(b, cap, "SPARKLE LENGTH x%.2f (octaves decay)", sparkleLenK(s.t2)); } }
}
} // namespace kai
