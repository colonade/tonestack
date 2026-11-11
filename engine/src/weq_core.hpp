// Wien-Brücken-Equalizer nach den veröffentlichten Angaben zum adt-audio ToolMod TM205 (5-Band-Stereo-Mastering-EQ). Kein Schaltplan veröffentlicht:
// das Prinzip (aktive Wien-Robinson-Brücke) und die Bedienung stammen von ADT, die Umsetzung ist eigener Entwurf (docs/wieneq/ENTWURF.md).
//  - MID 1/2/3 vollparametrisch: Mittenfrequenz 20 Hz–2 kHz, 60 Hz–7 kHz, 250 Hz–25 kHz (negativ logarithmisch, 1 : 100), Güte wie bei ADT
//    angegeben von etwa 5 Oktaven (Q = 0,18) bis zur Terz (Q = 4,32), Bandbreite jeweils an den −3-dB-Punkten des Bandpasses gemessen,
//    Q-Regler logarithmisch; Anheben/Absenken mit Rastung bei 0 dB.
//  - LOW: Kuhschwanzfilter 1. Ordnung (6 dB/Oktave) bei 40 / 60 / 100 / 160 Hz (zwei Tasten).
//  - HIGH: Kuhschwanzfilter 1. Ordnung bei 20 kHz (keine Taste) oder 12 / 8 / 4 kHz (zwei Tasten). Auf Wunsch des Nutzers seit 1.2 in allen
//    Stellungen Kuhschwanz; ADT beschreibt für die Tastenstellungen von HIGH ein schwach ausgeprägtes Maximum (bis 1.1: Glocken mit Q 1/3).
//    Die genannte Frequenz ist die Mitte der Flanke: dort wirkt die halbe Anhebung in dB.
//  - MID: Anheben H = 1 + G·B(s), Absenken H = 1 / (1 + G·B(s)) (Bandpass in der Gegenkopplung), G = 10^(|dB|/20) − 1, B Bandpass mit Spitze 1.
//  - LOW/HIGH: g = 10^(|dB|/20); Tiefen H = (s + ω√g)/(s + ω/√g), Höhen H = (√g·s + ω)/(s/√g + ω); Absenken ist der Kehrwert.
//  - Regelbereich ±6 / ±12 / ±20 dB (Ausführungen des TM205; Standard ±12), Reglerkennlinie um 0 dB gespreizt: dB = R·(|p| + p²)/2.
// Linear (Aussteuerungsreserve +30 dBu). Digital: je Band ein Abschnitt (MID 2. Ordnung mit angepasstem Betrag nach Vicanek 2016, LOW/HIGH 1. Ordnung), unter 80 kHz Abtastrate
// doppelt so schnell gerechnet (eigenes Halbbandfilter, bis 20 kHz flach), damit MID 3 bis 25 kHz und HIGH im Hörbereich wie analog verlaufen.
#pragma once
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <cstring>

namespace weq {
constexpr double PI = 3.14159265358979323846;
constexpr int NB = 5;   // LOW, MID 1, MID 2, MID 3, HIGH
static const double MID_LO[3] = {20, 60, 250}, MID_HI[3] = {2000, 7000, 25000};
static const double LOW_F[4] = {40, 60, 100, 160};          // keine Taste, Taste 60, Taste 100, beide
static const double HIGH_F[4] = {20000, 12000, 8000, 4000};  // keine Taste, Taste 12k, Taste 8k, beide
static const double RANGE_DB[3] = {6, 12, 20};
constexpr double QMIN = 0.1824791693, QMAX = 4.318473046;   // N Oktaven: Q = 1 / (2 sinh(N ln 2 / 2)); 5 Oktaven bzw. Terz (N = 1/3)
enum { BELL = 0, LOSHELF = 1, HISHELF = 2 };
struct Setting {
  double gain[NB] = {0, 0, 0, 0, 0};   // Reglerstellung −1 … +1 (0 = Rastung)
  double q[3] = {0.5, 0.5, 0.5};       // 0 = breit … 1 = Terz
  double f[3] = {0.5, 0.5, 0.5};       // 0 … 1 über den Bereich des Bandes
  int lowSel = 0, highSel = 0, range = 1; bool on = true;
};
inline double gainDb(double p, int range) { p = std::max(-1.0, std::min(1.0, p)); double a = std::fabs(p); return (p < 0 ? -1 : 1) * RANGE_DB[std::max(0, std::min(2, range))] * (a + a * a) / 2; }
inline double midFreq(int k, double x) { x = std::max(0.0, std::min(1.0, x)); return MID_LO[k] * std::pow(MID_HI[k] / MID_LO[k], x); }
inline double midQ(double x) { x = std::max(0.0, std::min(1.0, x)); return QMIN * std::pow(QMAX / QMIN, x); }
struct Band { double f, q, db; int kind = BELL; };   // q nur für BELL
inline void bands(const Setting &s, Band *b) {
  b[0] = {LOW_F[std::max(0, std::min(3, s.lowSel))], 1, gainDb(s.gain[0], s.range), LOSHELF};
  for (int k = 0; k < 3; k++) b[1 + k] = {midFreq(k, s.f[k]), midQ(s.q[k]), gainDb(s.gain[1 + k], s.range)};
  b[4] = {HIGH_F[std::max(0, std::min(3, s.highSel))], 1, gainDb(s.gain[4], s.range), HISHELF}; }
// analoges Band (s = jω)
inline std::complex<double> bandS(std::complex<double> s, const Band &b) {
  if (b.kind != BELL) { std::complex<double> x = s / (2 * PI * b.f); double r = std::pow(10.0, std::fabs(b.db) / 40); std::complex<double> h = b.kind == LOSHELF ? (x + r) / (x + 1 / r) : (r * x + 1.0) / (x / r + 1.0); return b.db >= 0 ? h : 1.0 / h; }
  std::complex<double> x = s / (2 * PI * b.f), bp = (x / b.q) / (1.0 + x / b.q + x * x); double G = std::pow(10.0, std::fabs(b.db) / 20) - 1;
  return b.db >= 0 ? 1.0 + G * bp : 1.0 / (1.0 + G * bp); }
inline std::complex<double> response(double f, const Setting &s) { if (!s.on) return 1; Band b[NB]; bands(s, b); std::complex<double> h = 1, jw(0, 2 * PI * f); for (auto &x : b) h *= bandS(jw, x); return h; }

// Abschnitt 2. Ordnung, Direktform I: die Zustände sind die letzten Ein- und Ausgangswerte; gleitende Koeffizienten (Regler drehen, Tasten,
// Regelbereich) erzeugen so keine Sprünge aus inneren Zuständen, die zu den alten Koeffizienten gehörten (Direktform II transponiert: bis −40 dB)
struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  double step(double x) { double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
  void reset() { x1 = x2 = y1 = y2 = 0; } };
// Entwurf des ANHEBENS (Band mit |dB|): Pole wie analog (z = e^{sT}, Nenner 1 + s/(Qω) + (s/ω)²). Zähler über seinen Betrag im Quadrat
// |B|² = B0 φ0 + B1 φ1 + B2 φ2 (φ0 = cos², φ1 = sin², φ2 = 4 φ0 φ1 von ω/2; M. Vicanek, „Matched Second Order Digital Filters“, 2016):
//  - Weg A: B0, B1, B2 so, dass der Betrag bei 0 Hz, fs/2 und fc dem analogen gleicht (Vicanek)
//  - Weg B: B0 bei 0 Hz exakt, B1 und B2 per Ausgleichsrechnung über 48 Frequenzen (fc/30 … 22 kHz, relativer Fehler)
//  - genommen wird der Weg mit dem kleineren größten Fehler an 16 Prüffrequenzen (20 Hz … 22 kHz, unter 0,45·fs); B nur, wenn er gültig ist
//    (A ist bei tiefen, schmalen Bändern besser, B bei breiten, deren Wirkung bis in die Höhen reicht)
// Gemessen gegen analog (Raster 20 Hz–25 kHz, Q 0,18 … 4,32, 0,3 … 20 dB, Rechenrate 88,2 … 192 kHz): höchstens 0,016 dB.
// ABSENKEN ist genau der Kehrwert (Bandpass in der Gegenkopplung): Zähler und Nenner getauscht, also derselbe Fehler gespiegelt.
// |1 + G·B(jω)|² des Anhebens in reeller Rechnung (ohne pow/complex; für den Entwurf, der oft läuft)
inline double boost2(double f, const Band &b, double G) { double x = f / b.f, a = 1 - x * x, k = x / b.q, g = 1 + G; return (a * a + k * k * g * g) / (a * a + k * k); }
inline bool numLS(double &B1, double &B2, double B0, double A0, double A1, double A2, const Band &b, double fs) {
  const int K = 48; const double G = std::pow(10.0, std::fabs(b.db) / 20) - 1; double fhi = std::min(22000.0, 0.45 * fs), flo = std::max(10.0, b.f / 30), s1 = 0, s2 = 0; double p1v[K], p2v[K], tv[K], wv[K];
  for (int k = 0; k < K; k++) { double f = flo * std::pow(fhi / flo, k / (K - 1.0)), sn = std::sin(PI * f / fs), p1 = sn * sn, p0 = 1 - p1, p2 = 4 * p0 * p1;
    double T = boost2(f, b, G) * (A0 * p0 + A1 * p1 + A2 * p2); p1v[k] = p1; p2v[k] = p2; tv[k] = T - B0 * p0; wv[k] = 1 / (T * T); s1 = std::max(s1, p1); s2 = std::max(s2, p2); }
  double m11 = 0, m12 = 0, m22 = 0, v1 = 0, v2 = 0;
  for (int k = 0; k < K; k++) { double q1 = p1v[k] / s1, q2 = p2v[k] / s2, w = wv[k]; m11 += w * q1 * q1; m12 += w * q1 * q2; m22 += w * q2 * q2; v1 += w * q1 * tv[k]; v2 += w * q2 * tv[k]; }
  double D = m11 * m22 - m12 * m12; if (!(std::fabs(D) > 0)) return false; B1 = (v1 * m22 - v2 * m12) / D / s1; B2 = (m11 * v2 - m12 * v1) / D / s2; return std::isfinite(B1) && std::isfinite(B2) && B1 > 0; }
// Kuhschwanz 1. Ordnung (ANHEBEN, g > 1; Absenken = Kehrwert). |H|² eines Abschnitts 1. Ordnung ist (N0 + N1 φ)/(D0 + D1 φ) mit φ = sin²(ω/2).
//  - Tiefen: Pol und Nullstelle wie analog (z = e^{−ωT}), Verstärkung bei 0 Hz exakt. Die Ecken liegen weit unter der Abtastrate: Fehler < 0,001 dB.
//  - Höhen: bei 0 Hz exakt 1; N1 und D1 (auf D0 = 1 bezogen) per gewichteter Ausgleichsrechnung über 48 Frequenzen (20 Hz … 22 kHz, relativer Fehler).
//    Pol und Nullstelle wie analog verfehlten 20 kHz / 20 dB bei 88,2 kHz um 0,18 dB; so sind es höchstens 0,011 dB (88,2 … 192 kHz, bis 20 dB).
inline void designShelf(Biquad &d, const Band &band, double fs) {
  double db = std::fabs(band.db), b0 = 1, b1 = 0, a1 = 0;
  if (db > 1e-4) { const double g = std::pow(10.0, db / 20), r = std::sqrt(g), w = 2 * PI * band.f / fs;
    if (band.kind == LOSHELF) { double p = std::exp(-w / r), z0 = std::exp(-w * r); b0 = g * (1 - p) / (1 - z0); b1 = -b0 * z0; a1 = -p; }
    else { const int K = 48; double fhi = std::min(22000.0, 0.45 * fs), m11 = 0, m12 = 0, m22 = 0, v1 = 0, v2 = 0;
      for (int k = 0; k < K; k++) { double f = 20 * std::pow(fhi / 20, k / (K - 1.0)), sn = std::sin(PI * f / fs), ph = sn * sn, x = (f / band.f) * (f / band.f), T = (g * x + 1) / (x / g + 1);
        double c1 = ph / T, c2 = -ph, y = (T - 1) / T; m11 += c1 * c1; m12 += c1 * c2; m22 += c2 * c2; v1 += c1 * y; v2 += c2 * y; }   // (N1 − T·D1)·φ = T − 1, je Zeile durch T geteilt
      double D = m11 * m22 - m12 * m12, N1 = (v1 * m22 - v2 * m12) / D, D1 = (m11 * v2 - m12 * v1) / D;
      auto root = [](double q) { return std::fabs(q) < 1e-9 ? -q / 4 : (-(q + 2) + 2 * std::sqrt(q + 1)) / q; };   // a aus q = −4a/(1 + a)², |a| < 1
      if (std::isfinite(N1) && std::isfinite(D1) && N1 > -1 && D1 > -1) { a1 = root(D1); double rr = root(N1); b0 = (1 + a1) / (1 + rr); b1 = b0 * rr; }
      else { double p = std::exp(-w * r), z0 = std::exp(-w / r); b0 = (1 - p) / (1 - z0); b1 = -b0 * z0; a1 = -p; } } }   // Rückfall: wie analog
  if (band.db >= 0) { d.b0 = b0; d.b1 = b1; d.b2 = 0; d.a1 = a1; d.a2 = 0; }
  else { d.b0 = 1 / b0; d.b1 = a1 / b0; d.b2 = 0; d.a1 = b1 / b0; d.a2 = 0; } }
inline void design(Biquad &d, const Band &band, double fs) {
  if (band.kind != BELL) { designShelf(d, band, fs); return; }
  const Band b{band.f, band.q, std::fabs(band.db)};
  double w0 = 2 * PI * b.f / fs, z = 1 / (2 * b.q), e = std::exp(-z * w0);
  double a1 = z <= 1 ? -2 * e * std::cos(std::sqrt(1 - z * z) * w0) : -2 * e * std::cosh(std::sqrt(z * z - 1) * w0), a2 = e * e;
  double A0 = (1 + a1 + a2) * (1 + a1 + a2), A1 = (1 - a1 + a2) * (1 - a1 + a2), A2 = -4 * a2;
  const double G = std::pow(10.0, b.db / 20) - 1; auto m2 = [&](double fr) { return boost2(fr, b, G); };
  double B0 = m2(0) * A0, B1, B2;
  static const double FK[16] = {20, 31.9, 50.88, 81.15, 129.44, 206.46, 329.3, 525.23, 837.73, 1336.18, 2131.2, 3399.26, 5421.81, 8647.76, 13793.14, 22000};   // 20 · 1100^(k/15): 20 Hz … 22 kHz, logarithmisch
  double q0[16], q1[16], q2[16], qt[16]; int nk = 0;
  for (double f : FK) { if (f > 0.45 * fs) break; double sn = std::sin(PI * f / fs), p1 = sn * sn, p0 = 1 - p1; q0[nk] = p0; q1[nk] = p1; q2[nk] = 4 * p0 * p1; qt[nk] = m2(f) * (A0 * p0 + A1 * p1 + A2 * q2[nk]); nk++; }
  auto err = [&](double c1, double c2) { double w = 1; for (int k = 0; k < nk; k++) { double num = B0 * q0[k] + c1 * q1[k] + c2 * q2[k]; if (!(num > 0)) return 1e9; double r = num / qt[k]; w = std::max(w, std::max(r, 1 / r)); } return w; };   // größtes Verhältnis |H|² digital/analog
  { double sn = std::sin(w0 / 2), p1 = sn * sn, p0 = 1 - p1, p2 = 4 * p0 * p1; B1 = m2(fs / 2) * A1; B2 = (m2(b.f) * (A0 * p0 + A1 * p1 + A2 * p2) - B0 * p0 - B1 * p1) / p2; }
  if (b.db > 0) { double L1, L2; if (numLS(L1, L2, B0, A0, A1, A2, b, fs)) { double W = (std::sqrt(B0) + std::sqrt(L1)) / 2; if (W * W + L2 >= 0 && err(L1, L2) < err(B1, B2)) { B1 = L1; B2 = L2; } } }
  double r0 = std::sqrt(B0), r1 = std::sqrt(B1), W = (r0 + r1) / 2, b0 = (W + std::sqrt(std::max(0.0, W * W + B2))) / 2, b1 = (r0 - r1) / 2, b2 = W - b0;
  if (band.db >= 0) { d.b0 = b0; d.b1 = b1; d.b2 = b2; d.a1 = a1; d.a2 = a2; }
  else { d.b0 = 1 / b0; d.b1 = a1 / b0; d.b2 = a2 / b0; d.a1 = b1 / b0; d.a2 = b2 / b0; } }

// Halbbandfilter für 2-fache Rechenrate (Kaiser-Fenster, linearphasig): Durchlass bis 0,4535·fs (20 kHz bei 44,1 kHz) innerhalb ±0,005 dB,
// Sperrbereich ab 0,5465·fs. Nur unter 80 kHz Abtastrate. Verzögerung hin und zurück zusammen HB_LAT Abtastwerte der Grundrate.
constexpr int HB_N = 127, HB_LAT = (HB_N - 1) / 2;   // je Filter (HB_N − 1)/2 Abtastwerte der doppelten Rate, zusammen HB_N − 1 = 2·HB_LAT → HB_LAT Grundrate
inline const std::vector<double> &halfband() { static std::vector<double> h = [] { std::vector<double> c(HB_N); const double beta = 9.0, M = (HB_N - 1) / 2.0;
    auto I0 = [](double x) { double s = 1, t = 1; for (int k = 1; k < 60; k++) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
    double sum = 0; for (int n = 0; n < HB_N; n++) { double m = n - M, r = m / M, w = I0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / I0(beta), sinc = m == 0 ? 0.5 : std::sin(PI * m / 2) / (PI * m); c[n] = sinc * w; sum += c[n]; }
    for (auto &x : c) x /= sum; return c; }(); return h; }

// Ein Kanal: Glättung der Reglerwerte (20 ms), Koeffizienten alle 16 Abtastwerte neu; Schalter (Tasten, Bereich) gleiten mit; EQ ein/aus blendet 20 ms über (Smoothstep)
struct Channel {
  double fs = 48000, fsI = 96000; int os = 2; Biquad bq[NB]; std::vector<double> ub, db, dry; int ui = 0, di = 0, ri = 0;
  double cur[NB][3]; bool init = false;   // geglättet: ln f, ln Q, dB
  double mix = 1; int cnt = 0;
  void setup(double fs_) { fs = fs_; os = fs < 80000 ? 2 : 1; fsI = fs * os; ub.assign(2 * HB_N, 0); db.assign(2 * HB_N, 0); dry.assign(latency() + 1, 0); ui = di = ri = 0; for (auto &b : bq) b.reset(); init = false; cnt = 0; }
  int latency() const { return os == 2 ? HB_LAT : 0; }
  void targets(const Setting &s, double t[NB][3]) const { Band b[NB]; bands(s, b); for (int i = 0; i < NB; i++) { t[i][0] = std::log(b[i].f); t[i][1] = std::log(b[i].q); t[i][2] = b[i].db; } }
  void redesign(int i) { design(bq[i], Band{std::exp(cur[i][0]), std::exp(cur[i][1]), cur[i][2], i == 0 ? LOSHELF : i == NB - 1 ? HISHELF : BELL}, fsI); }
  void redesign() { for (int i = 0; i < NB; i++) redesign(i); }
  void snap(const Setting &s) { targets(s, cur); mix = s.on ? 1 : 0; redesign(); init = true; }
  void reset() { for (auto &b : bq) b.reset(); std::fill(ub.begin(), ub.end(), 0.0); std::fill(db.begin(), db.end(), 0.0); std::fill(dry.begin(), dry.end(), 0.0); }
  double eq(double x) { for (auto &b : bq) x = b.step(x); return x; }
  // n Abtastwerte an Ort und Stelle
  void process(float *buf, int n, const Setting &s) {
    if (!init) snap(s);
    double t[NB][3]; targets(s, t); const double k = 1 - std::exp(-16.0 / (0.02 * fsI * 1.0)), mt = s.on ? 1 : 0, ms = 1.0 / (0.02 * fs);
    const std::vector<double> &h = halfband(); const int L = latency();
    for (int i = 0; i < n; i++) {
      if (cnt-- <= 0) { cnt = 16 / os - 1; for (int b = 0; b < NB; b++) { bool ch = false; for (int j = 0; j < 3; j++) { double d = t[b][j] - cur[b][j]; if (d != 0) { ch = true; cur[b][j] = std::fabs(d) < 1e-6 ? t[b][j] : cur[b][j] + k * d; } } if (ch) redesign(b); } }
      double x = buf[i], y;
      if (os == 1) y = eq(x);
      else {   // 2-fach: v[2n] = x[n], v[2n+1] = 0; u = 2·(h ∗ v) → u[2n] = 2 Σ h[2j] x[n−j], u[2n+1] = 2 Σ h[2j+1] x[n−j]
        ui = ui == 0 ? HB_N - 1 : ui - 1; ub[ui] = ub[ui + HB_N] = x;   // ub[ui + j] = x[n − j]
        double u0 = 0, u1 = 0; for (int m = 0; m < HB_N; m += 2) u0 += h[m] * ub[ui + m / 2]; for (int m = 1; m < HB_N; m += 2) u1 += h[m] * ub[ui + m / 2];
        // EQ in zeitlicher Folge; Ausgang w[2n] = Σ h[k] e[2n−k] (gerader Index: Verzögerung genau HB_LAT Grundabtastwerte), danach e[2n+1] nachschieben
        di = di == 0 ? HB_N - 1 : di - 1; db[di] = db[di + HB_N] = eq(2 * u0);
        y = 0; for (int m = 0; m < HB_N; m++) y += h[m] * db[di + m];
        di = di == 0 ? HB_N - 1 : di - 1; db[di] = db[di + HB_N] = eq(2 * u1); }
      double d = x; if (L > 0) { dry[ri] = x; int r = ri - L; if (r < 0) r += L + 1; d = dry[r]; if (++ri > L) ri = 0; }
      if (mix != mt) mix = mt > mix ? std::min(mt, mix + ms) : std::max(mt, mix - ms);
      double w = mix * mix * (3 - 2 * mix); buf[i] = (float)(w >= 1 ? y : w <= 0 ? d : d + w * (y - d)); } }
};
} // namespace weq
