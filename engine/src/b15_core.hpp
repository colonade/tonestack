// Ampeg B-15NF (Portaflex, Werksschaltplan 1-66, Technical Data 1968): ganze Schaltung im Zeitbereich,
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
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) { case K_6SL7: triode(T6SL7, v, i, J); break; case K_6L6: pentode(P6L6, v, i, J); break; case K_RECT: vdiode(v, i, J); break; default: sdiode(v, i, J); } }

// ---------- Bauteile und Einstellungen ----------
struct Parts {   // Werte laut Schaltplan 1-66 (Ω, F); geschätzt markiert
  double rin = 120e3, rpad = 100e3, rg1 = 5.6e6, rg2 = 2.2e6, rk1 = 5.6e3, ra1 = 470e3, cc1 = 0.022e-6, vol = 1e6, rk2 = 2.2e3, ra2 = 220e3, cc2 = 0.022e-6;
  double ts_r1 = 220e3, ts_bass = 1e6, ts_r2 = 22e3, ts_c1 = 0.001e-6, ts_c2 = 0.01e-6, ts_r3 = 270e3, ts_r4 = 120e3, ts_c3 = 0.00047e-6, ts_treb = 1e6, ts_c4 = 0.0047e-6;
  double treb1 = 1e6;   // Stand 4-65/9-65: Kanal 1 Treble als "4 M" eingezeichnet
  double rmix = 270e3, pi_ra = 120e3, pi_rk = 220, pi_rt = 1e3, pi_ct = 25e-6, pi_rf1 = 470e3, pi_rf2 = 510e3, pi_cf = 0.022e-6, pi_rg = 470e3, rnfb = 10e3;
  double cc3 = 0.022e-6, rgl = 270e3, rgs = 1e3;
  double c_b1 = 30e-6, r_b12 = 1e3, c_b2 = 40e-6, r_b23 = 22e3, c_b3 = 40e-6, r_b24 = 22e3, c_b4 = 40e-6;
  double bias_r1 = 100e3, bias_r2 = 56e3, bias_r3 = 47e3, bias_c = 100e-6;
  // Übertrager OT-214: 6,5 kΩ : 8 Ω / 16 Ω, Gleichstromwiderstand primär 290 Ω, sekundär 1 Ω (8 Ω) / 1,4 Ω (16 Ω) – Technical Data
  double ot_zp = 6500, ot_rp = 290, ot_rs8 = 1.0, ot_rs16 = 1.4, ot_lp = 40, ot_kps = 0.9998, ot_kpp = 0.9995;   // Lp, Kopplungen: geschätzt
  // Netztrafo PT-108: 750 V Mittelanzapfung, 160 mA; Innenwiderstand je Hälfte geschätzt (an 430 V im Ruhezustand angepasst)
  double hv_rms = 375, hv_r = 380, mains_hz = 60;
  // Lautsprecher CTS 15" 8 Ω im geschlossenen Portaflex-Gehäuse: alles geschätzt
  double spk_re = 6.3, spk_le = 1.0e-3, spk_fs = 40, spk_qms = 5, spk_qes = 0.55, spk_vas = 280, spk_vb = 85;
  double rsrc = 1e3;    // Innenwiderstand der Quelle (Instrument/DI), geschätzt
};
// Buchsen: 0 = Kanal 1 "Bass" (ohne Teiler), 1 = Kanal 1 "Guitar" (mit 120 k + 100 k der leeren Bass-Buchse), 2 = Kanal 2
struct Setting { double vol1 = 0.5, bass1 = 0.5, treb1 = 0.5, vol2 = 0.5, bass2 = 0.5, treb2 = 0.5; int jack = 0; bool tap16 = false; };
inline double logTaper(double x) { const double m = 0.15; if (x <= 0) return 0; if (x >= 1) return 1; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // 15 % in Mittelstellung

// Ausgänge der Netzliste
enum { O_SPK = 0, O_MOT = 1, O_B1 = 2, O_BIAS = 3, O_NO = 4 };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; };   // grp: Lösungsgruppe je Gerät (0/1 Vorstufen, 2 Phasenumkehr+Endstufe, 3 Netzteil)
inline Built build(const Parts &v, const Setting &s) {
  Built B; dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k) { int a = o.port(g, k); o.port(p, k); dev.push_back({K_6SL7, a, 2, nullptr});
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
    tri("g" + c + "a", "p" + c + "a", "k" + c + "a"); o.r("k" + c + "a", "0", v.rk1); o.r("p" + c + "a", "B4", v.ra1);
    o.c("p" + c + "a", "vt" + c, v.cc1); pot("vt" + c, "g" + c + "b", "0", v.vol, logTaper(ch == 1 ? s.vol1 : s.vol2));
    tri("g" + c + "b", "p" + c + "b", "k" + c + "b"); o.r("k" + c + "b", "0", v.rk2); o.r("p" + c + "b", "B4", v.ra2);
    o.c("p" + c + "b", "I" + c, v.cc2);
    // Klangregler (Baxandall): Bass 1 M linear mit .001/.01 zum Schleifer, Treble 1 M linear mit .00047/.0047
    double xb = ch == 1 ? s.bass1 : s.bass2, xt = ch == 1 ? s.treb1 : s.treb2;
    o.r("I" + c, "P" + c, v.ts_r1); pot("P" + c, "W" + c, "Q" + c, v.ts_bass, 1 - xb); o.r("Q" + c, "0", v.ts_r2);
    o.c("P" + c, "W" + c, v.ts_c1); o.c("Q" + c, "W" + c, v.ts_c2); o.r("W" + c, "0", v.ts_r3); o.r("W" + c, "Y" + c, v.ts_r4);
    o.c("I" + c, "T" + c, v.ts_c3); pot("T" + c, "Y" + c, "U" + c, ch == 1 ? v.treb1 : v.ts_treb, 1 - xt); o.c("U" + c, "0", v.ts_c4);
    o.r("Y" + c, "mix", v.rmix); G = ch - 1; mark(); }
  G = 2;
  // --- Phasenumkehr V3 (6SL7, schwebende Paraphase) ---
  tri("mix", "pA", "kA"); o.r("pA", "B3", v.pi_ra); o.r("kA", "N", v.pi_rk);
  tri("gB", "pB", "N"); o.r("pB", "B3", v.pi_ra); o.r("N", "0", v.pi_rt); o.c("N", "0", v.pi_ct);   // 1 k über F (Sekundärwicklung, über den Lautsprecherstecker an Masse)
  o.r("pA", "J", v.pi_rf1); o.r("J", "pB", v.pi_rf2); o.c("J", "gB", v.pi_cf); o.r("gB", "0", v.pi_rg);
  o.r(s.tap16 ? "s8" : "s16", "kA", v.rnfb);   // Gegenkopplung 10 k vom 16-Ω-Abgriff an die Kathode von V3A
  // --- Endstufe V4/V5 (6L6GC), feste Vorspannung über 270 k ---
  o.c("pA", "c5", v.cc3); o.r("c5", "BIAS", v.rgl); o.r("c5", "g5", v.rgs);
  o.c("pB", "c4", v.cc3); o.r("c4", "BIAS", v.rgl); o.r("c4", "g4", v.rgs);
  pen("g5", "B2", "a5", "0"); pen("g4", "B2", "a4", "0"); mark(); G = 3;
  // --- Ausgangsübertrager: zwei Primärhälften, Sekundär 0–8 Ω–16 Ω; Gleichstromwiderstände als Reihenwiderstände ---
  { double np = 1, n8 = std::sqrt(8 / v.ot_zp), n16 = std::sqrt(16 / v.ot_zp);   // Windungen relativ zur ganzen Primärwicklung
    double n[4] = {np / 2, np / 2, n8, n16 - n8}, L0 = v.ot_lp; dk::Mat L(4, 4);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? v.ot_kpp : v.ot_kps); if (i >= 2 && j >= 2 && i != j) k = 0.99999; L(i, j) = k * n[i] * n[j] * L0; }
    o.r("a5", "a5w", v.ot_rp / 2); o.r("a4", "a4w", v.ot_rp / 2); o.r("s8w", "s8", v.ot_rs8); o.r("s16w", "s16", v.ot_rs16 - v.ot_rs8);
    o.coupled({{"a5w", "B1"}, {"B1", "a4w"}, {"s8w", "0"}, {"s16w", "s8"}}, L);
    o.r("s16", "0", 1e6); }
  // --- Lautsprecher (geschätzt): Re, Le, Bewegung als Parallelkreis (geschlossenes Gehäuse) ---
  { std::string sp = s.tap16 ? "s16" : "s8"; double a = v.spk_vas / v.spk_vb, fc = v.spk_fs * std::sqrt(1 + a), qec = v.spk_qes * std::sqrt(1 + a), qmc = v.spk_qms * std::sqrt(1 + a);
    double res = v.spk_re * qmc / qec, cmes = qec / (2 * PI * fc * v.spk_re), lces = 1 / ((2 * PI * fc) * (2 * PI * fc) * cmes);
    o.r(sp, "sl", v.spk_re); dk::Mat Le(1, 1); Le(0, 0) = v.spk_le; o.coupled({{"sl", "sm"}}, Le);
    o.r("sm", "0", res); o.c("sm", "0", cmes); dk::Mat Lm(1, 1); Lm(0, 0) = lces; o.coupled({{"sm", "0"}}, Lm); }
  // --- Netzteil: Hochspannungswicklung (Mittelanzapfung an Masse), 5AR4, Siebung ---
  o.src("hva", v.hv_r, 1); o.src("hvb", v.hv_r, 2);
  { int p = o.port("hva", "B1"); dev.push_back({K_RECT, p, 1, nullptr}); p = o.port("hvb", "B1"); dev.push_back({K_RECT, p, 1, nullptr}); }
  o.c("B1", "0", v.c_b1); o.r("B1", "B2", v.r_b12); o.c("B2", "0", v.c_b2); o.r("B2", "B3", v.r_b23); o.c("B3", "0", v.c_b3); o.r("B2", "B4", v.r_b24); o.c("B4", "0", v.c_b4);
  // Vorspannung: von einem Ende der HV-Wicklung über 100 k, 56 k an Masse, 10D6 (Anode am Vorspannungsknoten), 47 k und 100 µF
  o.r("hva", "br", v.bias_r1); o.r("br", "0", v.bias_r2); { int p = o.port("BIAS", "br"); dev.push_back({K_SI, p, 1, nullptr}); }
  o.r("BIAS", "0", v.bias_r3); o.c("BIAS", "0", v.bias_c);
  mark();
  o.out(s.tap16 ? "s16" : "s8"); o.out("sm"); o.out("B1"); o.out("BIAS");
  return B;
}

// ---------- Laufzeit ----------
struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; double t = 0, w = 0; std::vector<std::string> nodeNames;
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; cur = s; Built b = build(v, s); net = b.net; run.dev = b.dev; run.devGrp = b.grp; run.fn = devFn; run.set(dk::compile(net, fs)); w = 2 * PI * v.mains_hz / fs;
    // Startwerte: Kondensatoren auf die Spannungen der Spannungstabelle (Technical Data), damit das Netzteil nicht minutenlang einschwingen muss
    std::map<std::string, double> g = {{"B1", 430}, {"B2", 425}, {"B3", 345}, {"B4", 350}, {"BIAS", -50}, {"br", -60}, {"p1a", 165}, {"p2a", 165}, {"p1b", 175}, {"p2b", 175},
      {"k1a", 2.2}, {"k2a", 2.2}, {"k1b", 2}, {"k2b", 2}, {"pA", 235}, {"pB", 225}, {"kA", 2.4}, {"N", 2.2}, {"J", 230}, {"c5", -50}, {"c4", -50}, {"g5", -50}, {"g4", -50}};
    auto V = [&](int n) { for (auto &kv : net.idx) if (kv.second == n) { auto it = g.find(kv.first); return it == g.end() ? 0.0 : it->second; } return 0.0; };
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V(net.C[k].a) - V(net.C[k].b));
    // Torspannungen als Newton-Startwerte
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V(net.P[k].a) - V(net.P[k].b);
  }
  // Abtastwert: x = Eingangsspannung (V); y[O_NO] Ausgänge
  void step(double x, double *y) { double u[3] = {x, 0, 0}; double m = std::sqrt(2.0) * v.hv_rms * std::sin(t); u[1] = m; u[2] = -m; t += w; if (t > 2 * PI) t -= 2 * PI; run.step(u, y); }
  void settle(double seconds) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) step(0, y); }
  void setSetting(const Setting &s) { cur = s; Built b = build(v, s); dk::Sys ns = dk::compile(b.net, fs); net = b.net; run.dev = b.dev; run.devGrp = b.grp; run.set(ns); }
};
} // namespace b15
