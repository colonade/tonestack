// Wisent Echoreck = Nachbau des Binson Echorec PE 603 T (Röhren-Echogerät mit Magnettrommel, ein Aufnahme- und vier Wiedergabeköpfe).
// Quelle Schaltung: Nachzeichnung „Echorec PE603T“ (H.-J. Duchert, Stand 12.10.97, „Alle Angaben ohne Gewähr“) und der Werksplan „Wiring diagram of the
// P.E. 603-T Echorec“ (beide nicht im Repository, docs/wisent/SCHALTPLAN.md). Trommel: Messwerte von Effectrode („Binson Echorec memory system“: Trommel
// 4,7″, 18 ips = 457 mm/s bei 50 Hz, Kopfabstand 34 mm → 74/148/222/296 ms, Stahldraht 0,1 mm, Spalt Aufnahme 10 µm, Wiedergabe 5 µm, Löschen mit
// Dauermagneten). Elektronik als ein Netz im Zeitbereich (dk.hpp); die Trommel liegt zwischen Aufnahmekopf-Strom (Ausgang des Netzes) und den vier
// Kopfspannungen (Eingänge des Netzes) und ist mindestens 62 ms lang, daher ohne Rückwirkung im selben Abtastwert.
// Signalweg: Eingang → V4A (ECC83) → [TD 1 M: trocken → 330 k → V4B] / [100 k ∥ 820 pF → INPUT 0,5 M → V5A]; V5A + V5B (gemeinsame Anode, 47 k):
//   Mischer aus Eingang und Rückkopplung (V5B ← LENGTH OF SWELL ← TF ← F) → 330 k ∥ 150 pF / 120 k / 560 k ∥ 10 nF → V6A (ECC82) → 27 k → Aufnahmekopf.
//   Wiedergabe je Kopf: Kopf ∥ 220 pF → 2,2 nF → ½ ECC83 (V1A/B, V2A/B, 10 M, Kathode an Masse) → 47 k; Anode → 20 nF + TSC 10 k, 330 pF, 10 nF → H_k;
//   H_k → TSW 0,5 M (Abgriff → 330 k → Swell-Schiene W); H_k → obere Taste → 100 k → T → 220 k → N (270 k) bzw. untere Taste → 100 k → B → 220 k → X (270 k).
//   Betriebsart: ECHO G–X; REPEAT G–X und F–N; SWELL G–W und F–N (Deutung der Schaltermatrix, SCHALTPLAN.md). G (470 k) → V3A → 22 nF → BASS/TREBLE
//   (passiv, Baxandall) → VOLUME 0,5 M → 330 k → Mischpunkt (47 k) ← 330 k ← trocken; 10 nF → V4B (Kathodenfolger, Gitterableitung gebootstrappt); Ausgang 270 nF / 47 k.
#pragma once
#define DK_FAST    // Abkürzungen im Löser und in den Kennlinien wie beim Loldano (dk.hpp, b15_core.hpp)
#define B15_FAST
#define DK_KEEPLU
#define B15_TAB
#include "b15_core.hpp"
#include "dk.hpp"
#include <atomic>
#include <cstdint>
#include <complex>
#include <string>
#include <vector>
namespace wisent {
using b15::PI;
static const b15::TriodeD T12AX7 = {2.019059e-3, 2.02928, 100, 1.26};   // wie Laney/Loldano
enum Kind { K_AX7 = 0, K_AU7 = 1 };
inline void devFn(const dk::Dev &d, const double *v, double *i, double *J) { if (d.kind == K_AU7) b15::triodeK(b15::K12AU7, v, i, J); else b15::triodeD(T12AX7, v, i, J); }
inline double taperA(double x) { const double s = 1 / 0.15 - 1, b = s * s; return (std::pow(b, x) - 1) / (b - 1); }   // „A“ = logarithmisch (15 % in der Mitte, Annahme wie beim Loldano)
inline double clamp01(double x) { return std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }

struct Parts {
  // Eingang, V4A (Plan)
  double rin = 47e3, cin = 47e-9, rg4 = 1e6, ra4 = 100e3, c41 = 47e-9, ptd = 1e6, td = 0.67;   // TD (Trocken-Trimmer): 0,06 V → 0,04 V laut Plan (Abgleich des Ausgangs; die Stellung kommt aus Setting::dry)
  double rpre = 100e3, cpre = 820e-12, rsh = 4.7e3, pin = 0.5e6, c5a = 10e-9, rg5a = 1e6;
  // Mischer V5A/V5B
  double rk5 = 560, ra5 = 47e3, c5b = 10e-9, cg5b = 470e-12, rg5b = 1e6, ptf = 0.25e6, tf = 0.35, plos = 0.25e6, rlos = 47e3;   // TF: Rückkopplungs-Trimmer (abgeglichen, Prüfung)
  // Aufnahmeverstärker V6A, Aufnahmekopf
  double c5o = 47e-9, ptm = 0.5e6, rx1 = 330e3, cx1 = 150e-12, rx2 = 120e3, rx3 = 560e3, cx3 = 10e-9, ra6 = 47e3, rk6 = 1.5e3, ck6 = 22e-9, c6o = 47e-9, rrec = 27e3;
  double rh_rec = 450, lh_rec = 0.1;   // Aufnahmekopf: 400–500 Ω (Effectrode); Induktivität angenommen
  // Wiedergabe je Kopf (V1A, V1B, V2A, V2B)
  double rh_pb = 650, lh_pb = 0.1, ch_pb = 220e-12, cpb = 2.2e-9, rgpb = 10e6, rapb = 47e3, ctsc = 20e-9, ptsc = 10e3, tsc = 0.5, cpbs = 330e-12, cpbo = 10e-9, ptsw = 0.5e6, tsw = 1.0, rsw = 330e3;
  double vapb = 120, iapb = 0.85e-3;   // Arbeitspunkt (Plan: 120 V an 47 k aus 160 V)
  // Schaltermatrix
  double rhead = 100e3, rtn = 220e3, rn = 270e3, rbx = 220e3, rx = 270e3, rg3 = 470e3;
  // V3A, Klangregelung, VOLUME, V4B
  double ra3 = 47e3, rk3 = 1.5e3, ck3 = 22e-9, c3o = 22e-9, rbs1 = 47e3, pbass = 1e6, cb1 = 2.2e-9, cb2 = 22e-9, rbs2 = 10e3, rbo = 100e3, ctr1 = 220e-12, ptreb = 1e6, ctr2 = 2.2e-9, pvol = 0.5e6;
  double rmix = 330e3, rdry = 330e3, rmx = 47e3, cmx = 10e-9, rk4b = 1.5e3, rb4b = 47e3, rgl4b = 1e6, cg4b = 100e-12, cout = 270e-9, rout = 47e3;
  double va3 = 174.6, ia3 = 0.94e-3, va4b = 125, ia4b = 0.72e-3;   // Arbeitspunkte V3A (Plan: Kathode 1,4 V an 1,5 k) und V4B (Rechnung mit Röhrenmodell)
  double cgk = 1.6e-12, cga = 1.7e-12;   // Röhrenkapazitäten 12AX7 (Datenblatt)
  double b1 = 280, b2 = 220, b3 = 160;   // Anodenspannungen laut Plan (Netzteil ideal, Annahme)
  double ron = 1.0, roff = 1e11;         // Schalter
};
enum Mode { M_ECHO = 0, M_REPEAT = 1, M_SWELL = 2 };
struct Setting {
  double input = 0.25, swell = 0.5, volume = 1.0, bass = 0.5, treble = 0.5;   // Regler 0 … 1
  double dry = 0.67;   // Trimmer TD (Trocken-Anteil, im Gerät innen; 0,67 = Planwert 0,04 V), als Modell-Regler DRY
  int mode = M_REPEAT; bool out[4] = {false, true, false, false}, fb[4] = {false, true, false, false};   // untere Reihe: Köpfe zum Ausgang, obere: Köpfe in die Rückkopplung
};
// Schwelle der Schleife (REPEAT, ein Rückkopplungskopf): LENGTH OF SWELL (0 … 1), ab der die Schleifenverstärkung bei irgendeiner Frequenz 0 dB erreicht,
// über der Bandgeschwindigkeit r = v/v(50 Hz). Langsamer: mehr Abstandsverlust, schwingt unter 0,75 nicht mehr; schneller: heller, schwingt früher.
// Gemessen mit Core::gains (wisenttest thr prüft die Tabelle); > 1 = schwingt nicht.
inline double loopThreshold(double r) {
  static const double R[] = {0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.4, 1.7, 2.0, 2.2, 2.4}, T[] = {1.05, 0.97, 0.93, 0.89, 0.82, 0.75, 0.62, 0.46, 0.31, 0.23, 0.16};
  if (!(r > R[0])) return 2; if (r >= R[10]) return T[10]; int i = 0; while (r > R[i + 1]) i++; const double a = (r - R[i]) / (R[i + 1] - R[i]); return T[i] + a * (T[i + 1] - T[i]); }
// Reglerstellungen gleiten (höchstens st je Aufruf, Plugin und Seite: alle 32 Abtastwerte), Tasten wirken sofort; true = Einstellung geändert
inline bool glideTo(Setting &c, const Setting &t, double st = 0.01) { bool ch = false;
  auto g = [&](double &x, double y) { if (x != y) { x = std::fabs(y - x) <= st ? y : x + (y > x ? st : -st); ch = true; } };
  g(c.input, t.input); g(c.swell, t.swell); g(c.volume, t.volume); g(c.bass, t.bass); g(c.treble, t.treble); g(c.dry, t.dry);
  if (c.mode != t.mode) { c.mode = t.mode; ch = true; } for (int k = 0; k < 4; k++) { if (c.out[k] != t.out[k]) { c.out[k] = t.out[k]; ch = true; } if (c.fb[k] != t.fb[k]) { c.fb[k] = t.fb[k]; ch = true; } }
  return ch; }
inline bool sameKnobs(const Setting &a, const Setting &b) { return a.input == b.input && a.swell == b.swell && a.volume == b.volume && a.bass == b.bass && a.treble == b.treble && a.dry == b.dry; }
// Zwei Teilnetze (gleiche Rechnung, weniger Aufwand als ein gemeinsames): Aufnahme (V4A, V5A/V5B, V6A; nichtlinear) und Wiedergabe (linear: vier Kopfstufen,
// Schaltermatrix, V3A, Klangregelung, V4B). Übergabe: Abgriff TD (trocken; in der Aufnahme mit 330 k + 47 k belastet, wie am Mischpunkt fast an Masse)
// und Gitter V5B (in der Wiedergabe mit 10 nF, 470 pF, 1 M gerechnet, in der Aufnahme als Quelle; Gitterstrom von V5B dabei ohne Rückwirkung).
enum { RO_REC = 0, RO_MTR, RO_DRY, RO_NO };
enum { RU_IN = 0, RU_B1, RU_B2, RU_B3, RU_G5B, RU_NO };
enum { PO_OUT = 0, PO_G5B, PO_NO };
enum { PU_DRY = 0, PU_H0, PU_B2 = PU_H0 + 4, PU_NO };
struct Built { dk::Net net; std::vector<dk::Dev> dev; std::vector<int> grp; };

// Kleinsignalwerte einer 12AX7 am Arbeitspunkt (Ua, Ia): Steilheit, Innenwiderstand
inline void smallSignal(double va, double ia, double &gm, double &rp) {
  double lo = -5, hi = 1; for (int it = 0; it < 80; it++) { double m = 0.5 * (lo + hi), o[3]; b15::dempwolf(T12AX7, m, va, o); if (o[0] > ia) hi = m; else lo = m; }
  double o[3]; b15::dempwolf(T12AX7, 0.5 * (lo + hi), va, o); gm = o[1]; rp = 1 / o[2]; }

struct Helpers { dk::Net &o; std::vector<dk::Dev> &dev; Built &B; const Parts &v; int G = 0;
  void mark() { while (B.grp.size() < dev.size()) B.grp.push_back(G); G++; }
  int tri(int kind, const std::string &g, const std::string &p, const std::string &k) { int a = o.port(g, k); o.port(p, k); dev.push_back({kind, a, 2, nullptr}); o.c(g, k, v.cgk); o.c(g, p, v.cga); return a; }
  void pot(const std::string &a, const std::string &w, const std::string &b, double R, double x) { o.r(a, w, std::max(R * (1 - x), 1.0)); o.r(w, b, std::max(R * x, 1.0)); }
  void coil(const std::string &a, const std::string &b, double L) { dk::Mat m(1, 1); m(0, 0) = L; o.coupled({{a, b}}, m); } };

inline Built buildRec(const Parts &v, const Setting &s) {
  Built B; dk::Net &o = B.net; Helpers h{o, B.dev, B, v};
  o.src("B1", 1.0, RU_B1); o.src("B2", 1.0, RU_B2); o.src("B3", 1.0, RU_B3);
  // Eingang, V4A
  o.src("in", 1.0, RU_IN); o.r("in", "0", v.rin); o.c("in", "g4", v.cin); o.r("g4", "0", v.rg4); h.tri(K_AX7, "g4", "p4", "0"); o.r("B3", "p4", v.ra4);
  h.mark();
  o.c("p4", "p1", v.c41); h.pot("p1", "dry", "0", v.ptd, clamp01(s.dry)); o.r("dry", "0", v.rdry + v.rmx);
  o.r("p1", "n6", v.rpre); o.c("p1", "n6", v.cpre);   // 4,7 k von n6 führt zur Buchse 7 (offen)
  h.pot("n6", "iw", "0", v.pin, taperA(clamp01(s.input))); o.c("iw", "g5a", v.c5a); o.r("g5a", "0", v.rg5a);
  // Mischer V5A + V5B (Gitter V5B aus der Wiedergabe)
  h.tri(K_AX7, "g5a", "pm", "k5a"); o.r("k5a", "0", v.rk5); o.src("g5b", 1.0, RU_G5B); h.tri(K_AX7, "g5b", "pm", "k5b"); o.r("k5b", "0", v.rk5); o.r("B2", "pm", v.ra5);
  h.mark();
  // Aufnahmeverstärker V6A (ECC82), Aufnahmekopf mit 1-Ω-Messwiderstand
  o.c("pm", "r1", v.c5o); o.r("r1", "0", v.ptm); o.r("r1", "x6", v.rx1); o.c("r1", "x6", v.cx1); o.r("x6", "y6", v.rx2); o.r("y6", "0", v.rx3); o.c("y6", "0", v.cx3);
  h.tri(K_AU7, "x6", "p6", "k6"); o.r("B1", "p6", v.ra6); o.r("k6", "0", v.rk6); o.c("k6", "0", v.ck6);
  o.c("p6", "q6", v.c6o); o.r("q6", "tr", v.rrec); o.r("tr", "trl", v.rh_rec); h.coil("trl", "trs", v.lh_rec); o.r("trs", "0", 1.0);
  h.mark();
  o.out("trs"); o.out("r1"); o.out("dry");
  return B; }

inline Built buildPb(const Parts &v, const Setting &s) {
  Built B; dk::Net &o = B.net; Helpers h{o, B.dev, B, v};
  auto sw = [&](const std::string &a, const std::string &b, bool on) { o.r(a, b, on ? v.ron : v.roff); };
  o.src("dry", 1.0, PU_DRY);
  // Wiedergabe: vier gleiche Stufen, linear (Pegel im Millivoltbereich): Steilheit und Innenwiderstand am Arbeitspunkt
  double gm, rp; smallSignal(v.vapb, v.iapb, gm, rp);
  for (int k = 0; k < 4; k++) { std::string K = std::to_string(k), ha = "ha" + K, hb = "hb" + K, g = "pg" + K, p = "pp" + K, hh = "h" + K;
    o.src(ha, v.rh_pb, PU_H0 + k); h.coil(ha, hb, v.lh_pb); o.c(hb, "0", v.ch_pb); o.c(hb, g, v.cpb); o.r(g, "0", v.rgpb);
    o.vccs(p, "0", g, "0", gm); o.r(p, "0", rp); o.c(g, p, v.cga); o.r(p, "0", v.rapb);   // Anodenwiderstand nach B3 = für Wechselstrom an Masse
    o.c(p, "ts" + K, v.ctsc); o.r("ts" + K, "0", std::max(v.ptsc * v.tsc, 1.0)); o.c(p, "0", v.cpbs); o.c(p, hh, v.cpbo);
    h.pot(hh, "sw" + K, "0", v.ptsw, v.tsw); o.r("sw" + K, "W", v.rsw);
    o.r(hh, "T", s.fb[k] ? v.rhead : v.roff); o.r(hh, "B", s.out[k] ? v.rhead : v.roff); }
  // Schaltermatrix, Betriebsart
  o.r("T", "N", v.rtn); o.r("N", "0", v.rn); o.r("B", "X", v.rbx); o.r("X", "0", v.rx); o.r("G", "0", v.rg3);
  sw("G", "X", s.mode != M_SWELL); sw("G", "W", s.mode == M_SWELL); sw("F", "N", s.mode != M_ECHO); o.r("W", "0", v.roff); o.r("F", "0", v.roff);
  // Rückkopplung: F → TF (Trimmer) → LENGTH OF SWELL → 10 nF → Gitter V5B (470 pF, 1 M)
  h.pot("F", "lt", "0", v.ptf, v.tf); h.pot("lt", "lw", "lb", v.plos, clamp01(s.swell)); o.r("lb", "0", v.rlos);
  o.c("lw", "g5b", v.c5b); o.c("g5b", "0", v.cg5b); o.r("g5b", "0", v.rg5b);
  // V3A als Röhre (nichtlinear): bei gesättigtem Draht und vor allem bei schwingender Schleife (hohe Frequenzen, Ableitung) übersteuert ihr Gitter;
  // sie begrenzt dann den Echopegel wie im Gerät. Danach Klangregelung, VOLUME. V4B linear (Kathodenfolger, Kleinsignal am Arbeitspunkt).
  o.src("B2p", 1.0, PU_B2); h.tri(K_AX7, "G", "p3", "k3"); o.r("B2p", "p3", v.ra3); o.r("k3", "0", v.rk3); o.c("k3", "0", v.ck3); o.c("p3", "t1", v.c3o); h.mark();
  o.r("t1", "b4", v.rbs1); h.pot("b4", "bw", "b5", v.pbass, clamp01(s.bass)); o.r("b5", "0", v.rbs2); o.c("b4", "bw", v.cb1); o.c("bw", "b5", v.cb2); o.r("bw", "vo", v.rbo);   // Rechtsanschlag (1) = mehr Tiefen (geprüft: wisenttest tone)
  o.c("t1", "tt", v.ctr1); h.pot("tt", "tw", "tb", v.ptreb, clamp01(s.treble)); o.c("tb", "0", v.ctr2); o.r("tw", "vo", 1.0);
  h.pot("vo", "vw", "0", v.pvol, taperA(clamp01(s.volume)));
  // V4B: Mischung trocken + Echo, Kathodenfolger
  o.r("vw", "mx", v.rmix); o.r("dry", "mx", v.rdry); o.r("mx", "0", v.rmx); o.c("mx", "g4b", v.cmx); o.c("g4b", "0", v.cg4b); o.r("g4b", "bs", v.rgl4b);
  { double g4, r4; smallSignal(v.va4b, v.ia4b, g4, r4); o.vccs("0", "k4b", "g4b", "k4b", g4); o.r("0", "k4b", r4); o.c("g4b", "k4b", v.cgk); o.c("g4b", "0", v.cga); }
  o.r("k4b", "bs", v.rk4b); o.r("bs", "0", v.rb4b); o.c("k4b", "out", v.cout); o.r("out", "0", v.rout);
  o.out("out"); o.out("g5b");
  return B; }


// ---- 2-fach-Überabtastung mit IIR-Halbbändern (zwei Allpassketten, Polyphasen; Entwurf nach dem elliptischen Verfahren von L. de Soras, „hiir“) ----
// Keine Vorschwinger, kleine Gruppenlaufzeit; Sperrdämpfung und Laufzeit prüft wisenttest os.
struct HBCoef { std::vector<double> c;
  explicit HBCoef(int n = 8, double tbw = 0.04) {
    double k = std::tan((1 - 2 * tbw) * PI / 4); k *= k; const double ks = std::pow(1 - k * k, 0.25), e = 0.5 * (1 - ks) / (1 + ks), e4 = e * e * e * e;
    const double q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4))); const int order = 2 * n + 1; c.resize(n);
    for (int idx = 0; idx < n; idx++) { const int cc = idx + 1; double num = 0, den = 0;
      { int i = 0, j = 1; double t; do { t = std::pow(q, i * (i + 1)) * std::sin((2 * i + 1) * cc * PI / order) * j; num += t; j = -j; i++; } while (std::fabs(t) > 1e-100); num *= std::pow(q, 0.25); }
      { int i = 1, j = -1; double t; do { t = std::pow(q, i * i) * std::cos(2 * i * cc * PI / order) * j; den += t; j = -j; i++; } while (std::fabs(t) > 1e-100); den += 0.5; }
      double ww = num / den; ww *= ww; const double x = std::sqrt((1 - ww * k) * (1 - ww / k)) / (1 + ww); c[idx] = (1 - x) / (1 + x); } } };
inline const HBCoef &hbCoef() { static const HBCoef h; return h; }
struct Up2 { double x[8] = {}, y[8] = {};   // ein Eingangswert → zwei Werte der doppelten Rate
  void reset() { for (int i = 0; i < 8; i++) x[i] = y[i] = 0; }
  void run(double in, double &o0, double &o1) { const std::vector<double> &c = hbCoef().c; double ev = in, od = in;
    for (int i = 0; i < 8; i += 2) { double t0 = (ev - y[i]) * c[i] + x[i], t1 = (od - y[i + 1]) * c[i + 1] + x[i + 1]; x[i] = ev; x[i + 1] = od; y[i] = t0; y[i + 1] = t1; ev = t0; od = t1; }
    o0 = ev; o1 = od; } };
struct Down2 { double x[8] = {}, y[8] = {};   // zwei Werte der doppelten Rate → ein Ausgangswert
  void reset() { for (int i = 0; i < 8; i++) x[i] = y[i] = 0; }
  double run(double i0, double i1) { const std::vector<double> &c = hbCoef().c; double s0 = i1, s1 = i0;
    for (int i = 0; i < 8; i += 2) { double t0 = (s0 - y[i]) * c[i] + x[i], t1 = (s1 - y[i + 1]) * c[i + 1] + x[i + 1]; x[i] = s0; x[i + 1] = s1; y[i] = t0; y[i + 1] = t1; s0 = t0; s1 = t1; }
    return 0.5 * (s0 + s1); } };
// ---- Trommel: Stahldraht, Aufnahme mit Vormagnetisierung (Sättigung), vier Wiedergabeköpfe, Löschmagnet ----
struct Drum {
  double fs = 48000, v = 0.457, circ = 0.375, gap = 34e-3;   // Bandgeschwindigkeit (m/s), Umfang (m), Kopfabstand (m)
  double dsp = 31e-6, gpb = 5e-6, fRec = 12000, isat = 1, eraseRes = 0.0032, hiss = 0, wowA = 0.0005, kh = 1;   // Abstand Kopf–Draht, Spalt, Aufnahme-Tiefpass, Sättigung, Löschrest −50 dB, Rauschen, Gleichlauf, Kopfempfindlichkeit
  // Spur: buf = Magnetisierung (für den Löschrest), fbuf = daraus die Kopfspannung (FIR beim Schreiben; die vier Köpfe lesen dieselbe Spur verzögert)
  std::vector<float> buf, fbuf; int N = 0, w = 0; double lp = 0, aLp = 0, ph = 0; uint32_t rng = 22222;
  // Wiedergabe-FIR: d/dt × Abstandsverlust × Spaltverlust, linearphasig, ±TW lang (Tukey-Fenster); gleiche Dauer bei jeder Rate
  static constexpr double TW = 1.2e-3; int nf = 0, M = 0; std::vector<double> fir; std::vector<std::vector<double>> firT;
  double tau1 = 0.074, trev = 0.82;
  double v50 = 0.457, vT = 0.457, aV = 0;
  // Geschwindigkeiten: Netz 50/60 Hz mal Varispeed 0,25 … 2 (Umbau, nicht am PE 603 T). Wiedergabe-FIRs auf einem Raster v50 · √1,2^i, i = −16 … +10
  // (0,23 … 2,49 · v50; 50 und 60 Hz liegen genau auf dem Raster), dazwischen linear gemischt.
  static constexpr int NT = 27, I0 = 16; static constexpr double RS = 1.0954451150103321;
  static double vMin(double v50) { return v50 * std::pow(RS, -I0); } static double vMax(double v50) { return v50 * std::pow(RS, NT - 1 - I0); }
  // Trommel einrichten (Puffer für die langsamste Geschwindigkeit), Startdrehzahl nach hz und vs
  void setup(double fs_, double hz, double vs = 1) {
    fs = fs_; aV = 1 - std::exp(-1 / (0.4 * fs)); N = (int)std::ceil((circ / vMin(v50) + 0.05) * fs) + 8; buf.assign(N, 0.f); fbuf.assign(N, 0.f); w = 0; lp = 0; ph = 0;
    M = (int)std::ceil(TW * fs); nf = 2 * M + 1; fir.assign(nf, 0.0); firT.assign(NT, {});
    const double g50 = design(v50, firT[I0], 0); for (int i = 0; i < NT; i++) if (i != I0) design(v50 * std::pow(RS, i - I0), firT[i], g50);
    speed(hz, vs, true); }
  // Drehzahl: Spaltpolmotor folgt der Netzfrequenz, Varispeed vs (Umbau); ohne „now“ gleitet sie in etwa 0,4 s (Schwungmasse)
  void speed(double hz, double vs = 1, bool now = false) { vT = std::min(vMax(v50), std::max(vMin(v50), v50 * hz / 50 * vs)); if (now) { v = vT; mixFir(); } }
  // FIR, Aufnahme-Tiefpass (Selbstlöschung durch die Vormagnetisierung: hängt an der Wellenlänge, also proportional zur Geschwindigkeit) und Zeiten zur Geschwindigkeit v
  void mixFir() { double x = std::log(v / v50) / std::log(RS) + I0; x = std::min((double)NT - 1, std::max(0.0, x)); int i = std::min(NT - 2, (int)x); double a = x - i;
    const double *f0 = firT[i].data(), *f1 = firT[i + 1].data(); for (int n = 0; n < nf; n++) fir[n] = (1 - a) * f0[n] + a * f1[n];
    aLp = 1 - std::exp(-2 * PI * fRec * (v / v50) / fs); tau1 = gap / v; trev = circ / v; }
  // Entwurf für Bandgeschwindigkeit vv: H(f) = j·2πf · e^{−2π·d·f/v} · sinc(g·f/v) (Frequenzabtastung bis fs/2 im Abstand ≤ 25 Hz, Tukey-Fenster 50 %).
  // Abweichung vom Ziel (Prüfung wisenttest fir): 50 Hz −0,7 dB, ab 300 Hz unter 0,15 dB.
  // Rückgabe: Normierungsfaktor; g > 0 übernimmt ihn (60 Hz mit dem von 50 Hz: bei höherer Geschwindigkeit weniger Abstandsverlust, +0,6 dB bei 1 kHz)
  double design(double vv, std::vector<double> &out, double g) {
    out.assign(nf, 0.0); const int K = std::max(512, (int)std::ceil(0.5 * fs / 25)); const double df = 0.5 * fs / K;
    for (int k = 1; k < K; k++) { const double f = df * k, u = PI * gpb * f / vv; double a = 2 * PI * f * std::exp(-2 * PI * dsp * f / vv) * (u > 1e-9 ? std::sin(u) / u : 1);
      const double th = 2 * PI * f / fs, c2 = 2 * std::cos(th); double s0 = 0, s1 = std::sin(th);   // sin(m·θ) für m = 0, 1, 2, … (Rekursion)
      for (int m = 1; m <= M; m++) { out[M + m] -= a * s1; out[M - m] += a * s1; const double s2 = c2 * s1 - s0; s0 = s1; s1 = s2; } }   // −sin: +j (Ableitung, positiv)
    for (int n = 0; n < nf; n++) { const double x = std::fabs((double)(n - M)) / (M + 1); out[n] *= x < 0.5 ? 1.0 : 0.5 + 0.5 * std::cos(PI * (x - 0.5) / 0.5); }
    // auf die Ableitung bei 1 kHz normieren (Kopfempfindlichkeit kh gilt bei 1 kHz, bei 50 Hz Netzfrequenz)
    if (g <= 0) { std::complex<double> H = 0; for (int n = 0; n < nf; n++) H += out[n] * std::polar(1.0, -2 * PI * 1000.0 * (n - M) / fs); g = std::abs(H) / (2 * PI * 1000); }
    if (g > 0) for (double &x : out) x /= g; return g; }
  std::complex<double> firH(double f) const { std::complex<double> H = 0; for (int n = 0; n < nf; n++) H += fir[n] * std::polar(1.0, -2 * PI * f * (n - M) / fs); return H; }
  void clear() { std::fill(buf.begin(), buf.end(), 0.f); std::fill(fbuf.begin(), fbuf.end(), 0.f); lp = 0; }
  // Strom im Aufnahmekopf (A) einschreiben
  void write(double i) {
    lp += aLp * (i - lp); double m = isat * std::tanh(lp / isat);
    int old = w - (int)std::lround(trev * fs); while (old < 0) old += N; m += eraseRes * buf[old];   // Dauermagnet löscht nicht vollständig: Rest der vorigen Umdrehung
    if (hiss > 0) { rng = rng * 1664525u + 1013904223u; m += hiss * ((rng >> 8) / 16777216.0 - 0.5); }
    buf[w] = (float)m;
    // Kopfspannung (je Sekunde; FIR bei 1 kHz auf 2π·1000 normiert), M Abtastwerte verzögert
    double y = 0; const int n1 = std::min(nf, w + 1); const float *b = buf.data();
    for (int n = 0; n < n1; n++) y += fir[n] * b[w - n];
    for (int n = n1; n < nf; n++) y += fir[n] * b[w - n + N];
    fbuf[w] = (float)(kh * y); if (++w == N) w = 0; }
  // Kopfspannungen für den nächsten Abtastwert (Gleichlauf: Exzentrizität der Trommel, einmal je Umdrehung)
  void read(double *emf) {
    if (v != vT) { v += aV * (vT - v); if (std::fabs(v - vT) < 1e-7) v = vT; mixFir(); }
    ph += 1 / (trev * fs); if (ph >= 1) ph -= 1; const double wow = wowA * std::sin(2 * PI * ph);
    for (int k = 0; k < 4; k++) { double d = (k + 1) * tau1 * (1 + wow) * fs - M; int di = (int)d; double fr = d - di;
      auto at = [&](int j) { int q = w - 1 - j; while (q < 0) q += N; return (double)fbuf[q]; };
      double ym1 = at(di - 1), y0 = at(di), y1 = at(di + 1), y2 = at(di + 2);   // kubisch (Hermite)
      double c1 = 0.5 * (y1 - ym1), c2 = ym1 - 2.5 * y0 + 2 * y1 - 0.5 * y2, c3 = 0.5 * (y2 - ym1) + 1.5 * (y0 - y1); emf[k] = ((c3 * fr + c2) * fr + c1) * fr + y0; } }
};

// ---- ganzes Gerät ----
struct Net1 { dk::Net net; dk::Runner run; dk::Compiler comp; Built bl;
  void apply(const Built &b, double fs) { bl = b; net = bl.net; run.dev = bl.dev; run.devGrp = bl.grp; run.fn = devFn; run.set(comp.compile(net, fs)); }
  void fresh() { comp = dk::Compiler(); run = dk::Runner(); run.dampFull = true; run.reuseStep = true; run.reuseRho = 0.1; }
  std::complex<double> response(double fs, double f, int in, const std::string &a) { (void)fs; std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int x = 0; x < d.np; x++) for (int c = 0; c < d.np; c++) b[x * d.np + c] = run.Jd[(size_t)(d.p0 + x) * 16 + c]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    return dk::acResponse(net, J, dp, f, in, net.idx.at(a), 0); }
  // Kondensatorspannungen auf eine andere Abtastrate übertragen (Spulen ohne Gleichstrom)
  void carry(const Net1 &src, double fsSrc, const std::vector<double> &u) { dk::Vec V = dk::nodeVoltages(src.net, fsSrc, src.run.h, u, src.run.ip);
    for (size_t k = 0; k < net.C.size(); k++) run.h[k] = -run.s.gc[k] * (V[net.C[k].a] - V[net.C[k].b]); for (size_t k = net.C.size(); k < run.h.size(); k++) run.h[k] = 0;
    run.vp = src.run.vp; run.vp1 = src.run.vp; }
};
struct Core {
  // Aufnahmeseite (Röhren, Sättigung des Drahts) und Trommel mit doppelter Rate, Wiedergabe mit einfacher; Gruppenlaufzeit der Halbbänder: LAT Abtastwerte.
  // Ab 88,2 kHz rechnet die Aufnahmeseite mit der einfachen Rate (os = 1, keine Halbbänder): die Rate liegt dann schon über der doppelten von 44,1/48 kHz.
  // Ab 176,4 kHz rechnet das ganze Gerät mit der halben, ab 352,8 kHz mit einem Viertel der Rate (dec; Halbbänder hinunter und hinauf, tick statt step).
  // lat: Latenz in Abtastwerten der Plugin-Rate (wisenttest lat misst sie).
  static constexpr int LAT = 3; int os = 2, lat = LAT, dec = 1;
  Down2 dIn[2]; Up2 uOut[2]; int dph = 0; double dx[4] = {}, dq[4] = {};
  double fs = 48000, fs2 = 96000, vin = 0.2, gOut = 1, kRec = 1, hz = 50, vs = 1; Parts v; Setting cur; Net1 rec, pb; Drum drum; double meter = 0, emf[4] = {}, g5b = 0, dry = 0;
  Up2 upX, upG; Down2 dnDry, dnE[4]; bool drumOn = true; double eye = 0, aEyeA = 0, aEyeR = 0;   // Anzeige (EM84): Spitzenwert des Aufnahmestroms je Sättigung
  void setHz(double h) { setSpeed(h, vs); }
  // Netzfrequenz (50/60 Hz) und Varispeed (0,25 … 2, Umbau); die Trommel gleitet in etwa 0,4 s zur neuen Drehzahl
  void setSpeed(double h, double x) { x = std::isfinite(x) ? std::min(2.0, std::max(0.25, x)) : 1.0; if (h != hz || x != vs) { hz = h; vs = x; drum.speed(hz, vs); } }
  void apply(const Setting &s) { cur = s; rec.apply(buildRec(v, s), fs2); pb.apply(buildPb(v, s), fs); }
  void stepRec(double x, double g, double ramp, double *yr) { double ur[RU_NO] = {x, v.b1 * ramp, v.b2 * ramp, v.b3 * ramp, g}; rec.run.step(ur, yr); }
  void stepPb(double *yp, double ramp = 1.0) { double up[PU_NO] = {dry, emf[0], emf[1], emf[2], emf[3], v.b2 * ramp}; pb.run.step(up, yp); g5b = yp[PO_G5B]; }
  // fs: Abtastrate, hz: Netzfrequenz (Trommeldrehzahl), vin: Spannung bei 0 dBFS, vs: Varispeed
  // Abgleich des Ausgangs mit dem Trimmer TD auf dem Planwert (v.td); die Stellung aus s.dry gilt danach
  void init(double fsOuter, const Setting &s0, double hz_ = 50, double vin_ = 0.2, double vs_ = 1) {
    Setting s = s0; s.dry = v.td;
    dec = fsOuter >= 352800 ? 4 : fsOuter >= 176400 ? 2 : 1; const double fs_ = fsOuter / dec; dph = 0; for (int i = 0; i < 4; i++) dx[i] = dq[i] = 0; for (auto &d : dIn) d.reset(); for (auto &u : uOut) u.reset();
    hz = hz_; vin = vin_; os = fs_ < 88000 ? 2 : 1; lat = os == 2 ? LAT : dec == 1 ? 0 : dec == 2 ? DLAT2 : DLAT4; vs = std::isfinite(vs_) ? std::min(2.0, std::max(0.25, vs_)) : 1.0; drum.setup(os * fs_, hz, vs); for (double &e : emf) e = 0; g5b = dry = 0;
    // Einschwingen mit 8 kHz (beide Teilnetze; langsame Glieder: gebootstrappte Gitterableitung V4B, Koppelkondensatoren), Anodenspannungen in 0,2 s
    // hochfahren, dann die Kondensatorspannungen auf die eigentlichen Abtastraten übertragen
    fs = fs2 = 8000; rec.fresh(); pb.fresh(); apply(s);
    double yr[RO_NO], yp[PO_NO]; const long nr = (long)(0.2 * fs), nt = (long)(4.0 * fs);
    for (long i = 0; i < nt; i++) { const double rp = i < nr ? (double)i / nr : 1.0; stepRec(0, g5b, rp, yr); dry = yr[RO_DRY]; stepPb(yp, rp); }
    { Net1 r0 = rec, p0 = pb; double fsl = fs; fs = fs_; fs2 = os * fs_; rec.fresh(); pb.fresh(); apply(s);
      rec.carry(r0, fsl, {0, v.b1, v.b2, v.b3, g5b}); pb.carry(p0, fsl, {dry, 0, 0, 0, 0, v.b2}); }
    upX.reset(); upG.reset(); dnDry.reset(); for (auto &d : dnE) d.reset();
    for (double &d : upG.x) d = g5b; for (double &d : upG.y) d = g5b; for (double &d : dnDry.x) d = dry; for (double &d : dnDry.y) d = dry;
    drumOn = false; for (long i = 0; i < (long)(0.05 * fs); i++) step(0); drumOn = true;
    // Abgleich (Planangaben): 1 V an der Anode V6A bei 1 kHz ergibt 0,5 V am Wiedergabeausgang H; Ausgang so, dass trocken = Eingang (1 kHz)
    std::complex<double> ir = rec.response(fs2, 1000, RU_IN, "trs"), vp = rec.response(fs2, 1000, RU_IN, "p6"), hh = pb.response(fs, 1000, PU_H0, "h0");
    double ipv = std::abs(ir / vp), lp = 1 / std::sqrt(1 + (1000 / drum.fRec) * (1000 / drum.fRec));
    drum.kh = 0.5 / (std::abs(hh) * 2 * PI * 1000 * ipv * lp); drum.isat = 3 * std::sqrt(2.0) * ipv; drum.hiss = 4e-5 * drum.isat;
    kRec = ipv; gOut = 1 / std::abs(rec.response(fs2, 1000, RU_IN, "dry") * pb.response(fs, 1000, PU_DRY, "out")); drum.clear();
    aEyeA = 1 - std::exp(-1 / (0.01 * fs)); aEyeR = 1 - std::exp(-1 / (0.1 * fs)); eye = 0;
    if (s0.dry != s.dry) apply(s0); }   // OA81 lädt 100 nF über 470 k (schnell), 1 M entlädt (0,1 s)
  // ein Abtastwert der Plugin-Rate (bei dec > 1 gesammelt, dec Werte später gerechnet)
  static constexpr int DLAT2 = 8, DLAT4 = 22;   // gemessen (wisenttest lat): Sammeln + Halbbänder
  double tick(double x) {
    if (dec == 1) return step(x);
    dx[dph] = x; const double y = dq[dph]; if (++dph < dec) return y; dph = 0;
    if (dec == 2) { const double yi = step(dIn[0].run(dx[0], dx[1])); uOut[0].run(yi, dq[0], dq[1]); return y; }
    const double a = dIn[0].run(dx[0], dx[1]), b = dIn[0].run(dx[2], dx[3]), yi = step(dIn[1].run(a, b)); double u0, u1; uOut[1].run(yi, u0, u1);
    uOut[0].run(u0, dq[0], dq[1]); uOut[0].run(u1, dq[2], dq[3]); return y; }
  // ein Abtastwert der inneren Rate fs (x in Einheiten von vin), Rückgabe Ausgang (dieselben Einheiten)
  double step(double x) {
    if (os == 1) { double yr[RO_NO], yp[PO_NO]; if (drumOn) drum.read(emf); else for (double &e : emf) e = 0; stepRec(x * vin, g5b, 1.0, yr); if (drumOn) drum.write(yr[RO_REC]);
      dry = yr[RO_DRY]; meter = yr[RO_MTR]; double t = std::fabs(yr[RO_REC]) / drum.isat; eye += (t > eye ? aEyeA : aEyeR) * (t - eye); stepPb(yp); return yp[PO_OUT] * gOut / vin; }
    double xs[2], gs[2], dr[2], e2[2][4], yr[RO_NO], yp[PO_NO]; upX.run(x * vin, xs[0], xs[1]); upG.run(g5b, gs[0], gs[1]);
    double ir = 0; for (int j = 0; j < 2; j++) { if (drumOn) drum.read(e2[j]); else for (double &e : e2[j]) e = 0; stepRec(xs[j], gs[j], 1.0, yr); if (drumOn) drum.write(yr[RO_REC]); dr[j] = yr[RO_DRY]; meter = yr[RO_MTR]; ir = std::max(ir, std::fabs(yr[RO_REC])); }
    { double t = ir / drum.isat; eye += (t > eye ? aEyeA : aEyeR) * (t - eye); }
    dry = dnDry.run(dr[0], dr[1]); for (int k = 0; k < 4; k++) emf[k] = dnE[k].run(e2[0][k], e2[1][k]);
    stepPb(yp); return yp[PO_OUT] * gOut / vin; }
  // ---- Kleinsignal im Arbeitspunkt (Anzeigen der Seite, Prüfung) ----
  // Trommel: Strom im Aufnahmekopf (A) → Spannung am Wiedergabekopf (V) bei f, ohne Sättigung und Verzögerung (Aufnahme-Tiefpass, Wiedergabe-FIR, Kopfempfindlichkeit)
  std::complex<double> drumH(double f) const { const double w = 2 * PI * f / drum.fs, a = drum.aLp; return drum.kh * drum.firH(f) * a / (1.0 - (1 - a) * std::polar(1.0, -w)); }
  // Übertragung je Volt am Eingang (Ausgang in denselben Einheiten wie step): trocken; Aufnahmestrom; über Kopf k zum Ausgang (Echo); über Kopf k zurück in den Aufnahmestrom (Schleife)
  struct Gains { std::complex<double> dry, rec, echo, loop; };
  Gains gains(double f, int k) { Gains g; std::complex<double> d = drumH(f); g.rec = rec.response(fs2, f, RU_IN, "trs"); g.dry = rec.response(fs2, f, RU_IN, "dry") * pb.response(fs, f, PU_DRY, "out") * gOut;
    g.echo = g.rec * d * pb.response(fs, f, PU_H0 + k, "out") * gOut; g.loop = d * pb.response(fs, f, PU_H0 + k, "g5b") * rec.response(fs2, f, RU_G5B, "trs"); return g; }
  // Echomuster für einen kurzen Ton der Frequenz f (Kleinsignal): Pegel (Betrag je Eingang) zur Zeit n·τ1 für n = 0 … N−1 (n = 0: trocken)
  void pattern(double f, int N, double *out) { std::complex<double> E[4], Lp[4]; Gains g0 = gains(f, 0); for (int k = 0; k < 4; k++) { Gains g = k ? gains(f, k) : g0; E[k] = g.echo / g.rec; Lp[k] = g.loop; }
    std::vector<std::complex<double>> r(N, 0.0); r[0] = g0.rec;
    for (int n = 0; n < N; n++) { if (n) for (int k = 0; k < 4 && k < n; k++) r[n] += Lp[k] * r[n - k - 1];
      std::complex<double> o = n ? 0.0 : g0.dry; for (int k = 0; k < 4 && k < n; k++) o += E[k] * r[n - k - 1]; out[n] = std::abs(o); } }
};
}  // namespace wisent
