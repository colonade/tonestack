// AmpFett 76: UREI 1176LN (Schaltplan „Solid State Limiting Amplifier Model 1176LN“, gültig ab Seriennummer-Endung K, mit Gleichspannungen
// vom 23.10.2002; Revisionen bis S) als Schaltung im Zeitbereich (DK-Löser, dk.hpp), Bauteil für Bauteil nach dem Plan:
// Eingang IC2 (symmetrisch, Verstärkung 1), INPUT R5 10 k, R6 27 k auf den Spannungsteiler mit dem FET Q1 (BF245A; Gate über R7/R9 2,2 M,
// C3/C4 200 p, C5 0,22 µ vom Gegenkopplungsknoten: halbe Drain-Spannung aufs Gate), Vorverstärker Q2–Q4 (Gegenkopplung R21 7,5 k über
// R15 150 + R16 100 auf den Emitter von Q2, Bootstrap der Source über C2 100 µ), OUTPUT R27 100 k, Ausgangsverstärker Q5–Q9
// (Q6/Q7 PNP, CR1/CR2, Q8/Q9 im Gegentakt, Gegenkopplung R34 4,3 k), C15 470 µ, Ausgangsübertrager T2.
// Seitenkette: vom Vorverstärker-Ausgang über R26 56 k, R25 68 k, R24 56 k, R2 56 k, R23 47 k (Ratio-Tasten S1B–S4B greifen ab),
// S9 (Attack OFF erdet den Eingang), GR-Verstärker Q12–Q15 mit Gegenkopplung R68 47 k, Vollweg-Gleichrichter CR3/CR4 (Gegentakt
// von Q13-Emitter und Q15-Emitter), Rücklauf über R74/R75 47 k auf die Schwelle (Knoten 21: Ratio-Tasten S1A–S4A an der Kette
// R87 150, R42 470, R43 560, R44 1,5 k, R45 2,2 k nach −10 V), ATTACK R77 25 k (R76 470 ∥ C22 0,022 µ), Haltekondensator C27 0,22 µ,
// RELEASE R78 5 M ∥ R79 270 k zum „Q“-Bias R81 2 k (R82 10 k / R83 3,9 k an −10 V).
// Abgleich wie im Manual: „Q“-Bias so, dass der FET ohne Seitenkette (Attack OFF) 1,0 dB dämpft (Gyraf-Anleitung bzw. UA-Manual S. 26).
// Nicht nach Plan (Annahmen, docs/u76/SCHALTPLAN.md): Kennwerte der Halbleiter (Datenblattbereiche), Übertrager ideal 1:1, Netzteil ideal
// (+30,3 V, −10 V), Kennlinie der Potis, Element „T+C“ (UA4706, Werkswert ohne Angabe) nicht bestückt; IC2 als idealer Verstärker.
// Erweiterungen nach Origin Effects Cali76 (eigener Entwurf, keine Schaltung veröffentlicht): stufenlose Ratio (Tandem-Abgriff über die
// beiden Ratio-Ketten), Hochpass in der Seitenkette, DRY (Parallelmischung), zweite Stufe (Stacked).
#pragma once
#include "gk_core.hpp"   // expg, Jfet/jfetI, Clamp/clampI, potTaper
#include <atomic>
#include <cmath>
#include <complex>
#include <map>
#include <string>
#include <vector>
namespace u76 {
constexpr double PI = 3.14159265358979323846;
using gk::Jfet; using gk::potTaper;

// ---------- Halbleiter ----------
// Bipolartransistor nach Ebers-Moll (Transportmodell) mit Early-Effekt im Transportstrom (Faktor 1 − Ubc/VA).
// Tore (B−E), (B−C); Torströme: Tor 1 = Emitterstrom (fließt B→E), Tor 2 = −Ic (B→C). pol = +1 NPN, −1 PNP (gespiegelt, Ableitungen gleich)
struct Bjt { double is, bf, br, pol, va = 100; };
constexpr double VT = 0.025852;   // kT/q bei 27 °C
inline void bjtI(const Bjt &q, const double *v, double *i, double *J) {
  const double s = q.pol; double ef, df, er, dr; gk::expg(s * v[0] / VT, ef, df, 30); gk::expg(s * v[1] / VT, er, dr, 30);
  const double k = 1 - s * v[1] / q.va, it = q.is * (ef - er);
  double ic = it * k - q.is / q.br * (er - 1), ib = q.is / q.bf * (ef - 1) + q.is / q.br * (er - 1);
  double dic_dbe = q.is * df / VT * k, dic_dbc = -q.is * dr / VT * k - it / q.va - q.is / q.br * dr / VT, dib_dbe = q.is / q.bf * df / VT, dib_dbc = q.is / q.br * dr / VT;
  i[0] = s * (ic + ib); i[1] = s * (-ic);
  J[0] = dic_dbe + dib_dbe; J[1] = dic_dbc + dib_dbc; J[2] = -dic_dbe; J[3] = -dic_dbc; }
struct Diode { double is, nvt, ir = -1; };   // ir: Sperrstrom (A), wenn er kleiner sein soll als der Sättigungsstrom der Durchlasskennlinie; −1 = wie is
inline void diodeI(const Diode &d, double v, double *i, double *J) { double e, de; gk::expg(v / d.nvt, e, de, 30); double a = v < 0 && d.ir >= 0 ? d.ir : d.is; i[0] = a * (e - 1); J[0] = a * de / d.nvt; }
enum Kind { K_BJT = 0, K_DIODE = 1, K_JFET = 2, K_CLAMP = 3 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) { case K_BJT: bjtI(*(const Bjt *)d.model, v, i, J); break; case K_DIODE: diodeI(*(const Diode *)d.model, v[0], i, J); break;
    case K_JFET: gk::jfetI(*(const Jfet *)d.model, v, i, J); break; default: gk::clampI(*(const gk::Clamp *)d.model, v[0], i, J); } }

// ---------- Bauteile ----------
struct Parts {   // Werte laut Plan (Ω, F, V); Annahmen markiert
  double v30 = 30.3, vneg = -10.0, r30 = 0.5, rneg = 5;   // Netzteil: VR1 7824 mit R87/R89 (Plan: +30,3 V), −10 V über CR6 (Annahme: ideal hinter 0,5 bzw. 5 Ω)
  // Eingang IC2 (Verstärkung 1, symmetrisch; hier unsymmetrisch angesteuert), C33 6,8 µ, INPUT R5 10 k, R6 27 k
  double ic2_rout = 50, ic2_r = 10e3, ic2_c = 220e-12, ic2_vs = 13.5;   // IC2-A: 10 k ∥ 220 p Gegenkopplung (Tiefpass 72 kHz); Aussteuerung ±13,5 V um 15 V (Annahme)
  double c33 = 6.8e-6, r5 = 10e3, r6 = 27e3;
  // FET-Abschwächer und Vorverstärker
  double c1 = 1e-6, r10 = 1e3, c6 = 27e-12, r11 = 560e3, r12 = 1e6, r13 = 180, r14 = 180, r15 = 150, r16 = 100, c2 = 100e-6, r8 = 10e3;
  double r17 = 6.8e3, c7 = 100e-6, r18 = 22e3, r19 = 82, r20 = 1.8e3, c8 = 100e-6, r21 = 7.5e3, c5 = 0.22e-6, r9 = 2.2e6, c4 = 200e-12, r7 = 2.2e6, c3 = 200e-12, c27 = 0.22e-6;
  double dist = 0.5;   // R16 „DIST.“ 100 Ω (Trimmer): Mittelstellung wie in der Gyraf-Anleitung ohne Klirrfaktormesser
  // OUTPUT und Ausgangsverstärker
  double c9 = 47e-6, r27 = 100e3, c10 = 1e-6, r28 = 10e3, c11 = 47e-6, r29 = 1e6, r30b = 200e3, r31 = 68e3, r32 = 2e3, r33 = 11e3, c13 = 100e-6, r34 = 4.3e3;
  double r35 = 10e3, r36 = 100, c12 = 10e-12, r37 = 1e3, r38 = 1.2e3, r39 = 1.2e3, c14 = 22e-6, c29 = 47e-12, r40 = 4.3, r41 = 4.3, c15 = 470e-6;
  double r84 = 1e3, rload = 10e3;   // T2 ideal 1:1 (Annahme), R84 1 k und Last (Annahme 10 k) auf die Primärseite übertragen
  // Seitenkette
  double r26 = 56e3, r25 = 68e3, r24 = 56e3, r2 = 56e3, r23 = 47e3;
  double c17 = 1e-6, r60 = 1e6, r61 = 470e3, r62 = 47e3, r63 = 4.7e3, r64 = 2.4e3, r65 = 270, r66 = 182e3, r67 = 38.3e3, r69 = 44.2e3, r71 = 7.68e3, r72 = 2.4e3, r73 = 180, r68 = 47e3;
  double r70 = 4.7e3, c18 = 47e-6, c19 = 6.8e-6, c20 = 6.8e-6, r74 = 47e3, r75 = 47e3, c21 = 100e-6;
  double r76 = 470, c22 = 0.022e-6, r77 = 25e3, r78 = 5e6, r79 = 270e3;
  double r87 = 150, r46 = 10e6, r42 = 470, r43 = 560, r44 = 1.5e3, r45 = 2.2e3, r81 = 2e3, r82 = 10e3, r83 = 3.9e3;
  double qbias = 0.53813;   // R81 Schleifer (0 = Masse, 1 = Knoten R82/R83): abgeglichen auf 1,0 dB Dämpfung des FET bei Attack OFF (UA-Manual S. 26, Gyraf), Prüfprogramm u76test
  // Halbleiter (Annahmen nach Datenblatt, siehe SCHALTPLAN.md)
  Jfet q1 = {4.0e-3, -2.0, 0.02};    // BF245A: Idss 2–6,5 mA, Ugs(200 µA) −0,4…−2,2 V; Abschnürung ≈ −2 V (Gyraf)
  Bjt q2 = {1e-14, 350, 2, 1}, q5 = {1e-14, 250, 2, 1}, q6 = {1e-14, 400, 2, -1}, q8 = {1e-13, 100, 2, 1}, q9 = {1e-13, 100, 2, -1}, q12 = {1e-14, 250, 2, 1};   // 2N3391A, „5117“, 2N5087, MPSU05, MPSU55, 2N3707
  Diode d4003x2 = {14.1e-9, 2 * 1.984 * VT}, dfd = {2.52e-9, 1.752 * VT, 0.1e-9};   // CR1+CR2 1N4003 in Reihe (zwei gleiche Dioden: doppelte Steigungsspannung)
  // CR3/CR4 FD333: Durchlasskennlinie wie 1N4148 (Annahme); Sperrstrom 0,1 nA (Annahme; Datenblatt FDH333: höchstens 3 nA bei 125 V, „low leakage“ –
  // die 1N4148 darf 25 nA bei 20 V). Mit den 2,5 nA der 1N4148-Kennlinie fielen am RELEASE-Zweig bis 14 mV ab, die Ruhedämpfung des FET hinge vom RELEASE-Regler ab.
  bool oneGroup = false;   // Prüfzweck: alle Halbleiter gemeinsam lösen statt in Gruppen
  // Erweiterungen (eigener Entwurf)
  double hpf_c = 47e-9, hpf_lo = 20, hpf_hi = 600;   // Hochpass 2. Ordnung (Sallen-Key, Q 0,707) hinter einem Puffer, Grenzfrequenz logarithmisch 20…600 Hz
};
struct Setting {
  double input = 0.5, output = 0.5, attack = 0.5, release = 0.5;   // Knöpfe 0…1 (INPUT/OUTPUT: Schleiferstellung vor Kennlinie; ATTACK/RELEASE: 1 = schnell; ATTACK < 0 = OFF)
  int ratio = 0;           // 0 = 4, 1 = 8, 2 = 12, 3 = 20, 4 = alle Tasten, 5 = stufenlos (Cali76)
  double rvar = 0;         // stufenlos: 0 = 4:1 … 1/3 = 8:1 … 2/3 = 12:1 … 1 = 20:1
  bool hpf = false; double hpfF = 0;   // Hochpass in der Seitenkette (Cali76 Bass): 0…1 → 20…600 Hz
  int taper = 0;           // INPUT/OUTPUT: 0 log (15 % in der Mitte), 2 linear
};

enum { O_OUT = 0, O_PRE, O_GATE, O_HOLD, O_NO };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; Parts p; gk::Clamp clamp; int pFet = 0; };
inline double hpfFreq(const Parts &v, double x) { x = std::min(1.0, std::max(0.0, x)); return v.hpf_lo * std::pow(v.hpf_hi / v.hpf_lo, x); }
inline void buildInto(Built &B, const Parts &v, const Setting &s) {
  dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; B.p = v; B.clamp = {v.ic2_vs}; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { x = std::min(1.0, std::max(0.0, x)); o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };   // x = 1: Schleifer an a
  auto sw = [&](const std::string &a, const std::string &b, bool on) { o.r(a, b, on ? 1e-2 : 1e9); };
  auto bjt = [&](const std::string &b, const std::string &c, const std::string &e, const Bjt *m) { int p = o.port(b, e); o.port(b, c); dev.push_back({K_BJT, p, 2, m}); };
  auto diode = [&](const std::string &a, const std::string &k, const Diode *m) { int p = o.port(a, k); dev.push_back({K_DIODE, p, 1, m}); };
  auto tp = [&](double x) { return potTaper(std::min(1.0, std::max(0.0, x)), s.taper); };
  const Parts &P = B.p;
  // --- Netzteil ---
  o.src("V30", v.r30, 1); o.src("VN", v.rneg, 2);
  // --- Eingang IC2 (Wechselanteil; Ruhepunkt 15 V liegt vor C33) ---
  o.src("ii", 1.0, 0); o.r("ii", "if", v.ic2_r); o.c("if", "0", v.ic2_c);   // Eingang; Tiefpass der Gegenkopplung 10 k ∥ 220 p als RC davor (gleicher Pol)
  o.vccs("0", "io", "if", "io", 1e5 / v.ic2_rout); o.r("io", "0", v.ic2_rout); { int p = o.port("io", "0"); dev.push_back({K_CLAMP, p, 1, &B.clamp}); }
  o.c("io", "i5", v.c33); pot("i5", "i5w", "0", v.r5, tp(s.input)); o.r("i5w", "D", v.r6);
  mark(); G = 1;
  // --- FET-Abschwächer Q1 und Vorverstärker Q2–Q4 ---
  B.pFet = o.port("G1", "S1"); o.port("D", "S1"); dev.push_back({K_JFET, B.pFet, 2, &P.q1});
  o.r("S1", "0", v.r8); o.c("S1", "S13", v.c2);
  o.c("D", "c1", v.c1); o.r("c1", "B2", v.r10); o.c("B2", "0", v.c6); o.r("B2", "E3n", v.r11);
  o.r("V30", "P26", v.r17); o.c("P26", "0", v.c7);
  bjt("B2", "C2", "E2", &P.q2); o.r("P26", "C2", v.r12); o.r("E2", "S13", v.r13); o.r("S13", "0", v.r14);
  o.r("E2", "r15", v.r15); o.r("r15", "F", std::max(v.r16 * v.dist, 0.01));
  bjt("C2", "C3", "E3", &P.q2); o.r("P26", "C3", v.r18); o.r("E3", "E3n", v.r19); o.r("E3n", "0", v.r20); o.c("E3n", "0", v.c8);
  bjt("C3", "V30", "O4", &P.q2); o.r("O4", "F", v.r21);
  o.c("F", "H", v.c5); o.r("H", "G1", v.r9); o.c("H", "G1", v.c4); o.r("G1", "B", v.r7); o.c("G1", "B", v.c3); o.c("B", "0", v.c27);
  o.c("O4", "o15", v.c9); pot("o15", "w27", "0", v.r27, tp(s.output));
  mark(); G = 2;
  // --- Ausgangsverstärker Q5–Q9, C15, T2 (ideal 1:1, Last auf die Primärseite übertragen) ---
  o.c("w27", "B5", v.c10); o.r("V30", "N28", v.r28); o.c("N28", "0", v.c11); o.r("N28", "B5", v.r29); o.r("B5", "0", v.r30b);
  bjt("B5", "A", "E5", &P.q5); o.r("V30", "A", v.r31); o.r("E5", "0", v.r32); o.r("E5", "e33", v.r33); o.c("e33", "0", v.c13); o.r("OUT", "E5", v.r34);
  bjt("A", "D7", "Bq", &P.q6); o.r("V30", "Bq", v.r35); bjt("Bq", "D7", "E7", &P.q6); o.r("V30", "E7", v.r36);
  diode("D7", "E9", &P.d4003x2); o.r("D7", "E9", 10e6); o.c("A", "c12", v.c12); o.r("c12", "E9", v.r37);   // CR1+CR2 als ein Tor (gleicher Strom); 10 M nur numerisch (Knoten D7 sonst ohne Leitwert im linearen Netz), 0,13 µA; C12 + R37: Kompensation
  o.r("E9", "Fb", v.r38); o.r("Fb", "0", v.r39); o.c("OUT", "Fb", v.c14); o.c("E9", "0", v.c29);
  bjt("D7", "V30", "e8", &P.q8); o.r("e8", "OUT", v.r40); bjt("E9", "0", "e9", &P.q9); o.r("e9", "OUT", v.r41);
  o.c("OUT", "t1", v.c15); o.r("t1", "0", v.r84 * v.rload / (v.r84 + v.rload));
  mark(); G = 3;
  // --- Seitenkette: Ratio-Kette B, S9, (Hochpass), GR-Verstärker Q12–Q15 ---
  // Hochpass (Cali76 Bass, eigener Entwurf): Puffer am Ausgang des Vorverstärkers, Sallen-Key-Hochpass 2. Ordnung (gleiche C, R1 = 2·R2 für Q 0,707), Puffer;
  // er speist die Ratio-Kette B an Stelle von o15. So bleiben Kette und Belastung durch den GR-Verstärker wie im Plan (ein Puffer hinter dem Abgriff hob
  // den Pegel der Seitenkette um 2,4 dB). Das Filter ist immer im Netz (gleiche Kondensatorliste in jeder Schalterstellung, sonst gingen beim Umschalten
  // die Zustände verloren); der Schalter wählt nur, woher R26 gespeist wird.
  { double fc = hpfFreq(v, s.hpfF), R2 = 1 / (2 * PI * fc * v.hpf_c * std::sqrt(2.0)), R1 = 2 * R2;
    o.vccs("0", "hb", "o15", "hb", 1e5 / 50); o.r("hb", "0", 50);
    o.c("hb", "hm", v.hpf_c); o.c("hm", "hp", v.hpf_c); o.r("hm", "hq", R1); o.r("hp", "0", R2);
    o.vccs("0", "hq", "hp", "hq", 1e5 / 50); o.r("hq", "0", 50); }
  o.r(s.hpf ? "hq" : "o15", "Y", v.r26); o.r("V", "0", v.r23);
  // Ratio: Tasten S1–S4 (B: V, W, Z, Y; A: A0, A1, A2, A3); alle Tasten; stufenlos = Tandem-Abgriff, der über beide Ketten gleitet
  bool all = s.ratio == 4; const char *tb[4] = {"V", "W", "Z", "Y"}, *ta[4] = {"A0", "A1", "A2", "A3"};
  double rb[3] = {v.r2, v.r24, v.r25}, ra[3] = {v.r42, v.r43, v.r44};
  // Kette A: Q18 → R87 → A0 → R42 → A1 → R43 → A2 → R44 → A3 (V10) → R45 → −10 V; R46 10 M von A0 zum Abgriff AC
  o.r("Q18", "A0", v.r87); o.r("A0", "AC", v.r46); o.r("A3", "VN", v.r45);
  if (s.ratio == 5) {   // stufenlos: Kette B und A in drei Abschnitte geteilt, Abgriff im Abschnitt k bei Anteil f (beide Ketten gleich)
    double t = std::min(1.0, std::max(0.0, s.rvar)) * 3; int k = std::min(2, (int)t); double f = t - k;
    for (int j = 0; j < 3; j++) { std::string lo = tb[j], hi = tb[j + 1], alo = ta[j], ahi = ta[j + 1];
      if (j != k) { o.r(lo, hi, rb[j]); o.r(alo, ahi, ra[j]); continue; }
      o.r(lo, "TB", std::max(rb[j] * f, 0.01)); o.r("TB", hi, std::max(rb[j] * (1 - f), 0.01)); o.r(alo, "TA", std::max(ra[j] * f, 0.01)); o.r("TA", ahi, std::max(ra[j] * (1 - f), 0.01)); }
    sw("TB", "SC", true); sw("TA", "AC", true);
  } else {
    o.r("V", "W", v.r2); o.r("W", "Z", v.r24); o.r("Z", "Y", v.r25);
    o.r("A0", "A1", v.r42); o.r("A1", "A2", v.r43); o.r("A2", "A3", v.r44);
    for (int j = 0; j < 4; j++) { bool on = all || s.ratio == j; if (on) { sw(tb[j], "SC", true); sw(ta[j], "AC", true); } } }
  // S9 (auf R77): ATTACK ganz links = OFF → Eingang des GR-Verstärkers an Masse
  bool off = s.attack < 0;
  if (off) o.r("s22", "0", 1e-2); else o.r("SC", "s22", 1e-2);
  o.r("V30", "P28", v.r70); o.c("P28", "0", v.c18);
  o.c("s22", "B12", v.c17); o.r("K12", "B12", v.r60); o.r("B12", "0", v.r61);
  bjt("B12", "K12", "E12", &P.q12); o.r("P28", "K12", v.r62); o.r("E12", "J64", v.r63);
  bjt("K12", "V30", "E13", &P.q12); o.r("E13", "J64", v.r64); o.r("J64", "0", v.r65); o.r("E13", "B14", v.r66); o.r("B14", "PP", v.r67);
  bjt("B14", "K14", "E14", &P.q12); o.r("E14", "PP", v.r71); o.r("P28", "K14", v.r69);
  bjt("K14", "V30", "E15", &P.q12); o.r("E15", "PP", v.r72); o.r("PP", "0", v.r73); o.r("E15", "E12", v.r68);
  mark(); G = 4;
  // --- Gleichrichter, Schwelle, Attack/Release, „Q“-Bias ---
  o.c("E15", "r1", v.c19); o.c("E13", "r2", v.c20); o.r("r1", "AC", v.r74); o.r("r2", "AC", v.r75); o.c("AC", "0", v.c21);
  diode("r1", "K19", &P.dfd); diode("r2", "K19", &P.dfd);
  o.r("K19", "a1", v.r76); o.c("K19", "a1", v.c22); o.r("a1", "B", std::max(v.r77 * (1 - std::min(1.0, std::max(0.0, s.attack))), 1.0));   // ATTACK: rechts schnell (0 Ω)
  { double x = std::min(1.0, std::max(0.0, s.release)), top = v.r78 * (1 - x), bot = v.r78 * x;   // RELEASE: rechts schnell (Schleifer oben); R79 parallel zum unteren Teil
    o.r("B", "rw", std::max(top, 1.0)); o.r("rw", "Q18", std::max(bot, 1.0)); o.r("rw", "Q18", v.r79); }
  pot("K82", "Q18", "0", v.r81, v.qbias); o.r("VN", "K82", v.r82); o.r("K82", "0", v.r83);
  mark();
  o.out("t1"); o.out("O4"); o.out("G1", "S1"); o.out("B");
}
inline Built build(const Parts &v, const Setting &s) { Built B; buildInto(B, v, s); return B; }

struct Core {
  double fs = 96000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; dk::Compiler comp;
  std::vector<long> moff;   // je Gerät: Abstand des Modells in Parts (−1 = Klemme)
  void fixDev() { for (size_t k = 0; k < run.dev.size(); k++) run.dev[k].model = moff[k] < 0 ? (const void *)&bl.clamp : (const void *)((const char *)&bl.p + moff[k]); }   // nach Kopie
  void apply(const Setting &s) { cur = s; Built nb = build(v, s); bl.p = nb.p; bl.clamp = nb.clamp; bl.pFet = nb.pFet; net = std::move(nb.net);
    moff.clear(); for (auto &x : nb.dev) { const char *base = (const char *)&nb.p, *m = (const char *)x.model; moff.push_back(x.kind == K_CLAMP ? -1 : (long)(m - base)); }
    { std::vector<dk::Dev> old = std::move(run.dev); run.dev = nb.dev; dk::Runner::keepMem(run.dev, old); } fixDev(); run.devGrp = nb.grp; if (v.oneGroup) run.devGrp.assign(nb.grp.size(), 0); run.fn = devFn; run.set(comp.compile(net, fs)); }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); run.dampFull = true; apply(s);
    // Startwerte: Gleichspannungen aus dem Plan
    std::map<std::string, double> g = {{"V30", 30.3}, {"VN", -10}, {"P26", 25.8}, {"B2", 1.02}, {"C2", 1.76}, {"E2", 0.51}, {"C3", 11.98}, {"O4", 11.5}, {"F", 11.5 * 0.1}, {"E3", 1.2}, {"E3n", 1.1},
      {"N28", 27}, {"B5", 4.98}, {"E5", 4.54}, {"A", 28.6}, {"Bq", 29.1}, {"E7", 29.7}, {"D7", 14.06}, {"E9", 12.77}, {"Fb", 6.4}, {"e8", 13.47}, {"e9", 13.35}, {"OUT", 13.40},
      {"P28", 27.8}, {"K12", 15.1}, {"B12", 4.55}, {"E12", 4.03}, {"E13", 14.54}, {"J64", 1.5}, {"B14", 3.51}, {"K14", 17.25}, {"E15", 16.72}, {"PP", 1.17}, {"E14", 2.9}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  // Zustand aus Knotenspannungen setzen (Kondensatoren stromlos, Tore aus den Spannungen)
  void setVolts(const std::map<std::string, double> &g) { std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  // Einschwingen in zwei Schritten: erst mit niedriger Rate (langsame Kondensatoren bis 100 µ: mehrere Sekunden), dann auf der eigenen Rate
  // Ruhezustand: grob bei 6 kHz, dann bei Zielrate. Zwei Hilfen, damit 12 s reichen (der Gleichstrom-Arbeitspunkt hängt von beiden nicht ab):
  //  – C15 (470 µ an der Last) und C9 (47 µ am OUTPUT-Poti 100 k), beide τ ≈ 4,7 s, für die grobe Stufe auf 1/50; sonst blieben einige mV
  //    Gleichanteil am Ausgang. Nur diese beiden: kleinere Siebkondensatoren, ein kleineres C2 oder C5 ändern das Verhalten der Verstärker (der Zustand lag danach bis 1,4 V daneben).
  //  – Haltenetz (C27, C5 an Megaohm, τ ≈ 1,5 s): nach der halben Zeit (Einschaltvorgänge abgeklungen; vorher lädt der Gleichrichter es wieder um) auf seinen Ruhewert gesetzt = Spannung des „Q“-Bias-Knotens minus Sperrstrom der
  //    beiden Gleichrichterdioden mal RELEASE-Zweig (etwa 5 nA · 2,7 MΩ = 14 mV). Start bei 0 V ließe nach 12 s noch einige mV Rest, die sekundenlang
  //    nachlaufen; der FET arbeitet in Ruhe wenige mV neben der Abschnürung, 1 mV ändert die Ruhedämpfung merklich.
  void settleFast(double slow = 12, double fine = 0.3, const std::atomic<bool> *cancel = nullptr) { double f0 = fs; Parts pv = v; v.c15 /= 50; v.c9 /= 50;
    fs = 6000; apply(cur); settle(slow / 2, cancel); auto V = volts();
    { double x = std::min(1.0, std::max(0.0, cur.release)), top = std::max(v.r78 * (1 - x), 1.0), bot = std::max(v.r78 * x, 1.0), low = bot * v.r79 / (bot + v.r79), il = 2 * (v.dfd.ir >= 0 ? v.dfd.ir : v.dfd.is);
      for (const char *n : {"B", "G1", "H", "a1", "K19"}) V[n] = V["Q18"] - il * (top + low); V["rw"] = V["Q18"] - il * low; }
    setVolts(V); settle(slow / 2, cancel);
    V = volts(); v = pv; fs = f0; apply(cur); setVolts(V); settle(fine, cancel); }
  void step(double x, double *y) { double u[3] = {x, v.v30, v.vneg}; run.step(u, y); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); } }
  // Kleinsignal-Frequenzgang im aktuellen Arbeitspunkt: Eingang (1 V) → Knoten (Standard: Ausgang t1)
  std::complex<double> response(double f, const char *node = "t1") { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.idx.at(node), 0); }
  // Knotenspannungen (Prüfung)
  std::map<std::string, double> volts() { dk::Vec u = {0, v.v30, v.vneg}; dk::Vec V = dk::nodeVoltages(net, fs, run.h, u, run.ip); std::map<std::string, double> m; for (auto &kv : net.idx) m[kv.first] = V[kv.second]; return m; }
};

// Gain Reduction wie der Meter-FET Q10 (gleiche Gatespannung): Leitwert des FET bei Uds ≈ 0 gegen R6 + Innenwiderstand von INPUT,
// bezogen auf den Ruhewert (Nullabgleich „Zero Set“ ohne Signal)
inline double fetG(const Jfet &F, double vgs) { double ov = vgs - F.vp; return ov > 0 ? 2 * F.idss / (F.vp * F.vp) * ov : 0; }
// Ein Gerät mit allen Einstellungen; Stufe 2 (Stacked) ist ein zweites, gleiches Gerät hinter dem ersten
struct Amp {
  double fsB = 48000; int cfg = 1; Parts v; Core c[2], c0[2]; Setting tgt[2]; bool ready = false; int cnt = 0;   // cfg: Kennung des Aufbaus (Plugin)
  bool stack = false; double vin = 1, dry = 0, dryCur = 0, stk = 0;   // vin: Volt am Eingang je digitaler Einheit; dry: Anteil des Eingangs am Ausgang (0…1); stk: Stufe 2 überblendet
  double g0[2] = {0, 0}, grPk[2] = {0, 0}, outPk = 0, outSq = 0; long outN = 0; int bad = 0;
  void refZero(int k) { g0[k] = fetG(c[k].bl.p.q1, c[k].run.vp[c[k].bl.pFet]); }
  void init(double fs, const Parts &pv, const Setting &s1, const Setting &s2, const std::atomic<bool> *cancel = nullptr, double slow = 12, double fine = 0.3) { fsB = fs; v = pv; tgt[0] = s1; tgt[1] = s2;
    for (int k = 0; k < 2; k++) { c[k].init(fs, v, tgt[k]); c[k].settleFast(slow, fine, cancel); c0[k] = c[k]; c0[k].fixDev(); refZero(k); }
    ready = true; }
  Amp() {} Amp(const Amp &o) { *this = o; }
  Amp &operator=(const Amp &o) { fsB = o.fsB; v = o.v; for (int k = 0; k < 2; k++) { c[k] = o.c[k]; c[k].fixDev(); c0[k] = o.c0[k]; c0[k].fixDev(); tgt[k] = o.tgt[k]; g0[k] = o.g0[k]; grPk[k] = o.grPk[k]; }
    ready = o.ready; cnt = o.cnt; cfg = o.cfg; stack = o.stack; vin = o.vin; dry = o.dry; dryCur = o.dryCur; stk = o.stk; outPk = o.outPk; outSq = o.outSq; outN = o.outN; bad = o.bad; return *this; }
  static bool sameSw(const Setting &a, const Setting &b) { return a.ratio == b.ratio && a.hpf == b.hpf && a.taper == b.taper && (a.attack < 0) == (b.attack < 0); }
  static bool sameAll(const Setting &a, const Setting &b) { return sameSw(a, b) && a.input == b.input && a.output == b.output && a.attack == b.attack && a.release == b.release && a.rvar == b.rvar && a.hpfF == b.hpfF; }
  // Schalter sofort (Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void setTarget(const Setting &s1, const Setting &s2) { const Setting *s[2] = {&s1, &s2};
    for (int k = 0; k < 2; k++) { tgt[k] = *s[k]; if (!sameSw(tgt[k], c[k].cur)) { Setting n = c[k].cur; n.ratio = tgt[k].ratio; n.hpf = tgt[k].hpf; n.taper = tgt[k].taper;
        if ((tgt[k].attack < 0) != (n.attack < 0)) n.attack = tgt[k].attack; c[k].apply(n); } } }
  void snap() { for (int k = 0; k < 2; k++) c[k].apply(tgt[k]); cnt = 0; dryCur = dry; stk = stack ? 1 : 0; }
  void resettle(double fine, const std::atomic<bool> *cancel = nullptr) { snap(); for (int k = 0; k < 2; k++) { c[k].settleFast(12, fine, cancel); c0[k] = c[k]; c0[k].fixDev(); refZero(k); } }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB));
    for (int k = 0; k < 2; k++) { Setting n = c[k].cur; bool moved = false;
      double *f[6] = {&n.input, &n.output, &n.attack, &n.release, &n.rvar, &n.hpfF}; const double *t[6] = {&tgt[k].input, &tgt[k].output, &tgt[k].attack, &tgt[k].release, &tgt[k].rvar, &tgt[k].hpfF};
      for (int i = 0; i < 6; i++) { double d = *t[i] - *f[i]; if (d == 0) continue; if (i == 2 && (*t[i] < 0 || *f[i] < 0)) { *f[i] = *t[i]; moved = true; continue; }   // ATTACK OFF ist ein Schalter (setTarget)
        moved = true; if (std::fabs(d) > 1e-4) *f[i] += d * a; else *f[i] = *t[i]; }
      if (moved) { bool sw = !sameSw(n, c[k].cur); c[k].apply(n); if (sw) refZero(k); } } }
  double stage(int k, double x) { double y[O_NO]; c[k].step(x, y); double g = fetG(c[k].bl.p.q1, c[k].run.vp[c[k].bl.pFet]); double gr = 20 * std::log10((1 + v.r6 * g) / (1 + v.r6 * g0[k]));
    if (gr > grPk[k]) grPk[k] = gr; return y[O_OUT]; }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    const double st = 1.0 / (0.02 * fsB), kd = 1 - std::exp(-1.0 / (0.02 * fsB));
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-30.0, std::min(30.0, buf[i] * vin)) : 0; tick();
      double y = stage(0, x);
      double tS = stack ? 1 : 0; if (stk < tS) stk = std::min(tS, stk + st); else if (stk > tS) stk = std::max(tS, stk - st);
      if (stk > 0) { double y2 = stage(1, y); double w = stk * stk * (3 - 2 * stk); y += w * (y2 - y); }   // Stufe 2 zu-/abschalten: 20 ms Smoothstep
      dryCur += (dry - dryCur) * kd; y += dryCur * x;
      if (!(std::fabs(y) < 300)) { y = 0;   // entgleist (NaN oder unmögliche Spannung): eingeschwungenen Zustand zurückholen, höchstens einmal je 0,1 s
        if (bad <= 0) { for (int k = 0; k < 2; k++) { Setting s = c[k].cur; c[k] = c0[k]; c[k].fixDev(); c[k].apply(s); } bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      if (std::fabs(y) > outPk) outPk = std::fabs(y); outSq += y * y; outN++;
      buf[i] = (float)(y / vin); } }
  // Anzeigen seit der letzten Abfrage: GR je Stufe (dB, Spitze), Ausgang (Volt Spitze, Volt eff.)
  void takeGR(double *g) { for (int k = 0; k < 2; k++) { g[k] = grPk[k]; grPk[k] = 0; } }
  void takeOut(double *pk, double *rms) { *pk = outPk; *rms = outN ? std::sqrt(outSq / outN) : 0; outPk = 0; outSq = 0; outN = 0; }
};
} // namespace u76
