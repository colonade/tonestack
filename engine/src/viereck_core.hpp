// Viereck 34: Klangteil des Quad 34 (Vorverstärker, 1982–1996) als Schaltung im Zeitbereich (DK-Löser, dk.hpp).
// Quelle: Schaltplan „QUAD 34 circuit diagram, M12746 iss. 2 onward“ (Nachzeichnung mit Werten, keith-snook.info) und
// Quad 34 Service Data (Beschreibung Tilt, Bass, Filter; Fig. 3 Tilt, Fig. 8(b) Bass, Fig. 9 Steller, Fig. 10 Filter, Fig. 11/12 Filterschaltung).
// Abschrift: docs/viereck/SCHALTPLAN.md. Gerechnet wird der linke Kanal (IC12 Tilt/Bass, RV4 A Balance, IC19 Filter, Ausgang):
//   C36 → Tilt-/Bassstufe IC12 („Ambler tone control“, invertierend, TILT RV2: Schalter mit 7 Stellungen ±3 dB, BASS RV3: Step 300/170/100 Hz, Lift +3/+6/+9)
//   → C48 → RV4 Balance → Filter IC19 (F1/F2 6 dB/Okt., mit Slope 12 dB/Okt. als Sallen-Key; ohne Filter immer Sallen-Key oberhalb des Hörbereichs)
//   → C77 → R117/R118/R119 → OUTPUT (Nennpegel 0,5 V an 830 Ω).
// Modell (nicht am Gerät): TILT stufenlos (Schleifer auf derselben Bahn statt der 7 Rasten), Drehpunkt des Tilt 80 Hz … 4 kHz (die vier Kondensatoren
// C40/C41/C45/C46 gemeinsam skaliert; Original etwa 850 Hz).
// Geschätzt: OPV TL071 als gesteuerte Quelle (Verstärkung 10⁴, 100 Ω) mit Aussteuerungsgrenze ±7,5 V an +8,6/−9,4 V, Schalter 4066 als 120 Ω.
#pragma once
#include "dk.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <string>
#include <vector>
namespace viereck {
constexpr double PI = 3.14159265358979323846;

// Begrenzung eines OPV am Ausgang (wie gk::Clamp): oberhalb ±vs zieht das Tor Strom ab (Knie 2 mV, dahinter 0,1 mΩ)
struct Clamp { double vs; };
inline void clampI(const Clamp &c, double v, double *i, double *J) { const double w = 0.002, rc = 1e-4;
  auto sp = [](double x, double &s, double &ds) { if (x > 35) { s = x; ds = 1; } else if (x < -40) { s = 0; ds = 0; } else { double e = std::exp(x); s = std::log1p(e); ds = e / (1 + e); } };
  double s1, d1, s2, d2; sp((v - c.vs) / w, s1, d1); sp((-v - c.vs) / w, s2, d2); i[0] = w / rc * (s1 - s2); J[0] = (d1 + d2) / rc; }
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) { clampI(*(const Clamp *)d.model, v[0], i, J); }

// ---------- Bauteile (linker Kanal, Plan M12746) ----------
struct Parts {
  double rsrc = 10;   // Quelle: Ausgang der Lautstärkestufe IC9 (TL071 mit Gegenkopplung, niederohmig; geschätzt)
  double c36 = 220e-9, r55 = 2.2e6, r54 = 820e3, r66 = 820e3, c47 = 220e-9, c51 = 68e-9;   // Ein- und Gegenkopplung IC12 (Fig. 8(a): Ci 68n, Co 220n)
  double r51 = 91e3, r52 = 91e3, r53 = 56e3, c40 = 3.3e-9, c41 = 1e-9;   // Tilt-Netz Eingangsseite (A → P1: C40+R52 ∥ C41+R53; A → P3: R51 gekreuzt)
  double r63 = 91e3, r64 = 91e3, r65 = 56e3, c45 = 3.3e-9, c46 = 1e-9;   // Tilt-Netz Ausgangsseite (C → P3: R64+C45 ∥ R65+C46; C → P1: R63 gekreuzt)
  double rt[8] = {13e3, 9.1e3, 9.1e3, 9.1e3, 9.1e3, 9.1e3, 9.1e3, 13e3};   // RV2 Tilt (Fig. 9(b)): Kette Anschluss 1 … 3, Abgriffe dazwischen = Stellungen
  double r47 = 100e3, c38 = 220e-9;   // Bass Step: A → R47 ∥ C38 → RV3 (Anschluss 1)
  double rstep[3] = {4.3e3, 7.0e3, 11.7e3};   // RV3 Step 300 / 170 / 100 Hz (Fig. 9(a): 4K3, +2K7, +4K7 von Anschluss 1)
  double rlift[3] = {10.7e3, 6.0e3, 3.3e3};   // RV3 Lift +3 / +6 / +9 dB (Fig. 9(a): 3K3, +2K7, +4K7 von Anschluss 3)
  double c48 = 100e-6, rv4 = 5e3;   // C48; RV4 Balance (Schieberegler, Bauteilliste nur „R34BALB“ ohne Wert): 5 k angepasst an Fig. 10 (Innenwiderstand vor dem Filter; 0,32 dB rms)
  double r73 = 15e3, r74 = 8.2e3, r75 = 18e3, r76 = 39e3, c61 = 680e-12, c62 = 1e-9, r86 = 2.2e6, r91 = 10e3, r92 = 22e3;   // Filter (Fig. 11, linker Kanal)
  double c70 = 1e-9, r86b = 2.2e6, c71 = 220e-12, r111 = 8.2e3, c75 = 120e-12, c77 = 100e-6;   // IC19 (Folger), C77
  double r117 = 2.7e3, r118 = 2.2e3, r119 = 1e3;   // Ausgang: 830 Ω, Teiler 1/5,9
  double ron = 120, roff = 1e10;   // Analogschalter 4066 (an +8,6/−9,4 V wie die OPV, SCHALTPLAN.md; Widerstände geschätzt)
  double op_a = 1e4, op_rout = 100, op_vs = 7.5;   // TL071 (vereinfacht), Aussteuerungsgrenze an +8,6/−9,4 V (geschätzt)
};
constexpr double DIV = 1e3 / (2.7e3 + 2.2e3 + 1e3);   // Ausgangsteiler R119/(R117+R118+R119)

// Einstellungen. tilt −3 … +3 (positiv = Höhen hoch, Tiefen runter); stepped: auf ganze Stellungen; pivot in Hz (Drehpunkt; PIVOT0 = Original)
// bass: −3 Step 300 Hz, −2 Step 170 Hz, −1 Step 100 Hz, 0 aus, +1/+2/+3 Lift +3/+6/+9 dB; filter: 0 aus, 1 F1, 2 F2; slope (nur mit F1/F2)
struct Setting { double tilt = 0, pivot = 0, balance = -1;   // balance: Schleiferanteil von RV4, < 0 = Mittenstellung (balC)
   int bass = 0, filter = 0; bool slope = false, stepped = true; };
inline double pivot0();   // Drehpunkt des Originals (aus der Schaltung, etwa 850 Hz)

// Schleiferstellung der Tilt-Bahn (Widerstand Anschluss 1 → Schleifer) zu tilt −3 … +3: ganze Werte = Abgriffe, dazwischen linear auf der Bahn
inline double tiltPos(const Parts &v, double t) { double tot = 0, cum[9]; cum[0] = 0; for (int k = 0; k < 8; k++) { tot += v.rt[k]; cum[k + 1] = tot; }
  t = std::max(-3.0, std::min(3.0, t)); double x = 3 - t;   // tilt +3 → Abgriff nach R1 (cum[1]); −3 → cum[7]
  int k = std::min(5, (int)std::floor(x)); double f = x - k; return cum[k + 1] + f * (cum[k + 2] - cum[k + 1]); }
inline double tiltTotal(const Parts &v) { double t = 0; for (double r : v.rt) t += r; return t; }

// BALANCE −1 … +1 zur Schleiferstellung von RV4 (Anteil der Bahn über dem Schleifer gegen Masse: 1 = oben an C48) im Kanal ch (0 links: RV4 A, 1 rechts: RV4 B).
// Abgleichanleitung (Service Data, Test 3 „Channel Balance“): aus der Mitte zum einen Ende +2,5 dB, zum anderen Ende −65 dB (Balance-Kennlinie „MN“).
// Mitte: Anteil so, dass das Ende bei Last 2,5 dB lauter ist (balC); dazwischen zum lauten Ende in dB linear, zum leisen Ende quadratisch bis −65 dB (Verlauf zwischen den
// Endpunkten nicht angegeben, geschätzt).
inline double balC();
inline double balFrac(double b) { b = std::max(-1.0, std::min(1.0, b)); double c = balC();
  if (b >= 0) return c * std::pow(1 / c, b); double fmin = c * std::pow(10.0, -65 / 20.0); return std::max(fmin, c * (1 + b) * (1 + b)); }
inline double balPos(double b, int ch) { return balFrac(ch == 0 ? -b : b); }   // BALANCE nach rechts (+): rechter Kanal lauter, linker leiser
enum { O_OUT = 0, O_TONE, O_FILT, O_NO };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; Clamp clamp; };
inline void buildInto(Built &B, const Parts &v, const Setting &s, double kC) {
  dk::Net &o = B.net; auto &dev = B.dev; B.clamp = {v.op_vs};
  auto sw = [&](const std::string &a, const std::string &b, bool on) { o.r(a, b, on ? v.ron : v.roff); };
  auto opamp = [&](const std::string &inp, const std::string &inn, const std::string &out) { o.vccs("0", out, inp, inn, v.op_a / v.op_rout); o.r(out, "0", v.op_rout);
    int p = o.port(out, "0"); dev.push_back({0, p, 1, nullptr}); B.grp.push_back((int)B.grp.size()); };
  // Tilt-/Bassstufe IC12
  o.src("in", v.rsrc, 0); o.c("in", "A", v.c36); o.r("A", "0", v.r55);
  o.r("A", "N", v.r54); o.r("N", "O", v.r66); o.c("O", "Cb", v.c47); o.c("W", "N", v.c51); opamp("0", "N", "O");
  o.c("A", "a1", v.c40 * kC); o.r("a1", "P1", v.r52); o.c("A", "a2", v.c41 * kC); o.r("a2", "P1", v.r53); o.r("A", "P3", v.r51);
  o.r("P3", "b1", v.r64); o.c("b1", "Cb", v.c45 * kC); o.r("P3", "b2", v.r65); o.c("b2", "Cb", v.c46 * kC); o.r("Cb", "P1", v.r63);
  double t = s.stepped ? std::round(s.tilt) : s.tilt, ra = tiltPos(v, t); o.r("P1", "W", std::max(ra, 1.0)); o.r("W", "P3", std::max(tiltTotal(v) - ra, 1.0));
  o.r("A", "D", v.r47); o.c("A", "D", v.c38);
  if (s.bass < 0) o.r("D", "0", v.rstep[std::max(0, s.bass + 3)]);   // −3 → 300 Hz (4K3) … −1 → 100 Hz (11K7)
  if (s.bass > 0) o.r("Cb", "0", v.rlift[std::min(2, s.bass - 1)]);
  if (s.bass >= 0) o.r("D", "0", v.roff);   // Step-Bahn offen (Knoten bleibt im Netz)
  // Balance RV4 A (Schleifer nach Einstellung; Mitte balC)
  o.c("O", "E", v.c48); { double x = s.balance < 0 ? balC() : std::min(1.0, s.balance); o.r("E", "F", std::max(v.rv4 * (1 - x), 1.0)); o.r("F", "0", std::max(v.rv4 * x, 1.0)); }
  // Filter IC19: Schalter A (R74/R75 zu), B (C62 ∥ C61), C (C70 an Masse), D (Ausgang auf Y = Sallen-Key)
  bool A = s.filter != 2, Bs = s.filter != 0, C = s.filter != 0, D = s.filter == 0 || s.slope;
  o.r("F", "X", v.r73); sw("F", "M", A); o.r("M", "X", v.r74); o.r("M", "Y", v.r75); o.r("F", "Y", v.r76); o.c("X", "Y", v.c61);
  o.c("X", "K", v.c62); o.r("K", "Y", v.r86); sw("K", "Y", Bs);
  o.r("X", "Z", v.r91); o.r("Y", "Z", v.r92); o.c("Z", "L", v.c70); o.r("L", "0", v.r86b); sw("L", "0", C); o.c("Z", "0", v.c71);
  opamp("Z", "Nf", "Of"); o.r("Of", "Nf", v.r111); o.c("Of", "Nf", v.c75);
  o.c("Of", "G", v.c77); sw("G", "Y", D);
  o.r("G", "H", v.r117); o.r("H", "OUT", v.r118); o.r("OUT", "0", v.r119);
  o.out("OUT"); o.out("O"); o.out("Of");
}

struct Core {
  double fs = 48000; Parts v; Setting cur; double kC = 1; dk::Net net; dk::Runner run; Built bl; dk::Compiler comp;
  void fixDev() { for (auto &d : run.dev) d.model = &bl.clamp; }
  Core() {} Core(const Core &o) { *this = o; }
  Core &operator=(const Core &o) { if (this == &o) return *this; fs = o.fs; v = o.v; cur = o.cur; kC = o.kC; net = o.net; run = o.run; bl = o.bl; fixDev(); return *this; }
  // Netz neu; ändert sich ein Kondensator (Drehpunkt), wird sein Gedächtnis mit gc_neu/gc_alt umgerechnet (Spannung und Strom bleiben, wie Loldano 1.8)
  void apply(const Setting &s) { cur = s; double p0 = pivot0(); kC = s.pivot > 0 ? p0 / s.pivot : 1;
    bl = Built(); buildInto(bl, v, s, kC); net = bl.net; run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn;
    const std::vector<double> gc0 = run.s.gc; run.set(comp.compile(net, fs)); run.stepLim = 8; fixDev();
    if (gc0.size() == run.s.gc.size() && run.h.size() >= gc0.size()) for (size_t k = 0; k < gc0.size(); k++) if (run.s.gc[k] != gc0[k] && gc0[k] > 0) run.h[k] *= run.s.gc[k] / gc0[k]; }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s); }
  // Ausgang in Volt an OUTPUT; tone/filt: Ausgänge IC12 und IC19 (für Anzeigen)
  void step(double x, double *y) { double u[1] = {x}; run.step(u, y); }
  std::complex<double> response(double f, int o = O_OUT) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.O[o].a, net.O[o].b); }
};

// Kleinsignal-Frequenzgang ohne Laufzeitzustand (Diagramm, Prüfung): eigenes Netz, Begrenzer im linearen Bereich
inline std::complex<double> response(const Parts &v, const Setting &s, double f, int o = O_OUT) {
  Built B; buildInto(B, v, s, s.pivot > 0 ? pivot0() / s.pivot : 1); std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
  for (auto &d : B.dev) { J.push_back({0.0}); dp.push_back({d.p0, d.np}); }
  return dk::acResponse(B.net, J, dp, f, 0, B.net.O[o].a, B.net.O[o].b); }
// Drehpunkt des Originals: Frequenz, bei der TILT +3 und TILT 0 gleich laut sind (Halbierung zwischen 300 Hz und 3 kHz), einmal berechnet
inline double pivot0() { static const double p = [] { Parts v; Setting a, b; a.tilt = 3; b.tilt = 0; a.pivot = b.pivot = -1;   // −1: kein Skalieren (kC = 1)
    auto d = [&](double f) { return std::abs(response(v, a, f)) / std::abs(response(v, b, f)) - 1; };
    double lo = 300, hi = 3000; for (int i = 0; i < 80; i++) { double m = std::sqrt(lo * hi); if (d(m) < 0) lo = m; else hi = m; } return std::sqrt(lo * hi); }(); return p; }

// Verstärkung Eingang → OUTPUT bei flacher Einstellung, BALANCE in der Mitte, 1 kHz (Bezug für Pegel und Diagramme: dort 0 dB)
// Mittenstellung der Balance-Bahn: Anteil c, bei dem Endstellung (1) gegen c bei 1 kHz ohne Filter 2,5 dB lauter ist (mit der Last des Filters)
inline double balC() { static const double c = [] { Parts v; Setting a, m; a.balance = 1; double ga = std::abs(response(v, a, 1000)), lo = 0.5, hi = 1;
    for (int i = 0; i < 60; i++) { double x = 0.5 * (lo + hi); m.balance = x; if (20 * std::log10(ga / std::abs(response(v, m, 1000))) > 2.5) lo = x; else hi = x; } return 0.5 * (lo + hi); }(); return c; }
inline double gain0() { static const double g = std::abs(response(Parts(), Setting(), 1000)); return g; }
// ---------- Gerät mit Reglerglättung und Pegelbezug ----------
// Eingang: 1,0 = vin Volt Spitze am Eingang der Tilt-Stufe (Ausgang IC9). Ausgang: OUTPUT-Spannung / (vin · gain0()), Vorzeichen umgekehrt
// (IC12 kehrt um, IC9 im Gerät ebenfalls: der Quad 34 ist insgesamt nicht invertierend) → bei flacher Einstellung, BALANCE Mitte 1:1.
struct Unit {
  double fs = 48000; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 1.0; double peak[2] = {0, 0}; Core c0; int bad = 0;
  static Setting norm(Setting s) { if (s.balance < 0) s.balance = balC(); return s; }   // Mittenstellung als Schleiferanteil, damit die Balance gleiten kann
  // Der Kern rechnet den TILT immer auf der stufenlosen Bahn: bei STEPS ist nur das Ziel gerundet, der Schleifer läuft in 15 ms hinüber
  // (wie der Schalter beim Drehen über die Zwischenstellungen; ein Sprung über mehrere Rasten gäbe sonst ein Knacken).
  static Setting coreOf(Setting s) { s = norm(s); if (s.stepped) s.tilt = std::round(s.tilt); s.stepped = false; return s; }
  void init(double fs_, const Parts &pv, const Setting &s0) { fs = fs_; v = pv; tgt = norm(s0); c.init(fs, v, coreOf(tgt)); double y[O_NO]; for (int i = 0; i < (int)(0.05 * fs); i++) c.step(0, y); c0 = c; ready = true; }
  static bool swEq(const Setting &a, const Setting &b) { return a.bass == b.bass && a.filter == b.filter && a.slope == b.slope; }
  // Schalter sofort (Netz neu), TILT, Drehpunkt und Balance gleiten (15 ms)
  void setTarget(const Setting &s0) { tgt = norm(s0); if (!swEq(tgt, c.cur)) { Setting n = c.cur; n.bass = tgt.bass; n.filter = tgt.filter; n.slope = tgt.slope; c.apply(n); } }
  void snap() { c.apply(coreOf(tgt)); cnt = 0; }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fs)); Setting n = c.cur, t = coreOf(tgt); bool moved = false;
    auto glide = [&](double &x, double t, double eps) { double d = t - x; if (d == 0) return; moved = true; if (std::fabs(d) > eps) x += d * a; else x = t; };
    glide(n.tilt, t.tilt, 1e-4);
    { double lp = std::log(n.pivot > 0 ? n.pivot : pivot0()), lt = std::log(t.pivot > 0 ? t.pivot : pivot0()); glide(lp, lt, 1e-5); n.pivot = std::exp(lp); }
    glide(n.balance, t.balance, 1e-4);
    if (moved) c.apply(n); }
  double run1(double x, double *yr = nullptr) { double y[O_NO]; c.step(x, y); peak[0] = std::max(peak[0], std::fabs(y[O_TONE])); peak[1] = std::max(peak[1], std::fabs(y[O_FILT])); if (yr) for (int k = 0; k < O_NO; k++) yr[k] = y[k]; return -y[O_OUT] / (vin * gain0()); }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0; tick(); double y = run1(x);
      if (!(std::fabs(y) < 100)) { y = 0; if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); bad = (int)(0.1 * fs); } }
      if (bad > 0) bad--; buf[i] = (float)y; } }
  void takePeaks(double *out) { out[0] = peak[0] / v.op_vs; out[1] = peak[1] / v.op_vs; peak[0] = peak[1] = 0; }   // Aussteuerung IC12, IC19 relativ zur Grenze
};
} // namespace viereck
