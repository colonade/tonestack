// Gegenrechnung des AmpFett B-15NF mit SPICE (ngspice): dieselbe Schaltung, dieselben Bauteilwerte, dieselben Röhrengleichungen,
// aber ein fremder Löser (veränderliche Schrittweite, eigenes Newton-Verfahren, Arbeitspunkt aus der Gleichstromrechnung).
// Stimmen beide überein, rechnet unser Kern die Schaltung richtig; ob die Schaltung und die Röhrenmodelle das Gerät treffen, ist eine andere Frage.
//
//   b15_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [vol bass treble jack]
//     eingang.f32: mono, 32 Bit Gleitkomma, Abtastrate FS_EIN; VIN = Volt bei 1,0; gerechnet werden die ersten SEKUNDEN
//   schreibt  AUS-ORDNER/b15.cir        Netzliste für ngspice (ngspice -b b15.cir → spice.txt)
//             AUS-ORDNER/kern.txt       unsere Rechnung: Zeit und dieselben Knoten, Abtastrate FS_KERN
// Vereinfachungen für den Vergleich (auf beiden Seiten gleich): einfaches Netzteil (psu 0), Ausgangsübertrager ohne Kernsättigung/Hysterese.
#include "b15_core.hpp"
#include "dk_spice.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace b15;
static const double FS_KERN = 192000;
static const char *PROBE[] = {"g1a", "p1a", "k1a", "g1b", "p1b", "Y1", "mix", "pA", "pB", "kA", "g5", "g4", "a5", "a4", "s8", "B1", "B2", "B4", "BIAS"};
static const int NPROBE = sizeof PROBE / sizeof *PROBE;
static std::string n(double v) { return dkspice::num(v); }

int main(int argc, char **argv) {
  if (argc < 6) { std::printf("b15_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [vol bass treble jack]\n"); return 1; }
  std::string dir = argv[1]; double fsIn = std::atof(argv[3]), vin = std::atof(argv[4]), secs = std::atof(argv[5]);
  Parts v; v.psu = 0; v.ot_sat = false; Setting s; s.vol1 = argc > 6 ? std::atof(argv[6]) : 0.5; s.bass1 = argc > 7 ? std::atof(argv[7]) : 0.5; s.treb1 = argc > 8 ? std::atof(argv[8]) : 0.5; s.jack = argc > 9 ? std::atoi(argv[9]) : 0;
  std::vector<float> x; { FILE *f = std::fopen(argv[2], "rb"); if (!f) { std::printf("Eingang fehlt\n"); return 1; } x.resize((size_t)(secs * fsIn) + 2); x.resize(std::fread(x.data(), 4, x.size(), f)); std::fclose(f); }
  if (!x.empty()) x[0] = 0;   // beide Rechnungen beginnen in Ruhe (SPICE rechnet den Arbeitspunkt mit dem Eingangswert bei t = 0)
  // Eingang als Polygonzug (so liest ihn auch SPICE): u(t) = lineare Interpolation der Abtastwerte
  auto uin = [&](double t) { double p = t * fsIn; long i = (long)p; if (i < 0) return 0.0; if (i + 1 >= (long)x.size()) return 0.0; double a = p - i; return vin * ((1 - a) * x[i] + a * x[i + 1]); };

  // ---------- SPICE-Netzliste ----------
  Built B = build(v, s);
  std::string pwl = "PWL("; { char b[64]; for (size_t i = 0; i + 1 < x.size(); i++) { std::snprintf(b, sizeof b, "%s%.9g %.7g", i ? "\n+ " : "", i / fsIn, vin * x[i]); pwl += b; } pwl += ")"; }
  auto devs = [&](const dkspice::Names &N) { std::string o;
    // Gleichungen wie in b15_core.hpp. sp(x) = ln(1 + e^x); Gitterstrom nach Dempwolf/Zölzer; Triode Dempwolf-Form; Pentode Reefman-Form ohne Sekundäremission
    // Für ngspice ohne Fallunterscheidungen geschrieben (gleiche Werte): beide Zweige eines ?: werden ausgewertet, und am Knick der Pentode bei
    // Ua = 0 (Anode schwingt beim Umschalten der Endröhren unter 0 V) bleibt die Schrittweitensteuerung hängen → pos(x) = max(x, 0), auf 0,1 mV gerundet
    o += ".func sp(x) {ln(1 + exp(max(min(x, 35), -60))) + max(x - 35, 0)}\n";
    o += ".func pos(x) {0.5 * (x + sqrt(x * x + 1e-8))}\n";
    o += ".func gi(v) {6.177e-4 * pwr(sp(9.901 * v) / 9.901, 1.314)}\n";
    { const TriodeD &T = T6SL7; o += ".func tria(vg, vp) {" + n(T.G) + " * pwr(sp(" + n(T.C) + " * (vp / " + n(T.mu) + " + vg)) / " + n(T.C) + ", " + n(T.ga) + ") * (1 - exp(-max(vp, 0)))}\n"; }
    { const PentodeD &P = D6L6;   // E1 = g2/kp·sp(kp·(1/mu + g1/√(kvb + g2²))), IP = E1^ex, f = 1/(1 + β·va)
      o += ".func pe1(g1, g2) {g2 / " + n(P.kp) + " * sp(" + n(P.kp) + " * (1 / " + n(P.mu) + " + g1 / sqrt(" + n(P.kvb) + " + g2 * g2)))}\n";
      o += ".func pip(g1, g2) {pwr(pe1(g1, max(g2, 1)), " + n(P.ex) + ")}\n";
      o += ".func pf(va) {1 / (1 + " + n(P.be) + " * pos(va))}\n";
      o += ".func pig2(g1, g2, va) {" + n(P.sg) + " * pip(g1, g2) * (1 + " + n(P.als) + " * pf(va)) / " + n(P.kg2) + "}\n";
      o += ".func pia(g1, g2, va) {" + n(P.sa) + " * pip(g1, g2) * ((1 / " + n(P.kg1) + " - 1 / " + n(P.kg2) + ") * (1 - pf(va)) + " + n(P.A) + " * pos(va) / " + n(P.kg1) + ")}\n"; }
    o += ".func vdio(v) {3.21e-3 * pwr(0.5 * sp(v / 0.5), 1.5)}\n";
    o += ".func sdio(v) {1e-9 * (exp(min(v / 0.04136, 40)) * (1 + max(v / 0.04136 - 40, 0)) - 1)}\n";
    int k = 0; for (const dk::Dev &d : B.dev) { auto P = [&](int j) { return B.net.P[d.p0 + j]; }; auto V = [&](int j) { return "V(" + N[P(j).a] + "," + N[P(j).b] + ")"; }; std::string id = std::to_string(k++);
      switch (d.kind) {
        case K_6SL7: o += "BG" + id + " " + N[P(0).a] + " " + N[P(0).b] + " I = gi(" + V(0) + ")\n"; o += "BA" + id + " " + N[P(1).a] + " " + N[P(1).b] + " I = tria(" + V(0) + ", " + V(1) + ")\n"; break;
        case K_6L6: o += "BG" + id + " " + N[P(0).a] + " " + N[P(0).b] + " I = gi(" + V(0) + ")\n";
          o += "BS" + id + " " + N[P(1).a] + " " + N[P(1).b] + " I = " + n(d.sc) + " * pig2(" + V(0) + ", " + V(1) + ", " + V(2) + ")\n";
          o += "BA" + id + " " + N[P(2).a] + " " + N[P(2).b] + " I = " + n(d.sc) + " * pia(" + V(0) + ", " + V(1) + ", " + V(2) + ")\n"; break;
        case K_RECT: o += "BD" + id + " " + N[P(0).a] + " " + N[P(0).b] + " I = vdio(" + V(0) + ")\n"; break;
        case K_SI: o += "BD" + id + " " + N[P(0).a] + " " + N[P(0).b] + " I = sdio(" + V(0) + ")\n"; break;
        default: std::printf("Gerät %d nicht übersetzbar\n", d.kind); std::exit(1); } }
    return o; };
  char ttl[200]; std::snprintf(ttl, sizeof ttl, "AmpFett B-15NF, Kanal 1, VOLUME %.2f BASS %.2f TREBLE %.2f, Buchse %d; aus b15_core.hpp erzeugt (tools/spiceref)", s.vol1, s.bass1, s.treb1, s.jack);
  std::string cir = dkspice::netlist(B.net, {pwl, "DC " + n(v.dc_v), "DC " + n(v.bias_v)}, devs, ttl);
  // Knoten ohne Gleichstrompfad vermeiden: sehr großer Widerstand gegen Masse an jedem Knoten schadet nicht (1 GΩ)
  { dkspice::Names N(B.net); for (int i = 1; i <= B.net.nn; i++) cir += "RGND" + std::to_string(i) + " " + N[i] + " 0 1e12\n"; }
  cir += ".options method=trap reltol=1e-5 abstol=1e-11 vntol=1e-7 chgtol=1e-16 gmin=1e-13 itl1=500 itl4=200\n";
  { char b[400]; double h = 1 / FS_KERN; std::snprintf(b, sizeof b, ".control\nset wr_singlescale\nset wr_vecnames\noption numdgt=10\ntran %.9g %.9g 0 %.9g\nlinearize\n", h, secs, h / 2); cir += b;
    cir += "wrdata " + dir + "/spice.txt"; for (const char *p : PROBE) cir += std::string(" v(") + p + ")"; cir += "\nquit\n.endc\n.end\n"; }
  { FILE *f = std::fopen((dir + "/b15.cir").c_str(), "w"); std::fputs(cir.c_str(), f); std::fclose(f); }

  // ---------- unser Kern: eingeschwungen, dann derselbe Eingang; Knotenspannungen als zusätzliche Ausgänge ----------
  Core c; c.init(FS_KERN, v, s); c.settle(60.0);   // lange: Vorspannung (100 µF an 47 kΩ) und Siebkette brauchen viele Sekunden bis auf Millivolt
  int o0 = (int)c.net.O.size(); for (const char *p : PROBE) c.net.out(p); c.run.capOut = {c.bl.capSpk}; c.run.set(c.comp.compile(c.net, FS_KERN));
  std::vector<double> y(o0 + NPROBE); FILE *f = std::fopen((dir + "/kern.txt").c_str(), "w"); std::fprintf(f, "time"); for (const char *p : PROBE) std::fprintf(f, " %s", p); std::fprintf(f, "\n");
  long N = (long)(secs * FS_KERN); long its = 0;
  for (long i = 0; i <= N; i++) { double t = i / FS_KERN, u[3] = {uin(t), v.dc_v, v.bias_v};
    if (i == 0) { u[0] = 0; }   // Zeitpunkt 0: Ruhe (wie der Arbeitspunkt in SPICE); der Kern rechnet den Wert zum Ende des Schritts
    c.run.step(u, y.data()); its += c.run.lastIt; std::fprintf(f, "%.9g", t); for (int k = 0; k < NPROBE; k++) std::fprintf(f, " %.9g", y[o0 + k]); std::fprintf(f, "\n"); }
  std::fclose(f); std::printf("Kern: %ld Schritte bei %.0f kHz, %.2f Newton-Schritte je Abtastwert; Netzliste %zu Bytes\n", N, FS_KERN / 1000, (double)its / N, cir.size());
  return 0; }
