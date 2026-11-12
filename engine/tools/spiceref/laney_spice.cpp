// Gegenrechnung des Laney-AOR-50-Vorverstärkers mit SPICE (ngspice). Der Laney-Kern (core.hpp) ist älter als dk.hpp und rechnet die Vorstufe
// nicht als ein Netz, sondern als Kette von fünf Abschnitten mit Vereinfachungen. Die Netzliste hier ist deshalb von Hand als EIN Netz geschrieben
// (Bauteilwerte aus Parts/Setting, Röhrengleichungen wie im Kern) und zeigt, was die Vereinfachungen kosten:
//   1. Abschnitte: Gitterspannung der Folgestufe wird weitergereicht, deren Kathodenspannung für den Gitterstrom stammt vom vorigen Abtastwert
//   2. Gitter-Anoden-Kapazität der Stufen 1–4 als feste Miller-Ersatzkapazität gegen Masse (CMg1…4) statt zwischen Gitter und Anode
//   3. Überabtastung mit FIR-Filtern statt beliebig feiner Zeitschritte
//
//   laney_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [name=wert …]
//     eingang.f32: mono, 32 Bit Gleitkomma (für SPICE und die Knotenrechnung; am besten vorher bandbegrenzt hochgetastet), VIN = Volt bei 1,0
//     Einstellungen: v1 v2 pl t m b mv (0…1), mid top bass ch1 low soft bmod late (0/1), tube=N (alle Fassungen) oder tube1/2/3
//     plug=datei.f32 fsp=44100: dieselbe Aufnahme in der Abtastrate des Plugins; gerechnet wird wie im Plugin mit 1x, 2x, 4x, 8x
//   schreibt  laney_m.cir  Netzliste mit Miller-Ersatzkapazitäten (wie der Kern)          → spice_m.txt
//             laney_p.cir  Netzliste mit Cgk und Cga an den Röhren (wie im Gerät)          → spice_p.txt
//             laney_k.cir  wie laney_p, aber alle Röhren als 12AX7 nach Koren                    → spice_k.txt
//             kern.txt     Kern, Abtastrate FS_KERN ohne Überabtastungsfilter, alle Knoten
//             plug_N.txt   Kern wie im Plugin (Abtastrate fsp, N-fache Überabtastung): Ausgang W3 ohne Gleichanteil, Latenz schon abgezogen
// Die Versorgungsspannungen der Vorstufe sind auf beiden Seiten fest (240/252/252/260 V), ohne Endstufe.
#include "core.hpp"
#include "dk_spice.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace aor;
static const double FS_KERN = 192000;
static const char *PROBE[] = {"g1", "p1", "k1", "g2", "p2", "k2", "g3", "p3", "k3", "g4", "p4", "k4", "k5", "g6", "p6", "k6", "W3"};
static const int NPROBE = sizeof PROBE / sizeof *PROBE;
static std::string n(double v) { return dkspice::num(v); }

struct Cir { std::string s; int k = 0; bool koren = false;
  void r(const std::string &a, const std::string &b, double rr) { s += "R" + std::to_string(k++) + " " + a + " " + b + " " + n(std::max(rr, 1e-3)) + "\n"; }
  void c(const std::string &a, const std::string &b, double cc) { s += "C" + std::to_string(k++) + " " + a + " " + b + " " + n(cc) + "\n"; }
  void rcs(const std::string &a, const std::string &b, double rr, double cc) { std::string m = "rc" + std::to_string(k); r(a, m, rr); c(m, b, cc); }
  void tri(int t, const std::string &g, const std::string &p, const std::string &kk) { std::string id = std::to_string(k++), T = std::to_string(t);
    if (koren) { s += "RG" + id + " " + g + " gx" + id + " 2000\nDG" + id + " gx" + id + " " + kk + " DX\nBA" + id + " " + p + " " + kk + " I = kor(V(" + g + "," + kk + "), V(" + p + "," + kk + "))\n"; c(p, kk, 0.9e-12); return; }
    s += "BG" + id + " " + g + " " + kk + " I = gi(V(" + g + "," + kk + "))\n"; s += "BA" + id + " " + p + " " + kk + " I = tria" + T + "(V(" + g + "," + kk + "), V(" + p + "," + kk + "))\n"; } };

// die ganze Vorstufe als ein Netz; miller: Ersatzkapazitäten wie im Kern, sonst Cgk/Cga an jeder Röhre
// koren: 12AX7 nach Koren (1996) so, wie das Modell in LTspice-Bibliotheken verbreitet ist (Gitter: Diode hinter 2 kΩ; Cgk 2,3 pF, Cga 2,4 pF, Cak 0,9 pF)
static std::string circuit(const Parts &v, const Setting &s, bool miller, const std::string &pwl, const std::string &ttl, bool koren = false) {
  Cir o; o.koren = koren; const ModCaps mc = modCaps(v); const int *tb = s.tube; bool low = s.low > 0.5;
  auto cap = [&](int sock, bool gk) { if (koren) return gk ? 2.3e-12 : 2.4e-12; const TubeModel &T = TUBE_MODELS[tb[sock]]; return stockTubes(s) ? (gk ? v.cgk : v.cga) : (gk ? T.cgk : T.cga); };
  auto stage = [&](int sock, const std::string &g, const std::string &p, const std::string &k, double cm) { o.tri(tb[sock], g, p, k);
    if (miller) o.c(g, "0", cm); else { o.c(g, k, cap(sock, true)); o.c(g, p, cap(sock, false)); } };
  std::string head = "* " + ttl + "\n";
  head += ".func sp(x) {ln(1 + exp(max(min(x, 35), -60))) + max(x - 35, 0)}\n";
  head += ".func gi(v) {" + n(Tube::GG) + " * pwr(sp(" + n(Tube::CG) + " * v) / " + n(Tube::CG) + ", " + n(Tube::XI) + ")}\n";
  for (int t = 0; t < NTUBES; t++) { const TubeModel &T = TUBE_MODELS[t]; if (T.form != 1) { std::printf("Röhre %d nicht in Dempwolf-Form\n", t); std::exit(1); }
    head += ".func tria" + std::to_string(t) + "(vg, vp) {" + n(T.G) + " * pwr(sp(" + n(T.C) + " * (vp / " + n(T.mu) + " + vg)) / " + n(T.C) + ", " + n(T.ga) + ") * (1 - exp(-max(vp, 0)))}\n"; }
  head += ".func kor(vg, vp) {2 * pwr(max(vp / 600 * sp(600 * (1 / 100 + vg / sqrt(300 + vp * vp))), 0), 1.4) / 1060}\n.model DX D(IS=1n RS=1 CJO=10p TT=1n)\n";
  head += "VIN src 0 " + pwl + "\nVB1 B1 0 DC " + n(UB[0]) + "\nVB2 B2 0 DC " + n(UB[1]) + "\nVB3 B3 0 DC " + n(UB[2]) + "\nVB4 B4 0 DC " + n(UB[3]) + "\n";
  // Eingang und V101A
  if (low) { o.r("g1", "0", v.r1); o.r("src", "t2", 1e-3); } else o.r("src", "g1", v.r1);
  stage(0, "g1", "p1", "k1", CMg1); o.r("B1", "p1", 100e3); o.r("k1", "0", 2.7e3); o.c("k1", "0", 47e-6);
  o.c("p1", "n5", mc.c2); o.rcs("p1", "n5", swR(s.bmod, mc.c2x), mc.c2x); o.r("n5", "t1", v.r5);
  o.r("t1", "0", s.ch1 <= 0 ? v.ronvt + v.r6 : s.ch1 >= 1 ? 1e12 : blendR(v.ronvt + v.r6, 1e12, 1 - s.ch1, 5e4));
  o.r("t1", "g2", v.vr101 * (1 - s.v1)); o.r("g2", "0", v.vr101 * s.v1);
  // V101B
  stage(0, "g2", "p2", "k2", CMg2); o.r("B2", "p2", 100e3); o.r("k2", "0", 2.7e3); o.c("k2", "0", mc.c3s);
  o.rcs("k2", "0", swR(1 - s.soft, mc.c3a), mc.c3a); o.rcs("k2", "0", swR((1 - s.soft) * s.bmod, mc.c3x), mc.c3x);
  if (!low) { o.c("p2", "t2p", v.ck); o.r("t2p", "t2", lateR(v, s)); } else o.c("p2", "0", v.ck);
  o.r("t2", "w2", v.vr102 * (1 - s.v2)); o.r("w2", "0", v.vr102 * s.v2); o.r("w2", "g3", 68e3);
  // V102A
  stage(1, "g3", "p3", "k3", CMg3); o.r("B3", "p3", 100e3); o.r("k3", "0", 10e3);
  o.c("p3", "n12", v.c4); o.r("n12", "g4", 470e3); o.c("n12", "g4", v.c5); o.r("g4", "0", 470e3);
  // V102B, Kathodenfolger V103A, Klangregelung, V103B
  stage(1, "g4", "p4", "k4", CMg4); o.r("B4", "p4", 100e3); o.r("k4", "0", 820);
  o.tri(tb[2], "p4", "B4", "k5"); o.r("k5", "0", 100e3); o.c("p4", "0", cap(2, false)); o.c("p4", "k5", cap(2, true));
  o.c("k5", "X", v.c6); o.c("X", "Z", v.c7); o.rcs("X", "Z", s.mid >= 1 ? 1e-3 : s.mid <= 0 ? 10e6 : blendR(1e-3, 10e6, s.mid, 1 / (2 * PI * 1000 * v.c8)), v.c8);
  o.r("X", "Y", v.r18); o.c("Y", "W", v.c11);
  o.r("Z", "T", v.vr105 * (1 - s.t)); o.r("T", "W", v.vr105 * s.t); o.r("W", "P", v.vr106 * s.b);
  o.r("P", "Mw", v.vr107 * (1 - s.m)); o.r("Mw", "0", v.vr107 * s.m); o.c("Y", "Mw", v.c10);
  o.c("T", "n19", v.c9); o.r("n19", "g6", 1e6); o.r("g6", "0", 1e6); o.r("g6", "n20", 3.3e6); o.c("n20", "p6", v.c12);
  o.tri(tb[2], "g6", "p6", "k6"); o.r("B4", "p6", 100e3); o.r("k6", "0", 820); o.c("k6", "0", v.c13); o.c("g6", "k6", cap(2, true)); o.c("g6", "p6", cap(2, false));
  o.c("p6", "n24", mc.c14); o.rcs("p6", "n24", swR(s.bmod, mc.c14x), mc.c14x); o.r("n24", "J", 68e3); o.r("J", "Q", 33e3); o.c("Q", "0", v.c15);
  o.r("Q", "0", s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 33e3));
  o.r("J", "W4", v.vr104 * (1 - s.mv)); o.r("W4", "N2", v.vr104 * s.mv); o.r("N2", "0", 22e3); o.c("N2", "0", v.c16);
  o.r("N2", "0", s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 22e3));
  o.rcs("J", "W4", s.top >= 1 ? 22e3 : s.top <= 0 ? 1e9 : blendR(22e3, 1e9, s.top, 22e3), v.c106);
  o.r("W4", "W3", v.vr103 * (1 - s.pl)); o.r("W3", "B3x", std::max(v.vr103 * s.pl, 1e-3));
  o.r("B3x", "0", s.ch1 >= 1 ? v.ronvt : s.ch1 <= 0 ? 1e12 : blendR(v.ronvt, 1e12, s.ch1, v.vr103)); o.r("W3", "0", 1e6);
  return head + o.s; }

int main(int argc, char **argv) {
  if (argc < 6) { std::printf("laney_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [name=wert …]\n"); return 1; }
  std::string dir = argv[1], plug; { char rp[4096]; if (realpath(argv[1], rp)) dir = rp; } double fsIn = std::atof(argv[3]), vin = std::atof(argv[4]), secs = std::atof(argv[5]), fsp = 44100;
  Parts v = Parts::orig(); Setting s; s.v1 = s.v2 = s.pl = s.t = s.b = s.mv = 0.5; s.m = taper(0.5, 0); s.mid = s.top = s.bass = 1; s.ch1 = 1; s.soft = 1; s.bmod = 1;
  for (int i = 6; i < argc; i++) { std::string a = argv[i]; size_t e = a.find('='); if (e == std::string::npos) continue; std::string k = a.substr(0, e), w = a.substr(e + 1); double x = std::atof(w.c_str());
    if (k == "v1") s.v1 = x; else if (k == "v2") s.v2 = x; else if (k == "pl") s.pl = x; else if (k == "t") s.t = x; else if (k == "m") s.m = x; else if (k == "b") s.b = x; else if (k == "mv") s.mv = x;
    else if (k == "mid") s.mid = x; else if (k == "top") s.top = x; else if (k == "bass") s.bass = x; else if (k == "ch1") s.ch1 = x; else if (k == "low") s.low = x; else if (k == "soft") s.soft = x;
    else if (k == "bmod") s.bmod = x; else if (k == "late") s.late = x; else if (k == "tube") s.tube[0] = s.tube[1] = s.tube[2] = (int)x; else if (k == "tube1") s.tube[0] = (int)x; else if (k == "tube2") s.tube[1] = (int)x;
    else if (k == "tube3") s.tube[2] = (int)x; else if (k == "plug") plug = w; else if (k == "fsp") fsp = x; else { std::printf("unbekannt: %s\n", k.c_str()); return 1; } }
  auto load = [](const std::string &p, double sec, double fs) { std::vector<float> x; FILE *f = std::fopen(p.c_str(), "rb"); if (!f) { std::printf("Eingang fehlt: %s\n", p.c_str()); std::exit(1); }
    x.resize((size_t)(sec * fs) + 2); x.resize(std::fread(x.data(), 4, x.size(), f)); std::fclose(f); return x; };
  std::vector<float> x = load(argv[2], secs, fsIn); if (!x.empty()) x[0] = 0;
  auto uin = [&](double t) { double p = t * fsIn; long i = (long)p; if (i < 0 || i + 1 >= (long)x.size()) return 0.0; double a = p - i; return vin * ((1 - a) * x[i] + a * x[i + 1]); };

  // ---------- SPICE ----------
  std::string pwl = "PWL("; { char b[64]; for (size_t i = 0; i + 1 < x.size(); i++) { std::snprintf(b, sizeof b, "%s%.10g %.8g", i ? "\n+ " : "", i / fsIn, vin * x[i]); pwl += b; } pwl += ")"; }
  char ttl[300]; std::snprintf(ttl, sizeof ttl, "Laney AOR 50 Vorstufe: v1 %.2f v2 %.2f pl %.2f t %.2f m %.3f b %.2f mv %.2f mid %g top %g bass %g ch1 %g low %g soft %g bmod %g Roehren %d %d %d",
    s.v1, s.v2, s.pl, s.t, s.m, s.b, s.mv, s.mid, s.top, s.bass, s.ch1, s.low, s.soft, s.bmod, s.tube[0], s.tube[1], s.tube[2]);
  for (int var = 0; var < 3; var++) { const char *tag = var == 2 ? "k" : var ? "p" : "m";
    std::string cir = circuit(v, s, var == 0, pwl, std::string(ttl) + (var == 2 ? " (12AX7 nach Koren)" : var ? " (Cgk/Cga an den Roehren)" : " (Miller-Ersatzkapazitaeten wie im Kern)"), var == 2);
    cir += ".options method=trap reltol=1e-5 abstol=1e-11 vntol=1e-7 chgtol=1e-16 gmin=1e-13 itl1=500 itl4=200\n";
    char b[400]; double h = 1 / FS_KERN; std::snprintf(b, sizeof b, ".control\nset wr_singlescale\nset wr_vecnames\noption numdgt=10\ntran %.9g %.9g 0 %.9g\nlinearize\n", h, secs, h / 2); cir += b;
    cir += "wrdata " + dir + "/spice_" + tag + ".txt"; for (const char *p : PROBE) cir += std::string(" v(") + p + ")"; cir += "\nquit\n.endc\n.end\n";
    FILE *f = std::fopen((dir + "/laney_" + tag + ".cir").c_str(), "w"); std::fputs(cir.c_str(), f); std::fclose(f); }

  // ---------- Kern, alle Knoten, ohne Überabtastungsfilter ----------
  { Amp a(FS_KERN, 1, &tubeOf(0)); a.low = s.low > 0.5; a.init(v, s);
    FILE *f = std::fopen((dir + "/kern.txt").c_str(), "w"); std::fprintf(f, "time"); for (const char *p : PROBE) std::fprintf(f, " %s", p); std::fprintf(f, "\n");
    long N = (long)(secs * FS_KERN);
    for (long i = 0; i <= N; i++) { double t = i / FS_KERN; a.run(i ? uin(t) : 0, false);
      auto &P0 = a.sec[0].pv, &P1 = a.sec[1].pv, &P2 = a.sec[2].pv, &P3 = a.sec[3].pv, &P4 = a.sec[4].pv;
      double y[NPROBE] = {P0[0], P1[0], P1[1], P1[2], P2[0], P2[1], P2[2], P3[0], P3[1], P3[2], P4[0], P4[1], P4[2], P4[3], P4[4], P4[5], P4[6]};
      std::fprintf(f, "%.9g", t); for (int k = 0; k < NPROBE; k++) std::fprintf(f, " %.9g", y[k]); std::fprintf(f, "\n"); }
    std::fclose(f); std::printf("Kern: %ld Schritte bei %.0f kHz\n", N, FS_KERN / 1000); }

  // ---------- Kern wie im Plugin ----------
  if (!plug.empty()) { std::vector<float> xp = load(plug, secs, fsp); if (!xp.empty()) xp[0] = 0;
    for (int os : {1, 2, 4, 8}) { Amp a(fsp, os, &tubeOf(0)); a.low = s.low > 0.5; a.init(v, s); int lat = os > 1 ? 31 : 0;
      FILE *f = std::fopen((dir + "/plug_" + std::to_string(os) + ".txt").c_str(), "w"); std::fprintf(f, "time W3\n");
      for (long i = 0; i < (long)xp.size() + lat; i++) { double y = a.step(i < (long)xp.size() ? vin * xp[i] : 0); if (i >= lat) std::fprintf(f, "%.9g %.9g\n", (i - lat) / fsp, y); }
      std::fclose(f); } }
  return 0; }
