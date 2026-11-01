// AmpFett B-15NF: Nachbau des Ampeg B-15NF (Portaflex, Werksschaltplan 1-66, Technical Data 1968): ganze Schaltung im Zeitbereich,
// vom Eingang über beide Kanäle, Phasenumkehr, Gegentakt-Endstufe mit fester Gittervorspannung, Ausgangsübertrager
// und Gegenkopplung bis zum Lautsprecher, mit Netzteil (Gleichrichter 5AR4 am Netztrafo, Siebkette, Vorspannungs-Netzteil).
// Bauteilwerte und Knoten: docs/b15nf/SCHALTPLAN.md. Geschätzt (nicht im Plan): Übertrager-Induktivitäten, Trafo-Innenwiderstand,
// Lautsprecher-Daten; Röhrenmodelle an Datenblätter angepasst (engine/tools/b15_tubes.py).
#pragma once
#include "dk.hpp"
#include <cmath>
#include <string>
#include <vector>
namespace b15 {
constexpr double PI = 3.14159265358979323846;

// ---------- Röhren- und Diodenmodelle ----------
struct Triode { double mu, ex, kg1, kp, kvb; };
struct Pentode { double mu, ex, kg1, kg2, kp, kvb, c2; };
// 6SL7GT: Koren-Form, mu 70 und ex 1,4 fest, kg1/kp an RCA-Datenblatt (250 V, −2 V: 2,3 mA, 1,6 mA/V) angepasst
static const Triode T6SL7 = {70, 1.4, 1851.8, 281.0, 300};
// 6L6GC: Koren-Pentode, an sieben Ruhepunkte und S/ri des Datenblatts angepasst; Schirmgitterstrom-Aufteilung geschätzt (c2)
static const Pentode P6L6 = {8.753, 1.358, 1499.6, 26600, 38.93, 54.50, 1.0};
// Gitterstrom nach Dempwolf/Zölzer (2011), wie im Laney-Kern
inline void gridI(double v, double &i, double &d) { const double GG = 6.177e-4, CG = 9.901, XI = 1.314; double x = CG * v;
  if (x < -30) { i = 0; d = 0; return; }
  double sp, sg; if (x > 35) { sp = x; sg = 1; } else { double e = std::exp(x); sp = std::log1p(e); sg = e / (1 + e); }
  double s = sp / CG, p = std::pow(s, XI - 1); i = GG * p * s; d = GG * XI * p * sg; }
// Triode: Tore (g−k, p−k), Ströme (Gitter, Anode), Ableitungen 2×2
inline void triode(const Triode &T, const double *v, double *i, double *J) {
  double vg = v[0], vp = v[1], ig, dg; gridI(vg, ig, dg); i[0] = ig; J[0] = dg; J[1] = 0;
  if (vp <= 1e-6) { i[1] = 0; J[2] = 0; J[3] = 0; return; }
  double s = std::sqrt(T.kvb + vp * vp), q = T.kp * (1 / T.mu + vg / s), sp, sig;
  if (q > 35) { sp = q; sig = 1; } else if (q < -40) { i[1] = 0; J[2] = J[3] = 0; return; } else { double e = std::exp(q); sp = std::log1p(e); sig = e / (1 + e); }
  double e1 = vp / T.kp * sp, p = std::exp((T.ex - 1) * std::log(e1)), I = 2 * p * e1 / T.kg1, dI = 2 * T.ex * p / T.kg1;
  i[1] = I; J[2] = dI * vp * sig / s; J[3] = dI * (sp / T.kp - sig * vg * vp * vp / (s * s * s)); }
// Pentode: Tore (g1−k, g2−k, a−k), Ströme (g1, g2, a), 3×3
inline void pentode(const Pentode &P, const double *v, double *i, double *J) {
  for (int k = 0; k < 9; k++) J[k] = 0;
  double g1 = v[0], g2 = std::max(v[1], 1.0), va = v[2], ig, dg; gridI(g1, ig, dg); i[0] = ig; J[0] = dg;
  double q = P.kp * (1 / P.mu + g1 / g2), sp, sig;
  if (q < -40) { i[1] = i[2] = 0; return; }
  if (q > 35) { sp = q; sig = 1; } else { double e = std::exp(q); sp = std::log1p(e); sig = e / (1 + e); }
  double e1 = g2 / P.kp * sp, pw = std::exp(P.ex * std::log(e1)), dpw = P.ex * pw / e1;   // E1^ex und Ableitung nach E1
  double de1g1 = sig, de1g2 = sp / P.kp - sig * g1 / g2;                                   // ∂E1/∂g1, ∂E1/∂g2
  double vap = std::max(va, 0.0), at = std::atan(vap / P.kvb), dat = va > 0 ? (1 / P.kvb) / (1 + (vap / P.kvb) * (vap / P.kvb)) : 0;
  double ia = 2 * pw / P.kg1 * at; i[2] = ia;
  J[6] = 2 * dpw * de1g1 / P.kg1 * at; J[7] = 2 * dpw * de1g2 / P.kg1 * at; J[8] = 2 * pw / P.kg1 * dat;
  double f2 = 1 + P.c2 * (PI / 2 - at), ig2 = 2 * pw / P.kg2 * f2; i[1] = ig2;              // Schirmgitter: steigt, wenn die Anode tief schwingt
  J[3] = 2 * dpw * de1g1 / P.kg2 * f2; J[4] = 2 * dpw * de1g2 / P.kg2 * f2; J[5] = -2 * pw / P.kg2 * P.c2 * dat; }
// 5AR4/GZ34 (Vakuumdiode): i = k·v^1,5 (17 V bei 225 mA je Anode), sanfter Einsatz unter 0,5 V
inline void vdiode(const double *v, double *i, double *J) { const double k = 3.21e-3, w = 0.5;
  double x = v[0] / w, sp, sg; if (x > 35) { sp = x; sg = 1; } else if (x < -40) { i[0] = 0; J[0] = 0; return; } else { double e = std::exp(x); sp = std::log1p(e); sg = e / (1 + e); }
  double u = w * sp, r = std::sqrt(u); i[0] = k * u * r; J[0] = 1.5 * k * r * sg; }
// 10D6 (Silizium, 600 V, 1 A): Shockley
inline void sdiode(const double *v, double *i, double *J) { const double Is = 1e-9, nVt = 1.6 * 0.02585; double x = v[0] / nVt;
  if (x > 40) { double e = std::exp(40.0); i[0] = Is * (e * (1 + x - 40) - 1); J[0] = Is * e / nVt; return; }
  double e = std::exp(x); i[0] = Is * (e - 1); J[0] = Is * e / nVt; }

enum Kind { K_6SL7 = 0, K_6L6 = 1, K_RECT = 2, K_SI = 3 };
// 6SL7 mit der Formel statt einer Tabelle: eine bilinear interpolierte Tabelle erzeugte bei kleinem Signal unechte Oberwellen (H4/H5 um −60 dB)
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) { case K_6SL7: triode(T6SL7, v, i, J); break; case K_6L6: pentode(P6L6, v, i, J); break; case K_RECT: vdiode(v, i, J); break; default: sdiode(v, i, J); } }

// ---------- Bauteile und Einstellungen ----------
struct Parts {   // Werte laut Schaltplan 1-66 (Ω, F); geschätzt markiert
  double rin = 120e3, rpad = 100e3, rg1 = 5.6e6, rg2 = 2.2e6, rk1 = 5.6e3, ra1 = 470e3, cc1 = 0.022e-6, vol = 1e6, rk2 = 2.2e3, ra2 = 220e3, cc2 = 0.022e-6;
  double ts_r1 = 220e3, ts_bass = 1e6, ts_r2 = 22e3, ts_c1 = 0.001e-6, ts_c2 = 0.01e-6, ts_r3 = 270e3, ts_r4 = 120e3, ts_c3 = 0.00047e-6, ts_treb = 1e6, ts_c4 = 0.0047e-6;
  double treb1 = 1e6;   // Kanal 1 Treble: 1 M wie Kanal 2 und Technical Data (Stand 1-66); in 4-65/9-65 als „4 M“ eingezeichnet
  double rmix = 270e3, pi_ra = 120e3, pi_rk = 220, pi_rt = 1e3, pi_ct = 25e-6, pi_rf1 = 470e3, pi_rf2 = 510e3, pi_cf = 0.022e-6, pi_rg = 470e3, rnfb = 10e3;
  double cc3 = 0.022e-6, rgl = 270e3, rgs = 1e3;
  double c_b1 = 30e-6, r_b12 = 1e3, c_b2 = 40e-6, r_b23 = 22e3, c_b3 = 40e-6, r_b24 = 22e3, c_b4 = 40e-6;
  double bias_r1 = 100e3, bias_r2 = 56e3, bias_r3 = 47e3, bias_c = 100e-6;
  // Übertrager OT-214: 6,5 kΩ : 8 Ω / 16 Ω, Gleichstromwiderstand primär 290 Ω, sekundär 1 Ω (8 Ω) / 1,4 Ω (16 Ω) – Technical Data
  double ot_zp = 6500, ot_rp = 290, ot_rs8 = 1.0, ot_rs16 = 1.4, ot_lp = 40, ot_kps = 0.9998, ot_kpp = 0.9995;   // Lp, Kopplungen: geschätzt
  double ot_cpp = 500e-12, ot_rc = 150e3;   // geschätzt: Wicklungskapazität der Primärseite (Anode–Anode) und Eisenverluste; dämpfen das Umschalten der Endröhren wie im echten Übertrager
  // Netztrafo PT-108: 750 V Mittelanzapfung, 160 mA; Innenwiderstand je Hälfte geschätzt (an 430 V im Ruhezustand angepasst)
  double hv_rms = 375, hv_r = 380, mains_hz = 60;
  // Lautsprecher CTS 15" 8 Ω im geschlossenen Portaflex-Gehäuse: alles geschätzt
  double spk_r2 = 20;   // geschätzt: Wirbelstromverluste der Schwingspule (20 Ω parallel zu Le, Impedanz bei 10 kHz etwa 25 Ω statt 64 Ω)
  double spk_re = 6.3, spk_le = 1.0e-3, spk_fs = 40, spk_qms = 5, spk_qes = 0.55, spk_vas = 280, spk_vb = 85;
  double rsrc = 1e3;    // Innenwiderstand der Quelle (Instrument/DI), geschätzt
};
// Buchsen: 0 = Kanal 1 "Bass" (ohne Teiler), 1 = Kanal 1 "Guitar" (mit 120 k + 100 k der leeren Bass-Buchse), 2 = Kanal 2
struct Setting { double vol1 = 0.5, bass1 = 0.5, treb1 = 0.5, vol2 = 0.5, bass2 = 0.5, treb2 = 0.5; int jack = 0; bool tap16 = false; };   // Regler 0…1 (Knopf 0…10)
inline double &sfield(Setting &s, int i) { double *f[] = {&s.vol1, &s.treb1, &s.bass1, &s.vol2, &s.treb2, &s.bass2}; return *f[i]; }
inline double logTaper(double x) { const double m = 0.15; if (x <= 0) return 0; if (x >= 1) return 1; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // 15 % in Mittelstellung

// Ausgänge der Netzliste
enum { O_SPK = 0, O_MOT = 1, O_B1 = 2, O_BIAS = 3, O_NO = 4 };
struct Built { int lastTri = 0; dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; int mon[5] = {0, 0, 0, 0, 0}; int capSpk = -1; double cmes = 0; };   // mon: Tore g−k von V1A, V1B, V3A, V5, V4 (aktiver Kanal); capSpk: Membran-Kondensator   // grp: Lösungsgruppe je Gerät (0/1 Vorstufen, 2 Phasenumkehr+Endstufe, 3 Netzteil)
inline Built build(const Parts &v, const Setting &s) {
  Built B; dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k) { int a = o.port(g, k); o.port(p, k); dev.push_back({K_6SL7, a, 2, nullptr}); B.lastTri = a;
    o.c(g, p, 2.8e-12); };   // 6SL7: Cgp 2,8 pF (RCA; wirkt als Miller-Kapazität). Cgk 3,0 pF und Cpk 3,8 pF weggelassen: Grenzfrequenzen über 80 kHz
  auto pen = [&](const std::string &g1, const std::string &g2, const std::string &a, const std::string &k) { int p0 = o.port(g1, k); o.port(g2, k); o.port(a, k); dev.push_back({K_6L6, p0, 3, nullptr});
    };   // 6L6GC: Kapazitäten (Eingang 10 pF hinter 1 kΩ, g1−a 0,6 pF, Ausgang 6,5 pF) wirken erst weit über 100 kHz, weggelassen
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };
  // --- Eingänge ---
  if (s.jack == 2) { o.src("g2a", v.rsrc, 0); o.r("g1a", "j1", v.rin); o.r("j1", "0", v.rpad); }   // Kanal 2; Kanal 1 leer: geschlossene Bass-Buchse 120 k + 100 k
  else { o.src("in1", v.rsrc, 0); o.r("in1", "g1a", v.rin); if (s.jack == 1) { o.r("g1a", "j1", v.rin); o.r("j1", "0", v.rpad); }
    o.r("g2a", "0", 1.0); }                                                                            // Kanal 2 leer: geschlossene Buchse legt das Gitter an Masse
  // --- Vorstufen beider Kanäle ---
  for (int ch = 1; ch <= 2; ch++) { std::string c = std::to_string(ch);
    o.r("g" + c + "a", "0", ch == 1 ? v.rg1 : v.rg2);
    tri("g" + c + "a", "p" + c + "a", "k" + c + "a"); if ((ch == 2) == (s.jack == 2)) B.mon[0] = B.lastTri; o.r("k" + c + "a", "0", v.rk1); o.r("p" + c + "a", "B4", v.ra1);
    o.c("p" + c + "a", "vt" + c, v.cc1); pot("vt" + c, "g" + c + "b", "0", v.vol, logTaper(ch == 1 ? s.vol1 : s.vol2));
    tri("g" + c + "b", "p" + c + "b", "k" + c + "b"); if ((ch == 2) == (s.jack == 2)) B.mon[1] = B.lastTri; o.r("k" + c + "b", "0", v.rk2); o.r("p" + c + "b", "B4", v.ra2);
    o.c("p" + c + "b", "I" + c, v.cc2);
    // Klangregler (Baxandall): Bass 1 M linear mit .001/.01 zum Schleifer, Treble 1 M linear mit .00047/.0047
    // Rechtsanschlag (Knopf 10) = Schleifer am eingangsseitigen Ende (Bass: 220 k/.001, Treble: .00047) = Anhebung
    double xb = ch == 1 ? s.bass1 : s.bass2, xt = ch == 1 ? s.treb1 : s.treb2;
    o.r("I" + c, "P" + c, v.ts_r1); pot("P" + c, "W" + c, "Q" + c, v.ts_bass, xb); o.r("Q" + c, "0", v.ts_r2);
    o.c("P" + c, "W" + c, v.ts_c1); o.c("Q" + c, "W" + c, v.ts_c2); o.r("W" + c, "0", v.ts_r3); o.r("W" + c, "Y" + c, v.ts_r4);
    o.c("I" + c, "T" + c, v.ts_c3); pot("T" + c, "Y" + c, "U" + c, ch == 1 ? v.treb1 : v.ts_treb, xt); o.c("U" + c, "0", v.ts_c4);
    o.r("Y" + c, "mix", v.rmix); G = ch - 1; mark(); }
  G = 2;
  // --- Phasenumkehr V3 (6SL7, schwebende Paraphase) ---
  tri("mix", "pA", "kA"); B.mon[2] = B.lastTri; o.r("pA", "B3", v.pi_ra); o.r("kA", "N", v.pi_rk);
  tri("gB", "pB", "N"); o.r("pB", "B3", v.pi_ra); o.r("N", "0", v.pi_rt); o.c("N", "0", v.pi_ct);   // 1 k über F (Sekundärwicklung, über den Lautsprecherstecker an Masse)
  o.r("pA", "J", v.pi_rf1); o.r("J", "pB", v.pi_rf2); o.c("J", "gB", v.pi_cf); o.r("gB", "0", v.pi_rg);
  o.r(s.tap16 ? "s8" : "s16", "kA", v.rnfb);   // Gegenkopplung 10 k vom 16-Ω-Abgriff an die Kathode von V3A
  // --- Endstufe V4/V5 (6L6GC), feste Vorspannung über 270 k ---
  o.c("pA", "c5", v.cc3); o.r("c5", "BIAS", v.rgl); o.r("c5", "g5", v.rgs);
  o.c("pB", "c4", v.cc3); o.r("c4", "BIAS", v.rgl); o.r("c4", "g4", v.rgs);
  B.mon[3] = (int)o.P.size(); pen("g5", "B2", "a5", "0"); B.mon[4] = (int)o.P.size(); pen("g4", "B2", "a4", "0"); mark(); G = 3;
  // --- Ausgangsübertrager: zwei Primärhälften, Sekundär 0–8 Ω–16 Ω; Gleichstromwiderstände als Reihenwiderstände ---
  { double np = 1, n8 = std::sqrt(8 / v.ot_zp), n16 = std::sqrt(16 / v.ot_zp);   // Windungen relativ zur ganzen Primärwicklung
    double n[4] = {np / 2, np / 2, n8, n16 - n8}, L0 = v.ot_lp; dk::Mat L(4, 4);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? v.ot_kpp : v.ot_kps); if (i >= 2 && j >= 2 && i != j) k = 0.99999; L(i, j) = k * n[i] * n[j] * L0; }
    o.r("a5", "a5w", v.ot_rp / 2); o.r("a4", "a4w", v.ot_rp / 2); o.r("s8w", "s8", v.ot_rs8); o.r("s16w", "s16", v.ot_rs16 - v.ot_rs8);
    o.coupled({{"a5w", "B1"}, {"B1", "a4w"}, {"s8w", "0"}, {"s16w", "s8"}}, L);
    o.r("s16", "0", 1e6); o.c("a5w", "a4w", v.ot_cpp); o.r("a5w", "a4w", v.ot_rc); }
  // --- Lautsprecher (geschätzt): Re, Le, Bewegung als Parallelkreis (geschlossenes Gehäuse) ---
  { std::string sp = s.tap16 ? "s16" : "s8"; double a = v.spk_vas / v.spk_vb, fc = v.spk_fs * std::sqrt(1 + a), qec = v.spk_qes * std::sqrt(1 + a), qmc = v.spk_qms * std::sqrt(1 + a);
    double res = v.spk_re * qmc / qec, cmes = qec / (2 * PI * fc * v.spk_re), lces = 1 / ((2 * PI * fc) * (2 * PI * fc) * cmes);
    o.r(sp, "sl", v.spk_re); dk::Mat Le(1, 1); Le(0, 0) = v.spk_le; o.coupled({{"sl", "sm"}}, Le); o.r("sl", "sm", v.spk_r2);
    o.r("sm", "0", res); B.capSpk = (int)o.C.size(); B.cmes = cmes; o.c("sm", "0", cmes); dk::Mat Lm(1, 1); Lm(0, 0) = lces; o.coupled({{"sm", "0"}}, Lm); }
  // --- Netzteil: Hochspannungswicklung (Mittelanzapfung an Masse), 5AR4, Siebung ---
  o.src("hva", v.hv_r, 1); o.src("hvb", v.hv_r, 2);
  { int p = o.port("hva", "B1"); dev.push_back({K_RECT, p, 1, nullptr}); p = o.port("hvb", "B1"); dev.push_back({K_RECT, p, 1, nullptr}); }
  o.c("B1", "0", v.c_b1); o.r("B1", "B2", v.r_b12); o.c("B2", "0", v.c_b2); o.r("B2", "B3", v.r_b23); o.c("B3", "0", v.c_b3); o.r("B2", "B4", v.r_b24); o.c("B4", "0", v.c_b4);
  // Vorspannung: von einem Ende der HV-Wicklung über 100 k, 56 k an Masse, 10D6 (Anode am Vorspannungsknoten), 47 k und 100 µF
  // eigener Eingang für die Vorspannung: bleibt bei Standby an (Standby trennt nur die Mittelanzapfung)
  o.src("br", v.bias_r1 + v.hv_r, 3); o.r("br", "0", v.bias_r2); { int p = o.port("BIAS", "br"); dev.push_back({K_SI, p, 1, nullptr}); }
  o.r("BIAS", "0", v.bias_r3); o.c("BIAS", "0", v.bias_c);
  mark();
  o.out(s.tap16 ? "s16" : "s8"); o.out("sm"); o.out("B1"); o.out("BIAS");
  return B;
}

// ---------- Laufzeit ----------
struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; double t = 0, w = 0, hvOn = 1;
  void apply(const Setting &s) { cur = s; bl = build(v, s); net = std::move(bl.net); run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn; run.capOut = {bl.capSpk}; run.set(dk::compile(net, fs)); }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s); w = 2 * PI * v.mains_hz / fs; t = 0;
    // Startwerte: Kondensatoren auf die Spannungen der Spannungstabelle (Technical Data), damit das Netzteil nicht minutenlang einschwingen muss
    std::map<std::string, double> g = {{"B1", 430}, {"B2", 425}, {"B3", 345}, {"B4", 350}, {"BIAS", -50}, {"br", -60}, {"p1a", 165}, {"p2a", 165}, {"p1b", 175}, {"p2b", 175},
      {"k1a", 2.2}, {"k2a", 2.2}, {"k1b", 2}, {"k2b", 2}, {"pA", 235}, {"pB", 225}, {"kA", 2.4}, {"N", 2.2}, {"J", 230}, {"c5", -50}, {"c4", -50}, {"g5", -50}, {"g4", -50}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  // Abtastwert: x = Eingangsspannung (V); y[O_NO] Ausgänge
  void step(double x, double *y) { double m = std::sqrt(2.0) * v.hv_rms * std::sin(t), u[4] = {x, m * hvOn, -m * hvOn, m}; t += w; if (t > 2 * PI) t -= 2 * PI; run.step(u, y); }
  void settle(double seconds) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) step(0, y); }
  // Kleinsignal-Frequenzgang Eingang → Lautsprecherklemme im aktuellen Arbeitspunkt
  std::complex<double> response(double f) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    int o = spkAc ? net.idx.at("sm") : net.idx.at(cur.tap16 ? "s16" : "s8"); std::complex<double> r = dk::acResponse(net, J, dp, f, 0, o, 0);
    if (spkAc) { std::complex<double> jw(0, 2 * PI * f), wc = 2 * PI * 3000; r *= jw * bl.cmes * spkNorm * (wc * wc / (wc * wc + jw * wc / 0.7 + jw * jw)); }   // Membranstrom · Normierung · Kegel-Tiefpass
    return r; }
  bool spkAc = false; double spkNorm = 1;
};
// Lautsprecher-Schalldruck: Strom durch die Membran-Kapazität ~ Beschleunigung; Normierung so, dass bei 1 kHz Schall- und Klemmenpegel gleich sind
inline double accNorm(const Parts &v) { std::complex<double> j(0, 1); double a = v.spk_vas / v.spk_vb, fc = v.spk_fs * std::sqrt(1 + a), qec = v.spk_qes * std::sqrt(1 + a), qmc = v.spk_qms * std::sqrt(1 + a);
  double res = v.spk_re * qmc / qec, cmes = qec / (2 * PI * fc * v.spk_re), lces = 1 / ((2 * PI * fc) * (2 * PI * fc) * cmes), w = 2 * PI * 1000;
  std::complex<double> zm = 1.0 / (1.0 / res + 1.0 / (j * w * lces) + j * w * cmes); std::complex<double> zl = j * w * v.spk_le * v.spk_r2 / (j * w * v.spk_le + v.spk_r2); return std::abs((v.spk_re + zl + zm) / (zm * j * w * cmes)); }
struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void lp(double fc, double q, double fs) { double w0 = 2 * PI * fc / fs, c = std::cos(w0), al = std::sin(w0) / (2 * q), a0 = 1 + al; b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0; }
  double step(double x) { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } void reset() { z1 = z2 = 0; } };
// Verstärker mit Überabtastung, Reglerglättung, Schaltern und Anzeigen. Ausgabe: Lautsprecherklemme bzw. Schalldruck, 1,0 = V_FS Volt
constexpr double V_FS = 24.0;   // 30 W an 8 Ω = 21,9 V Spitze
struct Amp {
  double fsB = 48000; int os = 1; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 0.1; bool spk = false, power = true, standby = false; double spkMix = -1, pwr = 1;
  double nrm = 1; Biquad cone; double vgk0[5] = {-1, -1, -1, -1, -1}, peak[5] = {0, 0, 0, 0, 0}, outPk = 0;
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  Core c0; int bad = 0;   // c0: eingeschwungener Zustand nach init (Rückfall bei entgleister Rechnung); bad: Abtastwerte bis zum nächsten erlaubten Rückfall
  void init(double fs, int os_, const Parts &pv, const Setting &s) { fsB = fs; os = std::max(1, os_); v = pv; tgt = s;
    c.init(fsB * os, v, s); c.settle(1.5); c0 = c; for (int i = 0; i < 5; i++) vgk0[i] = c.run.vp[c.bl.mon[i]];
    nrm = accNorm(v); c.spkNorm = nrm; cone.lp(3000, 0.7, fsB * os); spkMix = spk ? 1 : 0; pwr = power ? 1 : 0;
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  // Schalter sofort (Buchse baut das Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void setTarget(const Setting &s) { bool rebuild = s.jack != c.cur.jack || s.tap16 != c.cur.tap16; tgt = s;
    if (rebuild) { Setting n = c.cur; n.jack = s.jack; n.tap16 = s.tap16; c.apply(n); for (int i = 0; i < 5; i++) vgk0[i] = c.run.vp[c.bl.mon[i]]; } }
  // alle 64 Abtastwerte (Grundrate) einen Schritt Richtung Ziel; Netz neu aufbauen kostet etwa 0,1 ms
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < 6; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO]; c.hvOn = standby ? 0 : 1; c.step(x, y);
    for (int i = 0; i < 5; i++) { double d = std::fabs(c.run.vp[c.bl.mon[i]] - vgk0[i]); if (d > peak[i]) peak[i] = d; }
    double el = y[O_SPK], ac = cone.step(c.run.capI[0] * nrm), st = 1.0 / (0.02 * c.fs);
    double tS = spk ? 1 : 0; if (spkMix < tS) spkMix = std::min(tS, spkMix + st); else if (spkMix > tS) spkMix = std::max(tS, spkMix - st);
    double tP = power ? 1 : 0; if (pwr < tP) pwr = std::min(tP, pwr + st); else if (pwr > tP) pwr = std::max(tP, pwr - st);
    double o = (el + spkMix * (ac - el)) * pwr; if (std::fabs(el) > outPk) outPk = std::fabs(el); return o / V_FS; }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();   // Eingang auf ±20 V begrenzt (Unsinnswerte; ein Bass liefert höchstens wenige Volt)
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 10)) { y = 0;   // entgleist (NaN oder über 240 V am Lautsprecher, physikalisch unmöglich): eingeschwungenen Zustand zurückholen (schnell, kein neues Einschwingen), höchstens einmal je 0,1 s
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); for (int k = 0; k < 5; k++) vgk0[k] = c.run.vp[c.bl.mon[k]]; std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); cone.reset(); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung je Stufe seit der letzten Abfrage: Gitterhub bezogen auf die Gittervorspannung (1 = Gitter erreicht 0 V)
  void takePeaks(double *out) { for (int i = 0; i < 5; i++) { out[i] = peak[i] / std::max(0.1, std::fabs(vgk0[i])); peak[i] = 0; } }
  double takeOut() { double p = outPk; outPk = 0; return p; }   // Spitzenspannung am Lautsprecher (V)
};
} // namespace b15
