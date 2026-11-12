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
#include <atomic>
#include <complex>
#include <map>
#include <string>
#include <vector>
namespace slo {
using b15::PI;
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};   // wie im Laney (an Datenblatt und Plan-Spannungen angepasst), Gitterstrom gemessen (Dempwolf)
enum Kind { K_TRI = 0, K_PEN = 1 };
constexpr double SI_U = 0.62, SI_R = 0.5;   // Gleichrichterdiode: Schwellspannung und Bahnwiderstand
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) {
    case K_TRI: b15::triodeD(T12AX7, v, i, J); break;
    default: b15::pentodeD(b15::D6L6, v, i, J); for (int k = 0; k < 3; k++) i[k] *= d.sc; for (int k = 0; k < 9; k++) J[k] *= d.sc; } }   // zwei 6L6GC je Seite: sc = 2
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
  // Ohne die Kapazitäten der beiden Kathodenfolger (V3A, V4A) und der zweiten Hälfte der Phasenumkehr (Gitter für Wechselspannung an Masse): sechs Zustände weniger,
  // Frequenzgang bis 20 kHz um höchstens 0,03 dB anders (gemessen gegen die Fassung mit allen zwanzig).
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
enum { M_V1B = 0, M_V2A, M_V1A, M_V3B, M_V4B, M_INV, M_6A, M_6B, M_NO };   // Tore für die Aussteuerungsanzeige (Gitter–Kathode)
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; int mon[M_NO] = {}; };
// Gruppen des Lösers: die ganze Vorstufe gemeinsam, Phasenumkehr mit Endstufe, Netzteil. Je Stufe einzeln (wie zuerst) brauchte Block-Gauß-Seidel bei
// Übersteuerung bis zu sechs Durchgänge (das Gitter der nächsten Stufe belastet die Anode der vorigen über den Koppelkondensator) und fiel dabei
// öfter in den Notweg; gemessen bei kräftigen Anschlägen (PREAMP 8, MASTER 6): 46 → 18 Newton-Schritte je Abtastwert, 89 → 0 Fehlversuche.
// Außerdem verstärkte der OVERDRIVE-Kanal den Rest der einzeln gelösten Stufen mit: Ruherauschen bei allen Reglern auf 10 −51 dBFS, gemeinsam gelöst −91 dBFS.
// Versucht und verworfen (1.1, Befehle je Abtastwert gezählt): die Röhren des nicht gewählten Kanals als eigene kleine Gruppe (±5 %), reuseJ über Abtastwerte
// (Ruhe −24 %, übersteuert +9 %), dünne statt dichter Produkte (+12 … +29 %).
static const int GRP[9] = {0, 0, 0, 0, 0, 0, 1, 2, 3};   // V1B, V2A, V2B, V1A, V3, V4 | Phasenumkehr + Endstufe | (Lautsprecher, Netzteil: keine Geräte)
inline Built build(const Parts &v, const Setting &s, const std::vector<std::string> *dbg = nullptr) {
  Built B; dk::Net &o = B.net; auto &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); G++; };
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k, bool caps = true) { int a = o.port(g, k); o.port(p, k); dev.push_back({K_TRI, a, 2, nullptr});
    if (caps) { o.c(g, k, v.cgk); o.c(g, p, v.cga); } return a; };
  auto pen = [&](const std::string &g1, const std::string &g2, const std::string &a) { int p0 = o.port(g1, "0"); o.port(g2, "0"); o.port(a, "0"); dk::Dev d{K_PEN, p0, 3, nullptr}; d.sc = 2; dev.push_back(d); return p0; };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };
  auto ldr = [&](const std::string &a, const std::string &b, bool on) { o.r(a, b, on ? v.ldr_on : v.ldr_off); };
  // Eingang, V1B
  o.src("in", 1.0, 0); o.r("in", "0", v.rin); o.r("in", "g1", v.rgs1); B.mon[M_V1B] = tri("g1", "p1", "k1"); o.r("B5", "p1", v.ra1); o.r("k1", "0", v.rk1); o.c("k1", "0", v.ck1); o.c("p1", "n1", v.cc1);
  mark();
  // OVERDRIVE: Abschwächer mit Höhenanhebung, GAIN, V2A, Abschwächer, kalte Stufe V2B
  o.r("n1", "gt", v.rod1); o.c("n1", "gt", v.cod1); pot("gt", "gw", "0", v.pgain, s.gain); o.c("gt", "gw", v.cbr); o.r("gw", "g2", v.rgs2);
  B.mon[M_V2A] = tri("g2", "p2", "k2"); o.r("B4", "p2", v.ra2); o.r("k2", "0", v.rk2); o.c("k2", "0", v.ck2); o.c("p2", "n2", v.cc2); o.r("n2", "g3", v.rod2); o.r("g3", "0", v.rod3);
  mark();
  ldr("g3", "0", !s.od);   // LDR1: im NORMAL-Betrieb liegt das Gitter der kalten Stufe an Masse
  tri("g3", "p3", "k3"); o.r("B4", "p3", v.ra3); o.c("B4", "p3", v.ca3); o.r("k3", "0", v.rk3); o.c("p3", "n3", v.cc3); o.r("n3", "0", v.rpop); ldr("n3", "M", s.od);   // LDR2
  mark();
  // NORMAL: Crunch/Clean-Abschwächer, GAIN mit Bright, V1A, 2,2 M ∥ 120 pF auf den Knoten M
  // Schalter als Widerstände (1 Ω / 100 GΩ), damit das Netz in jeder Stellung dieselben Kondensatoren hat und die Zustände beim Umschalten erhalten bleiben:
  // Clean: n1 –470 k– nc, nc –39 k– Masse, nc –470 k– nt (Bright 470 pF darüber). Crunch: n1 –470 k– nt (39 k abgetrennt, zweite 470 k überbrückt; Bright über den 470 k).
  { const double ON = 1.0, OFF = 1e11;   /* offen: 100 GΩ; mit 1 GΩ sprach das ungeteilte Signal in der Stellung CLEAN um 0,05 dB über */ o.r("n1", "nc", v.rcl1); o.r("nc", "ncg", s.crunch ? OFF : ON); o.r("ncg", "0", v.rcl2); o.r("nc", "nt", s.crunch ? ON : v.rcl3);
    o.c("nb", "nt", v.cnbr); o.r("nb", "nc", (s.bright && !s.crunch) ? ON : OFF); o.r("nb", "n1", (s.bright && s.crunch) ? ON : OFF); }   // ein Kondensator, dessen freies Ende der Schalter umlegt
  pot("nt", "gc", "0", v.pngain, s.ngain); B.mon[M_V1A] = tri("gc", "pc", "kc"); o.r("B5", "pc", v.rac); o.r("kc", "0", v.rkc); o.c("pc", "nm", v.ccc); o.r("nm", "M", v.rcm); o.c("nm", "M", v.ccm); o.r("M", "0", v.rgm);
  mark();
  // V3B und Kathodenfolger V3A, Effektweg (100 k / 1 k), V4B und Kathodenfolger V4A
  o.r("M", "g4", v.rgs4); B.mon[M_V3B] = tri("g4", "p4", "k4"); o.r("B3", "p4", v.ra4); o.r("k4", "0", v.rk4); o.c("k4", "0", v.ck4);
  tri("p4", "B3", "f1", false); o.r("f1", "fx", v.rcf1); o.r("fx", "0", v.rfx); o.c("fx", "send", v.cfx); o.r("send", "0", v.rret);
  mark();
  B.mon[M_V4B] = tri("send", "p5", "k5"); o.r("B3", "p5", v.ra5); o.r("k5", "0", v.rk5); o.c("k5", "0", v.ck5); tri("p5", "B3", "f2", false); o.r("f2", "0", v.rcf2);
  mark();
  // Klangregelung (Treble 250 k lin, Bass 1 M log als Stellwiderstand, Middle 25 k lin mit dem Mitten-Kondensator am Schleifer), beide Master am Treble-Schleifer
  o.c("f2", "tt", v.ct); o.r("f2", "ts", v.rsl); o.c("ts", "tb", v.cb); o.c("ts", "tm", v.cm);
  pot("tt", "tw", "tb", v.ptreb, s.treb); o.r("tb", "tbb", std::max(v.pbass * taperA(s.bass), 1.0)); pot("tbb", "tm", "0", v.pmid, s.mid);
  pot("tw", "mo", "0", v.pmv, taperA(s.master)); pot("tw", "mn", "0", v.pmv, taperA(s.nmaster)); ldr("mo", "mv", s.od); ldr("mn", "mv", !s.od);   // LDR3, LDR4
  // Phasenumkehr V5 (Schwanz 10 k auf den Gegenkopplungsknoten F: 4,7 k gegen Masse, 39 k vom 4-Ω-Abgriff, PRESENCE 25 k lin hinter 0,1 µF)
  o.c("mv", "ga", v.cpi); o.r("ga", "T", v.rgl); o.r("gb", "T", v.rgl); B.mon[M_INV] = tri("ga", "pa", "kp"); tri("gb", "pb", "kp", false); o.r("kp", "T", v.rkpi); o.r("T", "F", v.rtail);
  o.r("F", "0", v.rnt); o.c("gb", "F", v.cac); o.c("F", "px", v.cpr); o.r("px", "0", std::max(v.ppres * (1 - s.pres), 1.0));
  o.r("B3", "pa", v.rpa); o.r("B3", "pb", v.rpb); o.c("pa", "pb", v.csn);
  o.vccs("0", "s4", "s8", "0", std::sqrt(0.5)); o.r("s4", "0", 1.0); o.r("s4", "F", v.rnfb);   // 4-Ω-Abgriff = 0,707 × Spannung an der 8-Ω-Wicklung
  // Endröhren und Ausgangsübertrager
  o.c("pa", "c5", v.cco); o.r("c5", "BIAS", v.rgl2); o.r("c5", "g5", v.rgs); o.c("pb", "c6", v.cco); o.r("c6", "BIAS", v.rgl2); o.r("c6", "g6", v.rgs);
  o.r("s5", "B2", v.rscr); o.r("s6", "B2", v.rscr); B.mon[M_6A] = pen("g5", "s5", "a5"); B.mon[M_6B] = pen("g6", "s6", "a6");
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
  // Gleichrichter: an der Gleichspannungsquelle leitet er immer; statt als nichtlineares Tor daher als Schwellspannung und Bahnwiderstand im Innenwiderstand (Core::step)
  { o.src("B1", v.dc_r + SI_R, 1); o.c("B1", "0", v.c_b1);
    dk::Mat L(1, 1); L(0, 0) = v.ch_l; o.coupled({{"B1", "chk"}}, L); o.r("chk", "B2", v.ch_r); o.c("B2", "0", v.c_b2);
    o.r("B2", "B3", v.r_b3); o.c("B3", "0", v.c_b3); o.r("B3", "B4", v.r_b4); o.c("B4", "0", v.c_b4); o.r("B3", "B5", v.r_b5); o.c("B5", "0", v.c_b5);
    o.src("BIAS", v.bias_r, 2); o.c("BIAS", "0", v.c_bias); }
  mark();
  o.out("s8"); o.out("send"); o.out("B1"); if (dbg) for (auto &n : *dbg) o.out(n);
  for (auto &g : B.grp) g = GRP[g];
  return B; }

struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; dk::Compiler comp; const std::vector<std::string> *dbg = nullptr;
  void apply(const Setting &s) { cur = s; bl = build(v, s, dbg); net = std::move(bl.net); { std::vector<dk::Dev> old = std::move(run.dev); run.dev = bl.dev; dk::Runner::keepMem(run.dev, old); }
    run.devGrp = bl.grp; run.fn = devFn; run.set(comp.compile(net, fs)); }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); run.dampFull = true; run.reuseStep = true; run.reuseRho = 0.1; apply(s);   // gemeinsamer Solve gleich gedämpft, Zerlegung innerhalb eines Abtastwerts weiterverwenden (wie SVT-CL)
   
    // Startwerte: eingeschwungene Ruhespannungen aller Knoten (mit diesem Kern nach 40 s gerechnet; die Siebkette braucht sonst mehrere Sekunden)
    std::map<std::string, double> g = {{"B1", 496.996}, {"B2", 495.053}, {"B3", 379.672}, {"B4", 361.286}, {"B5", 352.776}, {"BIAS", -58.103}, {"F", 12.306}, {"T", 41.645}, {"a5", 495.248}, {"a5w", 496.998}, {"a6", 495.244}, {"a6w", 496.994}, {"c5", -58.103}, {"c6", -58.103}, {"chk", 496.996}, {"f1", 189.163}, {"f2", 203.924}, {"fx", 1.873}, {"g5", -58.103}, {"g6", -58.103}, {"ga", 41.645}, {"gb", 41.645}, {"k1", 1.465}, {"k2", 2.012}, {"k3", 4.210}, {"k4", 1.567}, {"k5", 1.766}, {"kc", 2.154}, {"kp", 43.024}, {"p1", 173.664}, {"p2", 249.505}, {"p3", 350.491}, {"p4", 188.121}, {"p5", 203.107}, {"pa", 251.865}, {"pb", 242.148}, {"pc", 254.882}, {"s5", 494.126}, {"s6", 494.126}, {"ts", 203.924}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  void step(double x, double *y) { double u[3] = {x, v.dc_v - SI_U, v.bias_v}; run.step(u, y); }
  double step(double x) { double y[O_NO + 32]; step(x, y); return y[O_SPK]; }
  int nout() const { return (int)net.O.size(); }
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { long n = (long)(seconds * fs); double y[O_NO + 32]; for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); } }
  // Kleinsignal-Frequenzgang im Arbeitspunkt: Lautsprecherspannung je Volt am Eingang
  std::complex<double> response(double f) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.idx.at("s8"), 0); }
};
// Verstärker mit Überabtastung, Reglerglättung, Schaltern und Anzeigen. Ausgabe: Lautsprecherspannung, 1,0 = VFS Volt.
// 100 W Sinus an 8 Ω sind 40 V Spitze (−8 dBFS); voll übersteuert liefert die Endstufe ein Rechteck von etwa ±46 V mit Schaltspitzen an der
// Lautsprecherinduktivität bis etwa 110 V (gemessen: alle Regler 10, 1 V am Eingang) – daher 100 V = 0 dBFS.
constexpr double VFS = 100.0, SETTLE = 1.0;
constexpr int NPEAK = 7;   // Anzeigen: V1B, V2A (OVERDRIVE) bzw. V1A (NORMAL), V3B, V4B, Phasenumkehr, 6L6GC Seite A, Seite B
inline double &sfield(Setting &s, int i) { double *f[] = {&s.gain, &s.ngain, &s.bass, &s.mid, &s.treb, &s.master, &s.nmaster, &s.pres}; return *f[i]; }
struct Amp {
  /* cfg: Kennung des Aufbaus (Plugin) */ double fsB = 48000; int os = 2, cfg = 2; Core c, c0; Setting tgt; bool ready = false; int cnt = 0, bad = 0; double vin = 0.1, outPk = 0, outSq = 0; long outN = 0;   // c0: eingeschwungener Zustand (Rückfall bei entgleister Rechnung)
  double vgk0[NPEAK] = {-1, -1, -1, -1, -1, -1, -1}, peak[NPEAK] = {};
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  int monPort(int i) const { static const int od[NPEAK] = {M_V1B, M_V2A, M_V3B, M_V4B, M_INV, M_6A, M_6B}; return c.bl.mon[(i == 1 && !c.cur.od) ? M_V1A : od[i]]; }
  void readBias() { for (int i = 0; i < NPEAK; i++) vgk0[i] = c.run.vp[monPort(i)]; }   // Ruhewert gilt für den eingeschwungenen Kern; nach dem Kanalwechsel für die neue Stufe neu gelesen (Näherung, solange Signal anliegt)
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) { fsB = fs; os = std::max(1, os_); tgt = s; c.init(fsB * os, pv, s); c.settle(SETTLE, cancel); c0 = c; readBias();
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  static bool sameSw(const Setting &a, const Setting &b) { return a.od == b.od && a.crunch == b.crunch && a.bright == b.bright; }
  static bool sameAll(const Setting &a, const Setting &b) { return sameSw(a, b) && a.gain == b.gain && a.ngain == b.ngain && a.bass == b.bass && a.mid == b.mid && a.treb == b.treb && a.master == b.master && a.nmaster == b.nmaster && a.pres == b.pres; }
  void setTarget(const Setting &s) { tgt = s; if (!sameSw(s, c.cur)) { Setting n = c.cur; n.od = s.od; n.crunch = s.crunch; n.bright = s.bright; c.apply(n); vgk0[1] = c0.run.vp[monPort(1)]; } }   // Schalter sofort (Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void snap() { c.apply(tgt); cnt = 0; }
  void resettle(double secs, const std::atomic<bool> *cancel = nullptr) { snap(); c.settle(secs, cancel); c0 = c; readBias(); }   // Hintergrund (Plugin): auf den aktuellen Stellungen einschwingen
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < 8; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO + 32]; c.step(x, y);
    for (int i = 0; i < NPEAK; i++) { double d = c.run.vp[monPort(i)] - vgk0[i]; if (d > peak[i]) peak[i] = d; }   // nur der Hub nach oben (zum Gitterstrom hin)
    double el = y[O_SPK]; if (std::fabs(el) > outPk) outPk = std::fabs(el); outSq += el * el; outN++; return el / VFS; }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();   // Eingang auf ±20 V begrenzt
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 20)) { y = 0;   // entgleist (NaN oder über 20-fache Vollaussteuerung): eingeschwungenen Zustand zurückholen, höchstens einmal je 0,1 s
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung je Stufe seit der letzten Abfrage: Gitterhub bezogen auf die Gittervorspannung (1 = Gitter erreicht die Kathodenspannung)
  void takePeaks(double *out) { for (int i = 0; i < NPEAK; i++) { out[i] = peak[i] / std::max(0.1, std::fabs(vgk0[i])); peak[i] = 0; } }
  double takeOut() { double p = outPk; outPk = 0; return p; }   // Spitzenspannung am Lautsprecher (V)
  double takePow() { double p = outN ? outSq / outN / 8 : 0; outSq = 0; outN = 0; return p; }   // Leistung seit der letzten Abfrage: Effektivspannung² / 8 Ω (W)
};
}
