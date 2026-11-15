// Blech 140: Hallplatte nach dem EMT 140 (Elektromesstechnik, ab 1957) als physikalisches Modell (docs/blech/ENTWURF.md).
// Platte: gespannte Kirchhoff-Platte aus Stahl, 2 m × 1 m × 0,5 mm, frei gelagert (Ränder gelenkig), ein Treiber, zwei Abnehmer (stereo).
//   ρH·ü = T0·∇²u − D·∇⁴u − 2σ(ω)·ρH·u̇ + f,  D = E·H³/(12(1 − ν²))
//   Moden Φ = sin(m1·π·x/Lx)·sin(m2·π·y/Ly), ω² = (T0/ρH)·k² + (D/ρH)·k⁴, k² = (m1π/Lx)² + (m2π/Ly)²  (alle Moden bis 18 kHz, bei 48 kHz etwa 23 000)
// Dämpfung je Mode σ(ω) (auf die Energie bezogen: T60 = 6·ln10/σ, die Amplitude klingt mit σ/2 ab) = thermoelastisch (Konstanten R1, C1 nach Arcas/Chaigne, wie in Russo, AAU 2023) + Dämpfungsplatte (∝ √f, Stärke so, dass die
// Nachhallzeit bei 500 Hz dem Regler DECAY folgt, 1 … 3,8 s wie das Gerät „1 bis 4 s bei 500 Hz“) + Hochton-Verlust (∝ f², angenommen: T60 bei 10 kHz
// etwa 0,9 s bei offener Dämpfung). Ausgang = Schnelle an den Abnehmern (jede Mode trägt mit gleicher Amplitude bei, Frequenzgang im Mittel eben).
// Rechnung: aus den Moden die exakte Impulsantwort (die ersten 100 ms als Summe gedämpfter Schwingungen im Zeitbereich, danach über das Spektrum:
// je Mode die Lorentz-Kurve des gedämpften Kosinus, 2^19 Punkte bei 48 kHz, eine inverse FFT), dann Faltung ohne Latenz (256 Taps direkt, Blöcke
// 256 und 4096 im Frequenzbereich, die großen über 16 Teilschritte verteilt). Neue Impulsantworten (DECAY gedreht) entstehen stückweise im Audio-Faden
// (wie beim Gerät fährt die Dämpfungsplatte langsam), dann 100 ms Überblendung; beide Faltungen teilen sich die Eingangsspektren.
// Über etwa 88 kHz rechnet der Hallweg bei fs/2 bzw. fs/4 (Resamp); der trockene Weg bleibt bei fs.
#pragma once
#include <cmath>
#include <complex>
#include <climits>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
namespace blech {
constexpr double PI = 3.14159265358979323846;
typedef std::complex<float> cf;

// ---- Platte (Werte und Annahmen siehe ENTWURF.md) ----
struct Plate {
  double Lx = 2.0, Ly = 1.0, H = 0.0005, E = 2.0e11, nu = 0.3, rho = 7850, T0 = 700;   // T0 (N/m) angenommen: wirkt nur unter etwa 50 Hz
  double R1 = 9.664e-3, C1 = 0.1855e-3;   // thermoelastisch (Arcas/Chaigne; Russo 2023, Gl. 3.12–3.13)
  double xin = 0.52, yin = 0.53;          // Treiber (relativ, wie Ducceschi/Webb bzw. Russo)
  double xl = 0.30, yl = 0.62, xr = 0.74, yr = 0.38;   // Abnehmer links/rechts (angenommen: versetzt, verschieden weit vom Treiber)
  double fmax = 18000, hf10k = 11.8;      // Moden bis 18 kHz; Hochton-Verlust σ_h(f) = 11,8·(f/10 kHz)² (angenommen, T60(10 kHz) ≈ 0,9 s offen)
};
inline double sigmaTE(const Plate &p, double w) { double a = w * w * p.R1 * p.C1, b = 2 * (w * w * p.H * p.H + p.C1 * p.C1 / (p.H * p.H)); return a / b; }
inline double sigmaH(const Plate &p, double f) { return p.hf10k * (f / 10000) * (f / 10000); }
// Stärke der Dämpfungsplatte s (σ_d = s·√(f/500)) für eine Nachhallzeit t500 bei 500 Hz; über 3,8 s reicht die Platte allein nicht weiter
inline double damperFor(const Plate &p, double t500) { double s = 6.0 * std::log(10.0) / t500 - sigmaTE(p, 2 * PI * 500) - sigmaH(p, 500); return std::max(0.0, s); }
inline double sigmaAt(const Plate &p, double f, double s) { return sigmaTE(p, 2 * PI * f) + s * std::sqrt(std::max(0.0, f) / 500) + sigmaH(p, f); }
inline double t60At(const Plate &p, double f, double t500) { return 6.0 * std::log(10.0) / sigmaAt(p, f, damperFor(p, t500)); }
constexpr double T500LO = 1.0, T500HI = 3.8;   // Regler DECAY (s bei 500 Hz)
inline double t500Of(double knob01) { double x = std::isfinite(knob01) ? std::min(1.0, std::max(0.0, knob01)) : 0.5; return T500LO + (T500HI - T500LO) * x; }

struct Mode { float f, aL, aR; };   // Frequenz (Hz), Beitrag an den Abnehmern
inline std::vector<Mode> modes(const Plate &p) {
  std::vector<Mode> m; const double rh = p.rho * p.H, D = p.E * p.H * p.H * p.H / (12 * (1 - p.nu * p.nu)), wmax = 2 * PI * p.fmax;
  for (int a = 1;; a++) { double ka = a * PI / p.Lx; if ((D / rh) * ka * ka * ka * ka > wmax * wmax) break;
    for (int b = 1;; b++) { double kb = b * PI / p.Ly, k2 = ka * ka + kb * kb, w = std::sqrt((p.T0 / rh) * k2 + (D / rh) * k2 * k2); if (w > wmax) break;
      double si = std::sin(a * PI * p.xin) * std::sin(b * PI * p.yin);
      m.push_back({(float)(w / (2 * PI)), (float)(si * std::sin(a * PI * p.xl) * std::sin(b * PI * p.yl)), (float)(si * std::sin(a * PI * p.xr) * std::sin(b * PI * p.yr))}); } }
  std::sort(m.begin(), m.end(), [](const Mode &x, const Mode &y) { return x.f < y.f; }); return m; }

// ---- FFT (radix 2, komplex, float; in Schritten ausführbar) ----
struct FFT {
  int n = 0, lg = 0; std::vector<int> rev; std::vector<cf> tw;
  void init(int n_) { n = n_; lg = 0; while ((1 << lg) < n) lg++; rev.assign(n, 0); for (int i = 0; i < n; i++) { int r = 0; for (int b = 0; b < lg; b++) if (i >> b & 1) r |= 1 << (lg - 1 - b); rev[i] = r; }
    tw.resize(n / 2); for (int i = 0; i < n / 2; i++) tw[i] = cf((float)std::cos(2 * PI * i / n), (float)-std::sin(2 * PI * i / n)); }
  void bitrev(cf *a) const { for (int i = 0; i < n; i++) if (i < rev[i]) std::swap(a[i], a[rev[i]]); }
  void stage(cf *a, int s, bool inv) const { stagePart(a, s, inv, 0, n / 2); }   // Stufe s = 0 … lg−1
  void stagePart(cf *a, int s, bool inv, int b0, int b1) const { int h = 1 << s, st = n / (2 * h);   // Schmetterlinge b0 … b1−1 der Stufe s (in Stücken rechenbar)
    for (int b = b0; b < b1; b++) { int j = b & (h - 1), i = ((b >> s) << (s + 1)) + j; cf w = tw[j * st]; if (inv) w = std::conj(w); cf u = a[i], v = a[i + h] * w; a[i] = u + v; a[i + h] = u - v; } }
  void run(cf *a, bool inv) const { bitrev(a); for (int s = 0; s < lg; s++) stage(a, s, inv); }   // inverse ohne 1/n
};

// ---- Faltung ohne Latenz, zwei Impulsantworten (Überblendung) mit gemeinsamen Eingangsspektren ----
struct IRSet { std::vector<float> headL, headR; std::vector<cf> GA, GB; int PB = 0; };   // GA: PA Spektren à 512 (L + iR), GB: PB à 8192
struct Conv {
  static const int B = 256, NA = 512, PA = 31, BB = 4096, NB = 8192, SUB = BB / B;
  FFT fa, fb; int PBmax = 0;
  std::vector<float> hist;   // Eingang: Ring für die direkten Taps (doppelt lang, linear lesbar)
  int hp = 0; std::vector<float> inA, inB; int posA = 0, blkA = 0;   // inA: letzte 512 Eingangswerte; inB: letzte 8192
  std::vector<cf> fdlA, fdlB; int fa0 = 0, fb0 = 0;   // Eingangsspektren (Ringe), Index des jüngsten
  std::vector<cf> accA[2], accB[2], tmpA, tmpB; std::vector<float> outA[2][2], outB[2][2], nextB[2][2];   // je Satz: Ausgang L/R der Ebenen
  IRSet ir[2]; bool used[2] = {false, false};
  void init(int maxLen) { fa.init(NA); fb.init(NB); PBmax = std::max(1, (maxLen - 2 * BB + BB - 1) / BB);
    hist.assign(2 * B, 0.f); hp = 0; inA.assign(NA, 0.f); inB.assign(NB, 0.f); posA = 0; blkA = 0;
    fdlA.assign((size_t)PA * NA, cf()); fdlB.assign((size_t)PBmax * NB, cf()); fa0 = fb0 = 0; tmpA.assign(NA, cf()); tmpB.assign(NB, cf());
    for (int s = 0; s < 2; s++) { accA[s].assign(NA, cf()); accB[s].assign(NB, cf()); for (int c = 0; c < 2; c++) { outA[s][c].assign(B, 0.f); outB[s][c].assign(BB, 0.f); nextB[s][c].assign(BB, 0.f); } } }
  void clear() { std::fill(hist.begin(), hist.end(), 0.f); hp = 0; std::fill(inA.begin(), inA.end(), 0.f); std::fill(inB.begin(), inB.end(), 0.f); posA = 0; blkA = 0;
    std::fill(fdlA.begin(), fdlA.end(), cf()); std::fill(fdlB.begin(), fdlB.end(), cf());
    for (int s = 0; s < 2; s++) for (int c = 0; c < 2; c++) { std::fill(outA[s][c].begin(), outA[s][c].end(), 0.f); std::fill(outB[s][c].begin(), outB[s][c].end(), 0.f); std::fill(nextB[s][c].begin(), nextB[s][c].end(), 0.f); } }
  // Ebene B, verteilt über die 16 Teilblöcke eines 4096er-Blocks J: k = 0 … 14 Multiplikationen (Partitionen in Scheiben), k = 15 inverse FFT
  // → Ausgang für Block J+1. Am Ende jedes Blocks (pushB) kommt das Spektrum des Fensters aus den beiden zuletzt vollständigen Blöcken in den Ring.
  // Partition q (Abgriffe (q+2)·4096 … (q+3)·4096) × Spektrum des Eingangsblocks J−1−q, ausgegeben in J+1: Abstand q+2 Blöcke ✓.
  void stepB(int k, const bool act[2]) {
    if (k < SUB - 1) { for (int s = 0; s < 2; s++) { if (!act[s] || !used[s]) continue; if (k == 0) std::fill(accB[s].begin(), accB[s].end(), cf());
        const IRSet &r = ir[s]; int P = r.PB, per = (P + SUB - 2) / (SUB - 1), q0 = k * per, q1 = std::min(P, q0 + per);
        cf *acc = accB[s].data(); for (int q = q0; q < q1; q++) { const cf *x = fdlB.data() + (size_t)((fb0 + q) % PBmax) * NB, *g = r.GB.data() + (size_t)q * NB; for (int i = 0; i < NB; i++) acc[i] += x[i] * g[i]; } } return; }
    for (int s = 0; s < 2; s++) { if (!act[s] || !used[s]) continue; std::copy(accB[s].begin(), accB[s].end(), tmpB.begin()); fb.run(tmpB.data(), true); const float sc = 1.f / NB;
      for (int i = 0; i < BB; i++) { nextB[s][0][i] = tmpB[BB + i].real() * sc; nextB[s][1][i] = tmpB[BB + i].imag() * sc; } } }
  void pushB() { for (int i = 0; i < NB; i++) tmpB[i] = cf(inB[i], 0.f); fb.run(tmpB.data(), false); fb0 = (fb0 + PBmax - 1) % PBmax; std::copy(tmpB.begin(), tmpB.end(), fdlB.begin() + (size_t)fb0 * NB); }
  // Ebene A am Ende jedes 256er-Blocks (Ergebnis für den nächsten Block)
  void blockA(const bool act[2]) {
    for (int i = 0; i < NA; i++) tmpA[i] = cf(inA[i], 0.f); fa.run(tmpA.data(), false); fa0 = (fa0 + PA - 1) % PA; std::copy(tmpA.begin(), tmpA.end(), fdlA.begin() + (size_t)fa0 * NA);
    for (int s = 0; s < 2; s++) { if (!act[s] || !used[s]) continue; cf *acc = accA[s].data(); std::fill(accA[s].begin(), accA[s].end(), cf());
      for (int p = 0; p < PA; p++) { const cf *x = fdlA.data() + (size_t)((fa0 + p) % PA) * NA, *g = ir[s].GA.data() + (size_t)p * NA; for (int i = 0; i < NA; i++) acc[i] += x[i] * g[i]; }
      std::copy(accA[s].begin(), accA[s].end(), tmpA.begin()); fa.run(tmpA.data(), true); const float sc = 1.f / NA; for (int i = 0; i < B; i++) { outA[s][0][i] = tmpA[B + i].real() * sc; outA[s][1][i] = tmpA[B + i].imag() * sc; } } }
  // ein Abtastwert: Ausgang beider Sätze (act: welche gerechnet werden)
  inline void step(float x, const bool act[2], float yl[2], float yr[2]) {
    hist[hp] = x; hist[hp + B] = x; const float *h = &hist[hp + 1];   // h[0] ältester, h[B−1] = x
    for (int s = 0; s < 2; s++) { yl[s] = yr[s] = 0; if (!act[s] || !used[s]) continue; const float *a = ir[s].headL.data(), *b = ir[s].headR.data(); float l = 0, r = 0;
      for (int i = 0; i < B; i++) { l += a[i] * h[B - 1 - i]; r += b[i] * h[B - 1 - i]; } int k = posA, kb = blkA * B + posA;
      yl[s] = l + outA[s][0][k] + outB[s][0][kb]; yr[s] = r + outA[s][1][k] + outB[s][1][kb]; }
    if (++hp == B) hp = 0;
    inA[B + posA] = x; inB[BB + blkA * B + posA] = x;
    if (++posA == B) { posA = 0; blockA(act); std::memmove(inA.data(), inA.data() + B, B * sizeof(float));
      stepB(blkA, act); if (++blkA == SUB) { blkA = 0; pushB(); std::memmove(inB.data(), inB.data() + BB, BB * sizeof(float)); for (int s = 0; s < 2; s++) for (int c = 0; c < 2; c++) std::swap(outB[s][c], nextB[s][c]); } } }
};
// Ebene B: das Eingangsspektrum muss an der Blockgrenze gerechnet werden (Fenster der beiden zuletzt vollständigen Blöcke); bis 2026-11-15 (Entwurf)
// geschah es einen Teilblock später mit schon überschriebenen Werten: −15 dB Abweichung gegen die direkte Faltung (blechtest conv).

// ---- Impulsantwort aus den Moden (teuer, etwa 0,7 s Rechenzeit bei 48 kHz): im Plugin in einem Hilfsfaden, auf der Seite in einem eigenen Worker;
// in Schritten rechenbar (work), Ergebnis hL/hR (normiert) und t500 ----
struct IRJob {
  enum { IDLE, EARLY, SPEC, BITREV, STAGES, ASSEMBLE, DONE };
  int phase = IDLE, idx = 0, sub = 0; double t500 = 2, s = 0, fs = 48000; int N = 0, NE = 0, L = 0; FFT big; std::vector<cf> S; std::vector<double> eL, eR; std::vector<float> hL, hR;
  std::vector<Mode> md; Plate pl; double gain = 1;
  static constexpr double TAU = 0.015, HWM = 30, HWT = 6;   // weicher Einsatz des Spektralteils; Breite je Mode: 30 Halbwertsbreiten der Mode bzw. 6 des Einsatzes (−60 dB gegen exakt)
  static int lenOf(double fs) { return (int)std::lround(8.0 * fs); }
  void setup(double fs_) { fs = fs_; md = modes(pl); L = lenOf(fs); N = 1; while (N < (int)(L * 1.07)) N <<= 1;
    NE = (int)std::lround(0.12 * fs); big.init(N); S.assign(N, cf()); eL.assign(NE, 0.0); eR.assign(NE, 0.0); hL.assign(L, 0.f); hR.assign(L, 0.f); phase = IDLE; }
  void start(double t) { t500 = t; s = damperFor(pl, t); phase = EARLY; idx = 0; sub = 0; std::fill(S.begin(), S.end(), cf()); std::fill(eL.begin(), eL.end(), 0.0); std::fill(eR.begin(), eR.end(), 0.0); }
  void run(double t) { start(t); work(LONG_MAX); }
  // Arbeit in Einheiten (etwa eine komplexe Operation); gibt true zurück, wenn fertig
  bool work(long units) {
    const std::vector<Mode> &m = md; const int M = (int)m.size(); const Plate *pl = &this->pl;
    while (units > 0 && phase != DONE && phase != IDLE) {
      if (phase == EARLY) {   // erste 120 ms im Zeitbereich (Rekursion des gedämpften Kosinus)
        for (; idx < M && units > 0; idx++) { double f = m[idx].f, sg = 0.5 * sigmaAt(*pl, f, s), r = std::exp(-sg / fs), th = 2 * PI * f / fs, c1 = 2 * r * std::cos(th), c2 = -r * r;
          double y2 = 1, y1 = r * std::cos(th), aL = m[idx].aL, aR = m[idx].aR; eL[0] += aL; eR[0] += aR; if (NE > 1) { eL[1] += aL * y1; eR[1] += aR * y1; }
          for (int n = 2; n < NE; n++) { double y = c1 * y1 + c2 * y2; y2 = y1; y1 = y; eL[n] += aL * y; eR[n] += aR * y; } units -= NE; }
        if (idx >= M) { phase = SPEC; idx = 0; } continue; }
      if (phase == SPEC) {   // Spektrum des späten Teils: je Mode der gedämpfte Kosinus mit weichem Einsatz (1 − e^{−t/τ})², τ = 15 ms (stetig samt Ableitung,
        // die Ausläufer fallen mit 1/f³ statt 1/f; mit hartem Einsatz klang ein Fehler mit 1/t nach: −35 dB bei 1 s, siehe ENTWURF.md). Der Einsatz liegt ganz
        // im Zeitbereichsteil (0 … 120 ms). Exakt als Summe dreier gedämpfter Exponentialfolgen z^n − 2(zq)^n + (zq²)^n, je DFT (1 − a^N)/(1 − a·w).
        const double df = fs / N, q = std::exp(-1 / (TAU * fs)), gT = 1 / (2 * PI * TAU);
        for (; idx < M && units > 0; idx++) { double f = m[idx].f, sg = 0.5 * sigmaAt(*pl, f, s), r = std::exp(-sg / fs), k0 = f / df, gam = sg / (2 * PI);
          int half = std::max(32, (int)std::ceil(std::max(HWM * gam, HWT * gT) / df)); int ka = (int)std::floor(k0) - half, kb = (int)std::ceil(k0) + half;
          double rj[3] = {r, r * q, r * q * q}, cj[3] = {1, -2, 1}, nr[3], ni[3];
          for (int j = 0; j < 3; j++) { std::complex<double> nm = 1.0 - std::polar(std::pow(rj[j], N), 2 * PI * k0); nr[j] = cj[j] * nm.real(); ni[j] = cj[j] * nm.imag(); }
          if (ka < 1) ka = 1; kb = std::min(kb, N / 2 - 1);
          std::complex<double> p = std::polar(1.0, 2 * PI * (k0 - ka) / N), rot = std::polar(1.0, -2 * PI / N); double pr = p.real(), pi_ = p.imag(); const double cr = rot.real(), ci = rot.imag();
          const double aL = 0.5 * m[idx].aL, aR = 0.5 * m[idx].aR;
          for (int k = ka; k <= kb; k++) { double xr = 0, xi = 0;
            for (int j = 0; j < 3; j++) { double dr = 1 - rj[j] * pr, di = -rj[j] * pi_, qq = 1 / (dr * dr + di * di); xr += (nr[j] * dr + ni[j] * di) * qq; xi += (ni[j] * dr - nr[j] * di) * qq; }
            float lr = (float)(aL * xr), li = (float)(aL * xi), rr = (float)(aR * xr), ri = (float)(aR * xi);
            S[k] += cf(lr - ri, li + rr); S[N - k] += cf(lr + ri, rr - li);   // L + i·R; Spiegel: conj(L) + i·conj(R)
            double t = pr * cr - pi_ * ci; pi_ = pr * ci + pi_ * cr; pr = t; if (((k - ka) & 255) == 255) { double nn = 1 / std::sqrt(pr * pr + pi_ * pi_); pr *= nn; pi_ *= nn; } }
          units -= 4 * (kb - ka + 1); }
        if (idx >= M) { phase = BITREV; } continue; }
      if (phase == BITREV) { big.bitrev(S.data()); units -= N; phase = STAGES; idx = 0; continue; }
      if (phase == STAGES) { int b0 = sub, b1 = std::min(N / 2, b0 + (int)std::max(1024L, std::min(units, (long)N / 2))); big.stagePart(S.data(), idx, true, b0, b1); units -= 2 * (b1 - b0); sub = b1;
        if (sub >= N / 2) { sub = 0; if (++idx >= big.lg) { phase = ASSEMBLE; idx = 0; } } continue; }
      if (phase == ASSEMBLE) {   // früh (Zeitbereich) und spät (Spektrum) überblenden 100 … 120 ms, Ende 7 … 8 s ausblenden, Energie normieren
        const float sc = 1.f / N; const int x0 = (int)std::lround(0.10 * fs), x1 = NE, f0 = (int)std::lround(7.0 * fs); double eSum = 0;
        for (int n = 0; n < L; n++) { double l = S[n].real() * sc, r = S[n].imag() * sc;
          if (n < x1) { double w = n < x0 ? 0.0 : 0.5 - 0.5 * std::cos(PI * (n - x0) / (x1 - x0)); l = (1 - w) * eL[n] + w * l; r = (1 - w) * eR[n] + w * r; }
          if (n >= f0) { double w = 0.5 + 0.5 * std::cos(PI * (n - f0) / (L - f0)); l *= w; r *= w; }
          hL[n] = (float)l; hR[n] = (float)r; eSum += 0.5 * (l * l + r * r); }
        // Hall bei Rauschen so laut wie trocken bei 2 s, mit der Nachhallzeit wachsend wie (T/2 s)^¼ (wie Kaiserin)
        gain = std::sqrt(std::sqrt(t500 / 2.0) / std::max(eSum, 1e-30)); for (int n = 0; n < L; n++) { hL[n] *= (float)gain; hR[n] *= (float)gain; }
        units -= 4 * L; phase = DONE; continue; }
    }
    return phase == DONE; }
};

// ---- Zerlegung einer Impulsantwort für die Faltung (billig, etwa 20 ms; stückweise im Audio-Faden) ----
struct Partitioner {
  int L = 0, idx = 0, phase = 0; bool busy = false; double t500 = 2; std::vector<float> hL, hR; IRSet out; FFT fa, fb;
  void setup(int L_) { L = L_; hL.assign(L, 0.f); hR.assign(L, 0.f); fa.init(Conv::NA); fb.init(Conv::NB);
    out.headL.assign(Conv::B, 0.f); out.headR.assign(Conv::B, 0.f); out.GA.assign((size_t)Conv::PA * Conv::NA, cf()); out.PB = (L - 2 * Conv::BB + Conv::BB - 1) / Conv::BB; out.GB.assign((size_t)out.PB * Conv::NB, cf()); }
  void start(double t) { t500 = t; busy = true; phase = 0; idx = 0; }   // hL/hR vorher gefüllt (getauscht)
  bool work(long units) {
    while (busy && units > 0) {
      if (phase == 0) { std::copy(hL.begin(), hL.begin() + Conv::B, out.headL.begin()); std::copy(hR.begin(), hR.begin() + Conv::B, out.headR.begin()); phase = 1; idx = 0; continue; }
      if (phase == 1) { for (; idx < Conv::PA && units > 0; idx++) { cf *g = out.GA.data() + (size_t)idx * Conv::NA; int o = (idx + 1) * Conv::B;
          for (int i = 0; i < Conv::NA; i++) g[i] = i < Conv::B && o + i < L ? cf(hL[o + i], hR[o + i]) : cf(); fa.run(g, false); units -= Conv::NA * 5; }
        if (idx >= Conv::PA) { phase = 2; idx = 0; } continue; }
      for (; idx < out.PB && units > 0; idx++) { cf *g = out.GB.data() + (size_t)idx * Conv::NB; int o = (idx + 2) * Conv::BB;
        for (int i = 0; i < Conv::NB; i++) g[i] = i < Conv::BB && o + i < L ? cf(hL[o + i], hR[o + i]) : cf(); fb.run(g, false); units -= Conv::NB * 7; }
      if (idx >= out.PB) busy = false; }
    return !busy; }
};

// ---- ganzes Gerät ----
struct Setting { double decay = 0.357, mix = 0.3, out = 0.5, pre = 0, bass = 0; };   // decay 0 … 1 (1 … 3,8 s bei 500 Hz; 0,357 = 2 s wie die Werkseinstellung), mix 0 … 1 (Anteil Hall), out 0 … 1 (−24 … +24 dB), pre 0 … 1 (0 … 200 ms), bass 0 … 3
static const double BASSCUT[4] = {20, 100, 200, 400};   // Tiefenabsenkung 0 … 3 (Hochpass 1. Ordnung am Hallausgang; Stellung 0 nur gegen Gleichanteil); Frequenzen angenommen
inline double outDb(double o) { return (std::min(1.0, std::max(0.0, o)) - 0.5) * 48; }
inline double preMs(double p) { double x = std::min(1.0, std::max(0.0, p)); return 200 * x * x; }
// Abtastratenwandler um den ganzen Faktor k (linearphasiges FIR, Kaiser-Fenster, 90 dB): über etwa 88 kHz rechnet der Hall bei fs/k (44 … 88 kHz),
// die Platte hat ohnehin nur Moden bis 18 kHz. Grenzfrequenz fsi/2, Übergang 18 kHz … fsi − 18 kHz (gespiegelte Anteile fallen über 18 kHz, wo die
// Platte nichts mehr überträgt). Laufzeit im Hallweg (N−1)/fs (bei 192 kHz etwa 0,5 ms), der trockene Weg bleibt ohne Verzögerung.
inline int factorOf(double fs) { return std::max(1, (int)(fs / 44000)); }
struct Resamp {
  int k = 1, N = 0, M = 0; std::vector<double> h, xd, yl, yr; int xp = 0, yp = 0;
  void setup(double fs, int k_) { k = std::max(1, k_); xd.clear(); yl.clear(); yr.clear(); h.clear(); N = M = 0; if (k == 1) return;
    const double fsi = fs / k, df = std::max(2000.0, fsi - 36000), A = 90, beta = 0.1102 * (A - 8.7), fc = 0.5 * fsi / fs;
    int n = (int)std::ceil((A - 7.95) / (2.285 * 2 * PI * df / fs)) + 1; M = (n + k - 1) / k; N = M * k; h.assign(N, 0.0);
    auto i0 = [](double x) { double s = 1, t = 1; for (int j = 1; j < 40; j++) { t *= (x / (2 * j)) * (x / (2 * j)); s += t; } return s; };
    double sum = 0; for (int t = 0; t < N; t++) { double m = t - 0.5 * (N - 1), r = 2 * m / (N - 1), w = i0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / i0(beta), x = 2 * fc * m;
      h[t] = 2 * fc * (std::fabs(x) < 1e-12 ? 1.0 : std::sin(PI * x) / (PI * x)) * w; sum += h[t]; }
    for (double &v : h) v /= sum; xd.assign(2 * N, 0.0); yl.assign(2 * M, 0.0); yr.assign(2 * M, 0.0); xp = yp = 0; }
  void clear() { std::fill(xd.begin(), xd.end(), 0.0); std::fill(yl.begin(), yl.end(), 0.0); std::fill(yr.begin(), yr.end(), 0.0); xp = yp = 0; }
  void push(double x) { xd[xp] = x; xd[xp + N] = x; if (++xp == N) xp = 0; }   // Eingang bei fs
  double decim() const { const double *a = &xd[xp]; double y = 0; for (int t = 0; t < N; t++) y += h[t] * a[N - 1 - t]; return y; }   // a[N−1] jüngster
  void interp(double l, double r, double *oL, double *oR) { yl[yp] = l; yl[yp + M] = l; yr[yp] = r; yr[yp + M] = r; if (++yp == M) yp = 0; const double *a = &yl[yp], *b = &yr[yp];
    for (int p = 0; p < k; p++) { double sl = 0, sr = 0; for (int m = 0; m < M; m++) { double g = h[p + m * k]; sl += g * a[M - 1 - m]; sr += g * b[M - 1 - m]; } oL[p] = k * sl; oR[p] = k * sr; } }
};

struct Reverb {
  Conv cv; Partitioner part; IRJob job; bool internalJob = true; double fs = 48000, fsi = 48000; int K = 1, ph = 0; Resamp rs; double obL[16] = {}, obR[16] = {};
  int act = 0; bool fading = false; int warm = 0, fadeN = 0, fadePos = 0; double t500Act = 2, t500New = 2, t500Tgt = 2;
  std::vector<float> pendL, pendR; double pendT = 0; bool pend = false;   // angebotene Impulsantwort, die noch nicht zerlegt werden konnte
  std::vector<float> pre; int pw = 0; double preS = 0, preT = 0; double hp1[2] = {}, hpx[2] = {}, kHp = 0; int bassAct = 0; double mixS = -1, outS = -1;
  long jobUnits = 800, partUnits = 3000;   // Rechenbudget je (interner) Abtastwert im Audio-Faden: Moden (nur internalJob, etwa 18 s je Platte bei 48 kHz) bzw. Zerlegung (etwa 0,1 s)
  // Hauptfaden: Moden, Puffer, erste Impulsantwort (DECAY t01) vollständig; Speicher aller Sätze vorab (danach kein malloc/free im Audio-Faden)
  void setup(double fs_, double t01 = 0.357) { fs = fs_; K = std::min(16, factorOf(fs)); fsi = fs / K; rs.setup(fs, K); ph = 0; std::fill(obL, obL + 16, 0.0); std::fill(obR, obR + 16, 0.0);
    job.setup(fsi); const int L = job.L; cv.init(L); part.setup(L); pendL.assign(L, 0.f); pendR.assign(L, 0.f);
    fadeN = (int)std::lround(0.1 * fsi); act = 0; warm = 0; fading = false; pend = false; pre.assign((size_t)(0.25 * fsi) + 4, 0.f); pw = 0; preS = 0; t500Tgt = t500Of(t01);
    job.run(t500Tgt); job.phase = IRJob::IDLE; std::swap(part.hL, job.hL); std::swap(part.hR, job.hR); part.start(t500Tgt); part.work(LONG_MAX); cv.ir[0] = part.out; cv.ir[1] = part.out;
    cv.used[0] = true; cv.used[1] = false; t500Act = t500New = t500Tgt; setBass(0); }
  // fertige Impulsantwort anbieten (Audio-Faden; L, R werden getauscht, der Aufrufer bekommt Puffer gleicher Größe zurück)
  void offer(std::vector<float> &L, std::vector<float> &R, double t) { if ((int)L.size() != part.L || (int)R.size() != part.L) return; std::swap(pendL, L); std::swap(pendR, R); pendT = t; pend = true; }
  // neue Impulsantwort einbauen: Speicher getauscht, nie freigegeben. Der neue Satz rechnet sofort mit; überblendet wird erst nach zwei
  // 4096er-Grenzen, wenn seine Ebene B (Abgriffe ab 8192) vollständig gerechnet ist.
  void install() { int s = 1 - act; std::swap(cv.ir[s], part.out); cv.used[s] = true; t500New = part.t500; warm = 2; fadePos = 0; }
  void setBass(int b) { bassAct = std::max(0, std::min(3, b)); kHp = std::exp(-2 * PI * BASSCUT[bassAct] / fsi); }
  void reset() { cv.clear(); rs.clear(); ph = 0; std::fill(obL, obL + 16, 0.0); std::fill(obR, obR + 16, 0.0); std::fill(pre.begin(), pre.end(), 0.f); pw = 0; for (int c = 0; c < 2; c++) hp1[c] = hpx[c] = 0; }
  double decayShown() const { if (!fading) return t500Act; double w = (double)fadePos / fadeN; return t500Act + (t500New - t500Act) * w; }   // „Zeiger“: wandert beim Überblenden
  bool changing() const { return pend || part.busy || fading || warm || (internalJob && job.phase != IRJob::IDLE); }
  // Hallweg bei der internen Rate fsi: Vorverzögerung, Faltung (mit Überblendung), Tiefenabsenkung
  inline void wetStep(double u, double &wl, double &wr) {
    pre[pw] = (float)u; int PN = (int)pre.size(); preS += (preT - preS) * 0.0005 * K; double d = std::min((double)PN - 3, preS); int di = (int)d; double fr = d - di;
    int a0 = pw - di; if (a0 < 0) a0 += PN; int a1 = a0 - 1; if (a1 < 0) a1 += PN; float x = (float)((1 - fr) * pre[a0] + fr * pre[a1]); if (++pw == PN) pw = 0;
    bool on[2] = {false, false}; on[act] = true; if (fading || warm) on[1 - act] = true;
    float yl[2], yr[2]; cv.step(x, on, yl, yr);
    if (warm && cv.posA == 0 && cv.blkA == 0 && --warm == 0) fading = true;   // eben eine 4096er-Grenze überschritten
    wl = yl[act]; wr = yr[act];
    if (fading) { double w = (double)fadePos / fadeN; w = 0.5 - 0.5 * std::cos(PI * w); wl = (1 - w) * yl[act] + w * yl[1 - act]; wr = (1 - w) * yr[act] + w * yr[1 - act];
      if (++fadePos >= fadeN) { fading = false; cv.used[act] = false; act = 1 - act; t500Act = t500New; } }
    double w2[2] = {wl, wr};
    for (int c = 0; c < 2; c++) { double v = w2[c], y = kHp * (hp1[c] + v - hpx[c]); hpx[c] = v; hp1[c] = std::fabs(y) > 1e-30 ? y : 0; w2[c] = hp1[c]; }   // Tiefenabsenkung (1. Ordnung)
    wl = w2[0]; wr = w2[1]; }
  void process(const float *inL, const float *inR, float *oL, float *oR, int n, const Setting &st) {
    t500Tgt = t500Of(st.decay); const long ni = std::max(1, n / K);   // Rechenbudget nach internen Abtastwerten
    if (internalJob) {   // ohne Hilfsfaden (Tests, Rückfall): Moden langsam im Audio-Faden
      if (job.phase == IRJob::IDLE && !pend && !part.busy && !fading && !warm && std::fabs(t500Tgt - t500Act) > 0.005) job.start(t500Tgt);
      if (job.phase != IRJob::IDLE && job.work(jobUnits * ni)) { std::swap(pendL, job.hL); std::swap(pendR, job.hR); pendT = job.t500; pend = true; job.phase = IRJob::IDLE; } }
    if (pend && !part.busy && !fading && !warm) { std::swap(part.hL, pendL); std::swap(part.hR, pendR); part.start(pendT); pend = false; }
    if (part.busy && part.work(partUnits * ni)) install();
    int b = (int)std::lround(std::min(3.0, std::max(0.0, st.bass))); if (b != bassAct) setBass(b);
    double mixT = std::min(1.0, std::max(0.0, st.mix)), outT = std::pow(10.0, outDb(st.out) / 20); if (mixS < 0) { mixS = mixT; outS = outT; }
    preT = preMs(st.pre) * 1e-3 * fsi; const double kS = 1 - std::exp(-1 / (0.02 * fs));
    for (int i = 0; i < n; i++) {
      double xl = inL[i], xr = inR ? inR[i] : xl; if (!std::isfinite(xl)) xl = 0; if (!std::isfinite(xr)) xr = 0; double u = 0.5 * (xl + xr), wl, wr;
      if (K == 1) wetStep(u, wl, wr);
      else { rs.push(u); if (++ph == K) { ph = 0; double a, c; wetStep(rs.decim(), a, c); rs.interp(a, c, obL, obR); } wl = obL[ph]; wr = obR[ph]; }
      mixS += (mixT - mixS) * kS; outS += (outT - outS) * kS; double dry = 1 - mixS, wet = mixS;
      oL[i] = (float)(outS * (dry * xl + wet * wl)); oR[i] = (float)(outS * (dry * xr + wet * wr)); } }
};
} // namespace blech
