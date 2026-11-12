// Gegenrechnung der Laney-AOR-50-Endstufe (laney_pa.hpp: Phasenumkehr, zwei EL34, Ausgangsübertrager, Netzteil, Lautsprecherlast) mit SPICE (ngspice).
// Das Netz ist schon ein dk-Netz und wird wie beim B-15NF aus dem Kern übersetzt (dk_spice.hpp); Röhren und Dioden mit denselben Gleichungen.
//   laney_pa_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [presence]
//     eingang.f32: Spannung am Eingang der Endstufe (Ausgang der Vorstufe ohne Gleichanteil), VIN = Volt bei 1,0
//   schreibt pa.cir (→ spice_pa.txt) und kern_pa.txt
// Auf beiden Seiten: Übertrager ohne Kernsättigung (Fixed::sat = 0), Netzteil als Gleichspannung hinter Innenwiderstand (wie im Plugin), Vorstufe in Ruhe.
#include "laney_pa.hpp"
#include "dk_spice.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace lpa;
static const double FS_KERN = 192000;
static const char *PROBE[] = {"ga", "gb", "k", "pa", "pb", "x", "g5", "g6", "a5", "a6", "s8", "B1", "B2", "B3", "BIAS", "R260"};
static const int NPROBE = sizeof PROBE / sizeof *PROBE;
static std::string n(double v) { return dkspice::num(v); }
int main(int argc, char **argv) {
  if (argc < 6) { std::printf("laney_pa_spice AUS-ORDNER eingang.f32 FS_EIN VIN SEKUNDEN [presence]\n"); return 1; }
  std::string dir = argv[1]; { char rp[4096]; if (realpath(argv[1], rp)) dir = rp; } double fsIn = std::atof(argv[3]), vin = std::atof(argv[4]), secs = std::atof(argv[5]);
  Vals v; v.pres = argc > 6 ? std::atof(argv[6]) : 0.15; Fixed f; f.sat = 0;
  std::vector<float> x; { FILE *h = std::fopen(argv[2], "rb"); if (!h) { std::printf("Eingang fehlt\n"); return 1; } x.resize((size_t)(secs * fsIn) + 2); x.resize(std::fread(x.data(), 4, x.size(), h)); std::fclose(h); }
  if (!x.empty()) x[0] = 0;
  auto uin = [&](double t) { double p = t * fsIn; long i = (long)p; if (i < 0 || i + 1 >= (long)x.size()) return 0.0; double a = p - i; return vin * ((1 - a) * x[i] + a * x[i + 1]); };
  Built B = build(f, v);
  std::string pwl = "PWL("; { char b[64]; for (size_t i = 0; i + 1 < x.size(); i++) { std::snprintf(b, sizeof b, "%s%.10g %.8g", i ? "\n+ " : "", i / fsIn, vin * x[i]); pwl += b; } pwl += ")"; }
  auto devs = [&](const dkspice::Names &N) { std::string o; const b15::TriodeD &T = T12AX7; const b15::PentodeD &P = DEL34;
    o += ".func sp(x) {ln(1 + exp(max(min(x, 35), -60))) + max(x - 35, 0)}\n.func pos(x) {0.5 * (x + sqrt(x * x + 1e-8))}\n";
    o += ".func gi(v) {6.177e-4 * pwr(sp(9.901 * v) / 9.901, 1.314)}\n";
    o += ".func tria(vg, vp) {" + n(T.G) + " * pwr(sp(" + n(T.C) + " * (vp / " + n(T.mu) + " + vg)) / " + n(T.C) + ", " + n(T.ga) + ") * (1 - exp(-max(vp, 0)))}\n";
    o += ".func pe1(g1, g2) {g2 / " + n(P.kp) + " * sp(" + n(P.kp) + " * (1 / " + n(P.mu) + " + g1 / sqrt(" + n(P.kvb) + " + g2 * g2)))}\n";
    o += ".func pip(g1, g2) {pwr(pe1(g1, 1 + pos(g2 - 1)), " + n(P.ex) + ")}\n.func pf(va) {1 / (1 + " + n(P.be) + " * pos(va))}\n";
    o += ".func pps(g1, g2, va) {" + n(P.S) + " * pos(va) * (1 + tanh(-" + n(P.ap) + " * (pos(va) - ((1 + pos(g2 - 1)) / " + n(P.lam) + " - " + n(P.nu) + " * g1 - " + n(P.w) + "))))}\n";
    o += ".func pig2(g1, g2, va) {" + n(P.sg) + " * pip(g1, g2) * (1 + " + n(P.als) + " * pf(va) + pps(g1, g2, va)) / " + n(P.kg2) + "}\n";
    o += ".func pia(g1, g2, va) {" + n(P.sa) + " * pip(g1, g2) * ((1 / " + n(P.kg1) + " - 1 / " + n(P.kg2) + ") * (1 - pf(va)) + " + n(P.A) + " * pos(va) / " + n(P.kg1) + " - pps(g1, g2, va) / " + n(P.kg2) + ")}\n";
    o += ".func rect(v) {0.05 * sp((v - 0.62) / 0.025)}\n";
    int k = 0; for (const dk::Dev &d : B.dev) { auto Pt = [&](int j) { return B.net.P[d.p0 + j]; }; auto V = [&](int j) { return "V(" + N[Pt(j).a] + "," + N[Pt(j).b] + ")"; }; std::string id = std::to_string(k++);
      auto src = [&](const char *nm, int j, const std::string &e) { o += std::string(nm) + id + " " + N[Pt(j).a] + " " + N[Pt(j).b] + " I = " + e + "\n"; };
      switch (d.kind) {
        case K_TRI: src("BG", 0, "gi(" + V(0) + ")"); src("BA", 1, "tria(" + V(0) + ", " + V(1) + ")"); break;
        case K_PEN: src("BG", 0, n(DEL34.gs) + " * gi(" + V(0) + ")"); src("BS", 1, n(d.sc) + " * pig2(" + V(0) + ", " + V(1) + ", " + V(2) + ")"); src("BA", 2, n(d.sc) + " * pia(" + V(0) + ", " + V(1) + ", " + V(2) + ")"); break;
        case K_SI: src("BD", 0, "rect(" + V(0) + ")"); break;
        default: std::printf("Gerät %d nicht übersetzbar\n", d.kind); std::exit(1); } }
    return o; };
  std::string cir = dkspice::netlist(B.net, {pwl, "DC " + n(f.dc_v), "DC 0", "DC 0", "DC 0", "DC " + n(f.bias_v)}, devs, "Laney AOR 50 Endstufe, Presence " + n(v.pres) + "; aus laney_pa.hpp erzeugt (tools/spiceref)");
  { dkspice::Names N(B.net); for (int i = 1; i <= B.net.nn; i++) cir += "RGND" + std::to_string(i) + " " + N[i] + " 0 1e12\n"; }
  cir += ".options method=trap reltol=1e-5 abstol=1e-11 vntol=1e-7 chgtol=1e-16 gmin=1e-13 itl1=500 itl4=200\n";
  { char b[400]; double h = 1 / FS_KERN; std::snprintf(b, sizeof b, ".control\nset wr_singlescale\nset wr_vecnames\noption numdgt=10\ntran %.9g %.9g 0 %.9g\nlinearize\n", h, secs, h / 2); cir += b;
    cir += "wrdata " + dir + "/spice_pa.txt"; for (const char *p : PROBE) cir += std::string(" v(") + p + ")"; cir += "\nquit\n.endc\n.end\n"; }
  { FILE *h = std::fopen((dir + "/pa.cir").c_str(), "w"); std::fputs(cir.c_str(), h); std::fclose(h); }
  Core c; c.f = f; c.init(FS_KERN, v); c.settle(30.0);
  for (const char *p : PROBE) c.net.out(p); c.run.set(c.comp.compile(c.net, FS_KERN));
  std::vector<double> y(O_NO + NPROBE); FILE *h = std::fopen((dir + "/kern_pa.txt").c_str(), "w"); std::fprintf(h, "time"); for (const char *p : PROBE) std::fprintf(h, " %s", p); std::fprintf(h, "\n");
  long N = (long)(secs * FS_KERN); long its = 0;
  for (long i = 0; i <= N; i++) { double t = i / FS_KERN, in[6] = {i ? uin(t) : 0, f.dc_v, 0, 0, 0, f.bias_v};
    c.run.step(in, y.data()); its += c.run.lastIt; std::fprintf(h, "%.9g", t); for (int k = 0; k < NPROBE; k++) std::fprintf(h, " %.9g", y[O_NO + k]); std::fprintf(h, "\n"); }
  std::fclose(h); std::printf("Kern: %ld Schritte, %.2f Newton-Schritte je Abtastwert\n", N, (double)its / N); return 0; }
