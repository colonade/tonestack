// Yamaha B100-112III / B100-115III (Service Manual 006594, 1984: Schaltplan Seite 14 = Blätter 13/14 des PDFs, Spezifikation Seite 1,
// Frontplatte Seite 2). Vorverstärker als Schaltung im Zeitbereich (DK-Löser, dk.hpp), Bauteil für Bauteil nach dem Plan:
// Eingang 560 / 47 p / 560 / 10 µ / 1 M, IC101 1/2 (×11, Doppeldiode D101 zwischen den Eingängen), VOLUME A25K, IC101 2/2 (×2),
// aktive Klangregelung IC103 1/2 (3,3 k / 3,3 k, drei G20K zwischen den Eingängen: TREBLE über 1 k + 0,22 µ, MIDDLE und BASS über
// Transistor-Gyratoren Tr103 / Tr104), zwei vollparametrische Entzerrer (je: Summierpaar IC104 1/2 + IC103 2/2 bzw. IC107 2/2 + IC107 1/2
// mit LEVEL G20K zwischen den Summenpunkten, Zustandsvariablenfilter mit C50K-Tandem und 0,068 µ, Q über A25K, Bandpass-Verstärker ×3,07,
// Ein/Aus-Transistoren Tr105 / Tr109, PRESET LEVEL G20K an IC106 2/2 bzw. IC110 1/2 mit Tr107 / Tr111), PRE AMP OUT / POWER AMP IN,
// die Lasten am Knoten PI (BAL OUTPUT LEVEL, Kopfhörer- und Weichen-Eingang), Eingang der Endstufe (560, 10 µ, 100 k, 47 p).
// Endstufe: Form aus der Gegenkopplung (330 k gegen 1 k + 10 µ, 220 k vom Gleichspannungs-Servo IC113), Pegel siehe pa_gain,
// weiche Begrenzung an der Versorgung, gleichspannungsgekoppelt an 8 Ω. Kein Lautsprecher, keine Frequenzweiche, kein symmetrischer Ausgang
// (nur ihre Lasten), kein Kopfhörerverstärker.
#pragma once
#include "gk_core.hpp"   // expg, Clamp/clampI, potTaper; aor::IIR, bilinear
#include <atomic>
#include <cmath>
#include <complex>
#include <string>
#include <vector>
namespace b3 {
constexpr double PI = 3.14159265358979323846;
using gk::Clamp; using gk::potTaper;

// Doppeldiode D101 (MC931, zwei Siliziumdioden gegeneinander): Kennwerte geschätzt wie 1S1555 im G100
struct DiodePair { double is, nvt; };
inline void diodePairI(const DiodePair &d, double v, double *i, double *J) { double e1, d1, e2, d2; gk::expg(v / d.nvt, e1, d1); gk::expg(-v / d.nvt, e2, d2);
  i[0] = d.is * (e1 - e2); J[0] = d.is * (d1 + d2) / d.nvt; }
enum Kind { K_CLAMP = 0, K_DIODE = 1 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  if (d.kind == K_CLAMP) gk::clampI(*(const Clamp *)d.model, v[0], i, J); else diodePairI(*(const DiodePair *)d.model, v[0], i, J); }

struct Parts {   // Werte laut Plan (Ω, F); Schätzungen markiert
  double rsrc = 1e3;   // Innenwiderstand der Quelle (geschätzt)
  double r_i1 = 560, c_i1 = 47e-12, r_i2 = 560, c_i2 = 10e-6, r_ig = 1e6, c_id = 47e-12, r1f = 10e3, r1g = 1e3, c1g = 10e-6;   // Eingang, IC101 1/2: 10 k / 1 k + 10 µ, 47 p zwischen den Eingängen
  double c_v1 = 10e-6, p_vol = 25e3, c_v2 = 10e-6, r_v2 = 560, r_v3 = 100e3, c_v3 = 220e-12, c_v4 = 100e-12, r2f = 1e3, r2g = 1e3, c2g = 10e-6;   // VOLUME A25K, IC101 2/2: 1 k / 1 k + 10 µ
  double r_t1 = 3.3e3, r_t2 = 3.3e3, c_t = 47e-12, p_tone = 20e3;   // IC103 1/2: 3,3 k Eingang, 3,3 k Gegenkopplung, 47 p zwischen den Eingängen; TREBLE/MIDDLE/BASS G20K
  double r_tr = 1e3, c_tr = 0.22e-6;   // TREBLE: 1 k + 0,22 µ gegen Masse
  double c_ms = 0.47e-6, c_bs = 2.2e-6, c_g1 = 0.015e-6, c_g2 = 0.033e-6, r_gb = 56e3, r_gs = 560, r_ge = 680, r_gx = 3.3e3;   // Gyratoren Tr103 (MIDDLE) / Tr104 (BASS)
  double t_gm = 0.168, t_rpi = 1.8e3;   // Tr103/Tr104 2SC2320 als Emitterfolger: Ic ≈ 4,4 mA (Basis über 56 k an Masse, 3,3 k nach −15 V), β ≈ 300 (geschätzt)
  double r_eqin = 10e3, r_mx = 10e3, r_xs = 10e3, r_ys1 = 10e3, r_ys2 = 4.7e3, p_lev = 20e3, r_yg = 1e3, c_yf = 220e-12;   // Summierpaar; PEQ2: 4,7 k von Y auf den Summenpunkt (Plan)
  double r_sx = 10e3, r_sf = 10e3, c_sf = 100e-12, r_sl = 10e3, r_f = 1e3, p_f = 50e3, c_f = 0.068e-6;   // Zustandsvariablenfilter
  double p_q = 25e3, r_qg = 4.7e3, r_qs = 560, r_bg = 2.7e3, r_bf = 5.6e3;   // Q (A25K, als Regelwiderstand), 4,7 k, 560; Bandpass-Verstärker 1 + 5,6 k / 2,7 k
  double r_l1 = 1e3, c_l1 = 22e-6, r_lg = 10e3, c_l2 = 22e-6, r_l2 = 1e3, r_sw = 1;   // LEVEL-Zuführung; Schalttransistoren 2SC2878 (Tr105/107/109/111) durchgeschaltet: Datenblatt RON 1 Ω typ. (IB 5 mA)
  double r_p1 = 3.3e3, c_pg = 220e-12, c_pd = 47e-12, r_pf = 3.3e3, p_pre = 20e3, r_pw = 1.5e3, c_pw = 4.7e-6, r_pq = 100e3;   // PRESET LEVEL
  double r_eq2in = 10e3;   // IC106 2/2 → PEQ2
  double c_po = 10e-6, r_po = 560, r_pog = 100e3, c_pog = 220e-12, r_pi = 560, c_pig = 220e-12;   // PO → 560 → PRE AMP OUT (100 k, 220 p) → POWER AMP IN → 560 → PI (220 p)
  double p_bal = 25e3, bal_x = 0.5, r_b1 = 560, c_b1 = 10e-6, r_b2 = 100e3, c_b2 = 47e-12;   // Last BAL OUTPUT LEVEL (A25K; Rückseite, Grundstellung Mitte laut Bedienungsanleitung) mit 560 / 10 µ / 100 k ∥ 47 p
  double c_h1 = 10e-6, r_h1 = 560, r_h2 = 100e3, c_h2 = 47e-12;   // Kopfhörer-/Weichenzweig: 10 µ, 560, 100 k ∥ 47 p (IC110 2/2; IC111 2/2 + hängt ohne Strom an derselben Leitung)
  double r_mi = 560, c_mi = 10e-6, r_mg = 100e3, c_mg = 47e-12;   // MI (über die Schaltkontakte von LPF/HPF OUTPUT) → 560 → 10 µ → Gate FET101 (100 k, 47 p)
  double op_a = 1e4, op_rout = 100, op_vs = 13.5;   // NJM2041/NJM4558 an ±15 V (geschätzt)
  double d_is = 2.5e-9, d_nvt = 0.045;   // D101 (geschätzt)
  // Endstufe: 1 + 330 k / (1 k + 1/(s·10 µ) ∥ 220 k am Servo) = 332,5 im Band; Servo IC113 (15 k · 10 µ, Umkehrer 10 k / 10 k) gibt einen Hochpass bei 1,6 Hz
  double pa_rf = 330e3, pa_rg = 1e3, pa_cg = 10e-6, pa_rs = 220e3, pa_ri = 15e3, pa_ci = 10e-6;
  double pa_gain = 115;   // Verstärkung der Endstufe im Band: Spezifikation POWER AMP IN → SPEAKER OUT 41 dB (115-fach); aus dem Plan folgen 332,5 (50,4 dB), siehe SCHALTPLAN.md
  double pa_knee = 37, pa_lim = 44, pa_rload = 8;   // Begrenzung (V Spitze an 8 Ω): 100 W bei 10 % Klirr = 40 V; weiches Knie (geschätzt)
};
// Regler 0…1 (Knopf 0…10; TREBLE/MIDDLE/BASS/LEVEL/PRESET: 0,5 = Mitte = 0); Schalter
struct Setting { double vol = 0.5, treb = 0.5, mid = 0.5, bass = 0.5, lev1 = 0.5, q1 = 0.5, f1 = 0.5, pre1 = 0.5, lev2 = 0.5, q2 = 0.5, f2 = 0.5, pre2 = 0.5;
  bool eq1 = true, eq2 = true; int taper = 0; };   // eq1/eq2: Entzerrer eingeschaltet (Fußschalter); taper: Kennlinie der A-Potis (0 log 15 %, 1 log 10 %, 2 linear)
constexpr int NKNOB = 12;
inline double &sfield(Setting &s, int i) { double *f[NKNOB] = {&s.vol, &s.treb, &s.mid, &s.bass, &s.lev1, &s.q1, &s.f1, &s.pre1, &s.lev2, &s.q2, &s.f2, &s.pre2}; return *f[i]; }
// FREQUENCY-Skala der Frontplatte: 50 (links), 100, 250 (oben), 500, 2.5K (rechts); Winkel der Marken 100 und 500 aus der Zeichnung (Seite 2): etwa 29 % und 71 %
inline double freqOfKnob(double x) { const double xs[5] = {0, 0.29, 0.5, 0.71, 1}, fs[5] = {50, 100, 250, 500, 2500}; x = std::min(1.0, std::max(0.0, x));
  for (int k = 0; k < 4; k++) if (x <= xs[k + 1]) { double t = (x - xs[k]) / (xs[k + 1] - xs[k]); return fs[k] * std::pow(fs[k + 1] / fs[k], t); } return fs[4]; }
// Bahnwiderstand des C50K-Tandems aus der Frequenz (1 k + R, 0,068 µ); oben begrenzt das Poti: R = 0 gibt 2,34 kHz (Skala: 2.5K)
inline double freqPotR(const Parts &v, double x) { double R = 1 / (2 * PI * freqOfKnob(x) * v.c_f) - v.r_f; return std::min(v.p_f, std::max(0.0, R)); }

enum { O_G = 0, O_A1, O_A2, O_T, O_X1, O_Y1, O_H1, O_B1, O_L1, O_K1, O_Z1, O_X2, O_Y2, O_H2, O_B2, O_L2, O_K2, O_Z2, O_PR, O_NO };   // Gate FET101, IC101 1/2, 2/2, IC103 1/2, je PEQ: X, Y, Hochpass, Bandpass, Tiefpass, Bandpass-Verstärker, PRESET-Ausgang; PRE AMP OUT
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; Clamp clamp; DiodePair dp; };
inline void buildInto(Built &B, const Parts &v, const Setting &s) {
  dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0; B.clamp = {v.op_vs}; B.dp = {v.d_is, v.d_nvt};
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { x = std::min(1.0, std::max(0.0, x)); o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };   // x = 1: Schleifer an a
  auto opamp = [&](const std::string &inp, const std::string &inn, const std::string &out) { o.vccs("0", out, inp, inn, v.op_a / v.op_rout); o.r(out, "0", v.op_rout);
    int p = o.port(out, "0"); dev.push_back({K_CLAMP, p, 1, &B.clamp}); };
  auto tp = [&](double x) { return potTaper(std::min(1.0, std::max(0.0, x)), s.taper); };
  auto clampx = [](double x) { return std::min(1.0, std::max(0.0, x)); };
  // --- Eingang: Buchse → 560 → 47 p → 560 → 10 µ → IC101 1/2 + (1 M); 47 p und D101 zwischen den Eingängen; 10 k / 1 k + 10 µ ---
  o.src("in", v.rsrc, 0); o.r("in", "i1", v.r_i1); o.c("i1", "0", v.c_i1); o.r("i1", "i2", v.r_i2); o.c("i2", "N", v.c_i2); o.r("N", "0", v.r_ig);
  opamp("N", "n1", "a1"); o.c("N", "n1", v.c_id); o.r("a1", "n1", v.r1f); o.r("n1", "n1g", v.r1g); o.c("n1g", "0", v.c1g);
  { int p = o.port("N", "n1"); dev.push_back({K_DIODE, p, 1, &B.dp}); }
  // --- VOLUME A25K (10 µ davor, Schleifer → 10 µ → 560 → IC101 2/2 + mit 100 k, 220 p gegen Masse und 100 p zwischen den Eingängen); 1 k / 1 k + 10 µ ---
  o.c("a1", "vt", v.c_v1); pot("vt", "vw", "0", v.p_vol, tp(s.vol)); o.c("vw", "vc", v.c_v2); o.r("vc", "P2", v.r_v2); o.r("P2", "0", v.r_v3); o.c("P2", "0", v.c_v3); o.c("P2", "n2", v.c_v4);
  opamp("P2", "n2", "a2"); o.r("a2", "n2", v.r2f); o.r("n2", "n2g", v.r2g); o.c("n2g", "0", v.c2g);
  mark();
  // --- Klangregelung IC103 1/2: a2 → 3,3 k → A = + Eingang; Gegenkopplung 3,3 k auf den − Eingang Bn; je ein G20K von A (links) nach Bn (rechts = CW = Anhebung),
  //     Schleifer auf den Reihenkreis: TREBLE 1 k + 0,22 µ; MIDDLE 0,47 µ + Gyrator Tr103 (0,015 µ, 56 k, 560, 680, 3,3 k); BASS 2,2 µ + Gyrator Tr104 (0,033 µ) ---
  G = 1; o.r("a2", "A", v.r_t1); o.r("aT", "Bn", v.r_t2); o.c("Bn", "A", v.c_t); opamp("A", "Bn", "aT");
  pot("Bn", "tw", "A", v.p_tone, clampx(s.treb)); o.r("tw", "tc", v.r_tr); o.c("tc", "0", v.c_tr);
  auto gyr = [&](const std::string &k, double x, double cs, double cg) {   // Schleifer → cs → g; g → cg → Basis (56 k gegen Masse) → 560 → Tr; Emitter (3,3 k nach −15 V) → 680 → g
    pot("Bn", "w" + k, "A", v.p_tone, clampx(x)); o.c("w" + k, "g" + k, cs); o.c("g" + k, "b" + k, cg); o.r("b" + k, "0", v.r_gb); o.r("b" + k, "bb" + k, v.r_gs);
    o.r("bb" + k, "e" + k, v.t_rpi); o.vccs("0", "e" + k, "bb" + k, "e" + k, v.t_gm); o.r("e" + k, "0", v.r_gx); o.r("e" + k, "g" + k, v.r_ge); };
  gyr("m", s.mid, v.c_ms, v.c_g1); gyr("b", s.bass, v.c_bs, v.c_g2);
  mark();
  // --- Parametrischer Entzerrer (zweimal): in → 10 k → M (Summenpunkt des Verbunds IC Y: + = M, − über 1 k an Masse und 220 p vom Ausgang Y);
  //     M → 10 k → X (Ausgang des Umkehrers, − = S, + Masse); X → 10 k → S; Y → ys → S; LEVEL G20K von M nach S (CW = S), Schleifer über 1 k, 22 µ,
  //     (10 k, Schalttransistor) gegen Masse, 22 µ, 1 k vom Bandpass-Verstärker; Zustandsvariablenfilter: X → 10 k → Summierer (− ; 10 k ∥ 100 p Gegenkopplung,
  //     10 k vom Tiefpass; + = Q-Knoten über 560), Integratoren 1 k + C50K und 0,068 µ; Bandpass → Q (A25K als Regelwiderstand) → 4,7 k gegen Masse;
  //     Bandpass-Verstärker (+ = Q-Knoten über 560, 2,7 k / 5,6 k); PRESET: Y → 3,3 k → P (220 p) = +, − = Nn (3,3 k vom Ausgang, 47 p nach P),
  //     G20K von Nn (CW) nach P, Schleifer → 1,5 k → 4,7 µ → (100 k, Schalttransistor) gegen Masse ---
  auto peq = [&](const std::string &k, const std::string &in, double rin, double rys, double lev, double q, double f, double pre, bool on) {
    std::string M = "M" + k, X = "X" + k, S = "S" + k, Y = "Y" + k, H = "H" + k, Bp = "BP" + k, L = "LP" + k;
    o.r(in, M, rin); o.r(M, X, v.r_mx); opamp("0", S, X); o.r(X, S, v.r_xs); o.r(Y, S, rys);
    opamp(M, "yn" + k, Y); o.r("yn" + k, "0", v.r_yg); o.c("yn" + k, Y, v.c_yf);
    pot(S, "lw" + k, M, v.p_lev, clampx(lev));
    o.r("lw" + k, "lc" + k, v.r_l2); o.c("lc" + k, "ls" + k, v.c_l2); o.r("ls" + k, "0", v.r_lg); o.r("ls" + k, "0", on ? 1e9 : v.r_sw);   // Tr105/Tr109: leitet bei ausgeschaltetem Entzerrer
    o.c("ls" + k, "ld" + k, v.c_l1); o.r("ld" + k, "K" + k, v.r_l1);
    o.r(X, "sn" + k, v.r_sx); o.r(H, "sn" + k, v.r_sf); o.c(H, "sn" + k, v.c_sf); o.r(L, "sn" + k, v.r_sl); opamp("qp" + k, "sn" + k, H);
    double R = v.r_f + freqPotR(v, f);
    o.r(H, "i1" + k, R); o.c(Bp, "i1" + k, v.c_f); opamp("0", "i1" + k, Bp); o.r(Bp, "i2" + k, R); o.c(L, "i2" + k, v.c_f); opamp("0", "i2" + k, L);
    o.r(Bp, "qn" + k, std::max(v.p_q * tp(q), 1.0)); o.r("qn" + k, "0", v.r_qg); o.r("qn" + k, "qp" + k, v.r_qs);
    opamp("qp" + k, "kn" + k, "K" + k); o.r("kn" + k, "0", v.r_bg); o.r("K" + k, "kn" + k, v.r_bf);
    std::string Pp = "pp" + k;   // + Eingang der PRESET-Stufe (eigener Name: "P2" ist der Eingang von IC101 2/2)
    o.r(Y, Pp, v.r_p1); o.c(Pp, "0", v.c_pg); opamp(Pp, "Nn" + k, "Z" + k); o.r("Z" + k, "Nn" + k, v.r_pf); o.c("Nn" + k, Pp, v.c_pd);
    pot("Nn" + k, "pw" + k, Pp, v.p_pre, clampx(pre)); o.r("pw" + k, "pc" + k, v.r_pw); o.c("pc" + k, "pq" + k, v.c_pw); o.r("pq" + k, "0", v.r_pq);
    o.r("pq" + k, "0", on ? v.r_sw : 1e9); };   // Tr107/Tr111: leitet bei eingeschaltetem Entzerrer (PRESET wirkt nur dann)
  G = 2; peq("1", "aT", v.r_eqin, v.r_ys1, s.lev1, s.q1, s.f1, s.pre1, s.eq1); mark();
  G = 3; peq("2", "Z1", v.r_eq2in, v.r_ys2, s.lev2, s.q2, s.f2, s.pre2, s.eq2); mark();
  // --- Ausgang: Z2 → 10 µ → PO → 560 → PRE AMP OUT (100 k, 220 p) → Schaltkontakt POWER AMP IN → 560 → PI (220 p, BAL-Pegelregler, Kopfhörer-/Weichenzweig)
  //     → Schaltkontakte LPF/HPF OUTPUT → MI → 560 → 10 µ → Gate FET101 (100 k, 47 p) ---
  G = 4; o.c("Z2", "PO", v.c_po); o.r("PO", "PR", v.r_po); o.r("PR", "0", v.r_pog); o.c("PR", "0", v.c_pog); o.r("PR", "PI", v.r_pi); o.c("PI", "0", v.c_pig);
  pot("PI", "bw", "0", v.p_bal, potTaper(v.bal_x, 0)); o.r("bw", "bc", v.r_b1); o.c("bc", "bi", v.c_b1); o.r("bi", "0", v.r_b2); o.c("bi", "0", v.c_b2);
  o.c("PI", "hc", v.c_h1); o.r("hc", "hi", v.r_h1); o.r("hi", "0", v.r_h2); o.c("hi", "0", v.c_h2);
  o.r("PI", "mc", v.r_mi); o.c("mc", "G", v.c_mi); o.r("G", "0", v.r_mg); o.c("G", "0", v.c_mg);
  mark();
  for (const char *n : {"G", "a1", "a2", "aT", "X1", "Y1", "H1", "BP1", "LP1", "K1", "Z1", "X2", "Y2", "H2", "BP2", "LP2", "K2", "Z2", "PR"}) o.out(n);
}

// ---------- Endstufe: H(s) = s·(1 + rf/rs + rf·Y1(s)) / (s + rf/(rs·ri·ci)), Y1 = s·cg / (1 + s·cg·rg) (Servo als Integrator + Umkehrer);
// skaliert auf pa_gain im Band; weiche Begrenzung; Ausgang gleichspannungsgekoppelt ----------
struct PowerAmp { aor::IIR fb; double lim = 44, knee = 37, pk = 0;
  void design(double fs, const Parts &v) { double a = 1 + v.pa_rf / v.pa_rs, w0 = v.pa_rf / (v.pa_rs * v.pa_ri * v.pa_ci), C = v.pa_cg, rg = v.pa_rg, k = v.pa_gain / (a + v.pa_rf / rg);
    // H = s·(a(1 + s C rg) + s C rf) / ((s + w0)(1 + s C rg)) = (a s + s²C(a rg + rf)) / (w0 + s(1 + w0 C rg) + s² C rg)
    double b[3] = {0, k * a, k * C * (a * rg + v.pa_rf)}, aa[3] = {w0, 1 + w0 * C * rg, C * rg}; fb.n = 2; aor::bilinear(b, aa, 2, fs, fb.b, fb.a);
    lim = v.pa_lim; knee = v.pa_knee; }
  double step(double x, bool clip) { double y = fb.step(x); double a = std::fabs(y); if (clip && a > knee) { double z = knee + (lim - knee) * std::tanh((a - knee) / (lim - knee)); y = y < 0 ? -z : z; a = z; } if (a > pk) pk = a; return y; }
  void reset() { fb.reset(); pk = 0; } };
inline std::complex<double> paResponse(double f, const Parts &v) { std::complex<double> s(0, 2 * PI * f); double a = 1 + v.pa_rf / v.pa_rs, w0 = v.pa_rf / (v.pa_rs * v.pa_ri * v.pa_ci), C = v.pa_cg, rg = v.pa_rg, k = v.pa_gain / (a + v.pa_rf / rg);
  return k * (a * s + s * s * C * (a * rg + v.pa_rf)) / (w0 + s * (1 + w0 * C * rg) + s * s * C * rg); }

struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl;
  void apply(const Setting &s) { cur = s; bl = Built(); buildInto(bl, v, s); net = bl.net; run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn; run.set(dk::compile(net, fs)); run.stepLim = 8; fixDev(); }
  void fixDev() { for (auto &d : run.dev) d.model = d.kind == K_CLAMP ? (const void *)&bl.clamp : (const void *)&bl.dp; }
  Core() {} Core(const Core &o) { *this = o; }
  Core &operator=(const Core &o) { if (this == &o) return *this; fs = o.fs; v = o.v; cur = o.cur; net = o.net; run = o.run; bl = o.bl; fixDev(); return *this; }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s); }
  void step(double x, double *y) { double u[1] = {x}; run.step(u, y); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); } }
  std::complex<double> response(double f, int o = O_G) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.O[o].a, net.O[o].b); }
};

constexpr double V_FS = 50.0;   // 1,0 = 50 V Spitze (100 W an 8 Ω sind 40 V Spitze)
struct Amp {
  double fsB = 48000; int os = 1; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 0.1; bool power = true;
  PowerAmp pa; double peak[5] = {0, 0, 0, 0, 0};   // Spitzen: IC101 1/2, IC101 2/2 + IC103 1/2, PEQ 1, PEQ 2 (V), Endstufe (V an 8 Ω)
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  Core c0; int bad = 0;
  // os mindestens 2 (unter 88 kHz): bei 1x und 44,1/48 kHz findet Newton bei harter Übersteuerung der Entzerrer-Verbünde (zwei OPVs in einer Schleife,
  // beide begrenzt) vereinzelt keine Lösung (1 von 40 Zufallsläufen); ab 88,2 kHz innerer Rate in 150 Läufen kein Fehlschlag
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) { fsB = fs; os = std::max(fs < 88000 ? 2 : 1, os_); v = pv; tgt = s;
    c.init(fsB * os, v, s); c.settle(0.3, cancel); c0 = c; pa.design(fsB * os, v);
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  static bool swEq(const Setting &a, const Setting &b) { return a.eq1 == b.eq1 && a.eq2 == b.eq2 && a.taper == b.taper; }
  void setTarget(const Setting &s) { bool rebuild = !swEq(s, c.cur); tgt = s; if (rebuild) { Setting n = c.cur; n.eq1 = s.eq1; n.eq2 = s.eq2; n.taper = s.taper; c.apply(n); } }
  void snap() { c.apply(tgt); cnt = 0; }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < NKNOB; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO]; c.step(x, y);
    auto up = [&](int k, double a) { if (a > peak[k]) peak[k] = a; };
    up(0, std::fabs(y[O_A1])); up(1, std::max(std::fabs(y[O_A2]), std::fabs(y[O_T])));
    double e1 = 0, e2 = 0; for (int k = O_X1; k <= O_Z1; k++) e1 = std::max(e1, std::fabs(y[k])); for (int k = O_X2; k <= O_Z2; k++) e2 = std::max(e2, std::fabs(y[k])); up(2, e1); up(3, e2);
    double el = pa.step(y[O_G], power); up(4, pa.pk); pa.pk = 0;
    return el / V_FS; }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 10)) { y = 0;
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); pa.reset(); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung seit der letzten Abfrage: OPV-Stufen relativ zu ihrer Grenze, Endstufe als Leistung (W an 8 Ω)
  void takePeaks(double *out) { for (int k = 0; k < 4; k++) out[k] = peak[k] / v.op_vs; out[4] = peak[4] * peak[4] / 2 / v.pa_rload; for (double &p : peak) p = 0; }
};
} // namespace b3
