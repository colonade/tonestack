// Cultec EQ-1A = Nachbau des Pultec EQP-1A „Program Equalizer“ (Pulse Techniques), ganze Schaltung im Zeitbereich als ein Netz (dk.hpp):
// Eingangsübertrager Triad HS-56 → passives LC-Netz (Abschrift, siehe docs/cultec/SCHALTPLAN.md) → Zwischenübertrager Triad HS-29 (1:2, Mittelanzapfung)
// → 12AX7 im Gegentakt → 12AU7 im Gegentakt → Ausgangsübertrager Peerless S-217-D mit Gegenkopplungswicklung an die Kathoden der 12AX7.
// Verstärker nach dem Werksplan DWG E-72,658-2 (alle Widerstände, Kondensatoren und Spannungen von dort). Das passive Netz des EQP-1A gibt es nur als
// Abschrift (recproaudio „Hosef 1997“, Gyraf 2001); die Werte sind gegen die Werkskurven gewählt bzw. angepasst (SCHALTPLAN.md, cultec_test).
// Geschätzt (kein Datenblatt mit diesen Größen): Induktivitäten und Kopplung der drei Übertrager, Wicklungskapazitäten, Spulengüte der Höhenanhebung,
// Windungszahl der Gegenkopplungswicklung (aus „Overall gain … is 0 dB“ bestimmt), Kennlinie der „Audio“-Potis.
// Ab 1.1 zweites Modell EQP-1R (partsR(): Werksplan des Netzes und Verstärker DWG E-112,353-7, 12AY7 statt 12AX7; gleiche Schaltungsform, andere Werte und Stellungen).
#pragma once
#include "b15_core.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <map>
#include <string>
#include <vector>
// Ruhespannungen aller Knoten (Regler 0, 30 s bei 48 kHz eingeschwungen; neu erzeugen mit build/cultec_test rest [r], wenn sich Bauteile ändern)
#define CULTEC_REST {{"A", 0.0000}, {"A1", 139.3750}, {"A2", 139.3750}, {"B", 0.0000}, {"B250", 252.4833}, {"B290", 291.7615}, {"B31", 29.4248}, {"B325", 327.9501}, {"B345", 346.0052}, {"B355", 354.9809}, {"Cc", 0.0000}, {"D", 0.0000}, {"F3", 1.1325}, {"F5", 1.1325}, {"G1", 0.0000}, {"G2", 0.0000}, {"H", 0.0000}, {"K1", 1.3361}, {"K2", 1.3361}, {"KA", 38.6560}, {"KB", 38.6560}, {"O", 0.0000}, {"OUT", -0.0000}, {"P", 0.0000}, {"P1", 289.2941}, {"P2", 289.2941}, {"Q", 0.0000}, {"Q1", 29.4248}, {"Q2", 29.4248}, {"S", 1.1311}, {"W", 0.0000}, {"X", 0.0000}, {"Xl", 0.0000}, {"Y", 0.0000}, {"Z", 0.0000}, {"f3w", 1.1311}, {"f5w", 1.1311}, {"g1w", 0.0000}, {"g2w", 0.0000}, {"in", 0.0000}, {"ip", 0.0000}, {"isw", 0.0000}, {"op", 0.0000}, {"p1w", 291.7615}, {"p2w", 291.7615}, {"sw", -0.0000}, {"zo", 0.0000}}
#define CULTEC_REST_R {{"A", 0.0000}, {"A1", 81.8102}, {"A2", 81.8102}, {"B", 0.0000}, {"B250", 206.3329}, {"B290", 259.2235}, {"B31", 24.0463}, {"B325", 289.5901}, {"B345", 306.0368}, {"B355", 322.4836}, {"Cc", 0.0000}, {"D", 0.0000}, {"F3", 1.3853}, {"F5", 1.3853}, {"G1", 0.0000}, {"G2", 0.0000}, {"H", 0.0000}, {"K1", 1.6344}, {"K2", 1.6344}, {"KA", 32.4371}, {"KB", 32.4371}, {"O", 0.0000}, {"OUT", 0.0000}, {"P", 0.0000}, {"P1", 257.1530}, {"P2", 257.1530}, {"Q", 0.0000}, {"Q1", 24.0463}, {"Q2", 24.0463}, {"S", 1.3836}, {"W", 0.0000}, {"X", 0.0000}, {"Xl", 0.0000}, {"Y", 0.0000}, {"Z", 0.0000}, {"f3w", 1.3836}, {"f5w", 1.3836}, {"g1w", 0.0000}, {"g2w", 0.0000}, {"in", 0.0000}, {"ip", 0.0000}, {"isw", 0.0000}, {"op", 0.0000}, {"p1w", 259.2235}, {"p2w", 259.2235}, {"sw", 0.0000}, {"zo", 0.0000}}   // EQP-1R
namespace cultec {
using b15::PI;
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};   // wie Laney/Loldano (an RCA-Datenblatt angepasst), Gitterstrom Dempwolf
enum Kind { K_AX7 = 0, K_AU7 = 1, K_AY7 = 2 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {   // 12AU7, 12AY7: Kennlinien aus Messungen (Reefman, TubeLib)
  if (d.kind == K_AX7) b15::triodeD(T12AX7, v, i, J); else if (d.kind == K_AY7) b15::triodeK(b15::K12AY7, v, i, J); else b15::triodeK(b15::K12AU7, v, i, J); }

// Kennlinie „Audio“ (Allen-Bradley Typ A): Anteil des Widerstands über dem Drehweg, 10 % in der Mitte (geschätzt; das Werksblatt nennt nur „Audio or Logarithmic“)
inline double taperA(double x, double mid = 0.10) { x = std::max(0.0, std::min(1.0, x)); const double s = 1 / mid - 1, b = s * s; return (std::pow(b, x) - 1) / (b - 1); }

constexpr int NLF = 4, NHF = 7, NHC = 3;   // EQP-1A
static const double LF_HZ[NLF] = {20, 30, 60, 100};
static const double HF_KHZ[NHF] = {3, 4, 5, 8, 10, 12, 16};
static const double HC_KHZ[NHC] = {5, 10, 20};
constexpr int NLF_R = 3, NHF_R = 5;          // EQP-1R (Werksplan des Netzes): Höhen-Atten fest (0,13 µF, etwa 10 kHz)
static const double LF_HZ_R[NLF_R] = {30, 60, 100};
static const double HF_KHZ_R[NHF_R] = {3, 5, 8, 10, 12};

struct Parts {
  int model = 0;                     // 0 EQP-1A, 1 EQP-1R (partsR())
  int nlf = NLF, nhf = NHF, nhc = NHC;
  const double *lfHz = LF_HZ, *hfKhz = HF_KHZ;
  // Quelle und Eingangsübertrager HS-56 (600 Ω: Primär 1–6, Brücke 3–4; Sekundär 7–12, Brücke 9–10; Gleichstromwiderstände laut Triad)
  double rsrc = 600, in_rp = 9.46 + 16.6 + 17.8 + 10.71, in_rs = 8.4 + 15.1 + 19.7 + 11.8, in_lp = 37.8, in_k = 0.99995, in_cw = 100e-12;   // Lp, k, Wicklungskapazität geschätzt
  double rterm = 620;   // Abschluss der Sekundärseite (Plan)
  // Passives Netz
  double phb = 10e3, pbw = 2.5e3, rbyp = 1e6, phc = 1e3, r75 = 75, plb = 10e3, pla = 100e3, r1k = 1e3, r10k = 10e3;
  double cLB[NLF] = {2.2e-6, 1e-6, 0.47e-6, 0.33e-6};   // Bass-Boost (Gyraf), passt zu den Werkskurven (SCHALTPLAN.md)
  double cLA[NLF] = {100e-9, 47e-9, 22e-9, 15e-9};      // Bass-Atten (Gyraf)
  double cHF[NHF] = {13.22e-9, 18e-9, 15e-9, 8.2e-9, 8.2e-9, 5.6e-9, 3.3e-9};   // Höhen-Boost C (recproaudio)
  double lHF[NHF] = {0.2041, 0.082, 0.068, 0.047, 0.033, 0.033, 0.027};        // Spulenabgriff (recproaudio)
  double qHF[NHF] = {20.0, 20.0, 20.0, 20.0, 20.0, 20.0, 20.0};      // Spulengüte bei der Resonanz (Verluste als Reihenwiderstand; an die Werkskurven angepasst, cultec_test fit)
  double cHC[NHC] = {288e-9, 136e-9, 68e-9};            // Höhen-Atten (recproaudio: 68 n fest + 220 n / 68 n)
  double taperLB = 0.10, taperLA = 0.10;                // „Audio“-Kennlinie der Bass-Potis (Mitte; geschätzt)
  // Zwischenübertrager HS-29 (Primär 1–4 mit 2–3 verbunden; Sekundär 5–8, Mittelanzapfung 6–7 an Masse; je Hälfte so viele Windungen wie die Primärseite)
  double is_rp = 935 + 1020, is_rs1 = 1650, is_rs2 = 2270, is_lp = 317.4, is_k = 0.99985, is_cs = 40e-12, is_rc = 41.5e3;   // Lp, k, Kapazität der Sekundärseite, Eisenverluste (parallel zur Primärseite) geschätzt bzw. angepasst
  double zob_c = 270e-12, zob_r = 43e3;   // Zobel-Glied über 5–8 (Plan)
  // V1 12AX7
  double ra1 = 200e3, cpp1 = 20e-12, rkk = 820, rkf = 360, cc = 0.25e-6, rg2 = 1e6, rd1 = 470e3, rd2 = 62e3;
  double ax_cgk = 1.6e-12, ax_cga = 1.7e-12, au_cgk = 1.6e-12, au_cga = 1.5e-12;   // Röhrenkapazitäten (Datenblätter)
  // V2 12AU7
  double rk2 = 4700, ck2 = 50e-6;
  // Ausgangsübertrager S-217-D: 12,5 kΩ Anode–Anode zu 600 Ω (CineMag CM-S217D: „12,500/300“ bzw. „3125:150/75“, Nachbau des Peerless); Lp, k, Wicklungswiderstände,
  // Wicklungskapazität geschätzt. Gegenkopplungswicklung 3–S–5 (Abgriff S über 1000 Ω an Masse); ihr Windungsverhältnis aus „Overall gain 0 dB“ (cultec_test gain).
  double ot_zp = 12500, ot_zs = 600, ot_lp = 100, ot_kpp = 0.99995, ot_kps = 0.9999, ot_rp = 600, ot_rs = 30, ot_cpp = 150e-12, ot_nf = 0.018747, ot_rf = 5, rS = 1000;
  int fbSign = 1;   // Wicklungssinn der Gegenkopplung (so, dass sie gegenkoppelt; cultec_test prüft es)
  double rout = 620, cout_ = 3e-9, rload = 600;   // Ausgang: 620 Ω ∥ 3 nF (Plan), Last 600 Ω
  // Netzteil (Plan): +355 V hinter dem Gleichrichter, 470 Ω, 1000 Ω, 2200 Ω, 47 k, je 40 µF; Heizungsvorspannung 270 k + 62 k
  double vdc = 355, rdc = 1, r470 = 470, r1000 = 1000, r2200 = 2200, r47k = 47e3, cb = 40e-6, rheat = 332e3;
  // Unterschiede der Schaltungsform (EQP-1R): Fußpunkt der Höhen-Atten-Kondensatoren fest an D statt über den IN-Schalter (Höhen-Atten wirkt auch
  // in Stellung OUT); Röhre V1; Heizungsvorspannung am dritten Siebknoten (+290 V) statt am ersten
  bool hcSw = true; int v1 = K_AX7; bool heatLate = false;
  double biasV1 = 1.3, biasV2 = 9.0;   // Anzeige: Vorspannung V1 (Kathode) und V2 (Kathode − Gitterableitung) laut Plan
};
// Pultec EQP-1R: Netz nach dem Werksplan „PULTEC EQP-1R EQUALIZER“ (PultecEQP.pdf S. 6), Verstärker DWG E-112,353-7 (S. 8). Abschrift: docs/cultec/SCHALTPLAN.md.
// Übertrager Peerless E-372-Q / G-212-Q statt Triad HS-56 / HS-29: keine Daten, darum die Werte des EQP-1A (geschätzt); S-217-D und die Gegenkopplungs-
// wicklung sind dasselbe Bauteil wie im EQP-1A. Spulengüte wie EQP-1A (geschätzt). Gleichrichterspannung so, dass der +290-V-Knoten 290 V hat (cultec_test dc r).
inline Parts partsR() {
  Parts p; p.model = 1; p.nlf = NLF_R; p.nhf = NHF_R; p.nhc = 1; p.lfHz = LF_HZ_R; p.hfKhz = HF_KHZ_R;
  // Netz: Höhenboost-Spulen 50 mH und 100 mH (getrennt gezeichnet), der Spulenschalter schließt Teile kurz: 3 kHz beide in Reihe, 5 kHz nur 100 mH,
  // 8 kHz nur 50 mH, 10/12 kHz beide parallel (Abgriff gegen beide Enden)
  const double cH[NHF_R] = {0.02e-6, 0.01e-6, 0.0062e-6, 0.0062e-6, 0.005e-6}, lH[NHF_R] = {0.150, 0.100, 0.050, 0.1 / 3, 0.1 / 3};
  for (int k = 0; k < NHF; k++) { p.cHF[k] = cH[std::min(k, NHF_R - 1)]; p.lHF[k] = lH[std::min(k, NHF_R - 1)]; }
  const double cB[NLF_R] = {1e-6, 0.5e-6, 0.28e-6}, cA[NLF_R] = {0.0362e-6, 0.02e-6, 0.01e-6};   // 30/60/100 Hz (0,5 µF und 0,28 µF schlecht lesbar)
  for (int k = 0; k < NLF; k++) { p.cLB[k] = cB[std::min(k, NLF_R - 1)]; p.cLA[k] = cA[std::min(k, NLF_R - 1)]; }
  for (int k = 0; k < NHC; k++) p.cHC[k] = 0.13e-6;
  p.r75 = 0; p.r1k = 0; p.r10k = 12e3; p.hcSw = false;   // kein 75 Ω, kein 1 kΩ, 12 kΩ statt 10 kΩ; die 820 Ω am IN-Schalter liegen laut Plan mit beiden Enden an D (ohne Wirkung)
  // Verstärker
  p.zob_c = 350e-12; p.zob_r = 22e3;                                    // 0,00035 µF in Reihe mit 22 k über der Sekundärseite
  p.in_k = 0.99999; p.is_k = 0.99992;                                   // Kopplung E-372-Q / G-212-Q geschätzt: Verstärker flach +0/−1 dB 20 Hz–20 kHz wie EQP-1A/EQH-2
  p.v1 = K_AY7; p.ax_cgk = 1.3e-12; p.ax_cga = 1.3e-12;                  // 12AY7 (Datenblatt-Kapazitäten)
  p.ra1 = 180e3; p.cpp1 = 15e-12; p.rkk = 750;                          // Anoden je 180 k (+85 V), 0,000015 µF zwischen den Anoden, 750 Ω zwischen den Kathoden
  p.r470 = 1000; p.heatLate = true; p.vdc = 322.5;                      // 6X4 → 1000 Ω (10 W) → 40 µ → 1000 Ω → +290 V (40 µ, 270 k/62 k Heizung) → 2,2 k → +268 V; → 47 k → +195 V
  p.biasV1 = 1.5; p.biasV2 = 10.0;                                      // Kathode 12AY7 +1,5 V; 12AU7 +33 V gegen +23 V
  return p; }
inline Parts partsFor(int model) { return model == 1 ? partsR() : Parts(); }
struct Setting {
  double lb = 0, la = 0, hb = 0, bw = 0.5, hc = 0;   // Drehwege 0…1 (vor der Kennlinie)
  int lf = 2, hf = 2, hcf = 1; bool in = true;         // Wahlschalter (Index), IN/OUT
};
enum { O_OUT = 0, O_IN, O_G1, O_G2, O_NO };   // Ausgang, Eingangsklemme, Gitter 12AX7 (5 gegen Masse), Gitter 12AU7 (Gegentakt)
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; };
// warpFs > 0: Spule der Höhenanhebung vorverzerrt, so dass die Resonanz der Trapezregel bei dieser Rate genau auf der analogen liegt (Güte gleich)
inline Built build(const Parts &v, const Setting &s, double warpFs = 0) {
  Built B; dk::Net &o = B.net; auto &dev = B.dev;
  const double ON = 0.01, OFF = 1e11;
  auto R = [&](const std::string &a, const std::string &b, double r) { o.r(a, b, std::max(r, ON)); };
  auto tri = [&](int kind, const std::string &g, const std::string &p, const std::string &k, double cgk, double cga) { int a = o.port(g, k); o.port(p, k); dev.push_back({kind, a, 2, nullptr}); o.c(g, k, cgk); o.c(g, p, cga); return a; };
  auto L1 = [&](const std::string &a, const std::string &b, double L) { dk::Mat m(1, 1); m(0, 0) = L; o.coupled({{a, b}}, m); };
  // Quelle, HS-56
  o.src("in", v.rsrc, 0); R("in", "ip", v.in_rp);
  { dk::Mat L(2, 2); L(0, 0) = L(1, 1) = v.in_lp; L(0, 1) = L(1, 0) = v.in_k * v.in_lp; o.coupled({{"ip", "0"}, {"isw", "0"}}, L); }
  o.c("isw", "0", v.in_cw); R("isw", "A", v.in_rs); R("A", "0", v.rterm);
  // Passives Netz. IN/OUT (zweipolig, Plan recproaudio SW2a/SW2b): IN legt das Ende des Höhenboost-Zweigs an B (1 MΩ darüber gegen Knacken) und den
  // Fußpunkt der Höhen-Atten-Kondensatoren an D; OUT schließt den Bass-Atten-Zweig (B–P) und D gegen Masse kurz – wie alle Regler auf 0.
  const double hb = std::max(0.0, std::min(1.0, s.hb)), hc = std::max(0.0, std::min(1.0, s.hc));
  R("A", "W", (1 - hb) * v.phb); R("W", "B", hb * v.phb);
  { int k = std::max(0, std::min(v.nhf - 1, s.hf)); double C = v.cHF[k], L = v.lHF[k], w0 = 1 / std::sqrt(L * C), RL = w0 * L / v.qHF[k];
    if (warpFs > 0) { double wa = 2 * warpFs * std::tan(w0 / (2 * warpFs)); L *= (w0 / wa) * (w0 / wa); RL *= w0 / wa; }
    o.c("W", "X", C); R("X", "Xl", RL); L1("Xl", "Y", L); }
  R("Y", "Z", std::max(0.0, std::min(1.0, s.bw)) * v.pbw); o.r("Z", "B", s.in ? ON : OFF); o.r("Z", "B", v.rbyp);
  R("B", "H", (1 - hc) * v.phc); R("H", "D", hc * v.phc); R("H", "Q", v.r75); o.c("Q", "Cc", v.cHC[std::max(0, std::min(v.nhc - 1, s.hcf))]); o.r("Cc", "D", s.in || !v.hcSw ? ON : OFF);
  { int k = std::max(0, std::min(v.nlf - 1, s.lf));
    R("D", "0", v.plb * taperA(s.lb, v.taperLB)); o.c("D", "0", v.cLB[k]); o.r("D", "0", s.in ? OFF : ON);
    R("B", "P", v.pla * taperA(s.la, v.taperLA)); o.c("B", "P", v.cLA[k]); o.r("B", "P", s.in ? OFF : ON); }
  R("P", "O", v.r1k); R("O", "D", v.r10k);
  // HS-29: Primär O–Masse, Sekundär 5 (Gitter V1A) … 6/7 (Masse) … 8 (Gitter V1B)
  R("O", "op", v.is_rp); R("op", "0", v.is_rc);
  { dk::Mat L(3, 3); for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) L(i, j) = (i == j ? 1 : v.is_k) * v.is_lp; o.coupled({{"op", "0"}, {"g1w", "0"}, {"0", "g2w"}}, L); }
  R("g1w", "G1", v.is_rs1); R("g2w", "G2", v.is_rs2); o.c("g1w", "g2w", v.is_cs); o.c("G1", "zo", v.zob_c); R("zo", "G2", v.zob_r);
  // V1 12AX7 (EQP-1R: 12AY7) im Gegentakt, Kathoden über 820 Ω (750 Ω) verbunden, je 360 Ω zur Gegenkopplungswicklung
  tri(v.v1, "G1", "A1", "K1", v.ax_cgk, v.ax_cga); tri(v.v1, "G2", "A2", "K2", v.ax_cgk, v.ax_cga);
  R("B250", "A1", v.ra1); R("B250", "A2", v.ra1); o.c("A1", "A2", v.cpp1); R("K1", "K2", v.rkk);
  R("K1", v.fbSign > 0 ? "F3" : "F5", v.rkf); R("K2", v.fbSign > 0 ? "F5" : "F3", v.rkf);
  // Kopplung, Gitterableitung an +31 V (Teiler 470 k / 62 k an +250 V)
  o.c("A1", "Q1", v.cc); o.c("A2", "Q2", v.cc); R("Q1", "B31", v.rg2); R("Q2", "B31", v.rg2); R("B250", "B31", v.rd1); R("B31", "0", v.rd2);
  // V2 12AU7 im Gegentakt, Kathoden je 4700 Ω ∥ 50 µF
  tri(K_AU7, "Q1", "P1", "KA", v.au_cgk, v.au_cga); tri(K_AU7, "Q2", "P2", "KB", v.au_cgk, v.au_cga);
  R("KA", "0", v.rk2); o.c("KA", "0", v.ck2); R("KB", "0", v.rk2); o.c("KB", "0", v.ck2);
  // S-217-D: Primärhälften (je n 0,5), Sekundär (n = √(Zs/Zp)), Gegenkopplung zwei Hälften je ot_nf
  { double ns = std::sqrt(v.ot_zs / v.ot_zp), n[5] = {0.5, 0.5, ns, v.ot_nf, v.ot_nf}; dk::Mat L(5, 5);
    for (int i = 0; i < 5; i++) for (int j = 0; j < 5; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? v.ot_kpp : v.ot_kps); L(i, j) = k * n[i] * n[j] * v.ot_lp; }
    R("P1", "p1w", v.ot_rp / 2); R("P2", "p2w", v.ot_rp / 2); R("sw", "OUT", v.ot_rs); R("f3w", "F3", v.ot_rf / 2); R("f5w", "F5", v.ot_rf / 2);
    o.coupled({{"p1w", "B290"}, {"B290", "p2w"}, {"sw", "0"}, {"f3w", "S"}, {"S", "f5w"}}, L); o.c("p1w", "p2w", v.ot_cpp); }
  R("S", "0", v.rS);
  R("OUT", "0", v.rout); o.c("OUT", "0", v.cout_); R("OUT", "0", v.rload);
  // Netzteil. Knotennamen nach den Spannungen des EQP-1A; im EQP-1R: B345 = erster Siebknoten, B325 = +290 V, B290 = +268 V (Mitte S-217-D), B250 = +195 V, B31 = +23 V
  o.src("B355", v.rdc, 1); R("B355", "B345", v.r470); o.c("B345", "0", v.cb); R(v.heatLate ? "B325" : "B345", "0", v.rheat);
  R("B345", "B325", v.r1000); o.c("B325", "0", v.cb); R("B325", "B290", v.r2200); o.c("B290", "0", v.cb); R("B325", "B250", v.r47k); o.c("B250", "0", v.cb);
  o.out("OUT"); o.out("in"); o.out("G1"); o.out("Q1", "Q2");
  for (size_t i = 0; i < dev.size(); i++) B.grp.push_back(0);   // ein Gegenkopplungskreis: alle vier Systeme gemeinsam
  return B; }

struct Core {
  double fs = 96000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; dk::Compiler comp; bool warp = true;
  void apply(const Setting &s) { cur = s; bl = build(v, s, warp ? fs : 0); net = std::move(bl.net);
    { std::vector<dk::Dev> old = std::move(run.dev); run.dev = bl.dev; dk::Runner::keepMem(run.dev, old); }
    run.devGrp = bl.grp; run.fn = devFn; const std::vector<double> gc0 = run.s.gc; run.set(comp.compile(net, fs));
    // Wahlschalter: ein Kondensator mit anderem Wert behält Spannung und Strom (Gedächtnis mit gc_neu/gc_alt), wie Loldano 1.8
    if (gc0.size() == run.s.gc.size() && run.h.size() >= gc0.size()) for (size_t k = 0; k < gc0.size(); k++) if (run.s.gc[k] != gc0[k] && gc0[k] > 0) run.h[k] *= run.s.gc[k] / gc0[k]; }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s); seedRest(); }   // Newton braucht hier einen Schritt je Abtastwert; dampFull/reuseStep machten es nur langsamer (gemessen)
  // Startwerte: Ruhespannungen (mit diesem Kern nach 30 s gerechnet, cultec_test dc); danach settle()
  void seedRest() { static const std::map<std::string, double> gA = CULTEC_REST, gR = CULTEC_REST_R; const auto &g = v.model == 1 ? gR : gA;
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  void step(double x, double *y) { double u[2] = {x, v.vdc}; run.step(u, y); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { long n = (long)(seconds * fs); double y[O_NO + 4]; for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); } }
  // Kleinsignal im Arbeitspunkt, von der Quellspannung zum Ausgang o (ohne Vorverzerrung, also analog)
  std::complex<double> response(double f, int o = O_OUT) const { Built b = build(v, cur, 0); std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> m((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) m[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(m); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(b.net, J, dp, f, 0, b.net.O[o].a, b.net.O[o].b); }
};

// ---------- Überabtastung: Kaiser-FIR (β 9) hinauf und hinunter, Latenz fest LAT Abtastwerte der Grundrate (auch bei 1x, dort als Verzögerung) ----------
// Länge os·LAT + 1; Mitte bei 0,48 der Grundrate (44,1 kHz: 21,2 kHz), Sperrbereich ab etwa 23 kHz −90 dB.
struct Os {
  static constexpr int LAT = 64;
  int os = 1, N = 1; std::vector<double> h, xb, yb; int xi = 0, yi = 0; std::vector<double> dl; int di = 0;
  static double I0(double x) { double s = 1, t = 1; for (int k = 1; k < 60; k++) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; if (t < 1e-17 * s) break; } return s; }
  void init(int os_) { os = os_ == 2 || os_ == 4 ? os_ : 1; xi = yi = di = 0;
    if (os == 1) { dl.assign(LAT, 0.0); h.clear(); return; }
    N = os * LAT + 1; h.assign(N, 0.0); const double fc = 0.48 / os, beta = 9, M = (N - 1) / 2.0; double sum = 0;
    for (int k = 0; k < N; k++) { double m = k - M, r = m / M, w = I0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / I0(beta); h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * w; sum += h[k]; }
    for (auto &x : h) x /= sum; xb.assign(2 * ((N + os - 1) / os), 0.0); yb.assign(2 * N, 0.0); }
  // ein Grundabtastwert: f(x) wird os-mal mit den hochgetasteten Werten gerufen und liefert die Ausgänge der hohen Rate
  template <class F> double run(double x, F &&f) {
    if (os == 1) { double y = f(x); double o = dl[di]; dl[di] = y; if (++di >= LAT) di = 0; return o; }
    double out = 0; const int M = (N + os - 1) / os;   // Polyphase: in Phase ph wirken nur die Abgriffe k = ph + j·os auf die Eingänge x[n − j]
    xi = xi == 0 ? M - 1 : xi - 1; xb[xi] = xb[xi + M] = x * os;
    for (int ph = 0; ph < os; ph++) {
      double u = 0; for (int j = 0, k = ph; k < N; j++, k += os) u += h[k] * xb[xi + j];
      double y = f(u); yi = yi == 0 ? N - 1 : yi - 1; yb[yi] = yb[yi + N] = y;
      if (ph == 0) for (int k = 0; k < N; k++) out += h[k] * yb[yi + k]; }   // Abgriff in Phase 0: Laufzeit genau (N − 1)/os = LAT
    return out; }
};
// ---------- Gerät: Reglerglättung, Pegelbezug, Überabtastung ----------
// Eingang: 1,0 = vin Volt Spitze an der Eingangsklemme (Quelle 600 Ω); Ausgang: Spannung an 600 Ω (∥ 620 Ω) / (vin · g0), g0 = Verstärkung Klemme → Ausgang
// bei 1 kHz mit allen Reglern auf 0 (laut Plan 0 dB; im Modell durch die Gegenkopplungswicklung darauf gesetzt). Flach also 1:1.
inline double dbuToVin(double dbu) { return 0.7746 * std::pow(10.0, dbu / 20) * std::sqrt(2.0); }
struct Unit {
  double fsB = 48000; int os = 2; Core c, c0; Parts v; Setting tgt; Os ov; bool ready = false; int cnt = 0, bad = 0;
  double vin = dbuToVin(18), kin = 1, g0 = 1;   // kin: Quellspannung je Volt an der Klemme (1 kHz, flach)
  double pkOut = 0, pkDrv = 0, pkV1 = 0;   // Spitzen: Ausgang (V), Gitter 12AU7 Gegentakt (V), Gitter 12AX7 (V)
  static constexpr int LAT = Os::LAT;
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) {
    fsB = fs; os = os_ == 2 || os_ == 4 ? os_ : 1; if (fs * os > 200000) os = std::max(1, (int)(200000 / fs));   // höchstens etwa 200 kHz intern
    v = pv; tgt = s; ov.init(os); c.init(fsB * os, v, s); c.settle(0.5, cancel);
    { Core f = c; f.cur = Setting(); std::complex<double> o = f.response(1000, O_OUT), i = f.response(1000, O_IN); kin = 1 / std::abs(i); g0 = std::abs(o / i); }
    c0 = c; ready = true; }
  static bool swEq(const Setting &a, const Setting &b) { return a.lf == b.lf && a.hf == b.hf && a.hcf == b.hcf && a.in == b.in; }
  void setTarget(const Setting &s) { tgt = s; if (!swEq(tgt, c.cur)) { Setting n = c.cur; n.lf = s.lf; n.hf = s.hf; n.hcf = s.hcf; n.in = s.in; c.apply(n); } }   // Schalter sofort
  void snap() { c.apply(tgt); cnt = 0; }
  // Regler gleiten (15 ms), Netz alle 64 Abtastwerte der Grundrate neu
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    auto glide = [&](double &x, double t) { double d = t - x; if (d == 0) return; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = t; };
    glide(n.lb, tgt.lb); glide(n.la, tgt.la); glide(n.hb, tgt.hb); glide(n.bw, tgt.bw); glide(n.hc, tgt.hc);
    if (moved) c.apply(n); }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    const double sc = 1 / (vin * g0); double yy[O_NO + 4];
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-4.0, std::min(4.0, (double)buf[i])) : 0; tick();
      double y = ov.run(x * vin * kin, [&](double u) { c.step(u, yy); pkOut = std::max(pkOut, std::fabs(yy[O_OUT])); pkDrv = std::max(pkDrv, std::fabs(yy[O_G2])); pkV1 = std::max(pkV1, std::fabs(yy[O_G1])); return yy[O_OUT]; }) * sc;
      if (!(std::fabs(y) < 100)) { y = 0; if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--; buf[i] = (float)y; } }
  // Anzeigen: Ausgang in dBm an 600 Ω (Spitze als Sinus), Aussteuerung der 12AU7 (Gitterspitze gegen die Vorspannung 9 V, EQP-1R 10 V) und der 12AX7 (gegen 1,3 V; 12AY7 1,5 V)
  void takePeaks(double *out) { out[0] = pkOut; out[1] = pkDrv / 2 / v.biasV2; out[2] = pkV1 / v.biasV1; pkOut = pkDrv = pkV1 = 0; }
};
} // namespace cultec
