// Ampeg SVT-CL: Nachbau nach dem Service Manual (Vorverstärker 07S519-12, Endstufe 07S419-12, Röhrenplatine 07S419-41, Lautsprecherplatine 06A319-14).
// Ganze Schaltung im Zeitbereich: Eingänge, V1B, ULTRA LOW, GAIN, V1A mit ULTRA HIGH, passive Klangregelung (BASS, TREBLE), V2B mit MIDDLE
// (Reihenschwingkreis mit angezapfter Spule, FREQUENCY fünf Stellungen), MASTER, Kathodenfolger V2A; Endstufe: Eingangsstufe mit Diodenbegrenzung
// und Gegenkopplung vom Übertrager, Kathodyn-Phasenumkehr, zwei 12AU7-Treiber mit Kathodenfolgern an den Gittern, 2 × 3 6550 im Gegentakt,
// Ausgangsübertrager mit 2-/4-Ω-Abgriff, Netzteil (660 V, Schirmgitter/Treiber aus eigener Wicklung). Vorstufen-Anodenspannung 345 V als ideale Quelle.
// Bauteilwerte und Knoten: docs/svtcl/SCHALTPLAN.md. Geschätzt (nicht im Plan): Übertrager-Induktivitäten und -Verluste, Spule L1 (Wicklungswiderstand,
// Kopplung), Innenwiderstände des Netzteils, Gegenkopplungs-Abgriff; Röhrenmodelle: 12AX7/12AU7 wie im Laney, 6550 an Datenblattwerte angepasst (engine/tools/tube_fits.py).
#pragma once
#define DK_BIGN 1   // entrollte Löser für die große Endstufen-Gruppe (dk.hpp); muss vor dem ersten Einbinden von dk.hpp stehen
#include "dk.hpp"
#include "b15_core.hpp"
#include <atomic>
#include <cmath>
#include <map>
#include <string>
#include <vector>
namespace svt {
constexpr double PI = 3.14159265358979323846;
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};    // wie TUBE_MODELS[0] in core.hpp
static const b15::TriodeD T12AU7 = {9.627442e-4, 0.350357, 19.74, 1.5};  // wie TUBE_MODELS[2] in core.hpp
// 6550: Derk-Form (wie D6L6), an Datenblattwerte angepasst (Tung-Sol/GE: 250/250/−14 V 140 mA, Ig2 12 mA, 11 mA/V, 15 kΩ; 400/225/−16,5 V 85–87 mA, Ig2 4–9 mA),
// Knie-Parameter (ex, β, αs) wie 6L6GC; engine/tools/tube_fits.py
static const b15::PentodeD D6550 = {7.584690749, 1.562959483, 671.7245705, 200, 300, 12377.82231, 0.0002564468176, 0.06717976054, 10, 0, 0, 0, 0, 1, 1, 1};
static const b15::Sat OT_SAT = {30, 6.0, 0.15};   // Ausgangsübertrager: Lp 30 H, Sättigungsfluss 6 V·s, Hysterese wie AmpFett (alles geschätzt)
enum Kind { K_AX7 = 0, K_AU7 = 1, K_6550 = 2, K_SAT = 4, K_CLAMP = 5 };
// 1N3070 (Eingangsbegrenzung der Endstufe): Shockley, Is 2 nA, n 1,9 (geschätzt aus dem Datenblatt: 0,8 V bei 100 mA)
inline void clampD(const double *v, double *i, double *J) { const double Is = 2e-9, nVt = 1.9 * 0.02585; double x = v[0] / nVt;
  if (x > 40) { double e = std::exp(40.0); i[0] = Is * (e * (1 + x - 40) - 1); J[0] = Is * e / nVt; return; }
  double e = std::exp(x); i[0] = Is * (e - 1); J[0] = Is * e / nVt; }
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) {
    case K_AX7: b15::triodeD(T12AX7, v, i, J); break;
    case K_AU7: b15::triodeD(T12AU7, v, i, J); break;
    case K_6550: b15::pentodeD(D6550, v, i, J); for (int k = 0; k < 3; k++) i[k] *= d.sc; for (int k = 0; k < 9; k++) J[k] *= d.sc; break;   // sc: drei Röhren parallel
    case K_SAT: b15::satI(d, *(const b15::Sat *)d.model, v[0], i, J); break;
    default: clampD(v, i, J); } }

struct Parts {   // Werte laut Plan (Ω, F); geschätzt markiert
  // Vorverstärker 07S519-12
  double r1 = 22e3, r3 = 100e3, r4 = 3.3e6, r6 = 4.7e3, c1 = 10e-6, r7 = 100e3, r8 = 1.5e3, c2 = 22e-6, c3 = 0.1e-6, c4 = 56e-12;
  double r11 = 100e3, c7 = 470e-12, r19 = 220e3, r20 = 330e3, r21 = 220e3, r22 = 3.3e6, r23 = 3.3e6, c8 = 0.0047e-6, gain = 1e6, c9 = 0.1e-6;
  double r26 = 1e6, r27 = 100e3, r28 = 1.5e3, r29 = 1.5e3, c10 = 0.033e-6, r31 = 100e3, c11 = 0.1e-6;
  double r48 = 220e3, bass = 1e6, c20 = 0.001e-6, c21 = 0.01e-6, r37 = 22e3, r30 = 100e3, c23 = 470e-12, treb = 1e6, c27 = 0.0047e-6, r32 = 100e3, c16 = 0.1e-6;
  double r40 = 470e3, r44 = 1.5e3, r45 = 2.2e3, r43 = 100e3, c5 = 0.1e-6, r12 = 100e3, r13 = 8.2e3, r36 = 470, mid = 50e3, c12 = 10e-6, master = 50e3, c6 = 0.1e-6;
  double l1a = 0.1, l1b = 0.3, l1c = 0.8, l1_r = 190, l1_k = 0.995;   // L1: 100/300/800 mH ab „CMN“ (Plan); Wicklungswiderstand (für 1 H gerechnet) und Kopplung geschätzt
  double c17 = 0.033e-6, c19 = 0.033e-6, c22 = 0.1e-6, c15 = 0.15e-6, c13 = 0.68e-6, rsw = 470e3;
  double r14 = 220e3, r15 = 4.7e3, r16 = 220e3, c26 = 0.68e-6, r33 = 1e6;
  double b_pre = 345;   // Vorstufen-Anodenspannung (Plan: +345 V; ideal angenommen)
  // Endstufe 07S419-12
  double r92 = 220, pr7 = 1e3, pr8 = 470e3, pr12 = 100e3, pr13 = 1e3, pr14 = 220, nfb_r = 68e3, nfb_c = 100e-12;
  double pc6 = 0.1e-6, pr16 = 1e6, pr17 = 820, pr18 = 68e3, pr15 = 68e3, pc7 = 50e-12;
  double pc8 = 0.1e-6, pr19 = 470e3, pr20 = 1.8e3, pr21 = 47e3, pc10 = 0.047e-6, pr22 = 150e3, pr23 = 150e3, pr26 = 47e3;
  double rg = 47e3, rscr = 22, rk = 10;   // Röhrenplatine je 6550: Gitter 47 k, Schirmgitter 22 Ω, Kathode 10 Ω
  double bias_rth = 54e3;   // Innenwiderstand am BIAS-Schleifer (120 k / 50 k / 68 k bei etwa −62 V)
  double v_neg = -180, v_bias = -59.384;   // −180 V; Schleifer so, dass jede 6550 in Ruhe 24 mA zieht (0,24 V an 10 Ω, Plan) – in svttest geeicht
  // Übertrager 94-602-12: 1850 Ω Anode–Anode (aus 372 V Anodenspannung bei 300 W an 4 Ω, Plan); 2-/4-Ω-Abgriff; Rest geschätzt
  double ot_zp = 1850, ot_rp = 40, ot_rs2 = 0.03, ot_rs4 = 0.02, ot_lp = 30, ot_kps = 0.9995, ot_kpp = 0.9998, ot_cpp = 1e-9, ot_rc = 50e3;   // Streuung/Kapazität so, dass
  // der ganze Verstärker bei 15 kHz −3 dB hat wie in der Spezifikation (mit 0,9985 Kopplung: Überhöhung +5 dB bei 24 kHz)
  int nfb_tap = 4;   // Gegenkopplung von J6-1: welcher Abgriff das ist, steht nicht im Plan (4 Ω angenommen)
  // Netzteil (geschätzt): Gleichspannungen hinter Innenwiderständen (die Gleichrichterbrücken als immer leitend, ohne Brumm):
  // Hochspannung 660 V, C15/C16 100 µ in Reihe, Bleeder 300 k;
  // Schirmgitter-/Treiberwicklung (Brücke D14–D17) → C21 47 µ (+380), R59 2,2 k → C22 47 µ (+365), D26 → Schirmgitter (345 V in Ruhe laut Röhrenplatine)
  double hv_v = 663.9, hv_r = 20, c_b1 = 50e-6, r_bleed = 300e3, sc_v = 347.4, sc_r = 60, c_b2 = 47e-6, r59 = 2.2e3, c_b3 = 47e-6;
  double rsrc = 1e3;   // Innenwiderstand der Quelle (Instrument), geschätzt
};
// Stellungen: jack 0 = 0 dB, 1 = −15 dB; freq 0 … 4 = 220 Hz … 3 kHz; tap 0 = 4 Ω (Last 4 Ω), 1 = 2 Ω (Last 2 Ω)
struct Setting { double gain = 0.5, bass = 0.5, mid = 0.5, treb = 0.5, master = 0.5; int freq = 2; bool ulo = false, uhi = false; int jack = 0, tap = 0, taper = 0; };
inline double &sfield(Setting &s, int i) { double *f[] = {&s.gain, &s.bass, &s.mid, &s.treb, &s.master}; return *f[i]; }
inline double potTaper(double x, int mode) { return b15::potTaper(x, mode); }
enum { O_SPK = 0, O_B1 = 1, O_SCR = 2, O_PRE = 3, O_GA = 4, O_GB = 5, O_NO = 6 };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; int mon[6] = {0, 0, 0, 0, 0, 0}; int pen[2] = {0, 0}; };   // mon: Tore g−k von V1B, V1A, V2B, V2A (Vorstufe), V1A Endstufe, V1B Endstufe
inline double loadOhm(const Setting &s) { return s.tap == 1 ? 2.0 : 4.0; }
inline Built build(const Parts &v, const Setting &s, const std::vector<std::string> *dbg = nullptr) {   // dbg: zusätzliche Ausgänge (Knotenspannungen, Prüfprogramme)
  Built B; dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto tri = [&](int kind, const std::string &g, const std::string &p, const std::string &k) { int a = o.port(g, k); o.port(p, k); dev.push_back({kind, a, 2, nullptr});
    o.c(g, p, kind == K_AX7 ? 1.7e-12 : 2.0e-12); return a; };   // Gitter–Anode (Miller); 12AX7 1,7 pF, 12AU7 2,0 pF (wie Laney)
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };
  const double xg = potTaper(s.gain, s.taper), xb = potTaper(s.bass, s.taper), xt = potTaper(s.treb, s.taper);   // GAIN, BASS, TREBLE: 1 M „A“
  o.src("Bp", 0.1, 6);   // +345 V Vorstufen
  // --- Eingänge ---
  o.src("in", v.rsrc, 0);
  if (s.jack == 0) { o.r("in", "G1", v.r1); o.r("in", "G1", v.r3); }   // 0 dB: R1 ∥ R3 (J2 leer verbindet R3 mit J1)
  else { o.r("in", "G1", v.r3); o.r("G1", "0", v.r1); }                // −15 dB: R3 in Reihe, R1 an die geschlossene J1-Spitze (Masse)
  o.r("G1", "0", v.r4);
  // --- V1B ---
  B.mon[0] = tri(K_AX7, "G1", "A1", "K1"); o.r("K1", "0", v.r8); o.c("K1", "0", v.c2); o.r("Bp", "Bd", v.r6); o.c("Bd", "0", v.c1); o.r("Bd", "A1", v.r7);
  o.c("A1", "X", v.c3); o.c("X", "0", v.c4);
  // --- ULTRA LOW, GAIN ---
  o.r("X", "L", v.r19); o.r("X", "Xr", v.r11); o.c("Xr", "T", v.c7); o.r("T", "0", v.r22); o.r("U", "0", v.r23); o.c("L", "U", v.c8);
  if (s.ulo) { o.r("L", "M", v.r21); o.r("M", "T", 0.1); o.r("U", "0", 0.1); o.r("L", "R20", v.r20); o.r("R20", "0", 1e9); }   // 8–9, 11–12
  else { o.r("L", "M", 0.1); o.r("L", "R20", v.r20); o.r("R20", "0", 0.1); }                                                   // 8–7, 11–10
  pot("M", "gw", "0", v.gain, xg); o.c("gw", "G2", v.c9); G = 0; mark();
  // --- V1A, ULTRA HIGH ---
  B.mon[1] = tri(K_AX7, "G2", "A2", "K2"); o.r("G2", "Kt2", v.r26); o.r("K2", "Kt2", v.r28); o.r("Kt2", "0", v.r29); o.r("Bp", "A2", v.r27);
  o.c("K2", "N", v.c10); o.r("N", "0", s.uhi ? 0.1 : v.r31); G = 1; mark();
  // --- Klangregelung ---
  o.c("A2", "P", v.c11); o.r("P", "B1n", v.r48); pot("B1n", "bw", "B2n", v.bass, xb); o.c("B1n", "bw", v.c20); o.c("bw", "B2n", v.c21); o.r("B2n", "0", v.r37); o.r("bw", "Q", v.r30);
  o.c("P", "TT", v.c23); pot("TT", "Q", "TB", v.treb, xt); o.c("TB", "0", v.c27);
  o.r("Q", "Qr", v.r32); o.c("Qr", "G3", v.c16);
  // --- V2B, MIDDLE, FREQUENCY, MASTER ---
  B.mon[2] = tri(K_AX7, "G3", "A3", "K3"); o.r("G3", "Kt3", v.r40); o.r("K3", "Kt3", v.r44); o.r("Kt3", "0", v.r45); o.r("Bp", "A3", v.r43);
  o.c("A3", "A3c", v.c5); o.r("A3c", "Kn", v.r12); o.r("Kn", "0", v.r13);
  o.c("K3", "mc", v.c12); pot("mc", "mw", "mr", v.mid, s.mid); o.r("mr", "Kn", v.r36);   // MIDDLE 50 k linear: CW-Ende an C12
  { double n3 = std::sqrt(v.l1a), n2 = std::sqrt(v.l1b), n1 = std::sqrt(v.l1c), n[3] = {n3, n2 - n3, n1 - n2}; dk::Mat L(3, 3);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) L(i, j) = (i == j ? 1 : v.l1_k) * n[i] * n[j];
    o.r("mw", "mwa", v.l1_r * n[0]); o.r("t3", "t3a", v.l1_r * n[1]); o.r("t2", "t2a", v.l1_r * n[2]);
    o.coupled({{"mwa", "t3"}, {"t3a", "t2"}, {"t2a", "t1"}}, L); }
  o.c("t3", "F5", v.c17); o.c("t2", "F4", v.c19); o.c("t2", "F3", v.c22); o.c("t1", "F2", v.c15); o.c("t1", "F1", v.c13); o.r("t1", "0", v.rsw);
  { const char *fn[5] = {"F1", "F2", "F3", "F4", "F5"}; int f = std::max(0, std::min(4, s.freq)); for (int i = 0; i < 5; i++) o.r(fn[i], "0", i == f ? 0.1 : v.rsw); }
  pot("Kn", "Mw", "0", v.master, s.master); o.c("Mw", "G4", v.c6); G = 2; mark();
  // --- V2A Kathodenfolger ---
  B.mon[3] = tri(K_AX7, "G4", "Bp", "K4"); o.r("G4", "Kt4", v.r14); o.r("K4", "Kt4", v.r15); o.r("Kt4", "0", v.r16);
  o.c("K4", "PO", v.c26); o.r("PO", "0", v.r33);   // V2A gehört zur Gruppe der Endstufe: die Begrenzerdioden D55/D56 belasten ihn stark
  // ===== Endstufe =====
  o.r("PO", "pi", v.r92); o.r("pi", "pg", v.pr7); o.r("pg", "0", v.pr8);
  { int p = o.port("pg", "0"); dev.push_back({K_CLAMP, p, 1, nullptr}); p = o.port("0", "pg"); dev.push_back({K_CLAMP, p, 1, nullptr}); }   // D55/D56
  B.mon[4] = tri(K_AX7, "pg", "pa", "pk"); o.r("pk", "F", v.pr13); o.r("F", "0", v.pr14); o.r("Bp", "pa", v.pr12);
  { std::string tp = v.nfb_tap == 2 ? "s2" : "s4"; o.r(tp, "F", v.nfb_r); o.c(tp, "F", v.nfb_c); }   // Gegenkopplung 68 k ∥ 100 pF
  o.c("pa", "gb", v.pc6); o.r("gb", "kt", v.pr16); B.mon[5] = tri(K_AX7, "gb", "pb", "kb"); o.r("kb", "kt", v.pr17); o.r("kt", "0", v.pr18); o.r("Bp", "pb", v.pr15); o.c("pb", "gb", v.pc7);
  // Treiber: Seite A von der Anode (V2), Seite B von der Kathode (V3) der Phasenumkehr
  const char *sd[2] = {"A", "B"}; const char *srcn[2] = {"pb", "kb"};
  for (int k = 0; k < 2; k++) { std::string S = sd[k];
    o.c(srcn[k], "dg" + S, v.pc8); o.r("dg" + S, "0", v.pr19); tri(K_AU7, "dg" + S, "dp" + S, "dk" + S); o.r("dk" + S, "0", v.pr20); o.r("B365", "dp" + S, v.pr21);
  }
  for (int k = 0; k < 2; k++) { std::string S = sd[k];
    o.c("dp" + S, "dn" + S, v.pc10); o.r("dn" + S, "fg" + S, v.pr22); o.r("dn" + S, "bw" + S, v.pr23); o.src("bw" + S, v.bias_rth, 4);
    tri(K_AU7, "fg" + S, "B380", "fk" + S); o.r("fk" + S, "NEG", v.pr26);
    o.r("fk" + S, "g" + S, v.rg / 3); o.r("SCR", "s" + S, v.rscr / 3); o.r("k" + S, "0", v.rk / 3);
    B.pen[k] = (int)o.P.size(); int p0 = o.port("g" + S, "k" + S); o.port("s" + S, "k" + S); o.port("a" + S, "k" + S); dk::Dev d{K_6550, p0, 3, nullptr}; d.sc = 3; dev.push_back(d); }
  o.src("NEG", 1.0, 3);
  { int p = o.port("aAw", "aBw"); dk::Dev d{K_SAT, p, 1, &OT_SAT}; b15::hystSetup(d, OT_SAT); dev.push_back(d); }
  G = 3; mark();   // Endstufe als eine Gruppe (Gegenkopplung, Gitterstrom über die Kathodenfolger; getrennt pendelte Gauß-Seidel bei Übersteuerung)
  // --- Ausgangsübertrager: zwei Primärhälften, Sekundär 0–2 Ω–4 Ω ---
  { double n2 = std::sqrt(2 / v.ot_zp), n4 = std::sqrt(4 / v.ot_zp), n[4] = {0.5, 0.5, n2, n4 - n2}; dk::Mat L(4, 4);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? v.ot_kpp : v.ot_kps); if (i >= 2 && j >= 2 && i != j) k = 0.99999; L(i, j) = k * n[i] * n[j] * v.ot_lp; }
    o.r("aA", "aAw", v.ot_rp / 2); o.r("aB", "aBw", v.ot_rp / 2); o.r("s2w", "s2", v.ot_rs2); o.r("s4w", "s4", v.ot_rs4);
    o.coupled({{"aAw", "B1"}, {"B1", "aBw"}, {"s2w", "0"}, {"s4w", "s2"}}, L);
    o.c("aAw", "aBw", v.ot_cpp); o.r("aAw", "aBw", v.ot_rc);
    if (s.tap == 1) { o.r("s2", "0", 2.0); o.r("s4", "0", 1e6); } else o.r("s4", "0", 4.0); }   // Last am gewählten Abgriff (ohmsch)
  // --- Netzteil ---
  o.src("B1", v.hv_r, 1); o.c("B1", "0", v.c_b1); o.r("B1", "0", v.r_bleed);
  o.src("B380", v.sc_r, 2); o.c("B380", "0", v.c_b2); o.r("B380", "B365", v.r59); o.c("B365", "0", v.c_b3);
  o.r("B380", "SCR", 1.0);   // D26 (immer leitend) als 1 Ω
  o.out(s.tap == 1 ? "s2" : "s4"); o.out("B1"); o.out("SCR"); o.out("PO"); o.out("gA"); o.out("gB");
  if (dbg) for (auto &n : *dbg) o.out(n);
  return B; }

struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; const std::vector<std::string> *dbg = nullptr;
  void apply(const Setting &s) { cur = s; bl = build(v, s, dbg); net = std::move(bl.net); { std::vector<dk::Dev> old = std::move(run.dev); run.dev = bl.dev; dk::Runner::keepMem(run.dev, old); } run.devGrp = bl.grp; run.fn = devFn; run.set(dk::compile(net, fs)); }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); run.dampFull = true; run.reuseStep = true; run.reuseRho = 0.1; apply(s);   // ab 1.2: gemeinsamer Solve gleich gedämpft (harter Anschlag: kaum noch Notfälle, kürzere Spitzen je Block); ab 1.4: Zerlegung der großen Gruppe innerhalb eines Abtastwerts weiterverwenden
    // Startwerte aus den Spannungen des Plans, damit das Netzteil nicht lange einschwingen muss
    std::map<std::string, double> g = {{"B1", 660}, {"hv", 661}, {"B380", 346}, {"B365", 330}, {"SCR", 345}, {"Bd", 340}, {"A1", 235}, {"K1", 1.5}, {"A2", 240}, {"K2", 1.6}, {"Kt2", 0.8},
      {"A3", 240}, {"K3", 2.3}, {"Kt3", 1.4}, {"mc", 2.3}, {"K4", 100}, {"Kt4", 98}, {"G4", 98}, {"pa", 200}, {"pk", 1.7}, {"F", 0.3}, {"pb", 250}, {"kb", 95}, {"kt", 94}, {"gb", 94},
      {"dpA", 180}, {"dpB", 180}, {"dkA", 7}, {"dkB", 7}, {"dnA", -61}, {"dnB", -61}, {"fgA", -61}, {"fgB", -61}, {"fkA", -40}, {"fkB", -40}, {"gA", -40}, {"gB", -40}, {"bwA", -61}, {"bwB", -61},
      {"aA", 659}, {"aB", 659}, {"aAw", 659}, {"aBw", 659}, {"sA", 345}, {"sB", 345}, {"kA", 0.8}, {"kB", 0.8}, {"NEG", -180}, {"Bp", 345}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  void step(double x, double *y) { double u[7] = {x, v.hv_v, v.sc_v, v.v_neg, v.v_bias, v.v_bias, v.b_pre}; run.step(u, y); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { std::vector<double> yv(net.O.size() + 1); double *y = yv.data(); long n = (long)(seconds * fs); for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); } }
  std::complex<double> response(double f) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.idx.at(cur.tap == 1 ? "s2" : "s4"), 0); }
};
// Verstärker mit Überabtastung, Reglerglättung, Schaltern und Anzeigen. Ausgabe: Spannung am gewählten Abgriff, 1,0 = vfs() Volt
// (4 Ω: 60 V Spitze = 0 dBFS, 300 W an 4 Ω sind 49 V Spitze; 2 Ω: 42,4 V, gleiche Leistung)
struct Amp {
  double fsB = 48000; int os = 1; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 0.1; bool power = true; double pwr = 1;
  double vgk0[6] = {-1, -1, -1, -1, -1, -1}, peak[6] = {0, 0, 0, 0, 0, 0}, outPk = 0;   // Gitterhub je Stufe (V1B, V1A, V2B, Endstufe V1A, 6550 A/B)
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  Core c0; int bad = 0;   // c0: eingeschwungener Zustand nach init (Rückfall bei entgleister Rechnung)
  double vfs() const { return 60 * std::sqrt(loadOhm(c.cur) / 4); }
  int monPort(int i) const { return i < 4 ? c.bl.mon[i == 3 ? 4 : i] : c.bl.pen[i - 4]; }
  void readBias() { for (int i = 0; i < 6; i++) vgk0[i] = c.run.vp[monPort(i)]; }
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) { fsB = fs; os = std::max(1, os_); v = pv; tgt = s;
    c.init(fsB * os, v, s); c.settle(1.5, cancel); c0 = c; readBias(); pwr = power ? 1 : 0;
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  static bool sameSw(const Setting &a, const Setting &b) { return a.freq == b.freq && a.ulo == b.ulo && a.uhi == b.uhi && a.jack == b.jack && a.tap == b.tap && a.taper == b.taper; }
  // Schalter sofort (Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void setTarget(const Setting &s) { tgt = s; if (!sameSw(s, c.cur)) { Setting n = c.cur; n.freq = s.freq; n.ulo = s.ulo; n.uhi = s.uhi; n.jack = s.jack; n.tap = s.tap; n.taper = s.taper; c.apply(n); readBias(); } }
  void snap() { c.apply(tgt); readBias(); pwr = power ? 1 : 0; cnt = 0; }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < 5; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO]; c.step(x, y);
    for (int i = 0; i < 6; i++) { double d = std::fabs(c.run.vp[monPort(i)] - vgk0[i]); if (d > peak[i]) peak[i] = d; }
    double st = 1.0 / (0.02 * c.fs), tP = power ? 1 : 0; if (pwr < tP) pwr = std::min(tP, pwr + st); else if (pwr > tP) pwr = std::max(tP, pwr - st);
    double el = y[O_SPK]; if (std::fabs(el) > outPk) outPk = std::fabs(el); return el * pwr / vfs(); }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();   // Eingang auf ±20 V begrenzt
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 10)) { y = 0;   // entgleist (NaN oder über 10-fache Vollaussteuerung, physikalisch unmöglich): eingeschwungenen Zustand zurückholen, höchstens einmal je 0,1 s
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); readBias(); std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung je Stufe seit der letzten Abfrage: Gitterhub bezogen auf die Gittervorspannung (1 = Gitter erreicht 0 V)
  void takePeaks(double *out) { for (int i = 0; i < 6; i++) { out[i] = peak[i] / std::max(0.1, std::fabs(vgk0[i])); peak[i] = 0; } }
  double takeOut() { double p = outPk; outPk = 0; return p; }   // Spitzenspannung am Abgriff (V)
};
} // namespace svt
