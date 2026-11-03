// GK 800RB (Gallien-Krueger, Service-Manual: Vorstufe 406-0045-C vom 13.8.1991, Endstufen 406-0044, Netzteil 406-0048):
// Vorstufe als Schaltung im Zeitbereich (DK-Löser, dk.hpp) vom Eingang bis zu den Eingängen der beiden Endstufen:
// Eingangsstufe U1a mit −10 dB und LO CUT, VOLUME mit HI BOOST, MID CONTOUR (Kerbfilter um U1b), U2a, die drei Bänder HI-MID/LO-MID/BASS
// (Brücken-T um U2b/U3a/U3b) mit TREBLE um U3b, Effektweg, Zenerdioden, BOOST-Regler mit Schalter, J113-Stufe, Frequenzweiche
// (Zustandsvariablenfilter U6/U7, FREQ. 88 Hz … 1 kHz), HI/LO MASTER, Eingangsnetzwerke der Endstufen. Operationsverstärker (LF353)
// als gesteuerte Quellen mit Begrenzung (±13,5 V), Zenerdioden 1N759 und der JFET als nichtlineare Tore.
// Endstufen (300 W an 4 Ω, 100 W an 8 Ω): Verstärkung aus der Gegenkopplung (56 k / 1 k + 10 µ), weiche Begrenzung an der Aussteuerungsgrenze.
// Geschätzt (nicht im Plan): J113-Kennwerte (Idss, Up), Aussteuerungsgrenzen der OPVs und Endstufen, Lautsprecher.
// Prüfwerte: Bedienungsanleitung (Treble 4 kHz, Hi-Mid 1 kHz, Lo-Mid 250 Hz, Bass 60 Hz, Contour-Kerbe ≈ 500 Hz, Boost 15 dB),
// Prüfanleitung 8/19/91 (−10 dB: 28 → 6 V, Boost 10 → 0: 28 → 3,1 V, Begrenzung 36 bzw. 28 V rms) und Oszillogramme (engine/tests/gktest.cpp).
#pragma once
#include "dk.hpp"
#include "core.hpp"   // IIR, bilinear, Speaker, spkMag (Lautsprecher wie beim Laney)
#include <atomic>
#include <cmath>
#include <string>
#include <vector>
namespace gk {
constexpr double PI = 3.14159265358979323846;

// ---------- Nichtlineare Bauteile ----------
// Geschützte Exponentialfunktion: über x0 linear fortgesetzt. x0 klein halten: der Löser führt p + K·i mitlaufend (dk.hpp); Ströme von
// 10¹³ A in einem Zwischenschritt (e⁴⁰ bei 1N759-Steilheit) löschten dabei die Stellen der Spannungen aus (Zustand lief weg, AmpFett-Dioden sind flacher)
inline void expg(double x, double &e, double &de, double x0 = 15) { if (x > x0) { double e0 = std::exp(x0); e = e0 * (1 + x - x0); de = e0; } else { e = std::exp(x); de = e; } }
// Zwei Zenerdioden 1N759 (12 V) gegeneinander in Reihe: leiten ab ±(12 V + 0,7 V), weicher Einsatz; oberhalb 13,45 V linear mit 65 A/V (15 mΩ)
inline void zenerPair(double v, double *i, double *J) { const double Vz = 12.7, nVt = 0.05, Is = 1e-6; double e1, d1, e2, d2; expg((v - Vz) / nVt, e1, d1); expg((-v - Vz) / nVt, e2, d2);
  i[0] = Is * (e1 - e2); J[0] = Is * (d1 + d2) / nVt; }
// Begrenzung eines Operationsverstärkers am Ausgang: der OPV ist im Netz eine lineare gesteuerte Quelle (Verstärkung A hinter Rout);
// dieses Tor zieht oberhalb ±Vs Strom ab (Knie 2 mV breit, dahinter 0,1 mΩ), so dass der Ausgang auf ±Vs stehen bleibt
struct Clamp { double vs; };
inline void clampI(const Clamp &c, double v, double *i, double *J) { const double w = 0.002, rc = 1e-4;
  auto sp = [](double x, double &s, double &ds) { if (x > 35) { s = x; ds = 1; } else if (x < -40) { s = 0; ds = 0; } else { double e = std::exp(x); s = std::log1p(e); ds = e / (1 + e); } };
  double s1, d1, s2, d2; sp((v - c.vs) / w, s1, d1); sp((-v - c.vs) / w, s2, d2); i[0] = w / rc * (s1 - s2); J[0] = (d1 + d2) / rc; }
// Sperrschicht-FET J113 (Shichman-Hodges, symmetrisch für Uds < 0) mit Gate-Diode; Tore (g−s, d−s), Ströme (Gate, Drain)
struct Jfet { double idss, vp, lam; };
inline void jfetI(const Jfet &F, const double *v, double *i, double *J) {
  double vgs = v[0], vds = v[1]; for (int k = 0; k < 4; k++) J[k] = 0;
  { const double Is = 1e-12, nVt = 0.03; double e, de; expg(vgs / nVt, e, de, 22); i[0] = Is * (e - 1); J[0] = Is * de / nVt; }   // Gate–Source-Diode (3,6 mA bei 0,66 V, darüber linear)
  double be = F.idss / (F.vp * F.vp), sgn = 1, vg = vgs, vd = vds; if (vds < 0) { sgn = -1; vd = -vds; vg = vgs - vds; }   // Drain und Source vertauscht
  double ov = vg - F.vp; if (ov <= 0) { i[1] = 0; return; }
  double id, dg, dd, m = 1 + F.lam * vd;
  if (vd < ov) { id = be * (2 * ov * vd - vd * vd) * m; dg = be * 2 * vd * m; dd = be * (2 * ov - 2 * vd) * m + be * (2 * ov * vd - vd * vd) * F.lam; }
  else { id = be * ov * ov * m; dg = be * 2 * ov * m; dd = be * ov * ov * F.lam; }
  if (sgn > 0) { i[1] = id; J[2] = dg; J[3] = dd; }
  else { i[1] = -id; J[2] = -dg; J[3] = -(-dg - dd); } }   // vg = vgs − vds, vd = −vds: ∂/∂vgs = dg, ∂/∂vds = −dg − dd (Vorzeichen des Stroms gedreht)
enum Kind { K_ZEN = 0, K_CLAMP = 1, K_JFET = 2 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) { case K_ZEN: zenerPair(v[0], i, J); break; case K_CLAMP: clampI(*(const Clamp *)d.model, v[0], i, J); break; default: jfetI(*(const Jfet *)d.model, v, i, J); } }

// ---------- Bauteile und Einstellungen ----------
struct Parts {   // Werte laut Plan 406-0045-C (Ω, F); Schätzungen markiert
  double rsrc = 1e3;   // Innenwiderstand der Quelle (geschätzt)
  double r1 = 12e3, c2 = 0.1e-6, c6 = 100e-12, r4 = 1e6, r5 = 33e3, r3 = 4.7e3, c11 = 10e-6, c25 = 0.22e-6, r24 = 3.3e3;   // Eingang, U1a, −10 dB, LO CUT
  double vol = 50e3, c26 = 0.01e-6, r29 = 470e3, r28 = 5.6e3, c21 = 3.3e-6;   // VOLUME (50KA), HI BOOST
  double r12 = 12e3, c14 = 4.7e-9, r13 = 120e3, c15 = 4.7e-9, r16 = 33e3, c20 = 560e-12, r19 = 220e3, c23 = 0.22e-6, r22 = 220e3, r27 = 33e3;   // MID CONTOUR um U1b
  double r30 = 4.7e3, r32 = 22e3, c31 = 100e-12;   // U2a
  double r35 = 12e3, r41 = 47e3, p_hm = 50e3, r39 = 12e3, c37 = 2.2e-9, c40 = 2.2e-9, r33 = 470e3, r38 = 470e3;   // HI-MID um U2b
  double c42 = 0.1e-6, r43 = 12e3, r48 = 47e3, p_lm = 50e3, r47 = 12e3, c46 = 0.01e-6, c54 = 0.01e-6, r44 = 470e3, r50 = 470e3, c49 = 47e-12;   // LO-MID um U3a
  double r51 = 5.6e3, r60 = 22e3, p_b = 50e3, r59 = 5.6e3, c62 = 0.1e-6, r57 = 47e3, r45 = 5.6e3, p_t = 50e3, r58 = 5.6e3, c56 = 2.2e-9;   // BASS und TREBLE um U3b
  double c61 = 0.22e-6, r63 = 4.7e3, c70 = 0.1e-6, r73 = 12e3;   // Effektweg (SEND/RETURN gebrückt)
  double p_boost = 50e3, r81 = 5.6e3, r42 = 220e3;   // BOOST (50KB), Schalter TL604
  double r90 = 1e3, c92 = 10e-6, r91 = 4.7e3, r88 = 12e3, c93 = 10e-6, r94 = 100, c95 = 0.22e-6;   // J113-Stufe
  double r101 = 22e3, r106 = 22e3, r97 = 22e3, r108 = 22e3, p_f = 50e3, r98 = 4.7e3, r96 = 4.7e3, r103 = 33e3, r111 = 33e3, c104 = 4.7e-9, c113 = 4.7e-9, c99 = 3.3e-6, c112 = 3.3e-6, r107 = 100e3, r116 = 100e3;   // Frequenzweiche
  double p_m = 50e3, c85 = 0.22e-6, c86 = 0.22e-6, pa_rin = 100e3, pa_r29 = 5.6e3, pa_r30 = 1e6;   // MASTER (50KB), Eingänge der Endstufen (100 k gegen Masse, 5,6 k auf den invertierenden Eingang, der dem Signal folgt; 1 M lokale Gegenkopplung)
  double pa_rf = 56e3, pa_rb = 12e3, pa_rg = 1e3, pa_cg = 10e-6;   // Gegenkopplung beider Endstufen: 56 k vom Ausgang auf den nicht invertierenden Eingang, 12 k ∥ (1 k + 10 µ) gegen Masse
  double pa300_lim = 49, pa300_knee = 44, pa100_lim = 40, pa100_knee = 36;   // Aussteuerungsgrenzen (V Spitze; 36 bzw. 28 V rms laut Prüfanleitung), geschätztes Knie
  double op_a = 1e4, op_rout = 100, op_vs = 13.5;   // LF353: Leerlaufverstärkung (vereinfacht), Ausgangswiderstand, Aussteuerungsgrenze an ±15 V (geschätzt)
  double j_idss = 12e-3, j_vp = -2.0, j_lam = 0.02, vcc = 15;   // J113 (Datenblatt: Up −0,5 … −3 V, Idss ≥ 2 mA): Mittelwerte geschätzt
};
// Regler 0…1 (Knopf 0…10); Schalter
struct Setting { double vol = 0.5, treb = 0.5, himid = 0.5, lomid = 0.5, bass = 0.5, boost = 0.5, freq = 0.5, hima = 0.5, loma = 0.5;
  bool pad = false, locut = false, contour = false, hiboost = false, boostOn = true, biamp = false; };
constexpr int NKNOB = 9;
inline double &sfield(Setting &s, int i) { double *f[NKNOB] = {&s.vol, &s.treb, &s.himid, &s.lomid, &s.bass, &s.boost, &s.freq, &s.hima, &s.loma}; return *f[i]; }
inline double logTaper(double x) { const double m = 0.15; if (x <= 0) return 0; if (x >= 1) return 1; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // VOLUME 50KA: 15 % in Mittelstellung

// Ausgänge der Netzliste
enum { O_PA300 = 0, O_PA100, O_DIRECT, O_U1, O_U2A, O_U2B, O_U3A, O_U3B, O_JD, O_NO };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; Clamp clamp; Jfet jf; int pJfet = 0; };
inline void buildInto(Built &B, const Parts &v, const Setting &s) {
  dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0; B.clamp = {v.op_vs}; B.jf = {v.j_idss, v.j_vp, v.j_lam};
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto sw = [&](const std::string &a, const std::string &b, bool closed) { o.r(a, b, closed ? 1e-3 : 1e8); };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };   // x = 1: Schleifer an a (Knopf 10)
  // Operationsverstärker: Strom A/Rout·(u+ − u−) in den Ausgang, Rout gegen Masse; mit Begrenzung (Tor) oder ohne (Stufen, die nie übersteuern)
  auto opamp = [&](const std::string &inp, const std::string &inn, const std::string &out, bool lim) { o.vccs("0", out, inp, inn, v.op_a / v.op_rout); o.r(out, "0", v.op_rout);
    if (lim) { int p = o.port(out, "0"); dev.push_back({K_CLAMP, p, 1, &B.clamp}); } };
  // --- Eingang, U1a (nicht invertierend, 33 k / 3,3 k; −10 dB: 4,7 k parallel zu 33 k; LO CUT: 0,22 µ in Reihe mit 3,3 k) ---
  o.src("in", v.rsrc, 0); o.r("in", "c2a", v.r1); o.c("c2a", "n1", v.c2); { int p = o.port("n1", "0"); dev.push_back({K_ZEN, p, 1, nullptr}); }
  o.c("n1", "0", v.c6); o.r("n1", "0", v.r4);
  opamp("n1", "u1n", "u1o", true); o.r("u1o", "u1n", v.r5); o.r("u1o", "u1n", s.pad ? v.r3 : 1e9);
  o.c("u1n", "lc1", v.c11); o.c("lc1", "lc2", v.c25); sw("lc1", "lc2", !s.locut); o.r("lc2", "0", v.r24);
  mark(); G = 1;
  // --- VOLUME (log), HI BOOST (10 n vom U1a-Ausgang über 5,6 k auf den Schleifer; aus: 470 k in Reihe) ---
  pot("u1o", "w", "0", v.vol, logTaper(s.vol));
  o.c("u1o", "hb", v.c26); o.r("hb", "h", v.r29); sw("hb", "h", s.hiboost); o.r("h", "w", v.r28);
  o.c("w", "m", v.c21);
  // --- MID CONTOUR: Kerbfilter um U1b (Eingang über 12 k, zwei Pfade auf den invertierenden Eingang), Schalter S3 wählt m oder das Filter ---
  o.r("m", "ci", v.r12); o.c("ci", "f1", v.c14); o.r("ci", "n2", v.r13); o.c("f1", "n2", v.c15); o.r("f1", "u1bo", v.r16); o.c("n2", "u1bo", v.c20); o.r("n2", "u1bo", v.r19);
  opamp("0", "n2", "u1bo", true); o.c("u1bo", "co", v.c23); o.r("co", "0", v.r22);
  sw("m", "p", !s.contour); sw("co", "p", s.contour); o.r("p", "0", v.r27);
  // --- U2a: nicht invertierend 1 + 22 k / 4,7 k ---
  opamp("p", "n3", "x", true); o.r("n3", "0", v.r30); o.r("x", "n3", v.r32); o.c("x", "n3", v.c31);
  mark(); G = 2;
  // --- HI-MID um U2b (invertierender Summierer 470 k / 470 k, Brücken-T mit 2,2 n / 2,2 n; Knopf 10 = Schleifer am 47-k-Ende = 0 dB) ---
  o.r("x", "L1", v.r35); o.r("L1", "a1", v.r41); pot("a1", "w1", "b1", v.p_hm, s.himid); o.r("b1", "y", v.r39); o.c("L1", "b1", v.c37); o.c("w1", "n4", v.c40);
  o.r("x", "n4", v.r33); o.r("y", "n4", v.r38); opamp("0", "n4", "y", true);
  mark(); G = 3;
  // --- LO-MID um U3a (0,1 µ Kopplung, 10 n / 10 n, 47 p über 470 k) ---
  o.c("y", "x2", v.c42); o.r("x2", "L2", v.r43); o.r("L2", "a2", v.r48); pot("a2", "w2", "b2", v.p_lm, s.lomid); o.r("b2", "y2", v.r47); o.c("L2", "b2", v.c46); o.c("w2", "n5", v.c54);
  o.r("x2", "n5", v.r44); o.r("y2", "n5", v.r50); o.c("y2", "n5", v.c49); opamp("0", "n5", "y2", true);
  mark(); G = 4;
  // --- BASS (5,6 k / 22 k, 0,1 µ, Schleifer über 47 k) und TREBLE (5,6 k beidseitig, Schleifer über 2,2 n) um U3b ---
  o.r("y2", "L3", v.r51); o.r("L3", "a3", v.r60); pot("a3", "w3", "b3", v.p_b, s.bass); o.r("b3", "y3", v.r59); o.c("L3", "b3", v.c62); o.r("w3", "n6", v.r57);
  o.r("y2", "a4", v.r45); pot("a4", "w4", "b4", v.p_t, s.treb); o.r("b4", "y3", v.r58); o.c("w4", "n6", v.c56); opamp("0", "n6", "y3", true);
  mark(); G = 5;
  // --- Effektweg (SEND an RETURN), Zenerdioden, DIRECT OUT (Puffer U4 belastet nicht), BOOST mit Schalter (aus: Abgriff unter dem Poti), J113 ---
  o.c("y3", "s1", v.c61); o.r("s1", "ret", v.r63); o.c("ret", "r73a", v.c70); o.r("r73a", "Z", v.r73); { int p = o.port("Z", "0"); dev.push_back({K_ZEN, p, 1, nullptr}); }
  pot("Z", "bw", "bz", v.p_boost, s.boost); o.r("bz", "0", v.r81); sw("bw", "g", s.boostOn); sw("bz", "g", !s.boostOn); o.r("g", "0", v.r42);
  o.src("c92", v.r90, 1); o.c("c92", "0", v.c92); o.r("c92", "jd", v.r91); o.src("js", v.r88, 2); o.c("js", "c93", v.c93); o.r("c93", "0", v.r94);
  { B.pJfet = o.port("g", "js"); o.port("jd", "js"); dev.push_back({K_JFET, B.pJfet, 2, &B.jf}); }
  o.c("jd", "pre", v.c95);
  mark();
  // --- Frequenzweiche: Zustandsvariablenfilter, U6a Summierer (HP), U6b und U7 Integratoren (BP, LP); FREQ. Tandempoti 50 k mit 4,7 k gegen Masse ---
  o.r("pre", "xp", v.r101); o.r("xp", "bp", v.r106); opamp("xp", "xn", "hp", false); o.r("hp", "xn", v.r97); o.r("lp", "xn", v.r108);
  pot("hp", "fw1", "fb1", v.p_f, s.freq); o.r("fb1", "0", v.r98); o.r("fw1", "bn", v.r103); o.c("bn", "bp", v.c104); opamp("0", "bn", "bp", false);
  pot("bp", "fw2", "fb2", v.p_f, s.freq); o.r("fb2", "0", v.r96); o.r("fw2", "ln", v.r111); o.c("ln", "lp", v.c113); opamp("0", "ln", "lp", false);
  o.c("hp", "hpo", v.c99); o.r("hpo", "0", v.r107); o.c("lp", "lpo", v.c112); o.r("lpo", "0", v.r116);
  // --- FULL / BIAMP, HI MASTER (100 W) und LO MASTER (300 W), Eingangsnetzwerke der Endstufen (0,22 µ, 100 k, 5,6 k / 12 k) ---
  sw("pre", "hit", !s.biamp); sw("hpo", "hit", s.biamp); sw("pre", "lot", !s.biamp); sw("lpo", "lot", s.biamp);
  pot("hit", "hiw", "0", v.p_m, s.hima); o.c("hiw", "pin100", v.c86); o.r("pin100", "0", v.pa_rin); o.r("pin100", "0", v.pa_r29 + v.pa_r30);
  pot("lot", "low", "0", v.p_m, s.loma); o.c("low", "pin300", v.c85); o.r("pin300", "0", v.pa_rin); o.r("pin300", "0", v.pa_r29 + v.pa_r30);
  o.out("pin300"); o.out("pin100"); o.out("Z"); o.out("u1o"); o.out("x"); o.out("y"); o.out("y2"); o.out("y3"); o.out("jd");
}
inline Built build(const Parts &v, const Setting &s) { Built B; buildInto(B, v, s); return B; }

// ---------- Endstufe: LF353 (Signal am invertierenden Eingang über 5,6 k, 1 M + 0,1 µ lokal), diskrete Stufen in der Schleife, Gegenkopplung
// 56 k vom Ausgang auf den nicht invertierenden Eingang mit 12 k ∥ (1 k + 10 µ) gegen Masse: Verstärkung 1 + 56 k/Zb → 5,7-fach bei Gleichspannung,
// oberhalb etwa 30 Hz 61,7-fach (35,8 dB); weiche Begrenzung an der Aussteuerungsgrenze ----------
struct PowerAmp { aor::IIR fb; double lim = 49, knee = 44; double pk = 0;
  void design(double fs, const Parts &v, bool big) { double C = v.pa_cg, rb = v.pa_rb, rg = v.pa_rg, rf = v.pa_rf;
    double b1[2] = {(rb + rf) / rb, C * (rb * rg + rf * (rb + rg)) / rb}, a1[2] = {1, C * rg}; fb.n = 1; aor::bilinear(b1, a1, 1, fs, fb.b, fb.a);
    lim = big ? v.pa300_lim : v.pa100_lim; knee = big ? v.pa300_knee : v.pa100_knee; }
  double step(double x, bool clip) { double y = fb.step(x); double a = std::fabs(y); if (a > pk) pk = a; if (!clip || a <= knee) return y; double z = knee + (lim - knee) * std::tanh((a - knee) / (lim - knee)); return y < 0 ? -z : z; }
  void reset() { fb.reset(); pk = 0; } };

// ---------- Laufzeit ----------
struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; double jdDC = 0;
  void apply(const Setting &s) { cur = s; bl = Built(); buildInto(bl, v, s); net = bl.net; run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn; run.set(dk::compile(net, fs)); run.stepLim = 8; fixDev(); }   // Newton-Schritt höchstens 8 V: alle Torspannungen liegen unter ±15 V (sonst riesige Zwischenströme der Begrenzer)
  void fixDev() { for (auto &d : run.dev) d.model = d.kind == K_CLAMP ? (const void *)&bl.clamp : d.kind == K_JFET ? (const void *)&bl.jf : nullptr; }   // Zeiger auf die eigenen Modelle (nach Kopie)
  Core() {} Core(const Core &o) { *this = o; }
  Core &operator=(const Core &o) { if (this == &o) return *this; fs = o.fs; v = o.v; cur = o.cur; net = o.net; run = o.run; bl = o.bl; jdDC = o.jdDC; fixDev(); return *this; }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s);
    // Startwerte der J113-Stufe (Ruhestrom etwa 1,35 mA: Source +1,3 V, Drain +7,3 V, Siebung +13,6 V), alles andere 0 V
    double id = 1.35e-3, vs = -v.vcc + id * v.r88, vd = v.vcc - id * (v.r90 + v.r91), vc = v.vcc - id * v.r90;
    std::map<std::string, double> g = {{"js", vs}, {"jd", vd}, {"c92", vc}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; jdDC = vd; }
  void step(double x, double *y) { double u[3] = {x, v.vcc, -v.vcc}; run.step(u, y); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); }
    step(0, y); jdDC = y[O_JD]; }
  // Kleinsignal-Frequenzgang Eingang → Ausgang o (Index der Netzliste) im aktuellen Arbeitspunkt
  std::complex<double> response(double f, int o = O_PA300) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.O[o].a, net.O[o].b); }
};
// Gesamtfrequenzgang (Kleinsignal) der Anlage: Vorstufe bis Endstufeneingang · Endstufe (57-fach) · Lautsprecher; bezogen auf den Lautsprecheranschluss
inline std::complex<double> paResponse(double f, const Parts &v) { std::complex<double> s(0, 2 * PI * f); double C = v.pa_cg, rb = v.pa_rb, rg = v.pa_rg, rf = v.pa_rf;
  return ((rb + rf) / rb + s * (C * (rb * rg + rf * (rb + rg)) / rb)) / (1.0 + s * C * rg); }

// Verstärker mit Überabtastung, Reglerglättung, Endstufen, Lautsprecher, Anzeigen. Ausgabe: Lautsprecherspannung (beide Endstufen) / V_FS
constexpr double V_FS = 60.0;   // 1,0 = 60 V Spitze (300 W an 4 Ω sind 49 V Spitze)
constexpr double V_DIRECT = 2.0;   // DIRECT OUT: 1,0 = 2 V Spitze
struct Amp {
  double fsB = 48000; int os = 1; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 0.1; bool spk = false, power = true, direct = false; double spkMix = -1, dirMix = -1;
  PowerAmp pa300, pa100; aor::Speaker spkF; double peak[5] = {0, 0, 0, 0, 0};   // Spitzen: U1a (V), EQ-Stufen (V), J113-Drain (V um den Ruhepunkt), 300-W- und 100-W-Endstufe (V am Lautsprecher)
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  Core c0; int bad = 0;   // c0: eingeschwungener Zustand nach init (Rückfall bei entgleister Rechnung)
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) { fsB = fs; os = std::max(1, os_); v = pv; tgt = s;
    c.init(fsB * os, v, s); c.settle(0.5, cancel); c0 = c;
    pa300.design(fsB * os, v, true); pa100.design(fsB * os, v, false); spkF.design(fsB * os); spkMix = spk ? 1 : 0; dirMix = direct ? 1 : 0;
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  static bool swEq(const Setting &a, const Setting &b) { return a.pad == b.pad && a.locut == b.locut && a.contour == b.contour && a.hiboost == b.hiboost && a.boostOn == b.boostOn && a.biamp == b.biamp; }
  // Schalter sofort (Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void setTarget(const Setting &s) { bool rebuild = !swEq(s, c.cur); tgt = s; if (rebuild) { Setting n = c.cur; n.pad = s.pad; n.locut = s.locut; n.contour = s.contour; n.hiboost = s.hiboost; n.boostOn = s.boostOn; n.biamp = s.biamp; c.apply(n); } }
  void snap() { c.apply(tgt); spkMix = spk ? 1 : 0; dirMix = direct ? 1 : 0; cnt = 0; }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < NKNOB; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO]; c.step(x, y);
    auto up = [&](int k, double a) { if (a > peak[k]) peak[k] = a; };
    up(0, std::fabs(y[O_U1])); up(1, std::max(std::max(std::fabs(y[O_U2A]), std::fabs(y[O_U2B])), std::max(std::fabs(y[O_U3A]), std::fabs(y[O_U3B])))); up(2, std::fabs(y[O_JD] - c.jdDC));
    double v300 = pa300.step(y[O_PA300], power), v100 = pa100.step(y[O_PA100], power); up(3, std::fabs(v300)); up(4, std::fabs(v100));
    double el = v300 + v100, st = 1.0 / (0.02 * c.fs);
    double ys = spkF.step(el); double tS = spk ? 1 : 0; if (spkMix < tS) spkMix = std::min(tS, spkMix + st); else if (spkMix > tS) spkMix = std::max(tS, spkMix - st);
    double o = (el + spkMix * (ys - el)) / V_FS, d = y[O_DIRECT] / V_DIRECT; double tD = direct ? 1 : 0; if (dirMix < tD) dirMix = std::min(tD, dirMix + st); else if (dirMix > tD) dirMix = std::max(tD, dirMix - st);
    return o + dirMix * (d - o); }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 10)) { y = 0;   // entgleist: eingeschwungenen Zustand zurückholen, höchstens einmal je 0,1 s
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); pa300.reset(); pa100.reset(); spkF.lo.reset(); spkF.le.reset(); spkF.cone.reset(); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung seit der letzten Abfrage: U1a und EQ relativ zur OPV-Grenze, J113 relativ zu 6 V Hub, Endstufen als Leistung (W an 4 bzw. 8 Ω)
  void takePeaks(double *out) { out[0] = peak[0] / v.op_vs; out[1] = peak[1] / v.op_vs; out[2] = peak[2] / 6.0; out[3] = peak[3] * peak[3] / 2 / 4; out[4] = peak[4] * peak[4] / 2 / 8; for (double &p : peak) p = 0; }
};
} // namespace gk
