// Loldano = Nachbau des Soldano SLO-100 (Super Lead Overdrive), ganze Schaltung im Zeitbereich als ein Netz (dk.hpp).
// Quelle: beschrifteter Gesamtplan von Rob Robinette (robrobinette.com, „Annotated Soldano SLO-100 Schematic“, V1.6k; nicht im Repository) – eine Nachzeichnung,
// kein Werksplan; dazu die Schaltungsanalyse von R. Kuehnel (ampbooks.com). Alle Bauteilwerte von dort; Schätzungen sind unten als solche bezeichnet.
// Signalweg OVERDRIVE: Eingang → V1B → (470 k ∥ 2 nF) → OD GAIN 500 k lin (1 nF Bright fest) → 470 k → V2A → 470 k / 1 M → V2B „kalte“ Stufe (39 k Kathode)
//   → LDR2 → Knoten M → 220 k → V3B → V3A Kathodenfolger → 100 k / 1 k (Effektweg, −40 dB) → V4B → V4A Kathodenfolger → Klangregelung → MASTER → Phasenumkehr V5
//   → 4 × 6L6GC → Ausgangsübertrager 2,2 kΩ. Gegenkopplung 39 k vom 4-Ω-Abgriff, PRESENCE 25 k lin.
// Signalweg NORMAL: V1B → Crunch/Clean-Abschwächer → NORMAL GAIN 500 k lin (Bright 470 pF) → V1A → 2,2 M ∥ 120 pF → Knoten M → wie oben, eigener MASTER.
// Im OVERDRIVE-Betrieb bleibt der Normal-Weg angeschlossen (gegenphasig, über 2,2 M), wie im Gerät. Die Kanalumschaltung geschieht mit Fotowiderständen
// (VTL5C1: leitend etwa 600 Ω, gesperrt 50 MΩ – Datenblattwerte, hier fest).
#pragma once
#include "b15_core.hpp"
#include <map>
#include <string>
#include <vector>
namespace slo {
using b15::PI;
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};   // wie im Laney (an Datenblatt und Plan-Spannungen angepasst), Gitterstrom gemessen (Dempwolf)
enum Kind { K_TRI = 0, K_PEN = 1, K_SI = 2 };
inline void rect(const double *v, double *i, double *J) { const double U0 = 0.62, W = 0.025, R = 0.5; double x = (v[0] - U0) / W;
  if (x > 35) { i[0] = (v[0] - U0) / R; J[0] = 1 / R; return; } if (x < -40) { i[0] = 0; J[0] = 0; return; }
  double e = std::exp(x); i[0] = W / R * std::log1p(e); J[0] = e / (1 + e) / R; }
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) {
    case K_TRI: b15::triodeD(T12AX7, v, i, J); break;
    case K_PEN: b15::pentodeD(b15::D6L6, v, i, J); for (int k = 0; k < 3; k++) i[k] *= d.sc; for (int k = 0; k < 9; k++) J[k] *= d.sc; break;   // zwei 6L6GC je Seite: sc = 2
    default: rect(v, i, J); } }
inline double taperA(double x) { const double s = 1 / 0.15 - 1, b = s * s; return (std::pow(b, x) - 1) / (b - 1); }   // „A“ im Plan = logarithmisch (15 % in der Mitte, Annahme)

struct Parts {
  // Vorstufe (Plan)
  double rin = 1e6, rgs1 = 68e3, ra1 = 220e3, rk1 = 1.8e3, ck1 = 1e-6, cc1 = 0.02e-6;
  double rod1 = 470e3, cod1 = 0.002e-6, pgain = 500e3, cbr = 0.001e-6, rgs2 = 470e3, ra2 = 100e3, rk2 = 1.8e3, ck2 = 1e-6, cc2 = 0.02e-6, rod2 = 470e3, rod3 = 1e6;
  double rk3 = 39e3, ra3 = 100e3, ca3 = 0.001e-6, cc3 = 0.02e-6, rpop = 2.2e6;
  double rcl1 = 470e3, rcl2 = 39e3, rcl3 = 470e3, cnbr = 470e-12, pngain = 500e3, rac = 100e3, rkc = 2.2e3, ccc = 0.02e-6, rcm = 2.2e6, ccm = 120e-12, rgm = 330e3;
  double rgs4 = 220e3, ra4 = 220e3, rk4 = 1.8e3, ck4 = 1e-6, rcf1 = 100e3, rfx = 1e3, cfx = 1e-6, rret = 1e6, ra5 = 220e3, rk5 = 2.2e3, ck5 = 25e-6, rcf2 = 100e3;
  // Klangregelung, Master, Phasenumkehr
  double ct = 470e-12, rsl = 47e3, cb = 0.02e-6, cm = 0.02e-6, ptreb = 250e3, pbass = 1e6, pmid = 25e3, pmv = 1e6, cpi = 0.02e-6;
  double rgl = 1e6, rkpi = 470, rtail = 10e3, rnt = 4.7e3, cac = 0.1e-6, cpr = 0.1e-6, ppres = 25e3, rpa = 82e3, rpb = 100e3, csn = 47e-12, rnfb = 39e3;
  // Endstufe: je Seite zwei 6L6GC parallel (Gitter 2,2 k und Schirm 470 Ω je Röhre → für das Paar halbiert)
  double cco = 0.047e-6, rgl2 = 220e3, rgs = 2.2e3 / 2, rscr = 470.0 / 2;
  // Röhrenkapazitäten 12AX7 (Datenblatt): hier als echte Kapazitäten zwischen den Elektroden (Miller-Effekt entsteht in der Rechnung)
  double cgk = 1.6e-12, cga = 1.7e-12;
  // Fotowiderstände (Datenblatt VTL5C1)
  double ldr_on = 600, ldr_off = 50e6;
  // Ausgangsübertrager: 2,2 kΩ Anode–Anode laut Plan; Induktivität, Kopplung, Wicklungswiderstände und Wicklungskapazität geschätzt (wie beim Laney)
  double ot_zp = 2200, ot_lp = 25, ot_rp = 50, ot_rs = 0.2, ot_kpp = 0.9999, ot_kps = 0.9998, ot_cpp = 400e-12, ot_rc = 100e3;   // Streuinduktivität 10 mH primär (Grenzfrequenz etwa 35 kHz); mit 50 mH schwingt die Gegenkopplungsschleife
  // Netzteil: Gleichspannung hinter Innenwiderstand (geschätzt), so geeicht, dass B+1 in Ruhe 497 V hat (Plan); Siebkette nach Plan, Drosselwiderstand geschätzt
  double dc_v = 510.45, dc_r = 80, c_b1 = 200e-6, ch_l = 10, ch_r = 100, c_b2 = 100e-6, r_b3 = 10e3, c_b3 = 40e-6, r_b4 = 15e3, c_b4 = 10e-6, r_b5 = 15e3, c_b5 = 10e-6;
  // Vorspannung: Gleichspannung (statt 50-V-Wicklung, 1N4007, 15 k / 56 k / 25-k-Trimmer); so geeicht, dass jede 6L6GC in Ruhe 35 mA zieht (Annahme, der Plan nennt keinen Wert)
  double bias_v = -58.103, bias_r = 15e3, c_bias = 10e-6;
  // Lautsprecher als Last (geschätzt: 4 × 12" in geschlossener Box, 8 Ω gesamt)
  double spk_re = 6.4, spk_le = 0.6e-3, spk_r2 = 30, spk_fs = 110, spk_qms = 6, spk_qes = 1.0;
};
struct Setting {
  double gain = 0.5, ngain = 0.5, bass = 0.5, mid = 0.5, treb = 0.5, master = 0.3, nmaster = 0.3, pres = 0.5;   // Reglerstellungen 0…1 (vor der Kennlinie)
  bool od = true, crunch = false, bright = false;   // Kanal OVERDRIVE; NORMAL: Crunch/Clean und Bright
};
enum { O_SPK = 0, O_SEND = 1, O_B1 = 2, O_NO = 3 };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; int monA = 0, monB = 0; };
inline Built build(const Parts &v, const Setting &s, const std::vector<std::string> *dbg = nullptr) {
  Built B; dk::Net &o = B.net; auto &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); G++; };
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k, bool caps = true) { int a = o.port(g, k); o.port(p, k); dev.push_back({K_TRI, a, 2, nullptr});
    if (caps) { o.c(g, k, v.cgk); o.c(g, p, v.cga); } };
  auto pen = [&](const std::string &g1, const std::string &g2, const std::string &a) { int p0 = o.port(g1, "0"); o.port(g2, "0"); o.port(a, "0"); dk::Dev d{K_PEN, p0, 3, nullptr}; d.sc = 2; dev.push_back(d); return p0; };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };
  auto ldr = [&](const std::string &a, const std::string &b, bool on) { o.r(a, b, on ? v.ldr_on : v.ldr_off); };
  // Eingang, V1B
  o.src("in", 1.0, 0); o.r("in", "0", v.rin); o.r("in", "g1", v.rgs1); tri("g1", "p1", "k1"); o.r("B5", "p1", v.ra1); o.r("k1", "0", v.rk1); o.c("k1", "0", v.ck1); o.c("p1", "n1", v.cc1);
  mark();
  // OVERDRIVE: Abschwächer mit Höhenanhebung, GAIN, V2A, Abschwächer, kalte Stufe V2B
  o.r("n1", "gt", v.rod1); o.c("n1", "gt", v.cod1); pot("gt", "gw", "0", v.pgain, s.gain); o.c("gt", "gw", v.cbr); o.r("gw", "g2", v.rgs2);
  tri("g2", "p2", "k2"); o.r("B4", "p2", v.ra2); o.r("k2", "0", v.rk2); o.c("k2", "0", v.ck2); o.c("p2", "n2", v.cc2); o.r("n2", "g3", v.rod2); o.r("g3", "0", v.rod3);
  mark();
  ldr("g3", "0", !s.od);   // LDR1: im NORMAL-Betrieb liegt das Gitter der kalten Stufe an Masse
  tri("g3", "p3", "k3"); o.r("B4", "p3", v.ra3); o.c("B4", "p3", v.ca3); o.r("k3", "0", v.rk3); o.c("p3", "n3", v.cc3); o.r("n3", "0", v.rpop); ldr("n3", "M", s.od);   // LDR2
  mark();
  // NORMAL: Crunch/Clean-Abschwächer, GAIN mit Bright, V1A, 2,2 M ∥ 120 pF auf den Knoten M
  if (s.crunch) { o.r("n1", "nt", v.rcl1); } else { o.r("n1", "nc", v.rcl1); o.r("nc", "0", v.rcl2); o.r("nc", "nt", v.rcl3); if (s.bright) o.c("nc", "nt", v.cnbr); }
  if (s.crunch && s.bright) o.c("n1", "nt", v.cnbr);
  pot("nt", "gc", "0", v.pngain, s.ngain); tri("gc", "pc", "kc"); o.r("B5", "pc", v.rac); o.r("kc", "0", v.rkc); o.c("pc", "nm", v.ccc); o.r("nm", "M", v.rcm); o.c("nm", "M", v.ccm); o.r("M", "0", v.rgm);
  mark();
  // V3B und Kathodenfolger V3A, Effektweg (100 k / 1 k), V4B und Kathodenfolger V4A
  o.r("M", "g4", v.rgs4); tri("g4", "p4", "k4"); o.r("B3", "p4", v.ra4); o.r("k4", "0", v.rk4); o.c("k4", "0", v.ck4);
  tri("p4", "B3", "f1"); o.r("f1", "fx", v.rcf1); o.r("fx", "0", v.rfx); o.c("fx", "send", v.cfx); o.r("send", "0", v.rret);
  mark();
  tri("send", "p5", "k5"); o.r("B3", "p5", v.ra5); o.r("k5", "0", v.rk5); o.c("k5", "0", v.ck5); tri("p5", "B3", "f2"); o.r("f2", "0", v.rcf2);
  mark();
  // Klangregelung (Treble 250 k lin, Bass 1 M log als Stellwiderstand, Middle 25 k lin mit dem Mitten-Kondensator am Schleifer), beide Master am Treble-Schleifer
  o.c("f2", "tt", v.ct); o.r("f2", "ts", v.rsl); o.c("ts", "tb", v.cb); o.c("ts", "tm", v.cm);
  pot("tt", "tw", "tb", v.ptreb, s.treb); o.r("tb", "tbb", std::max(v.pbass * taperA(s.bass), 1.0)); pot("tbb", "tm", "0", v.pmid, s.mid);
  pot("tw", "mo", "0", v.pmv, taperA(s.master)); pot("tw", "mn", "0", v.pmv, taperA(s.nmaster)); ldr("mo", "mv", s.od); ldr("mn", "mv", !s.od);   // LDR3, LDR4
  // Phasenumkehr V5 (Schwanz 10 k auf den Gegenkopplungsknoten F: 4,7 k gegen Masse, 39 k vom 4-Ω-Abgriff, PRESENCE 25 k lin hinter 0,1 µF)
  o.c("mv", "ga", v.cpi); o.r("ga", "T", v.rgl); o.r("gb", "T", v.rgl); tri("ga", "pa", "kp"); tri("gb", "pb", "kp"); o.r("kp", "T", v.rkpi); o.r("T", "F", v.rtail);
  o.r("F", "0", v.rnt); o.c("gb", "F", v.cac); o.c("F", "px", v.cpr); o.r("px", "0", std::max(v.ppres * (1 - s.pres), 1.0));
  o.r("B3", "pa", v.rpa); o.r("B3", "pb", v.rpb); o.c("pa", "pb", v.csn);
  o.vccs("0", "s4", "s8", "0", std::sqrt(0.5)); o.r("s4", "0", 1.0); o.r("s4", "F", v.rnfb);   // 4-Ω-Abgriff = 0,707 × Spannung an der 8-Ω-Wicklung
  // Endröhren und Ausgangsübertrager
  o.c("pa", "c5", v.cco); o.r("c5", "BIAS", v.rgl2); o.r("c5", "g5", v.rgs); o.c("pb", "c6", v.cco); o.r("c6", "BIAS", v.rgl2); o.r("c6", "g6", v.rgs);
  o.r("s5", "B2", v.rscr); o.r("s6", "B2", v.rscr); B.monA = pen("g5", "s5", "a5"); B.monB = pen("g6", "s6", "a6");
  { double n[3] = {0.5, 0.5, std::sqrt(8 / v.ot_zp)}; dk::Mat L(3, 3);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? v.ot_kpp : v.ot_kps); L(i, j) = k * n[i] * n[j] * v.ot_lp; }
    o.r("a5", "a5w", v.ot_rp / 2); o.r("a6", "a6w", v.ot_rp / 2); o.r("s8w", "s8", v.ot_rs);
    o.coupled({{"a5w", "B1"}, {"B1", "a6w"}, {"s8w", "0"}}, L); o.c("a5w", "a6w", v.ot_cpp); o.r("a5w", "a6w", v.ot_rc); }
  mark();
  // Lautsprecher: Re, Le mit Wirbelstromverlust, Grundresonanz (geschlossene Box)
  { double w = 2 * PI * v.spk_fs, res = v.spk_re * v.spk_qms / v.spk_qes, cmes = v.spk_qes / (w * v.spk_re), lces = 1 / (w * w * cmes);
    o.r("s8", "sl", v.spk_re); dk::Mat Le(1, 1); Le(0, 0) = v.spk_le; o.coupled({{"sl", "sm"}}, Le); o.r("sl", "sm", v.spk_r2);
    o.r("sm", "0", res); o.c("sm", "0", cmes); dk::Mat Lc(1, 1); Lc(0, 0) = lces; o.coupled({{"sm", "0"}}, Lc); }
  // Netzteil und Vorspannung
  { o.src("ha", v.dc_r, 1); int p = o.port("ha", "B1"); dev.push_back({K_SI, p, 1, nullptr}); o.c("B1", "0", v.c_b1);
    dk::Mat L(1, 1); L(0, 0) = v.ch_l; o.coupled({{"B1", "chk"}}, L); o.r("chk", "B2", v.ch_r); o.c("B2", "0", v.c_b2);
    o.r("B2", "B3", v.r_b3); o.c("B3", "0", v.c_b3); o.r("B3", "B4", v.r_b4); o.c("B4", "0", v.c_b4); o.r("B3", "B5", v.r_b5); o.c("B5", "0", v.c_b5);
    o.src("BIAS", v.bias_r, 2); o.c("BIAS", "0", v.c_bias); }
  mark();
  o.out("s8"); o.out("send"); o.out("B1"); if (dbg) for (auto &n : *dbg) o.out(n);
  return B; }

struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; dk::Compiler comp; const std::vector<std::string> *dbg = nullptr;
  void apply(const Setting &s) { cur = s; bl = build(v, s, dbg); net = std::move(bl.net); { std::vector<dk::Dev> old = std::move(run.dev); run.dev = bl.dev; dk::Runner::keepMem(run.dev, old); }
    run.devGrp = bl.grp; run.fn = devFn; run.set(comp.compile(net, fs)); }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s);
    // Startwerte nahe den Plan-Spannungen, damit das Einschwingen kurz bleibt
    std::map<std::string, double> g = {{"B1", 497}, {"B2", 495}, {"B3", 378}, {"B4", 350}, {"B5", 359}, {"chk", 496}, {"ha", 498}, {"a5", 496}, {"a6", 496}, {"a5w", 496}, {"a6w", 496}, {"s5", 494}, {"s6", 494},
      {"BIAS", -52}, {"c5", -52}, {"c6", -52}, {"g5", -52}, {"g6", -52}, {"p1", 230}, {"k1", 1.1}, {"p2", 240}, {"k2", 2}, {"p3", 330}, {"k3", 8}, {"pc", 250}, {"kc", 2.2}, {"p4", 250}, {"k4", 1.1}, {"f1", 251}, {"fx", 2.5},
      {"p5", 250}, {"k5", 1.3}, {"f2", 251}, {"pa", 280}, {"pb", 270}, {"kp", 34}, {"T", 33}, {"ga", 33}, {"gb", 33}, {"F", 10.5}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  void step(double x, double *y) { double u[3] = {x, v.dc_v, v.bias_v}; run.step(u, y); }
  double step(double x) { double y[O_NO + 32]; step(x, y); return y[O_SPK]; }
  void settle(double seconds) { long n = (long)(seconds * fs); double y[O_NO + 32]; for (long i = 0; i < n; i++) step(0, y); }
};
}
