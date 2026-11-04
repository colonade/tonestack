// Laney AOR 50: Federhall nach Schaltplan DRG 1054 (AOR 50/12 CIRCUIT, 2/4/85), ab Plugin 3.35.
// Der Hall sitzt im Effektweg zwischen Vorverstärker und Endstufe: IC203B (R201 1 M / R202 330 k, −0,33) → C201 2,2 µ → R203 1 k → Buchsen
// SEND/RETURN → C202 2,2 µ → Knoten RN; RN → R211 39 k → Summierer IC202A (R212 150 k ∥ C209 47 p) → R213 1 k → Endstufe („B“, etwa 1,7 V).
// Hallweg: RN → R204 33 k / R205 22 k → IC202B (+), nicht invertierend mit R208 150 k ∥ C204 270 p vom Tankeingang auf (−), (−) → R206 1 k →
// R207 39 k ∥ C203 22 n; IC202B → R209 100 Ω und der Folger IC203A → R210 100 Ω gemeinsam auf den Tankeingang (SKT104, „REVERB IN“) –
// die Gegenkopplung wird hinter den 100-Ω-Widerständen abgegriffen, die Spannung am Tank ist also geregelt (Verstärkung 4,75 unter 185 Hz,
// darüber steigend bis etwa 25–40). Tankausgang (REVERB OUT) → R215 100 k ∥ C205 47 p → IC201 (LF351, +), R217 220 k ∥ C207 270 p,
// R216 3k3 + C206 220 n (×67,7 ab 219 Hz) → C208 2,2 µ → 1 k (im Plan „R28“, Bezeichnung doppelt vergeben) → REVERB VR201 22 kA →
// Schleifer → R214 220 k → Summierer IC202A. Versorgung ±12 V (Plan: „+12 V“/„−12 V“ hinter R219/R220 82 Ω, Z-Dioden 15 V).
// Federtank: Laney-Teil 5564, Typ nicht im Plan; angenommen wie beim G100 Eingang B (28 Ω + 24 mH: zwei parallele LF353 zu je etwa 20 mA
// reichen dafür, für eine 8-Ω-Spule nicht) und Ausgang B (200 Ω + 0,357 H vor 100 kΩ), dasselbe Federmodell (spring.hpp) samt nichtlinearem Tank.
// Pegel im Modell der Endstufe: deren Netz rechnet den Trockenweg als Puffer mit Verstärkung 1 hinter C201/C202 (laney_pa.hpp); der Hallanteil
// wird mit demselben Maßstab addiert (Summierer: −150 k / 220 k gegen −150 k / 39 k für den Trockenweg).
#pragma once
#include "dk.hpp"
#include "spring.hpp"
#include <cmath>
#include <vector>
namespace lrev {
struct Clamp { double vs; };
// Begrenzung eines OPV-Ausgangs (wie gk::clampI): Tor zieht oberhalb ±vs Strom ab
inline void clampI(const Clamp &c, double v, double *i, double *J) { const double w = 0.002, rc = 1e-4;
  auto sp = [](double x, double &s, double &ds) { if (x > 35) { s = x; ds = 1; } else if (x < -40) { s = 0; ds = 0; } else { double e = std::exp(x); s = std::log1p(e); ds = e / (1 + e); } };
  double s1, d1, s2, d2; sp((v - c.vs) / w, s1, d1); sp((-v - c.vs) / w, s2, d2); i[0] = w / rc * (s1 - s2); J[0] = (d1 + d2) / rc; }
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) { clampI(*(const Clamp *)d.model, v[0], i, J); }
struct Parts {   // Werte laut Plan (Ω, F); Schätzungen markiert
  double r201 = 1e6, r202 = 330e3, c201 = 2.2e-6, r203 = 1e3, c202 = 2.2e-6, r211 = 39e3, r212 = 150e3, r214 = 220e3;   // Effektweg und Summierer
  double r204 = 33e3, r205 = 22e3, r206 = 1e3, r207 = 39e3, c203 = 22e-9, r208 = 150e3, c204 = 270e-12, r209 = 100, r210 = 100;   // Treiber IC202B + IC203A
  double tk_rin = 28, tk_lin = 24e-3, tk_rout = 200, tk_lout = 0.357;   // Tank (Typ unbekannt, wie G100 angenommen: Eingang B, Ausgang B)
  double r215 = 100e3, c205 = 47e-12, r217 = 220e3, c207 = 270e-12, r216 = 3.3e3, c206 = 220e-9, c208 = 2.2e-6, r218 = 1e3, p_rev = 22e3;   // IC201, REVERB VR201 22 kA
  double op_a = 1e4, op_rout = 100, op_vs = 10.5;   // LF351/LF353 an ±12 V: Aussteuerung etwa ±10,5 V (geschätzt)
  springrev::Params spring = []{ springrev::Params p; p.gain = 1.0; return p; }();   // Federmodell wie beim G100; Übertragung 1 V/A statt 16 V/A:
  // Typ-4-Tank, Ausgang B, gibt typisch 6,5 mV bei Nenn-Antrieb des B-Eingangs (6,5 mA eff.; sound-au.com, „Reverb Tanks“, Tabellen 1 und 3) → etwa 1 V/A.
  // Damit bleibt IC201 (×68) beim Nennpegel des Effektwegs (etwa 500 mV) unter der Aussteuerungsgrenze; mit 16 V/A begrenzte er schon dort.
};
enum { O_W = 0, O_TKI = 1, O_DR = 2, O_REC = 3, O_NO = 4 };   // Schleifer REVERB, Spannung am Spulenwiderstand, Tankeingang, IC201-Ausgang
// Hall-Block: Eingang = Spannung am Schleifer von PREAMP 1 LEVEL (W3, wie der Eingang des Endstufennetzes), Ausgang = Beitrag am selben Eingang
struct Rev {
  Parts v; double fs = 48000, x = 0, xt = 0; bool lin = false, on = false; int cnt = 0;
  dk::Net net; dk::Runner run; std::vector<dk::Dev> dev; Clamp cl{10.5}; springrev::Tank tank; double vrev = 0, kOut = 0;
  void build() { dk::Net o; dev.clear(); cl.vs = v.op_vs;
    auto opamp = [&](const char *p, const char *n, const char *out) { o.vccs("0", out, p, n, v.op_a / v.op_rout); o.r(out, "0", v.op_rout); int q = o.port(out, "0"); dev.push_back({0, q, 1, &cl}); };
    o.src("S", v.r203, 0); o.c("S", "RN", v.c201 * v.c202 / (v.c201 + v.c202)); o.r("RN", "0", v.r211);   // IC203B-Ausgang (Quelle) → C201, R203, C202 → RN; R211 an virtueller Masse
    o.r("RN", "Pd", v.r204); o.r("Pd", "0", v.r205); opamp("Pd", "Nd", "Od");
    o.r("DR", "Nd", v.r208); o.c("DR", "Nd", v.c204); o.r("Nd", "E", v.r206); o.r("E", "0", v.r207); o.c("E", "0", v.c203);
    opamp("Od", "Oa", "Oa"); o.r("Od", "DR", v.r209); o.r("Oa", "DR", v.r210);
    o.r("DR", "Lt", v.tk_rin); { dk::Mat L(1, 1); L(0, 0) = v.tk_lin; if (lin) o.coupled({{"Lt", "0"}}, L); else { o.coupled({{"Lt", "Lb"}}, L); o.src("Lb", 1e-3, 2); } }
    o.src("Po", v.tk_rout, 1); { dk::Mat L(1, 1); L(0, 0) = v.tk_lout; o.coupled({{"Po", "RO"}}, L); } o.r("RO", "0", v.r215); o.c("RO", "0", v.c205);
    opamp("RO", "N1", "O1"); o.r("O1", "N1", v.r217); o.c("O1", "N1", v.c207); o.r("N1", "E1", v.r216); o.c("E1", "0", v.c206);
    o.c("O1", "T", v.c208); o.r("T", "Tp", v.r218); { double p = std::min(1.0, std::max(0.0, x)); o.r("Tp", "W", std::max(v.p_rev * (1 - p), 1.0)); o.r("W", "0", std::max(v.p_rev * p, 1.0)); }
    o.r("W", "0", v.r214);
    o.out("W"); o.out("DR", "Lt"); o.out("DR"); o.out("O1");
    net = o; run.dev = dev; run.fn = devFn; run.set(dk::compile(net, fs)); run.stepLim = 8; for (auto &d : run.dev) d.model = &cl; }   // Zustände bleiben (Runner::set)
  // x: Schleifer 0…1 (mit Kennlinie), lin: Federtank linear
  void init(double fs_, double x_, bool lin_) { fs = fs_; x = xt = x_; lin = lin_; run = dk::Runner(); build(); tank.design(fs, v.spring); tank.nl = !lin; vrev = 0;
    kOut = -(v.r212 / v.r214) / ((v.r202 / v.r201) * (v.r212 / v.r211)) * (1 + v.r203 / (1 / (1 / v.r211 + 1 / (v.r204 + v.r205))));   // Hall in Einheiten des Trockenwegs am Endstufeneingang
    on = x > 0; }
  void setTarget(double x_, bool lin_) { xt = std::min(1.0, std::max(0.0, x_)); if (lin_ != lin) { lin = lin_; tank.nl = !lin; build(); } }
  void reset() { std::fill(run.h.begin(), run.h.end(), 0.0); run.vp.assign(run.vp.size(), 0.0); run.vp1 = run.vp; tank.reset(); vrev = 0; }
  Rev() {} Rev(const Rev &o) { *this = o; }
  Rev &operator=(const Rev &o) { if (this == &o) return *this; v = o.v; fs = o.fs; x = o.x; xt = o.xt; lin = o.lin; on = o.on; cnt = o.cnt; net = o.net; run = o.run; dev = o.dev; cl = o.cl; tank = o.tank; vrev = o.vrev; kOut = o.kOut;
    for (auto &d : run.dev) d.model = &cl; return *this; }
  // u: Spannung am Schleifer von PREAMP 1 LEVEL; Rückgabe: Hallbeitrag am Endstufeneingang (0, solange REVERB auf 0 steht – dann wird nicht gerechnet)
  double step(double u) {
    if (++cnt >= 256) { cnt = 0; if (x != xt) { double d = xt - x; x = std::fabs(d) > 1e-5 ? x + d * (1 - std::exp(-256 / (0.015 * fs))) : xt; if (x <= 0 && xt <= 0) x = 0; build(); } }
    if (x <= 0 && xt <= 0) { if (on) { on = false; reset(); } return 0; }
    on = true;
    double in[3] = {-(v.r202 / v.r201) * u, vrev, tank.vb}, y[O_NO]; run.step(in, y); vrev = tank.step(y[O_TKI] / v.tk_rin);
    return kOut * y[O_W]; }
};
} // namespace lrev
