// Laney AOR 50: Endstufe als ganze Schaltung (DK-Methode wie der AmpFett), nach Schaltplan DRG 1054:
//   Effektweg-Rückweg (C201/C202, Puffer) → C18 → Phasenumkehr V104 (ECC83, Long-Tail mit R34/R37 1 M, R36 470, R35 10 k, R105 4k7,
//   Gegenkopplung R38 100 k vom Lautsprecherabgriff, C19 10 n, Presence VR108 22 k/C110 100 n), Anodenwiderstände R39 82 k / R40 100 k, C20 47 p
//   → C21/C22 22 n, R42/R43 220 k an die Gittervorspannung, Gitterwiderstände R106/R107 5k6 → V105/V106 (EL34, Kathoden an Masse,
//   Schirmgitter über R108/R109 470 Ω an 398 V) → Ausgangsübertrager T102 (Mittelanzapfung an 400 V) → Lautsprecher am 8-Ω-Abgriff.
//   Netzteil: Brückengleichrichter D5–12 an der Hochspannungswicklung (50 Hz), C111+C112 (400 V), Drossel T103, C108+C109 (398 V), R41 10 k, C107 (326 V);
//   Vorspannung: Orange-Wicklung → D4 → R46 22 k → C24 (−48 V) → R45 15 k → −36 V (R44 + P1 nach Masse).
// Geschätzt (nicht im Plan): Röhrentyp EL34 (Serienbestückung laut Händlerangaben), Übertrager (4 kΩ : 8 Ω, 40 H, Streuung, Wicklungskapazität,
// Eisenverluste, Kernsättigung), Drossel, Netztrafo-Innenwiderstand, Spannung der Vorspannungswicklung (auf −36 V abgeglichen),
// C23, Last des Vorverstärkers an 326 V, Lautsprecher (Eminence CB158 im 78-l-Bassreflexgehäuse als elektrische Last).
#pragma once
#include "b15_core.hpp"   // Röhren- und Diodenmodelle (Koren-Form, Gitterstrom, Silizium-Diode, Kernsättigung)
#include <map>
namespace lpa {
constexpr double PI = 3.14159265358979323846;
// ECC83/12AX7, Dempwolf-Form (gleiche Werte wie TUBE_MODELS[0] in core.hpp)
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};
// EL34 bis 3.24: Koren (mu 11, ex 1,35, kp 60, kvb 24); kg1 und kg2 an den Datenblattpunkt (Ua = Ug2 = 250 V, Ug1 −12,2 V: Ia 100 mA, Ig2 14,9 mA) angepasst
static const b15::Pentode PEL34 = {11, 1.35, 737.1, 3674.9, 60, 24, 1.0};
// EL34 ab 3.25: Derk-Strahltetrode mit Sekundäremission, Parameter aus der uTracer-Messung einer EL34 (Reefman, ExtractModel, TubeLib.inc 2013),
// Ströme auf denselben Datenblattpunkt skaliert (Ia ×1,1854, Ig2 ×1,2193). bei Ug1 0, Ug2 250 V: 20 V Anode 193 mA, 50 V 247 mA (Koren 207 mA); ri 20 kΩ (Datenblatt 15–20 kΩ, Koren 39 kΩ); Ruhe im Laney 31,2 mA (Koren 31,4)
static const b15::PentodeD DEL34 = {12.5, 1.363, 217.7, 50.5, 1282.7, 1950.2, 1.6e-6 * 217.7, 0.105, 6.09, 0.022, 0.033, 64, 2.91, 4.23,
                                    1.1853654250061119, 1.2192711945050816};
// Übertrager-Kernsättigung: φs = 2,0 V·s über der ganzen Primärwicklung (wie das bisherige Modell: 0,09 V·s an 8 Ω); Einsatz bei Vollaussteuerung unterhalb etwa 50 Hz
static const b15::Sat OT_SAT = {40, 2.0, 0.15};   // Hysterese wie beim AmpFett geschätzt (Schleifenbreite 15 % bei φs/2)
#ifndef LPA_OUT_SCALE
#define LPA_OUT_SCALE 0.00579406058   // Pegel bei 1 kHz, kleinem Signal, Presence 5 wie das bisherige Modell (Laney 3.19; ab 3.21 und 3.25 neu abgeglichen)
#endif
constexpr double OUT_SCALE = LPA_OUT_SCALE;   // Ausgangsmaßstab für den Laney-Kern (siehe PaDk in core.hpp)
// Silizium-Gleichrichter (1N4007) als glatte Kennlinie: Durchlass ab etwa 0,6 V, Bahnwiderstand 0,5 Ω (i = w/R·ln(1 + e^((u − U0)/w))).
// Gegenüber der reinen Exponentialfunktion braucht Newton im Ladestoß 2 statt 5 Schritte; die Netzteilspannungen ändern sich um weniger als 0,1 V.
inline void rect(const double *v, double *i, double *J) { const double U0 = 0.62, W = 0.025, R = 0.5; double x = (v[0] - U0) / W;
  if (x > 35) { i[0] = (v[0] - U0) / R; J[0] = 1 / R; return; } if (x < -40) { i[0] = 0; J[0] = 0; return; }
  double e = std::exp(x); i[0] = W / R * std::log1p(e); J[0] = e / (1 + e) / R; }
enum Kind { K_TRI = 0, K_PEN = 1, K_SI = 2, K_SAT = 3 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) {
    case K_TRI: b15::triodeD(T12AX7, v, i, J); break;
    case K_PEN: b15::pentodeD(DEL34, v, i, J); if (d.sc != 1) { for (int k = 1; k < 3; k++) i[k] *= d.sc; for (int k = 3; k < 9; k++) J[k] *= d.sc; } break;
    case K_SAT: b15::satI(d, *(const b15::Sat *)d.model, v[0], i, J); break;
    default: rect(v, i, J); } }

// Bauteile aus dem Laney-Bauteilsatz (änderbar) und Einstellungen
struct Vals { double c201 = 2.2e-6, c202 = 2.2e-6, c18 = 22e-9, c19 = 10e-9, c20 = 47e-12, c21 = 22e-9, r105 = 4700, vr108 = 22e3, c110 = 100e-9, r38 = 100e3;
  double pres = 0.5, mis = 0;   // mis: Streuung der Endröhren (0,1 = 10 % Unterschied im Ruhestrom)
  bool operator==(const Vals &o) const { return !std::memcmp(this, &o, sizeof *this); } };
struct Fixed {   // Werte aus dem Plan bzw. Schätzungen
  double rbuf = 1e3, rfx = 1 / (1 / 39e3 + 1 / 55e3), rg = 1e6, rk = 470, rt = 10e3, ra1 = 82e3, ra2 = 100e3, rgl = 220e3, rgs = 5.6e3, rscr = 470;
  double ot_zp = 4000, ot_lp = 40, ot_rp = 80, ot_rs = 0.25, ot_kpp = 0.9995, ot_kps = 0.999, ot_cpp = 400e-12, ot_rc = 100e3;   // geschätzt
  // Netztrafo (geschätzt): Netz 230 V/50 Hz, Hauptinduktivität 5 H primär, Kopplung primär–sekundär 0,998 (Streuung), Hochspannungshälften 0,9995,
  // Vorspannungswicklung 0,997; Primärwiderstand 1 Ω, je Hochspannungshälfte 15 Ω; Spannungen auf 400 V und −36 V im Ruhezustand abgeglichen
  double mains_v = 230, mains_hz = 50, hv_rms = 297.5, mt_rp = 1, mt_rs = 15, mt_lm = 5, mt_k = 0.998, mt_kh = 0.9995, mt_kb = 0.997;
  double c_b1 = 100e-6, ch_l = 5, ch_r = 100, c_b2 = 100e-6, r41 = 10e3, c_b3 = 50e-6;   // Drossel geschätzt
  // Vorstufenschienen: R26, R14, R4 je 10 k, C106/C102 50 µF, C an 240 V geschätzt 47 µF; Ruhelast nach den Röhrenströmen im Plan (geschätzt)
  double r26 = 10e3, r14 = 10e3, r4 = 10e3, c106 = 50e-6, c102 = 50e-6, c240 = 47e-6, l260 = 100e3, l252 = 252e3, l240 = 400e3;
  double bias_rms = 86, bias_r = 50, r46 = 22e3, c24 = 22e-6, r45 = 15e3, c23 = 10e-6, r44 = 45e3;      // Vorspannungswicklung auf −36 V abgeglichen, C23 geschätzt
  int flip = 0, nofb = 0;
  // Netzteil ab 3.27 (dc = 1): Hochspannung dc_v hinter dc_r (aus dem Trafomodell: B1 400,5 V in Ruhe, 4 V Abfall bei 40 mA Zusatzlast → 97 Ω), Vorspannung bias_v hinter bias_r
  int dc = 1; double dc_v = 408.9, dc_r = 97, bias_v = -65.87;   // Prüfzwecke: Wicklungssinn der Sekundärseite, Gegenkopplung aus
  double spk_re = 6.15, spk_le = 0.33e-3, spk_r2 = 20, spk_fs = 34.2, spk_qms = 5, spk_qes = 0.365, spk_vas = 336, spk_vb = 78, spk_fb = 50, spk_ql = 7;
};
enum { O_SPK = 0, O_B1 = 1, O_BIAS = 2, O_R240 = 3, O_R252 = 4, O_R260 = 5, O_NO = 6 };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; int monA = 0, monB = 0; };   // mon: Tore g1−k der Endröhren
inline Built build(const Fixed &f, const Vals &v) {
  Built B; dk::Net &o = B.net; auto &dev = B.dev; int G = 0;
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k) { int a = o.port(g, k); o.port(p, k); dev.push_back({K_TRI, a, 2, nullptr}); };   // Cga 1,7 pF weggelassen: beide Gitter niederohmig gespeist (Puffer über C18 bzw. R105 über C19), Grenzfrequenz weit über 1 MHz
  auto pen = [&](const std::string &g1, const std::string &g2, const std::string &a, double sc) { int p0 = o.port(g1, "0"); o.port(g2, "0"); o.port(a, "0"); dk::Dev d{K_PEN, p0, 3, nullptr}; d.sc = sc; dev.push_back(d); return p0; };
  auto diode = [&](const std::string &a, const std::string &k) { int p = o.port(a, k); dev.push_back({K_SI, p, 1, nullptr}); };
  // Effektweg-Rückweg und Puffer (wie im bisherigen Modell)
  o.src("fa", f.rbuf, 0); o.c("fa", "fc", v.c201 * v.c202 / (v.c201 + v.c202)); o.r("fc", "0", f.rfx);   // C201 und C202 in Reihe
  o.vccs("0", "bb", "fc", "0", 1); o.r("bb", "0", 1); o.r("bb", "ga0", 1e3); o.c("ga0", "ga", v.c18);
  // Phasenumkehr V104
  o.r("ga", "n", f.rg); o.r("gb", "n", f.rg); tri("ga", "pa", "k"); tri("gb", "pb", "k"); o.r("k", "n", f.rk); o.r("n", "x", f.rt);
  o.r("x", "0", v.r105); o.r("x", "px", std::max(v.vr108 * (1 - v.pres), 1.0)); o.c("px", "0", v.c110); o.c("gb", "x", v.c19);
  o.r("pa", "B3", f.ra1); o.r("pb", "B3", f.ra2); o.c("pa", "pb", v.c20);
  if (!f.nofb) o.r("s8", "x", v.r38);   // Gegenkopplung vom Lautsprecherabgriff
  mark();   // Phasenumkehr und Endröhren in einer Gruppe: über die Gegenkopplung eng gekoppelt
  // Endröhren
  o.c("pa", "c5", v.c21); o.r("c5", "BIAS", f.rgl); o.r("c5", "g5", f.rgs);
  o.c("pb", "c6", v.c21); o.r("c6", "BIAS", f.rgl); o.r("c6", "g6", f.rgs);
  o.r("s5", "B2", f.rscr); o.r("s6", "B2", f.rscr);
  B.monA = pen("g5", "s5", "a5", 1 + v.mis / 2); B.monB = pen("g6", "s6", "a6", 1 - v.mis / 2);
  // Ausgangsübertrager: zwei Primärhälften, Sekundär 8 Ω
  { double n[3] = {0.5, 0.5, std::sqrt(8 / f.ot_zp)}; dk::Mat L(3, 3);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) { double k = i == j ? 1 : ((i < 2 && j < 2) ? f.ot_kpp : f.ot_kps); L(i, j) = k * n[i] * n[j] * f.ot_lp; }
    o.r("a5", "a5w", f.ot_rp / 2); o.r("a6", "a6w", f.ot_rp / 2); o.r("s8w", "s8", f.ot_rs);
    if (f.flip) o.coupled({{"a5w", "B1"}, {"B1", "a6w"}, {"0", "s8w"}}, L); else o.coupled({{"a5w", "B1"}, {"B1", "a6w"}, {"s8w", "0"}}, L);
    o.c("a5w", "a6w", f.ot_cpp); o.r("a5w", "a6w", f.ot_rc);
    int p = o.port("a5w", "a6w"); dk::Dev d{K_SAT, p, 1, &OT_SAT}; b15::hystSetup(d, OT_SAT); dev.push_back(d); }
  mark(); G = 1;
  // Lautsprecher als Last: Re, Le (mit Wirbelstromverlust), Bewegung als Parallelkreis, Bassreflex als Reihenkreis (Box-Nachgiebigkeit, Kanalmasse)
  { double w = 2 * PI * f.spk_fs, res = f.spk_re * f.spk_qms / f.spk_qes, cmes = f.spk_qes / (w * f.spk_re), lces = 1 / (w * w * cmes);
    double lceb = lces * f.spk_vb / f.spk_vas, wb = 2 * PI * f.spk_fb, cmep = 1 / (wb * wb * lceb), rb = std::sqrt(lceb / cmep) / f.spk_ql;
    o.r("s8", "sl", f.spk_re); dk::Mat Le(1, 1); Le(0, 0) = f.spk_le; o.coupled({{"sl", "sm"}}, Le); o.r("sl", "sm", f.spk_r2);
    o.r("sm", "0", res); o.c("sm", "0", cmes); dk::Mat Lc(1, 1); Lc(0, 0) = lces; o.coupled({{"sm", "0"}}, Lc);
    dk::Mat Lb(1, 1); Lb(0, 0) = lceb; o.coupled({{"sm", "sv"}}, Lb); o.r("sv", "sw", rb); o.c("sw", "0", cmep); }
  // Netztrafo (ab 3.21): Primärwicklung an der Netzspannung (Eingang 1), Hochspannung als zwei gegenphasige Hälften (für die Gleichspannungsseite
  // gleichwertig zur Brücke an der ganzen Wicklung, ohne schwebende Knoten), Vorspannungswicklung auf demselben Kern. Streuung und Wicklungswiderstände
  // geschätzt; gemeinsamer Primärwiderstand und Streuung lassen Hoch- und Vorspannung zusammen nachgeben wie am echten Trafo.
  if (f.dc) {   // ab 3.27: Gleichspannung hinter Innenwiderstand statt Netztrafo (kein Netzbrummen, Ruhezustand erreichbar); Werte am Trafomodell abgeglichen
    o.src("ha", f.dc_r, 1); diode("ha", "B1"); o.src("bs", f.bias_r, 5); diode("bm", "bs");
  } else {
  o.src("mp", f.mt_rp, 1);
  { double n[4] = {1, f.hv_rms / f.mains_v, f.hv_rms / f.mains_v, f.bias_rms / f.mains_v}; dk::Mat L(4, 4);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double k = i == j ? 1 : (i == 0 || j == 0) ? f.mt_k : ((i == 3 || j == 3) ? f.mt_kb : f.mt_kh); L(i, j) = k * n[i] * n[j] * f.mt_lm; }
    o.coupled({{"mp", "0"}, {"haw", "0"}, {"0", "hbw"}, {"bsw", "0"}}, L); }
  o.r("haw", "ha", f.mt_rs); o.r("hbw", "hb", f.mt_rs); diode("ha", "B1"); diode("hb", "B1");
  o.r("ha", "0", 100e3); o.r("hb", "0", 100e3); o.r("bs", "0", 100e3);   // Wicklungsenden nie ganz offen (sonst schwingt die Trapezregel bei gesperrten Dioden mit halber Abtastrate)
  o.r("bsw", "bs", f.bias_r); diode("bm", "bs"); }
  o.c("B1", "0", f.c_b1); { dk::Mat L(1, 1); L(0, 0) = f.ch_l; o.coupled({{"B1", "chk"}}, L); } o.r("chk", "B2", f.ch_r); o.c("B2", "0", f.c_b2);
  o.r("B2", "B3", f.r41); o.c("B3", "0", f.c_b3);
  // Vorstufen-Versorgung (ab 3.21): R26 → 260 V (C106), R14 → 252 V (C102), R4 → 240 V; Ruhelast je Schiene nach den Röhrenströmen,
  // dazu die Stromänderungen der Vorstufe (Eingänge 2–4). Die Vorstufe rechnet mit den Spannungsänderungen dieser Schienen.
  o.r("B3", "R260", f.r26); o.c("R260", "0", f.c106); o.r("R260", "0", f.l260); o.isrc("R260", "0", 4, 1);
  o.r("R260", "R252", f.r14); o.c("R252", "0", f.c102); o.r("R252", "0", f.l252); o.isrc("R252", "0", 3, 1);
  o.r("R252", "R240", f.r4); o.c("R240", "0", f.c240); o.r("R240", "0", f.l240); o.isrc("R240", "0", 2, 1);
  // Vorspannung
  o.r("bm", "b48", f.r46); o.c("b48", "0", f.c24); o.r("b48", "BIAS", f.r45); o.c("BIAS", "0", f.c23); o.r("BIAS", "0", f.r44);
  mark();
  o.out("s8"); o.out("B1"); o.out("BIAS"); o.out("R240"); o.out("R252"); o.out("R260");
  return B;
}

struct Core {
  double fs = 48000; Fixed f; Vals cur; dk::Net net; dk::Runner run; Built bl; double t = 0, w = 0, peak = 0, last = 0;
  dk::Compiler comp;   // ab 2026-11-08: Reglerbewegung nur als Korrektur der Inversen (dk.hpp)
  void apply(const Vals &v) { cur = v; bl = build(f, v); net = std::move(bl.net); { std::vector<dk::Dev> old = std::move(run.dev); run.dev = bl.dev; dk::Runner::keepMem(run.dev, old); }
    run.devGrp = bl.grp; run.fn = devFn; run.set(comp.compile(net, fs)); }
  void init(double fs_, const Vals &v) { fs = fs_; run = dk::Runner(); apply(v); w = 2 * PI * f.mains_hz / fs; t = 0;
    // Startwerte nach den Spannungen im Plan, damit das Netzteil nicht lange einschwingen muss
    std::map<std::string, double> g = {{"B1", 400}, {"B2", 398}, {"B3", 326}, {"pa", 225}, {"pb", 209}, {"k", 36.4}, {"n", 35.3}, {"x", 10.8}, {"ga", 35.3}, {"gb", 35.3},
      {"c5", -36}, {"c6", -36}, {"g5", -36}, {"g6", -36}, {"BIAS", -36}, {"b48", -48}, {"bm", -48}, {"s5", 395}, {"s6", 395}, {"a5", 400}, {"a6", 400}, {"a5w", 400}, {"a6w", 400}, {"chk", 399}, {"R260", 280}, {"R252", 265}, {"R240", 255}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; }
  double rail[3] = {0, 0, 0};   // Vorstufenschienen 240 / 252 / 260 V (nach dem letzten Abtastwert)
  // u: Eingangsspannung (V); di: Stromänderung der Vorstufe je Schiene (A, 240/252/260), nullptr = keine
  double step(double u, const double *di = nullptr) { double in[6] = {u, f.dc ? f.dc_v : std::sqrt(2.0) * f.mains_v * std::sin(t), di ? di[0] : 0, di ? di[1] : 0, di ? di[2] : 0, f.bias_v}, y[O_NO];
    t += w; if (t > 2 * PI) t -= 2 * PI;
    run.step(in, y); last = y[O_SPK]; rail[0] = y[O_R240]; rail[1] = y[O_R252]; rail[2] = y[O_R260]; if (std::fabs(last) > peak) peak = std::fabs(last); return last; }
  void settle(double seconds) { long n = (long)(seconds * fs); for (long i = 0; i < n; i++) step(0); peak = 0; }
  double vgk(int which) const { return run.vp[which ? bl.monB : bl.monA]; }
  // Kleinsignal-Frequenzgang Eingang → Lautsprecherklemme im aktuellen Arbeitspunkt (V/V)
  std::complex<double> response(double fr) const { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, fr, 0, net.idx.at("s8"), 0); }
};
// Linearisiertes Netz im aktuellen Arbeitspunkt: Röhren als gesteuerte Stromquellen (Ableitungen aus dem letzten Abtastwert), Gleichrichter und
// Übertragerkern weggelassen (im Ruhezustand sperrend bzw. ohne Wirkung). Für die lineare Betriebsart (PaLin in core.hpp).
inline dk::Net linearNet(const Core &c) { dk::Net n = c.net;
  for (const auto &d : c.run.dev) { if (d.kind != K_TRI && d.kind != K_PEN) continue;
    for (int a = 0; a < d.np; a++) for (int b = 0; b < d.np; b++) { double g = c.run.Jd[(size_t)(d.p0 + a) * 16 + b]; if (g == 0) continue;
      const auto &pa = c.net.P[d.p0 + a], &pb = c.net.P[d.p0 + b]; n.G.push_back({pa.a, pa.b, pb.a, pb.b, g}); } }
  n.P.clear(); return n; }
} // namespace lpa
