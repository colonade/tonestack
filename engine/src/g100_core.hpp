// Yamaha G100-212II (Service Manual YAMU-02234: Schaltplan Vorverstärker Seite 9 – JK/PA1/PA2-Platine der G100-115II/210/212II –,
// Netzteil und Endstufe Seite 8, Einstell- und Prüfanweisungen Seiten 4–6, Frequenzgang Seite 7).
// Vorverstärker als Schaltung im Zeitbereich (DK-Löser, dk.hpp), beide Kanäle gleichzeitig wie im Gerät:
// Eingangsstufe IC1 1/2 (×11, Dioden D101/D102 zwischen den Eingängen), die beiden Klangregelungen (Fender-Typ 2,7 n / 10 k / 1 µ / 0,5 µ,
// TREBLE und BASS A25K, MIDDLE A1K, PULL FAT 5,6 n + 330 k) an IC8 2/2 bzw. IC2 2/2 (×4,3), Kanal A: VOLUME mit PULL BRIGHT, IC8 1/2 (×6,6);
// Kanal B: GAIN mit PULL BRIGHT, IC2 1/2 (×5,7), Begrenzerdioden D105/D106 hinter 10 k, Stummschalter TR102, Source-Stufe TR101 (2SK30A),
// MASTER VOLUME; Relais A/B; parametrischer Entzerrer (IC4 Summierer, Zustandsvariablenfilter IC5/IC6, LEVEL ±, Q, FREQUENCY 100 Hz … 5 kHz),
// Hall: Treiber IC3 (TA7220P, Stromtreiber mit steigender Verstärkung), Federtank als physikalisches Modell (spring.hpp), Rückholverstärker IC7 1/2, REVERB,
// Ausgangsnetzwerk, Eingang der Endstufe (MA-Platine). Operationsverstärker NJM4558 als gesteuerte Quellen mit Begrenzung.
// Endstufe: Verstärkung aus der Gegenkopplung (1,8 M gegen 100 k ∥ (10 k + 1 µ): Form), Pegel nach Tabelle 3 des Service Manuals geeicht,
// weiche Begrenzung an der Aussteuerungsgrenze (100 W an 4 Ω bei 10 % Klirr), Ausgangselko 2200 µ an 4 Ω. Kein Lautsprecher.
// Geschätzt (nicht im Plan): 2SK30A-Kennwerte (an Tabelle 3, Kanal B, angepasst), Diodenmodell 1S1555, Aussteuerungsgrenzen der OPVs und der Endstufe,
// Einschaltwiderstand des Stummschalters, Innenwiderstand der Quelle.
#pragma once
#include "gk_core.hpp"   // expg, Clamp/clampI, Jfet/jfetI, potTaper; aor::IIR, bilinear
#include "spring.hpp"    // Federhall-Tank
#include <atomic>
#include <cmath>
#include <map>
#include <string>
#include <vector>
namespace g100 {
constexpr double PI = 3.14159265358979323846;
using gk::Clamp; using gk::Jfet; using gk::potTaper;

// ---------- Nichtlineare Bauteile ----------
// Zwei Siliziumdioden 1S1555 gegeneinander parallel (Is 2,5 nA, n·Ut 45 mV: 0,58 V bei 1 mA); oberhalb 0,68 V linear fortgesetzt (expg)
struct DiodePair { double is, nvt; };
inline void diodePairI(const DiodePair &d, double v, double *i, double *J) { double e1, d1, e2, d2; gk::expg(v / d.nvt, e1, d1); gk::expg(-v / d.nvt, e2, d2);
  i[0] = d.is * (e1 - e2); J[0] = d.is * (d1 + d2) / d.nvt; }
enum Kind { K_CLAMP = 0, K_DIODE = 1, K_JFET = 2, K_CLAMP1 = 3, K_CLAMP3 = 4 };   // K_CLAMP1: Begrenzung von IC1 (niedrigere Versorgung), K_CLAMP3: Halltreiber IC3 an ±20 V
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) {
  switch (d.kind) { case K_CLAMP: case K_CLAMP1: case K_CLAMP3: gk::clampI(*(const Clamp *)d.model, v[0], i, J); break; case K_DIODE: diodePairI(*(const DiodePair *)d.model, v[0], i, J); break; default: gk::jfetI(*(const Jfet *)d.model, v, i, J); } }

// ---------- Bauteile und Einstellungen ----------
struct Parts {   // Werte laut Plan (Ω, F); Schätzungen markiert
  double rsrc = 1e3;   // Innenwiderstand der Quelle (geschätzt)
  double r_high = 15e3, r_low = 47e3, r_in = 1e3, c_in = 0.047e-6, c_g = 47e-12, r_g = 1e6;   // JK-Platine (HIGH 15 k, LOW 47 k gegen 15 k), 1 k, 0,047 µ, 47 p, 1 M
  double r1f = 10e3, r1g = 1e3, c1g = 10e-6, c1d = 220e-12;   // IC1 1/2: 10 k / 1 k + 10 µ, 220 p zwischen den Eingängen
  double ct = 0.0027e-6, rsl = 10e3, cb = 1e-6, cm = 0.5e-6, p_t = 25e3, p_b = 25e3, p_m = 1e3, c_fat = 0.0056e-6, r_fat = 330e3;   // Klangregelung (je Kanal)
  double c_tw = 1e-6, r_tin = 100e3, c_tin = 560e-12, c_tfb = 100e-12, r2f = 3.3e3, r2g = 1e3;   // Schleifer → IC8 2/2 / IC2 2/2
  double c_a2 = 4.7e-6, r_a2 = 220, p_vol = 25e3, c_br = 0.0056e-6, r_br = 1e6, c_vw = 1e-6, r_vw = 220, r_vin = 100e3, c_vfb = 100e-12;   // VOLUME / GAIN, PULL BRIGHT, Eingang der zweiten Stufe
  double r3fa = 5.6e3, r3fb = 4.7e3, r3g = 1e3, c_a3 = 4.7e-6, r_a0 = 100e3;   // IC8 1/2 (A) / IC2 1/2 (B), Kanal A: 4,7 µ und 100 k vor dem Relais
  double c_x = 0.1e-6, r_x = 10e3, c_xg = 330e-12, r_xg = 100e3, r_mute = 500;   // Kanal B: 0,1 µ, 10 k, 330 p, 100 k, Dioden D105/D106; TR102 (Ein-Widerstand geschätzt)
  double r_dd = 560, c_dd = 220e-6, r_d = 5.6e3, r_s = 1e3, c_s = 47e-6, c_m = 1e-6, p_master = 25e3;   // TR101: 560 + 220 µ, 5,6 k, 1 k ∥ 47 µ; 1 µ auf MASTER VOLUME
  double r_a1 = 100e3, r_rv = 2.2e3, c_rv = 1e-6, r_rvg = 100e3, c_rvg = 68e-12;   // nach dem Relais: 100 k, Halltreiber-Eingang (2,2 k, 1 µ, 100 k, 68 p)
  double c_r34 = 100e-12, r_rf = 100e3, r_rg = 2.2e3, c_rg = 2.2e-9, c_ro = 10e-6, r_rd = 56, c_rd = 100e-12;   // IC3 TA7220P: 100 p zwischen den Eingängen, 100 k / (2,2 k + 2,2 n): Verstärkung 1 → 46 ab 720 Hz (+6 dB/Okt.), 10 µ, 56 Ω, 100 p
  double tk_rin = 28, tk_lin = 24e-3, tk_rout = 200, tk_lout = 0.357;   // Tank (Accutronics, Typ nicht im Manual): Eingang Kennbuchstabe B (150 Ω bei 1 kHz: 28 Ω + 24 mH – passt zur 720-Hz-Ecke des Treibers mit 56 Ω), Ausgang B (2250 Ω bei 1 kHz: 200 Ω + 0,357 H); geschätzt
  double c_pg = 1e-9, r7g = 100e3, c7d = 220e-12, r7f = 100e3, r7g2 = 1e3, c7g = 1e-6, c7o = 4.7e-6, p_rev = 25e3, r_rfs = 2.2e3, r_rmix = 47e3;   // Rückholverstärker IC7 1/2: 1 n am Tankausgang, 100 k, 220 p, 100 k / 1 k + 1 µ (×101), 4,7 µ, REVERB B25K, 2,2 k, 47 k
  double r_eq = 10e3, p_level = 50e3, r_lw = 680, p_q = 50e3, r_q1 = 4.7e3, r_q2 = 6.8e3, r_f = 1e3, p_f = 50e3, c_f1 = 0.033e-6, c_f2 = 0.03e-6;   // parametrischer EQ
  double c_o = 1e-6, r_o = 10e3, r_og = 100e3;   // Ausgang: 1 µ, 10 k, 100 k (dazu der Hallrückweg 47 k + 2,2 k + REVERB-Poti)
  double r_pin = 2.2e3, c_pin = 4700e-12, r_pa = 10e3, c_pa = 1e-6, r_pag = 100e3;   // Eingang Endstufe: 2,2 k, 4700 p, 10 k gegen Masse, 1 µ, 100 k Vorspannung
  double op_a = 1e4, op_rout = 100, op_vs = 13.5, op_vs1 = 11.0, op_vs3 = 17.0;   // NJM4558: Leerlaufverstärkung (vereinfacht), Ausgangswiderstand, Grenze an ±15 V; IC1 an ±12,4 V (560 Ω + Diode in der Zuleitung); TA7220P an ±20 V etwa ±17 V – geschätzt
  springrev::Params spring;   // Federtank (siehe spring.hpp): 2 Federn 33 / 37 ms, T60 2,75 s, fC 4,2 kHz; Pegel 16 V/A (bei REVERB 10 ist der Hall etwa so laut wie das Direktsignal) – alles geschätzt
  double d_is = 2.5e-9, d_nvt = 0.045;   // 1S1555 (geschätzt: Kleinsignal-Siliziumdiode)
  double j_idss = 4.0e-3, j_vp = -2.0, j_lam = 0.02, vcc = 15;   // 2SK30A (GR: Idss 2,6 … 6,5 mA, Up −0,4 … −5 V): Up Mitte des üblichen Bereichs, Idss so, dass Kanal B 13 dB über Kanal A liegt (Tabelle 3)
  double pa_rf = 1.8e6, pa_rb = 100e3, pa_rg = 10e3, pa_cg = 1e-6;   // MA-Platine: Gegenkopplung 1,8 M, 100 k auf die Mittenspannung, 10 k + 1 µ gegen Masse (Form: 199-fach über 16 Hz, 19-fach bei Gleichspannung)
  double pa_gain = 105;   // Verstärkung der Endstufe bei 1 kHz, geeicht an Tabelle 3 (−70 dB an HIGH → 1 dBm an 4 Ω, Kanal A, Tabelle-1-Stellung: 71 dB gesamt); aus den Gegenkopplungswerten folgten 199
  double pa_knee = 26, pa_lim = 31, pa_cout = 2200e-6, pa_rload = 4;   // Begrenzung (V Spitze an 4 Ω): 100 W bei 10 % Klirr = 28 V, höchstens 140 W = 33 V; weiches Knie (geschätzt); Ausgangselko 2200 µ an 4 Ω (2 × 8 Ω)
};
// Regler 0…1 (Knopf 0…10; LEVEL: 0 = −5, 1 = +5; FREQUENCY: Skala 100 Hz … 5 kHz); Schalter
struct Setting { double vola = 0.5, treba = 0.5, mida = 0.5, bassa = 0.5, gainb = 0.5, masterb = 0.5, trebb = 0.5, midb = 0.5, bassb = 0.5, level = 0.5, q = 0.5, freq = 0.5, reverb = 0;
  bool fata = false, brighta = false, fatb = false, brightb = false, chanB = false, low = false; int taper = 0; bool tankLin = false; };   // chanB: Kanal B gewählt; low: LOW-Buchse; taper: Kennlinie der A-Potis (0 log 15 %, 1 log 10 %, 2 linear); tankLin: Federtank linear wie bis 1.1 (sonst nichtlinear, siehe spring.hpp)
constexpr int NKNOB = 13;
inline double &sfield(Setting &s, int i) { double *f[NKNOB] = {&s.vola, &s.treba, &s.mida, &s.bassa, &s.gainb, &s.masterb, &s.trebb, &s.midb, &s.bassb, &s.level, &s.q, &s.freq, &s.reverb}; return *f[i]; }
// FREQUENCY-Skala der Frontplatte: 100 (Anschlag links), 200, 500 (Mitte), 1K, 5K (Anschlag rechts); dazwischen logarithmisch.
// Der Knopfwinkel der Marken 200 und 1K ist aus der Zeichnung der Frontplatte abgelesen (Seite 1: etwa 28 % und 72 % des Drehwegs).
inline double freqOfKnob(double x) { const double xs[5] = {0, 0.28, 0.5, 0.72, 1}, fs[5] = {100, 200, 500, 1000, 5000}; x = std::min(1.0, std::max(0.0, x));
  for (int k = 0; k < 4; k++) if (x <= xs[k + 1]) { double t = (x - xs[k]) / (xs[k + 1] - xs[k]); return fs[k] * std::pow(fs[k + 1] / fs[k], t); } return fs[4]; }
inline double freqPotR(const Parts &v, double x) { double f = freqOfKnob(x); double R = 1 / (2 * PI * f * std::sqrt(v.c_f1 * v.c_f2)) - v.r_f; return std::min(v.p_f, std::max(0.0, R)); }   // Bahnwiderstand des C50K-Tandems

// Ausgänge der Netzliste
enum { O_G = 0, O_A1, O_A2, O_A3, O_B2, O_B3, O_X, O_D, O_EQ, O_TKI, O_RDRV, O_RRET, O_NO };   // Gate der Endstufe, IC1, IC8 2/2, IC8 1/2, IC2 2/2, IC2 1/2, Diodenknoten, TR101-Drain, EQ-Ausgang (IC4 2/2), Spannung am Tank-Eingangswiderstand (→ Spulenstrom), Halltreiber-Ausgang, Rückholverstärker-Ausgang
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; Clamp clamp, clamp1, clamp3; DiodePair dp; Jfet jf; int pJfet = 0; };
inline void buildInto(Built &B, const Parts &v, const Setting &s) {
  dk::Net &o = B.net; std::vector<dk::Dev> &dev = B.dev; int G = 0; B.clamp = {v.op_vs}; B.clamp1 = {v.op_vs1}; B.clamp3 = {v.op_vs3}; B.dp = {v.d_is, v.d_nvt}; B.jf = {v.j_idss, v.j_vp, v.j_lam};
  auto mark = [&]() { while (B.grp.size() < dev.size()) B.grp.push_back(G); };
  auto sw = [&](const std::string &a, const std::string &b, bool closed) { o.r(a, b, closed ? 1e-3 : 1e8); };
  auto pot = [&](const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); };   // x = 1: Schleifer an a (Knopf 10)
  auto opamp = [&](const std::string &inp, const std::string &inn, const std::string &out, const Clamp *cl) { o.vccs("0", out, inp, inn, v.op_a / v.op_rout); o.r(out, "0", v.op_rout);
    if (cl) { int p = o.port(out, "0"); dev.push_back({cl == &B.clamp1 ? K_CLAMP1 : cl == &B.clamp3 ? K_CLAMP3 : K_CLAMP, p, 1, cl}); } };
  auto tp = [&](double x) { return potTaper(x, s.taper); };
  // --- JK-Platine, IC1 1/2: HIGH 15 k (LOW offen) bzw. LOW 47 k mit 15 k gegen Masse (Steckerschalter der HIGH-Buchse); 1 k, 0,047 µ, 47 p, 1 M; ×11; D101/D102 zwischen den Eingängen ---
  o.src("in", v.rsrc, 0); if (s.low) { o.r("in", "n0", v.r_low); o.r("n0", "0", v.r_high); } else o.r("in", "n0", v.r_high);
  o.r("n0", "n0b", v.r_in); o.c("n0b", "n1", v.c_in); o.c("n1", "0", v.c_g); o.r("n1", "0", v.r_g);
  opamp("n1", "i1n", "a1", &B.clamp1); o.r("a1", "i1n", v.r1f); o.r("i1n", "i1g", v.r1g); o.c("i1g", "0", v.c1g); o.c("n1", "i1n", v.c1d);
  { int p = o.port("n1", "i1n"); dev.push_back({K_DIODE, p, 1, &B.dp}); }
  mark();
  // --- Klangregelung (je Kanal): 2,7 n auf TREBLE (A25K, Schleifer → Ausgang), 10 k auf 1 µ (BASS oben) und 0,5 µ (BASS unten = MIDDLE oben);
  //     BASS und MIDDLE als Rheostat (Schleifer am oberen Ende; Knopf 10 = volle Bahn); PULL FAT: 5,6 n vom Eingang mit 330 k gegen Masse auf das TREBLE-Ende;
  //     Schleifer → 1 µ → IC + (100 k, 560 p; 100 p zwischen den Eingängen), 3,3 k / 1 k ---
  auto stack = [&](const std::string &k, double treb, double mid, double bass, bool fat) {
    o.c("a1", "T" + k, v.ct); o.r("a1", "N" + k, v.rsl); o.c("N" + k, "Tb" + k, v.cb); o.c("N" + k, "Mb" + k, v.cm);
    pot("T" + k, "tw" + k, "Tb" + k, v.p_t, tp(treb)); o.r("Tb" + k, "Mb" + k, std::max(v.p_b * tp(bass), 1.0)); o.r("Mb" + k, "0", std::max(v.p_m * tp(mid), 1.0));
    o.c("a1", "P" + k, v.c_fat); o.r("P" + k, "0", v.r_fat); sw("P" + k, "T" + k, fat);
    o.c("tw" + k, "i2p" + k, v.c_tw); o.r("i2p" + k, "0", v.r_tin); o.c("i2p" + k, "0", v.c_tin); o.c("i2p" + k, "i2n" + k, v.c_tfb);
    opamp("i2p" + k, "i2n" + k, "a2" + k, &B.clamp); o.r("a2" + k, "i2n" + k, v.r2f); o.r("i2n" + k, "0", v.r2g); };
  // --- Kanal A: 4,7 µ, 220, VOLUME (A25K) mit PULL BRIGHT (5,6 n auf den Schleifer, 1 M), 1 µ, 220 → IC8 1/2 (100 k, 100 p, 5,6 k / 1 k), 4,7 µ, 100 k, Relaiskontakt A ---
  G = 1; stack("a", s.treba, s.mida, s.bassa, s.fata);
  o.c("a2a", "a2c", v.c_a2); o.r("a2c", "V", v.r_a2); pot("V", "vw", "0", v.p_vol, tp(s.vola)); o.c("V", "Bc", v.c_br); o.r("Bc", "0", v.r_br); sw("Bc", "vw", s.brighta);
  o.c("vw", "vc", v.c_vw); o.r("vc", "i3p", v.r_vw); o.r("i3p", "0", v.r_vin); o.c("i3p", "i3n", v.c_vfb);
  opamp("i3p", "i3n", "a3", &B.clamp); o.r("a3", "i3n", v.r3fa); o.r("i3n", "0", v.r3g);
  o.c("a3", "A0", v.c_a3); o.r("A0", "0", v.r_a0); sw("A0", "A1", !s.chanB);
  mark();
  // --- Kanal B: 220, 4,7 µ, GAIN (A25K) mit PULL BRIGHT, 220, 1 µ → IC2 1/2 (100 k, 100 p, 4,7 k / 1 k), 0,1 µ, 10 k, Knoten X (330 p, 100 k, D105/D106, TR102 Stummschalter
  //     bei Kanal A), TR101 Source-Stufe (Gate X, 5,6 k an 560 + 220 µ, 1 k ∥ 47 µ), 1 µ, MASTER VOLUME (A25K), Relaiskontakt B ---
  G = 2; stack("b", s.trebb, s.midb, s.bassb, s.fatb);
  o.r("a2b", "gc0", v.r_a2); o.c("gc0", "Vb", v.c_a2); pot("Vb", "gw", "0", v.p_vol, tp(s.gainb)); o.c("Vb", "Bcb", v.c_br); o.r("Bcb", "0", v.r_br); sw("Bcb", "gw", s.brightb);
  o.r("gw", "gc", v.r_vw); o.c("gc", "i3pb", v.c_vw); o.r("i3pb", "0", v.r_vin); o.c("i3pb", "i3nb", v.c_vfb);
  opamp("i3pb", "i3nb", "a3b", &B.clamp); o.r("a3b", "i3nb", v.r3fb); o.r("i3nb", "0", v.r3g);
  o.c("a3b", "x1", v.c_x); o.r("x1", "X", v.r_x); o.c("X", "0", v.c_xg); o.r("X", "0", v.r_xg); { int p = o.port("X", "0"); dev.push_back({K_DIODE, p, 1, &B.dp}); }
  o.r("X", "0", s.chanB ? 1e9 : v.r_mute);
  o.src("vdd", v.r_dd, 1); o.c("vdd", "0", v.c_dd); o.r("vdd", "D", v.r_d); o.r("S", "0", v.r_s); o.c("S", "0", v.c_s);
  { B.pJfet = o.port("X", "S"); o.port("D", "S"); dev.push_back({K_JFET, B.pJfet, 2, &B.jf}); }
  o.c("D", "Mt", v.c_m); pot("Mt", "mw", "0", v.p_master, tp(s.masterb)); sw("mw", "A1", s.chanB);
  mark();
  // --- nach dem Relais: 100 k, Last des Halltreibers; parametrischer EQ: IC4 1/2 und 2/2 invertierend 10 k / 10 k, LEVEL (50 k, Mittelrastung) zwischen den beiden
  //     Summenpunkten, Schleifer über 680 vom Puffer IC5 2/2 (Q-Schleifer); Zustandsvariablenfilter: IC5 1/2 Summierer (10 k vom IC4-1/2-Ausgang, 10 k Rückführung,
  //     10 k vom Tiefpass; + Eingang = Q-Schleifer), Integratoren IC6 1/2 (1 k + C50K, 33 n) und IC6 2/2 (1 k + C50K, 30 n); Bandpass → 4,7 k → Q (A50K) → 6,8 k ---
  G = 3; o.r("A1", "0", v.r_a1); o.r("A1", "rv", v.r_rv); o.c("rv", "rv2", v.c_rv); o.r("rv2", "0", v.r_rvg); o.c("rv2", "0", v.c_rvg);
  o.r("A1", "j1", v.r_eq); opamp("0", "j1", "v1", &B.clamp); o.r("v1", "j1", v.r_eq); o.r("v1", "j2", v.r_eq); opamp("0", "j2", "v2", &B.clamp); o.r("v2", "j2", v.r_eq);
  { double p = std::min(1.0, std::max(0.0, s.level)); o.r("j1", "lw", std::max(v.p_level * p, 1.0)); o.r("lw", "j2", std::max(v.p_level * (1 - p), 1.0)); } o.r("lw", "vq", v.r_lw);   // LEVEL 10 (+5): Schleifer am Summenpunkt von IC4 2/2 = Anhebung
  o.r("v1", "sn", v.r_eq); o.r("lp", "sn", v.r_eq); opamp("qw", "sn", "hp", &B.clamp); o.r("hp", "sn", v.r_eq);
  { double R = v.r_f + freqPotR(v, s.freq); o.r("hp", "b1", R); opamp("0", "b1", "bp", &B.clamp); o.c("bp", "b1", v.c_f1); o.r("bp", "b2", R); opamp("0", "b2", "lp", &B.clamp); o.c("lp", "b2", v.c_f2); }
  o.r("bp", "qt", v.r_q1); pot("qt", "qw", "qb", v.p_q, 1 - tp(s.q)); o.r("qb", "0", v.r_q2);   // Q 0 (links, breit): Schleifer am Bandpass-Ende; Q 10 (schmal): am 6,8-k-Ende
  opamp("qw", "vq", "vq", nullptr);
  // --- Ausgang: 1 µ, 10 k, 100 k, Hallrückweg; Eingang der Endstufe: 2,2 k, 4700 p, 10 k, 1 µ, 100 k ---
  o.c("v2", "oc", v.c_o); o.r("oc", "O", v.r_o); o.r("O", "0", v.r_og);
  // --- Hall: Treiber IC3 (+ Eingang rv2; 100 p zwischen den Eingängen; Gegenkopplung 100 k gegen 2,2 k + 2,2 n), 10 µ, 56 Ω auf die Treiberspule (28 Ω + 24 mH) mit 100 p;
  //     Abnehmerspule als Quelle u[2] (Federmodell) hinter 200 Ω + 0,357 H, 1 n gegen Masse, IC7 1/2 (100 k, 220 p; 100 k / 1 k + 1 µ), 4,7 µ, REVERB (B25K), 2,2 k, 47 k auf O ---
  mark(); G = 4; opamp("rv2", "i3nr", "ar", &B.clamp3); o.c("rv2", "i3nr", v.c_r34); o.r("ar", "i3nr", v.r_rf); o.c("i3nr", "rgc", v.c_rg); o.r("rgc", "0", v.r_rg);
  o.c("ar", "rdc", v.c_ro); o.r("rdc", "Dt", v.r_rd); o.c("Dt", "0", v.c_rd); o.r("Dt", "Lt", v.tk_rin);
  if (s.tankLin) { dk::Mat L(1, 1); L(0, 0) = v.tk_lin; o.coupled({{"Lt", "0"}}, L); }
  else { dk::Mat L(1, 1); L(0, 0) = v.tk_lin; o.coupled({{"Lt", "Lb"}}, L); o.src("Lb", 1e-3, 3); }   // ab 1.2: Rückwirkung der Feder als Spannung u[3] in Reihe mit der Treiberspule
  o.src("Po", v.tk_rout, 2); { dk::Mat L(1, 1); L(0, 0) = v.tk_lout; o.coupled({{"Po", "Pt"}}, L); } o.c("Pt", "0", v.c_pg); o.r("Pt", "0", v.r7g); o.c("Pt", "i7n", v.c7d);
  opamp("Pt", "i7n", "a7", &B.clamp); o.r("a7", "i7n", v.r7f); o.r("i7n", "g7", v.r7g2); o.c("g7", "0", v.c7g);
  o.c("a7", "rvt", v.c7o); pot("rvt", "rvw", "0", v.p_rev, std::min(1.0, std::max(0.0, s.reverb))); o.r("rvw", "RFS", v.r_rfs); o.r("RFS", "O", v.r_rmix);
  o.r("O", "pi", v.r_pin); o.c("pi", "0", v.c_pin); o.r("pi", "0", v.r_pa); o.c("pi", "g", v.c_pa); o.r("g", "0", v.r_pag);
  mark();
  o.out("g"); o.out("a1"); o.out("a2a"); o.out("a3"); o.out("a2b"); o.out("a3b"); o.out("X"); o.out("D"); o.out("v2"); o.out("Dt", "Lt"); o.out("ar"); o.out("a7");
}
inline Built build(const Parts &v, const Setting &s) { Built B; buildInto(B, v, s); return B; }

// ---------- Endstufe (MA-Platine): Gegenkopplung 1,8 M / 100 k ∥ (10 k + 1 µ) gibt die Form (Schulter unter 16 Hz), der Pegel ist an Tabelle 3 geeicht;
// weiche Begrenzung; Ausgangselko 2200 µ an 4 Ω (Hochpass 18 Hz) ----------
struct PowerAmp { aor::IIR fb, hp; double lim = 31, knee = 26; double pk = 0;
  void design(double fs, const Parts &v) { double C = v.pa_cg, rb = v.pa_rb, rg = v.pa_rg, rf = v.pa_rf, ghf = 1 + rf / (rb * rg / (rb + rg)), k = v.pa_gain / ghf;
    double b1[2] = {k * (rb + rf) / rb, k * C * (rb * rg + rf * (rb + rg)) / rb}, a1[2] = {1, C * rg}; fb.n = 1; aor::bilinear(b1, a1, 1, fs, fb.b, fb.a);
    double t = v.pa_cout * v.pa_rload, b2[2] = {0, t}, a2[2] = {1, t}; hp.n = 1; aor::bilinear(b2, a2, 1, fs, hp.b, hp.a);
    lim = v.pa_lim; knee = v.pa_knee; }
  double step(double x, bool clip) { double y = fb.step(x); double a = std::fabs(y); if (clip && a > knee) { double z = knee + (lim - knee) * std::tanh((a - knee) / (lim - knee)); y = y < 0 ? -z : z; a = z; } if (a > pk) pk = a; return hp.step(y); }   // pk: Spitze hinter der Begrenzung, vor dem Ausgangselko
  void reset() { fb.reset(); hp.reset(); pk = 0; } };
// Kleinsignal-Übertragung der Endstufe (für Diagramme)
inline std::complex<double> paResponse(double f, const Parts &v) { std::complex<double> s(0, 2 * PI * f); double C = v.pa_cg, rb = v.pa_rb, rg = v.pa_rg, rf = v.pa_rf, ghf = 1 + rf / (rb * rg / (rb + rg)), k = v.pa_gain / ghf;
  std::complex<double> fbk = k * ((rb + rf) / rb + s * (C * (rb * rg + rf * (rb + rg)) / rb)) / (1.0 + s * C * rg); double t = v.pa_cout * v.pa_rload; return fbk * (s * t) / (1.0 + s * t); }

// ---------- Laufzeit ----------
struct Core {
  double fs = 48000; Parts v; Setting cur; dk::Net net; dk::Runner run; Built bl; double dDC = 0; springrev::Tank tank; double vrev = 0;   // tank: Federmodell zwischen Treiberspule und Abnehmerspule (ein Abtastwert Verzug)
  void apply(const Setting &s) { cur = s; tank.nl = !s.tankLin; bl = Built(); buildInto(bl, v, s); net = bl.net; run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn; run.set(dk::compile(net, fs)); run.stepLim = 8; fixDev(); }
  void fixDev() { for (auto &d : run.dev) d.model = d.kind == K_CLAMP ? (const void *)&bl.clamp : d.kind == K_CLAMP1 ? (const void *)&bl.clamp1 : d.kind == K_CLAMP3 ? (const void *)&bl.clamp3 : d.kind == K_JFET ? (const void *)&bl.jf : (const void *)&bl.dp; }   // Zeiger auf die eigenen Modelle (nach Kopie)
  Core() {} Core(const Core &o) { *this = o; }
  Core &operator=(const Core &o) { if (this == &o) return *this; fs = o.fs; v = o.v; cur = o.cur; net = o.net; run = o.run; bl = o.bl; dDC = o.dDC; tank = o.tank; vrev = o.vrev; fixDev(); return *this; }
  // Ruhestrom der Source-Stufe: Id = Idss·(1 − Id·Rs/Up)² (Shichman-Hodges, Gate auf Masse)
  double idQ() const { double id = 1e-3; for (int k = 0; k < 50; k++) { double u = 1 + id * v.r_s / v.j_vp; if (u < 0) u = 0; double f = id - v.j_idss * u * u, df = 1 - 2 * v.j_idss * u * v.r_s / v.j_vp; id -= f / df; } return id; }
  void init(double fs_, const Parts &pv, const Setting &s) { fs = fs_; v = pv; run = dk::Runner(); apply(s); tank.design(fs, v.spring); vrev = 0;
    double id = idQ(), vs = id * v.r_s, vd = v.vcc - id * (v.r_dd + v.r_d), vc = v.vcc - id * v.r_dd;
    std::map<std::string, double> g = {{"S", vs}, {"D", vd}, {"vdd", vc}, {"Mt", vd}};
    std::vector<double> V(net.nn + 1, 0.0); for (auto &kv : net.idx) { auto it = g.find(kv.first); if (it != g.end()) V[kv.second] = it->second; }
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]);
    for (size_t k = 0; k < net.P.size(); k++) run.vp[k] = V[net.P[k].a] - V[net.P[k].b];
    run.vp1 = run.vp; dDC = vd; }
  void step(double x, double *y) { double u[4] = {x, v.vcc, vrev, tank.vb}; run.step(u, y); vrev = tank.step(y[O_TKI] / v.tk_rin); }   // tank.vb: Rückwirkung (nur nichtlinear, sonst 0 und im Netz nicht verwendet)   // Spulenstrom aus der Spannung am Tank-Eingangswiderstand → Federmodell → Quelle im nächsten Abtastwert
  void settle(double seconds, const std::atomic<bool> *cancel = nullptr) { double y[O_NO]; long n = (long)(seconds * fs); for (long i = 0; i < n; i++) { if (cancel && (i & 2047) == 0 && cancel->load()) return; step(0, y); }
    step(0, y); dDC = y[O_D]; }
  // Kleinsignal-Frequenzgang Eingang → Ausgang o im aktuellen Arbeitspunkt
  std::complex<double> response(double f, int o = O_G) { std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int c = 0; c < d.np; c++) b[a * d.np + c] = run.Jd[(size_t)(d.p0 + a) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, 0, net.O[o].a, net.O[o].b); }
};

// Verstärker mit Überabtastung, Reglerglättung, Endstufe, Anzeigen. Ausgabe: Spannung an den Lautsprecherklemmen / V_FS
constexpr double V_FS = 40.0;   // 1,0 = 40 V Spitze (100 W an 4 Ω sind 28 V Spitze)
struct Amp {
  double fsB = 48000; int os = 1; Core c; Parts v; Setting tgt; bool ready = false; int cnt = 0; double vin = 0.1; bool power = true;
  PowerAmp pa; double peak[6] = {0, 0, 0, 0, 0, 0};   // Spitzen: IC1 (V), Kanal A OPVs (V), Kanal-B-Dioden (V am Knoten X), TR101-Drain (V um den Ruhepunkt), EQ-OPVs (V), Endstufe (V am Lautsprecher)
  int L = 0; std::vector<double> h, hp, xb, yb; int xi = 0, yi = 0;
  Core c0; int bad = 0;
  void init(double fs, int os_, const Parts &pv, const Setting &s, const std::atomic<bool> *cancel = nullptr) { fsB = fs; os = std::max(1, os_); v = pv; tgt = s;
    c.init(fsB * os, v, s); c.settle(0.5, cancel); c0 = c; pa.design(fsB * os, v);
    L = 0; if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, wd = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * wd; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); xi = yi = 0; }
    ready = true; }
  int latency() const { return os > 1 ? 31 : 0; }
  static bool swEq(const Setting &a, const Setting &b) { return a.fata == b.fata && a.brighta == b.brighta && a.fatb == b.fatb && a.brightb == b.brightb && a.chanB == b.chanB && a.low == b.low && a.taper == b.taper && a.tankLin == b.tankLin; }
  // Schalter sofort (Netz neu, Zustände bleiben), Regler gleiten (15 ms)
  void setTarget(const Setting &s) { bool rebuild = !swEq(s, c.cur); tgt = s; if (rebuild) { Setting n = c.cur; n.fata = s.fata; n.brighta = s.brighta; n.fatb = s.fatb; n.brightb = s.brightb; n.chanB = s.chanB; n.low = s.low; n.taper = s.taper; n.tankLin = s.tankLin; c.apply(n); } }
  void snap() { c.apply(tgt); cnt = 0; }
  void tick() { if (++cnt < 64) return; cnt = 0; double a = 1 - std::exp(-64 / (0.015 * fsB)); Setting n = c.cur; bool moved = false;
    for (int i = 0; i < NKNOB; i++) { double &x = sfield(n, i), d = sfield(tgt, i) - x; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-4) x += d * a; else x = sfield(tgt, i); }
    if (moved) c.apply(n); }
  double run1(double x) { double y[O_NO]; c.step(x, y);
    auto up = [&](int k, double a) { if (a > peak[k]) peak[k] = a; };
    up(0, std::fabs(y[O_A1])); up(1, std::max(std::fabs(y[O_A2]), std::fabs(y[O_A3]))); up(2, std::fabs(y[O_X])); up(3, std::fabs(y[O_D] - c.dDC)); up(4, std::fabs(y[O_EQ]));
    double el = pa.step(y[O_G], power); up(5, pa.pk); pa.pk = 0;
    return el / V_FS; }
  void process(float *buf, int n) { if (!ready) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    for (int i = 0; i < n; i++) { double x = std::isfinite(buf[i]) ? std::max(-20.0, std::min(20.0, buf[i] * vin)) : 0, y; tick();
      if (os == 1) y = run1(x);
      else { xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x; y = 0;
        for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
          double z = run1(os * u); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = z;
          if (ph == os - 2) for (int k = 0; k < L; k++) y += h[k] * yb[yi + k]; } }
      if (!(std::fabs(y) < 10)) { y = 0;
        if (bad <= 0) { Setting s = c.cur; c = c0; c.apply(s); std::fill(xb.begin(), xb.end(), 0.0); std::fill(yb.begin(), yb.end(), 0.0); pa.reset(); bad = (int)(0.1 * fsB); } }
      if (bad > 0) bad--;
      buf[i] = (float)y; } }
  // Aussteuerung seit der letzten Abfrage: IC1 relativ zu seiner Grenze, Kanal A und EQ relativ zur OPV-Grenze, Dioden relativ zu 0,6 V, TR101 relativ zu 6 V Hub, Endstufe als Leistung (W an 4 Ω)
  void takePeaks(double *out) { out[0] = peak[0] / v.op_vs1; out[1] = peak[1] / v.op_vs; out[2] = peak[2] / 0.6; out[3] = peak[3] / 6.0; out[4] = peak[4] / v.op_vs; out[5] = peak[5] * peak[5] / 2 / v.pa_rload; for (double &p : peak) p = 0; }
};
} // namespace g100
