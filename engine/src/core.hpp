// Laney AOR 50 Vorverstärker: Rechenkern in C++ (direkte Übertragung des Simulator-Kerns laney-worklet.js)
#pragma once
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <algorithm>
#include <complex>
#include "parts_gen.h"

namespace aor {
constexpr double PI = 3.14159265358979323846;

struct Parts {
#define X(n, a, b) double n;
  PARTS_LIST(X)
#undef X
  static Parts orig() { Parts p; 
#define X(n, a, b) p.n = a;
    PARTS_LIST(X)
#undef X
    return p; }
  static Parts umbau() { Parts p;
#define X(n, a, b) p.n = b;
    PARTS_LIST(X)
#undef X
    return p; }
  bool operator==(const Parts &o) const { return std::memcmp(this, &o, sizeof(Parts)) == 0; }
};

// Reglerstellungen als Anteile 0..1 (Middle und Presence bereits mit Kennlinie), Schalter 0/1
struct Setting {
  double v1 = 1, v2 = 1, t = 1, m = 1, b = 1, mv = 0.5, pl = 1, pres = 0.15;
  double mid = 0, top = 0, bass = 0, ch1 = 1, low = 0;
  int tube[3] = {0, 0, 0};   // Röhrentyp je Fassung V101, V102, V103 (Index in TUBE_MODELS; 0 = 12AX7/ECC83 wie im Gerät)
};
// Röhrenmodelle (Koren): 12AX7, 12AT7, 12AU7 nach Koren (Glass Audio 1996, Tabelle 1); 5751 und 12AY7 an Datenblatt-Arbeitspunkte angepasst (mu fest, kvb 300)
struct TubeModel { const char *name; double mu, ex, kg1, kp, kvb, cgk, cga; bool fitted; };
static const TubeModel TUBE_MODELS[5] = {
  {"12AX7 / ECC83", 100,  1.4,   1060, 600, 300, 1.6e-12, 1.7e-12, false},
  {"12AT7 / ECC81", 60,   1.35,  460,  300, 300, 2.7e-12, 2.2e-12, false},
  {"12AU7 / ECC82", 21.5, 1.3,   1180, 84,  300, 2.5e-12, 2.0e-12, false},
  {"5751",          70,   1.170, 1420, 377, 300, 1.6e-12, 1.4e-12, true},
  {"12AY7",         44,   1.700, 3781, 206, 300, 1.3e-12, 1.3e-12, true} };
constexpr int NTUBES = 5;
inline bool stockTubes(const Setting &s) { return s.tube[0] == 0 && s.tube[1] == 0 && s.tube[2] == 0; }
// Kleinsignalwerte eines Triodensystems im Arbeitspunkt (aus dem Gleichstromlöser des Röhrenkerns abgeleitet)
struct TriSS { double gm, rp, cgk, cga; };
inline double &sfield(Setting &s, int i) { double *f[] = {&s.v1,&s.v2,&s.t,&s.b,&s.m,&s.mv,&s.pl,&s.mid,&s.top,&s.bass}; return *f[i]; }

inline double taper(double x, int type) {     // 0: log 15 % in Mittelstellung, 1: log 10 %, 2: linear
  if (type == 2) return x;
  double mid = type == 1 ? 0.10 : 0.15, s = 1 / mid - 1, b = s * s;
  return (std::pow(b, x) - 1) / (b - 1);
}
inline double blendR(double ron, double roff, double x, double rref) {
  if (x >= 1) return ron;
  if (x <= 0) return roff;
  double yon = rref / (ron + rref), yoff = rref / (roff + rref), y = yoff + x * (yon - yoff);
  return rref * (1 / y - 1);
}
inline double rpOf(double Ia, const Parts &v) { return v.rp0 * std::pow(v.ia0 / Ia, 1.0 / 3); }

// ---------- generisches lineares Netz (Knotenanalyse, Trapezregel) ----------
struct LinSys { int nc = 0; std::vector<double> K, k, G2, c; double d = 0; std::vector<std::vector<double>> mc; std::vector<double> md; };
struct LinNet {
  std::map<std::string,int> idx; int n = 0;
  std::vector<std::array<double,3>> Rs, Cs; std::vector<std::array<double,4>> TR; std::vector<std::array<double,5>> BJ; int rcCount = 0; double fsRef = 0;
  int N(const std::string &k) { if (k == "0") return -1; auto it = idx.find(k); if (it != idx.end()) return it->second; idx[k] = n; return n++; }
  void R(const std::string &a, const std::string &b, double r) { Rs.push_back({(double)N(a), (double)N(b), 1 / std::max(r, 1e-3)}); }
  void C(const std::string &a, const std::string &b, double c, double fs) { Cs.push_back({(double)N(a), (double)N(b), 2 * c * fs}); fsRef = fs; }
  void RC(const std::string &a, const std::string &b, double r, double c, double fs) { std::string m = "rc" + std::to_string(rcCount++); R(a, m, r); C(m, b, c, fs); }
  void triode(const std::string &g, const std::string &p, const std::string &k, double Ia, const Parts &v, double fs) {
    double rp = rpOf(Ia, v); R(p, k, rp); TR.push_back({(double)N(g), (double)N(p), (double)N(k), v.mu / rp}); C(g, k, v.cgk, fs); C(g, p, v.cga, fs); }
  void vccs(const std::string &g, const std::string &p, const std::string &k, double gm) { TR.push_back({(double)N(g), (double)N(p), (double)N(k), gm}); }
  void bjt(const std::string &b, const std::string &cn, const std::string &e, double gm, double beta) { BJ.push_back({(double)N(b), (double)N(cn), (double)N(e), gm, beta}); }
  // Übersetzung: K, k, c, d sowie Monitorzeilen (Gitter-Kathoden-Spannungen)
  LinSys compile(const std::string &outName, const std::vector<std::pair<std::string,std::string>> &MON) const {
    int M = n + 1; std::vector<double> A(M * M, 0.0);
    auto st = [&](int a, int b, double g) { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
    for (auto &e : Rs) st((int)e[0], (int)e[1], e[2]);
    for (auto &e : Cs) st((int)e[0], (int)e[1], e[2]);
    for (auto &t : TR) { int g = (int)t[0], p = (int)t[1], k = (int)t[2]; double gm = t[3];
      if (p >= 0) { A[p*M+g] += gm; if (k >= 0) A[p*M+k] -= gm; } if (k >= 0) { A[k*M+g] -= gm; A[k*M+k] += gm; } }
    for (auto &t : BJ) { int bb = (int)t[0], cc = (int)t[1], ee = (int)t[2]; double gm = t[3], beta = t[4];
      auto add = [&](int row, int col, double g) { if (row >= 0 && col >= 0) A[row*M+col] += g; };
      for (auto [row, sg] : std::initializer_list<std::pair<int,double>>{{cc, 1.0}, {ee, -1.0}}) { if (row < 0) continue; add(row, bb, sg*gm); add(row, ee, -sg*gm); }
      for (auto [row, sg] : std::initializer_list<std::pair<int,double>>{{bb, 1.0}, {ee, -1.0}}) { if (row < 0) continue; add(row, bb, sg*gm/beta); add(row, ee, -sg*gm/beta); } }
    int si = idx.at("in"); A[si*M+n] = 1; A[n*M+si] = 1;
    std::vector<int> piv(M);
    for (int c = 0; c < M; c++) { int p = c; double best = -1; for (int r = c; r < M; r++) { double m = std::fabs(A[r*M+c]); if (m > best) { best = m; p = r; } }
      piv[c] = p; if (p != c) for (int q = 0; q < M; q++) std::swap(A[c*M+q], A[p*M+q]);
      double d = A[c*M+c]; for (int r = c + 1; r < M; r++) { double f = A[r*M+c] / d; if (f == 0) continue; A[r*M+c] = f; for (int q = c + 1; q < M; q++) A[r*M+q] -= f * A[c*M+q]; } }
    auto solve = [&](std::vector<double> &rhs) { for (int c = 0; c < M; c++) { int p = piv[c]; if (p != c) std::swap(rhs[c], rhs[p]); }
      for (int r = 1; r < M; r++) { double s2 = rhs[r]; for (int q = 0; q < r; q++) s2 -= A[r*M+q] * rhs[q]; rhs[r] = s2; }
      for (int r = M - 1; r >= 0; r--) { double s2 = rhs[r]; for (int q = r + 1; q < M; q++) s2 -= A[r*M+q] * rhs[q]; rhs[r] = s2 / A[r*M+r]; } };
    int nc = (int)Cs.size(); LinSys S; S.nc = nc; S.K.assign(nc * nc, 0); S.k.assign(nc, 0); S.G2.assign(nc, 0); S.c.assign(nc, 0);
    int out = idx.at(outName); std::vector<std::pair<int,int>> mon; for (auto &m : MON) mon.push_back({idx.at(m.first), idx.at(m.second)});
    S.mc.assign(mon.size(), std::vector<double>(nc, 0)); S.md.assign(mon.size(), 0);
    auto V = [](const std::vector<double> &x, int a) { return a >= 0 ? x[a] : 0.0; };
    for (int j = 0; j <= nc; j++) {
      std::vector<double> rhs(M, 0.0);
      if (j < nc) { int a = (int)Cs[j][0], b = (int)Cs[j][1]; if (a >= 0) rhs[a] += 1; if (b >= 0) rhs[b] -= 1; } else rhs[n] = 1;
      solve(rhs);
      for (int i = 0; i < nc; i++) { double v2 = V(rhs, (int)Cs[i][0]) - V(rhs, (int)Cs[i][1]); if (j < nc) S.K[i*nc+j] = v2; else S.k[i] = v2; }
      if (j < nc) { S.c[j] = rhs[out]; for (size_t m = 0; m < mon.size(); m++) S.mc[m][j] = rhs[mon[m].first] - rhs[mon[m].second]; }
      else { S.d = rhs[out]; for (size_t m = 0; m < mon.size(); m++) S.md[m] = rhs[mon[m].first] - rhs[mon[m].second]; }
    }
    for (int i = 0; i < nc; i++) S.G2[i] = 2 * Cs[i][2];
    return S;
  }
};
// Kleinsignal-Übertragung bei der Frequenz f (komplexe Knotenanalyse auf demselben Netz; Kapazitäten aus den Trapez-Leitwerten zurückgerechnet)
inline std::complex<double> responseAt(const LinNet &o, double f, const std::string &outName) {
  typedef std::complex<double> cd; int n = o.n, M = n + 1; std::vector<cd> A(M * M, cd(0, 0)); cd jw(0, 2 * PI * f);
  auto st = [&](int a, int b, cd g) { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
  for (auto &e : o.Rs) st((int)e[0], (int)e[1], cd(e[2], 0));
  for (auto &e : o.Cs) st((int)e[0], (int)e[1], jw * (e[2] / (2 * o.fsRef)));
  for (auto &t : o.TR) { int g = (int)t[0], p = (int)t[1], k = (int)t[2]; double gm = t[3];
    if (p >= 0) { A[p*M+g] += gm; if (k >= 0) A[p*M+k] -= gm; } if (k >= 0) { A[k*M+g] -= gm; A[k*M+k] += gm; } }
  for (auto &t : o.BJ) { int bb = (int)t[0], cc = (int)t[1], ee = (int)t[2]; double gm = t[3], beta = t[4];
    auto add = [&](int row, int col, double g) { if (row >= 0 && col >= 0) A[row*M+col] += g; };
    for (auto [row, sg] : std::initializer_list<std::pair<int,double>>{{cc, 1.0}, {ee, -1.0}}) { if (row < 0) continue; add(row, bb, sg*gm); add(row, ee, -sg*gm); }
    for (auto [row, sg] : std::initializer_list<std::pair<int,double>>{{bb, 1.0}, {ee, -1.0}}) { if (row < 0) continue; add(row, bb, sg*gm/beta); add(row, ee, -sg*gm/beta); } }
  int si = o.idx.at("in"); A[si*M+n] = 1; A[n*M+si] = 1;
  std::vector<cd> x(M, cd(0, 0)); x[n] = 1;
  for (int ccol = 0; ccol < M; ccol++) { int p = ccol; double best = -1; for (int r = ccol; r < M; r++) { double m = std::abs(A[r*M+ccol]); if (m > best) { best = m; p = r; } }
    if (p != ccol) { for (int q = 0; q < M; q++) std::swap(A[ccol*M+q], A[p*M+q]); std::swap(x[ccol], x[p]); }
    cd d = A[ccol*M+ccol]; for (int r = ccol + 1; r < M; r++) { cd fct = A[r*M+ccol] / d; if (fct == cd(0, 0)) continue; for (int q = ccol; q < M; q++) A[r*M+q] -= fct * A[ccol*M+q]; x[r] -= fct * x[ccol]; } }
  for (int r = M - 1; r >= 0; r--) { cd s = x[r]; for (int q = r + 1; q < M; q++) s -= A[r*M+q] * x[q]; x[r] = s / A[r*M+r]; }
  return x[o.idx.at(outName)];
}
static const std::vector<std::pair<std::string,std::string>> MON = {{"g1","k1"},{"g2","k2"},{"g3","k3"},{"g4","k4"},{"g6","k6"}};
static const double MONREF[5] = {1.6, 1.7, 3.1, 1.0, 1.0};

inline double vt1Div(const Parts &v, double rvt) {
  double w = 2 * PI * 1000, zs = 1 / (1 / 100e3 + 1 / 77.9e3), xc = -1 / (w * v.c2), z = 1 / (1 / v.vr101 + 1 / rvt);
  return z / std::hypot(zs + v.r5 + z, xc);
}
inline double vt1R(const Parts &v, double x) {
  double ron = v.ronvt + v.r6, roff = 1e12;
  if (x >= 1) return ron;
  if (x <= 0) return roff;
  double g1 = vt1Div(v, ron), g0 = vt1Div(v, roff), gt = std::exp(std::log(g0) + x * (std::log(g1) - std::log(g0)));
  double lo = std::log(ron), hi = std::log(roff);
  for (int i = 0; i < 60; i++) { double mid = (lo + hi) / 2; if (vt1Div(v, std::exp(mid)) > gt) hi = mid; else lo = mid; }
  return std::exp((lo + hi) / 2);
}
inline double chanShape(const Parts &v, double x) { return vt1Div(v, vt1R(v, 1 - x)); }

std::vector<TriSS> smallSignal(const Parts &v, const Setting &s, double fs);
inline LinNet netLin(const Parts &v, const Setting &s, double fs, const std::vector<TriSS> *ss = nullptr) {
  LinNet o; o.N("in"); int ti = 0;
  auto tri = [&](const std::string &g, const std::string &p, const std::string &k, double Ia) {
    if (ss) { const TriSS &t = (*ss)[ti]; o.R(p, k, t.rp); o.vccs(g, p, k, t.gm); o.C(g, k, t.cgk, fs); o.C(g, p, t.cga, fs); } else o.triode(g, p, k, Ia, v, fs); ti++; };
  if (s.low) { o.R("g1","0",v.r1); o.R("in","t2",1e-3); } else o.R("in","g1",v.r1);
  tri("g1","p1","k1",0.62e-3); o.R("k1","0",2.7e3); o.C("k1","0",47e-6,fs); o.R("p1","0",100e3);
  o.C("p1","n5",v.c2,fs); o.R("n5","t1",v.r5);
  o.R("t1","0", vt1R(v, 1 - s.ch1));
  o.R("t1","g2",v.vr101*(1-s.v1)); o.R("g2","0",v.vr101*s.v1);
  tri("g2","p2","k2",0.69e-3); o.R("k2","0",2.7e3); o.C("k2","0",v.c3,fs); o.R("p2","0",100e3);
  if (!s.low && v.rpad > 0) { o.C("p2","t2p",v.ck,fs); o.R("t2p","t2",v.rpad); } else o.C("p2", s.low ? "0" : "t2", v.ck, fs);   // Umbauidee „Später": Vorwiderstand vor Preamp 2 Volume
  o.R("t2","w2",v.vr102*(1-s.v2)); o.R("w2","0",v.vr102*s.v2); o.R("w2","g3",68e3);
  tri("g3","p3","k3",0.31e-3); o.R("k3","0",10e3); o.R("p3","0",100e3);
  o.C("p3","n12",v.c4,fs); o.R("n12","g4",470e3); o.C("n12","g4",v.c5,fs); o.R("g4","0",470e3);
  tri("g4","p4","k4",1.10e-3); o.R("k4","0",820); o.R("p4","0",100e3);
  tri("p4","0","k5",1.5e-3); o.R("k5","0",100e3);
  o.C("k5","X",v.c6,fs); o.C("X","Z",v.c7,fs); o.RC("X","Z", blendR(1e-3, 10e6, s.mid, 1 / (2 * PI * 1000 * v.c8)), v.c8, fs);
  o.R("X","Y",v.r18); o.C("Y","W",v.c11,fs);
  o.R("Z","T",v.vr105*(1-s.t)); o.R("T","W",v.vr105*s.t); o.R("W","P",v.vr106*s.b);
  o.R("P","Mw",v.vr107*(1-s.m)); o.R("Mw","0",v.vr107*s.m); o.C("Y","Mw",v.c10,fs);
  o.C("T","n19",v.c9,fs); o.R("n19","g6",1e6); o.R("g6","0",1e6); o.R("g6","n20",3.3e6); o.C("n20","p6",v.c12,fs);
  tri("g6","p6","k6",1.18e-3); o.R("k6","0",820); o.C("k6","0",v.c13,fs); o.R("p6","0",100e3);
  o.C("p6","n24",v.c14,fs); o.R("n24","J",68e3);
  o.R("J","Q",33e3); o.C("Q","0",v.c15,fs); o.R("Q","0", blendR(1e12, v.rts, s.bass, 33e3));
  o.R("J","W4",v.vr104*(1-s.mv)); o.R("W4","N2",v.vr104*s.mv);
  o.R("N2","0",22e3); o.C("N2","0",v.c16,fs); o.R("N2","0", blendR(1e12, v.rts, s.bass, 22e3));
  o.RC("J","W4", blendR(22e3, 1e9, s.top, 22e3), v.c106, fs);
  o.R("W4","W3",v.vr103*(1-s.pl)); o.R("W3","B3",std::max(v.vr103*s.pl, 1e-3));
  o.R("B3","0", blendR(v.ronvt, 1e12, s.ch1, v.vr103));
  o.R("W3","0",1e6);
  return o;
}
inline LinSys buildLin(const Parts &v, const Setting &s, double fs, const std::vector<TriSS> *ss = nullptr) { return netLin(v, s, fs, ss).compile("W3", MON); }
// Endstufe mit Gegenkopplung und Presence (wie engine.paBuild)
inline LinNet netPA(const Parts &v, double pres, double fs) {
  LinNet o; o.N("in");
  auto buf = [&](const std::string &a, const std::string &b) { o.vccs(a, "0", b, 1); o.R(b, "0", 1); };
  o.R("in","fa",1e3); o.C("fa","fb",v.c201,fs); o.C("fb","fc",v.c202,fs); o.R("fc","0",1/(1/39e3+1/55e3));
  buf("fc","bb"); o.R("bb","ga0",1e3); o.C("ga0","ga",v.c18,fs); o.R("ga","n",1e6); o.R("gb","n",1e6); o.R("n","k",470); o.R("n","x",10e3);
  o.triode("ga","pa","k",1.23e-3,v,fs); o.triode("gb","pb","k",1.17e-3,v,fs); o.R("pa","0",82e3); o.R("pb","0",100e3); o.C("pa","pb",v.c20,fs);
  o.C("pa","g1",v.c21,fs); o.R("g1","0",220e3); o.C("pb","g2",v.c21,fs); o.R("g2","0",220e3);
  o.C("gb","x",v.c19,fs); o.R("x","0",v.r105); o.R("x","px",v.vr108*(1-pres)); o.C("px","0",v.c110,fs);
  o.vccs("g2","0","br",1); o.vccs("g1","br","0",1); o.R("br","0",1);
  o.R("br","x", v.r38 * 0.5 / std::max(v.gp, 1e-6));
  return o;
}
inline LinSys buildPA(const Parts &v, double pres, double fs) { return netPA(v, pres, fs).compile("br", {}); }

// Zustandsrechner für ein lineares Netz mit Überblendung bei Neuaufbau
struct LinRunner {
  LinSys sys, prev; bool hasPrev = false; double blend = 1; std::vector<double> Ih, vc; double norm = 1, prevNorm = 1; int every = 256;
  void set(LinSys s, double nrm, bool keepState) {
    if (sys.nc > 0 && keepState) { prev = sys; hasPrev = true; blend = 0; prevNorm = norm; } else { hasPrev = false; blend = 1; prevNorm = nrm; }
    sys = std::move(s); norm = nrm;
    if ((int)Ih.size() != sys.nc) { Ih.assign(sys.nc, 0); vc.assign(sys.nc, 0); }
    if (hasPrev && prev.nc != sys.nc) hasPrev = false;
  }
  double step(double u, double *peak = nullptr) {
    const LinSys &S = sys; int nc = S.nc; double y, nrm;
    if (blend < 1 && hasPrev) {
      double a = blend, b = 1 - a; const LinSys &P = prev;
      y = (a * S.d + b * P.d) * u;
      for (int i = 0; i < nc; i++) { double s1 = S.k[i] * u, s0 = P.k[i] * u; int o = i * nc;
        for (int j = 0; j < nc; j++) { s1 += S.K[o + j] * Ih[j]; s0 += P.K[o + j] * Ih[j]; }
        vc[i] = a * s1 + b * s0; y += (a * S.c[i] + b * P.c[i]) * Ih[i]; }
      if (peak) for (size_t m = 0; m < S.mc.size(); m++) { double s1 = (a * S.md[m] + b * P.md[m]) * u; for (int j = 0; j < nc; j++) s1 += (a * S.mc[m][j] + b * P.mc[m][j]) * Ih[j]; peak[m] = std::max(peak[m], std::fabs(s1)); }
      nrm = a * norm + b * prevNorm; blend = std::min(1.0, blend + 1.0 / every);
    } else {
      y = S.d * u;
      for (int i = 0; i < nc; i++) { double s1 = S.k[i] * u; int o = i * nc; for (int j = 0; j < nc; j++) s1 += S.K[o + j] * Ih[j]; vc[i] = s1; y += S.c[i] * Ih[i]; }
      if (peak) for (size_t m = 0; m < S.mc.size(); m++) { double s1 = S.md[m] * u; for (int j = 0; j < nc; j++) s1 += S.mc[m][j] * Ih[j]; peak[m] = std::max(peak[m], std::fabs(s1)); }
      nrm = norm;
    }
    for (int i = 0; i < nc; i++) Ih[i] = S.G2[i] * vc[i] - Ih[i];
    return y / nrm;
  }
};

// ---------- Röhrenmodell (Koren 12AX7) mit Tabelle ----------
struct Tube {
  double KP = 600, MU = 100, EX = 1.4, KG1 = 1060, KVB = 300; static constexpr double RGI = 2000, SG = 0.02;
  static constexpr double G0 = -12, GS = 0.01, P0 = 0, PS = 1; static constexpr int GN = 1501, PN = 321;
  std::vector<float> TAB; int type = 0;
  void setModel(int t) { const TubeModel &m = TUBE_MODELS[t]; type = t; MU = m.mu; EX = m.ex; KG1 = m.kg1; KP = m.kp; KVB = m.kvb; }
  void tube(double vgk, double vpk, double *out) const {
    if (vpk <= 1e-6) { out[0] = out[1] = out[2] = 0; return; }
    double s = std::sqrt(KVB + vpk * vpk), q = KP * (1 / MU + vgk / s), sp, sig;
    if (q > 35) { sp = q; sig = 1; } else { double eq = std::exp(q); sp = std::log1p(eq); sig = eq / (1 + eq); }
    double e1 = vpk / KP * sp;
    if (e1 <= 1e-300) { out[0] = out[1] = out[2] = 0; return; }
    double p = std::exp((EX - 1) * std::log(e1)), I = 2 * p * e1 / KG1, dI = 2 * EX * p / KG1;
    out[0] = I; out[1] = dI * vpk * sig / s; out[2] = dI * (sp / KP - sig * vgk * vpk * vpk / (s * s * s));
  }
  void makeTab() { TAB.resize(GN * PN); double o[3]; for (int i = 0; i < GN; i++) for (int j = 0; j < PN; j++) { tube(G0 + i * GS, P0 + j * PS, o); TAB[i * PN + j] = (float)o[0]; } }
  void tubeT(double vgk, double vpk, double *out) const {
    double x = (vgk - G0) / GS, y = (vpk - P0) / PS;
    if (x < 0) x = 0; else if (x > GN - 1.000001) { tube(vgk, vpk, out); return; }
    if (y < 0) y = 0; else if (y > PN - 1.000001) { tube(vgk, vpk, out); return; }
    int i = (int)x, j = (int)y; double fx = x - i, fy = y - j; int o = i * PN + j;
    double a = TAB[o], b = TAB[o + 1], c = TAB[o + PN], d = TAB[o + PN + 1];
    double ab = a + (b - a) * fy, cd = c + (d - c) * fy;
    out[0] = ab + (cd - ab) * fx;
    out[1] = (vgk - G0 < 0) ? 0 : (cd - ab) / GS;
    out[2] = (vpk - P0 < 0) ? 0 : ((b - a) + ((d - c) - (b - a)) * fx) / PS;
  }
  static void grid(double v, double *out) { double x = v / SG; if (x > 35) { out[0] = v / RGI; out[1] = 1 / RGI; return; }
    double e = std::exp(x); out[0] = SG * std::log1p(e) / RGI; out[1] = e / (1 + e) / RGI; }
};
// Röhrentabellen je Typ, einmal je Prozess (bei Bedarf aufgebaut)
inline const Tube &tubeOf(int t) { static Tube bank[NTUBES]; static bool ready[NTUBES] = {false}; if (t < 0 || t >= NTUBES) t = 0; if (!ready[t]) { bank[t].setModel(t); bank[t].makeTab(); ready[t] = true; } return bank[t]; }

// ---------- nichtlineare Abschnitte ----------
struct Net {
  std::map<std::string,int> idx; int n = 0; std::vector<std::array<double,3>> R, C; std::vector<int> S; std::vector<std::array<int,2>> P; std::vector<int> probes; int rc = 0;
  int N(const std::string &k) { if (k == "0") return -1; auto it = idx.find(k); if (it != idx.end()) return it->second; idx[k] = n; return n++; }
  void r(const std::string &a, const std::string &b, double rr) { R.push_back({(double)N(a), (double)N(b), 1 / std::max(rr, 1e-3)}); }
  void c(const std::string &a, const std::string &b, double cc) { C.push_back({(double)N(a), (double)N(b), cc}); }
  void rcs(const std::string &a, const std::string &b, double rr, double cc) { std::string m = "_rc" + std::to_string(rc++); r(a, m, rr); c(m, b, cc); }
  int src(const std::string &a) { S.push_back(N(a)); return (int)S.size() - 1; }
  int port(const std::string &f, const std::string &t) { P.push_back({N(f), N(t)}); return (int)P.size() - 1; }
  int probe(const std::string &a) { probes.push_back(N(a)); return (int)probes.size() - 1; }
};
struct Sec {   // übersetzter Abschnitt + Arbeitsdaten
  int nc = 0, ns = 0, np = 0, npr = 0, ncol = 0, nu = 0, kn = -1, k = 0;
  std::vector<double> Mc, Mp, G2, dV, w, pv, J, F, I, pI, Ip2; std::vector<std::array<double,2>> capsAll; std::vector<std::vector<double>> full;
};
static const int UNK[5][5] = {{0},{0,1},{0,1},{0,1},{0,1,2,3,4}}; static const int UNKN[5] = {1,2,2,2,5}; static const int KNOWN[5] = {-1,2,2,2,5};
static const double UB[4] = {240, 252, 252, 260};
static const double CMg1 = 81e-12, CMg2 = 97e-12, CMg3 = 16e-12, CMg4 = 72e-12;

inline Sec compileSec(const Net &o, double fs, bool dc) {
  int n = o.n, ns = (int)o.S.size(), M = n + ns; std::vector<double> A(M * M, 0.0);
  auto st = [&](int a, int b, double g) { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
  for (auto &e : o.R) st((int)e[0], (int)e[1], e[2]);
  std::vector<std::array<double,3>> caps; if (!dc) for (auto &e : o.C) caps.push_back({e[0], e[1], 2 * e[2] * fs});
  for (auto &e : caps) st((int)e[0], (int)e[1], e[2]);
  for (int k = 0; k < ns; k++) { int a = o.S[k]; A[a*M + n + k] = 1; A[(n + k)*M + a] = 1; }
  std::vector<int> piv(M);
  for (int c = 0; c < M; c++) { int p = c; double b = -1; for (int r = c; r < M; r++) { double m = std::fabs(A[r*M+c]); if (m > b) { b = m; p = r; } }
    piv[c] = p; if (p != c) for (int q = 0; q < M; q++) std::swap(A[c*M+q], A[p*M+q]);
    double d = A[c*M+c]; for (int r = c + 1; r < M; r++) { double f = A[r*M+c] / d; if (f == 0) continue; A[r*M+c] = f; for (int q = c + 1; q < M; q++) A[r*M+q] -= f * A[c*M+q]; } }
  auto solve = [&](std::vector<double> &rhs) { for (int c = 0; c < M; c++) { int p = piv[c]; if (p != c) std::swap(rhs[c], rhs[p]); }
    for (int r = 1; r < M; r++) { double s = rhs[r]; for (int q = 0; q < r; q++) s -= A[r*M+q] * rhs[q]; rhs[r] = s; }
    for (int r = M - 1; r >= 0; r--) { double s = rhs[r]; for (int q = r + 1; q < M; q++) s -= A[r*M+q] * rhs[q]; rhs[r] = s / A[r*M+r]; } };
  Sec C; C.nc = (int)caps.size(); C.ns = ns; C.np = (int)o.P.size(); C.npr = (int)o.probes.size(); C.ncol = C.nc + ns + C.np;
  C.Mc.assign(C.nc * C.ncol, 0); C.Mp.assign(C.npr * C.ncol, 0);
  auto V = [](const std::vector<double> &x, int a) { return a >= 0 ? x[a] : 0.0; };
  for (int j = 0; j < C.ncol; j++) {
    std::vector<double> rhs(M, 0.0);
    if (j < C.nc) { int a = (int)caps[j][0], b = (int)caps[j][1]; if (a >= 0) rhs[a] += 1; if (b >= 0) rhs[b] -= 1; }
    else if (j < C.nc + ns) rhs[n + (j - C.nc)] = 1;
    else { auto &pt = o.P[j - C.nc - ns]; if (pt[0] >= 0) rhs[pt[0]] -= 1; if (pt[1] >= 0) rhs[pt[1]] += 1; }
    solve(rhs);
    for (int i = 0; i < C.nc; i++) C.Mc[i*C.ncol + j] = V(rhs, (int)caps[i][0]) - V(rhs, (int)caps[i][1]);
    for (int i = 0; i < C.npr; i++) C.Mp[i*C.ncol + j] = V(rhs, o.probes[i]);
    if (dc) C.full.push_back(std::vector<double>(rhs.begin(), rhs.begin() + n));
  }
  C.G2.resize(C.nc); for (int i = 0; i < C.nc; i++) C.G2[i] = 2 * caps[i][2];
  for (auto &e : o.C) C.capsAll.push_back({e[0], e[1]});
  return C;
}
inline std::vector<Net> defs(const Parts &v, const Setting &s) {
  std::vector<Net> S(5); const TubeModel &T3 = TUBE_MODELS[s.tube[2]]; double cgk3 = stockTubes(s) ? v.cgk : T3.cgk, cga3 = stockTubes(s) ? v.cga : T3.cga;
  { Net &o = S[0]; o.src("in"); o.r("in","g1",v.r1); o.c("g1","0",CMg1); o.port("g1","0"); o.probe("g1"); }
  { Net &o = S[1]; o.src("B"); o.r("B","p1",100e3); o.r("k1","0",2.7e3); o.c("k1","0",47e-6);
    o.c("p1","n5",v.c2); o.r("n5","t1",v.r5); o.r("t1","0", s.ch1 <= 0 ? v.ronvt + v.r6 : s.ch1 >= 1 ? 1e12 : blendR(v.ronvt + v.r6, 1e12, 1 - s.ch1, 5e4));
    o.r("t1","g2",v.vr101*(1-s.v1)); o.r("g2","0",v.vr101*s.v1); o.c("g2","0",CMg2);
    o.port("p1","k1"); o.port("g2","0"); o.port("0","k1"); o.probe("p1"); o.probe("k1"); o.probe("g2"); }
  { Net &o = S[2]; o.src("B"); o.r("B","p2",100e3); o.r("k2","0",2.7e3); o.c("k2","0",v.c3);
    o.src("lin"); o.r("lin","t2", s.low ? 1e-3 : 1e12);
    if (!s.low && v.rpad > 0) { o.c("p2","t2p",v.ck); o.r("t2p","t2",v.rpad); } else o.c("p2", s.low ? "0" : "t2", v.ck);   // Umbauidee „Später"
    o.r("t2","w2",v.vr102*(1-s.v2)); o.r("w2","0",v.vr102*s.v2); o.r("w2","g3",68e3); o.c("g3","0",CMg3);
    o.port("p2","k2"); o.port("g3","0"); o.port("0","k2"); o.probe("p2"); o.probe("k2"); o.probe("g3"); }
  { Net &o = S[3]; o.src("B"); o.r("B","p3",100e3); o.r("k3","0",10e3);
    o.c("p3","n12",v.c4); o.r("n12","g4",470e3); o.c("n12","g4",v.c5); o.r("g4","0",470e3); o.c("g4","0",CMg4);
    o.port("p3","k3"); o.port("g4","0"); o.port("0","k3"); o.probe("p3"); o.probe("k3"); o.probe("g4"); }
  { Net &o = S[4]; o.src("B");
    o.r("B","p4",100e3); o.r("k4","0",820);
    o.r("k5","0",100e3); o.c("p4","0",cga3); o.c("p4","k5",cgk3);
    o.c("k5","X",v.c6); o.c("X","Z",v.c7); o.rcs("X","Z", s.mid >= 1 ? 1e-3 : s.mid <= 0 ? 10e6 : blendR(1e-3, 10e6, s.mid, 1 / (2 * PI * 1000 * v.c8)), v.c8);
    o.r("X","Y",v.r18); o.c("Y","W",v.c11);
    o.r("Z","T",v.vr105*(1-s.t)); o.r("T","W",v.vr105*s.t); o.r("W","P",v.vr106*s.b);
    o.r("P","Mw",v.vr107*(1-s.m)); o.r("Mw","0",v.vr107*s.m); o.c("Y","Mw",v.c10);
    o.c("T","n19",v.c9); o.r("n19","g6",1e6); o.r("g6","0",1e6); o.r("g6","n20",3.3e6); o.c("n20","p6",v.c12);
    o.r("B","p6",100e3); o.r("k6","0",820); o.c("k6","0",v.c13); o.c("g6","k6",cgk3); o.c("g6","p6",cga3);
    o.c("p6","n24",v.c14); o.r("n24","J",68e3); o.r("J","Q",33e3); o.c("Q","0",v.c15);
    o.r("Q","0", s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 33e3));
    o.r("J","W4",v.vr104*(1-s.mv)); o.r("W4","N2",v.vr104*s.mv); o.r("N2","0",22e3); o.c("N2","0",v.c16);
    o.r("N2","0", s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 22e3));
    o.rcs("J","W4", s.top >= 1 ? 22e3 : s.top <= 0 ? 1e9 : blendR(22e3, 1e9, s.top, 22e3), v.c106);
    o.r("W4","W3",v.vr103*(1-s.pl)); o.r("W3","B3",std::max(v.vr103*s.pl, 1e-3));
    o.r("B3","0", s.ch1 >= 1 ? v.ronvt : s.ch1 <= 0 ? 1e12 : blendR(v.ronvt, 1e12, s.ch1, v.vr103)); o.r("W3","0",1e6);
    o.port("p4","k4"); o.port("0","k5"); o.port("p4","k5"); o.port("p6","k6"); o.port("g6","k6"); o.port("0","k4");
    for (const char *p : {"p4","k4","k5","g6","p6","k6","W3"}) o.probe(p); }
  return S;
}

struct AmpState { std::vector<double> Ih, vc; };
struct Snapshot { std::vector<AmpState> st; std::vector<std::vector<double>> I; double vkPrev[4], vgkDC[5], w3DC; };

struct Amp {
  int os; double fs; const Tube *tube; const Tube *tubes[3]; bool low = false;
  std::vector<Sec> sec, dcSec; std::vector<AmpState> st; double vkPrev[4] = {0,0,0,0}, vgkDC[5] = {0,0,0,0,0}, peak[5] = {0,0,0,0,0}, w3DC = 0;
  double g0 = 0, g1 = 0, g2 = 0, g3 = 0, g4 = 0, tb[3], gb[2], Ib[8], Dn[8]; double tol = 1e-9; bool dcMode = false, dcExact = false;
  void tubeAt(const Tube *T, double vgk, double vpk) { if (dcMode && (dcExact || vgk < Tube::G0)) T->tube(vgk, vpk, tb); else T->tubeT(vgk, vpk, tb); }   // Gleichstrom: links der Tabelle exakt, damit der Löser eine Richtung hat
  std::vector<double> h, hp; int L = 0; std::vector<double> xb, yb; int xi = 0, yi = 0;
  struct Blend { std::vector<double> Mp, Mc, dV; }; std::vector<Blend> bFrom, bTo; bool blending = false; int bpos = 0;
  Amp(double fsBase, int os_, const Tube *t) : os(os_), fs(os_ * fsBase), tube(t) { tubes[0] = tubes[1] = tubes[2] = t;
    if (os > 1) { L = 32 * os - 1; h.assign(L, 0); double fc = 0.229 * 2 / os, sum = 0;
      for (int k = 0; k < L; k++) { double m = k - (L - 1) / 2.0, w = 0.42 - 0.5 * std::cos(2 * PI * k / (L - 1)) + 0.08 * std::cos(4 * PI * k / (L - 1));
        h[k] = (m == 0 ? 2 * fc : std::sin(2 * PI * fc * m) / (PI * m)) * w; sum += h[k]; }
      for (int k = 0; k < L; k++) h[k] /= sum;
      hp.assign(os * 32, 0); for (int ph = 0; ph < os; ph++) for (int k = 0; k < 32; k++) hp[ph * 32 + k] = (k * os + ph < L) ? h[k * os + ph] : 0;
      xb.assign(64, 0); yb.assign(2 * L, 0); }
  }
  void prep(Sec &C, int k) {
    int nu = UNKN[k]; C.nu = nu; C.dV.assign(C.npr * nu, 0);
    for (int r = 0; r < C.npr; r++) for (int u = 0; u < nu; u++) C.dV[r * nu + u] = C.Mp[r * C.ncol + C.nc + C.ns + UNK[k][u]];
    C.kn = KNOWN[k]; C.w.assign(C.npr, 0); C.pv.assign(C.npr, 0); C.J.assign(nu * nu, 0); C.F.assign(nu, 0); C.I.assign(nu, 0); C.pI.assign(C.np, 0); C.k = k; C.Ip2.clear();
  }
  void setTubes(const Setting &s) { for (int i = 0; i < 3; i++) tubes[i] = &tubeOf(s.tube[i]); }
  void compileAll(const Parts &v, const Setting &s) { setTubes(s); auto D = defs(v, s); sec.resize(5); dcSec.resize(5);
    for (int k = 0; k < 5; k++) { sec[k] = compileSec(D[k], fs, false); prep(sec[k], k); dcSec[k] = compileSec(D[k], fs, true); prep(dcSec[k], k); } }
  void residual(Sec &C, double vg) {
    double *pv = C.pv.data(), *J = C.J.data(), *F = C.F.data(), *I = C.I.data(), *dV = C.dV.data(); int nu = C.nu, k = C.k;
    for (int q = 0; q < nu * nu; q++) J[q] = 0; for (int u = 0; u < nu; u++) J[u * nu + u] = 1;
    double a1, a2, a3;
    if (k == 0) { Tube::grid(pv[0] - vkPrev[0], gb); F[0] = I[0] - gb[0]; J[0] -= gb[1] * dV[0]; return; }
    if (k <= 3) {
      tubeAt(tubes[k <= 2 ? 0 : 1], vg - pv[1], pv[0] - pv[1]); F[0] = I[0] - tb[0]; a1 = tb[2]; a2 = -tb[1] - tb[2];   // k=1,2: V101; k=3: V102A
      for (int q = 0; q < 2; q++) J[q] -= a1 * dV[q] + a2 * dV[2 + q];
      Tube::grid(pv[2] - vkPrev[k], gb); F[1] = I[1] - gb[0];
      for (int q = 0; q < 2; q++) J[2 + q] -= gb[1] * dV[4 + q];
      return; }
    tubeAt(tubes[1], vg - pv[1], pv[0] - pv[1]); F[0] = I[0] - tb[0]; a1 = tb[2]; a2 = -tb[1] - tb[2];   // V102B
    for (int q = 0; q < 5; q++) J[q] -= a1 * dV[q] + a2 * dV[5 + q];
    tubeAt(tubes[2], pv[0] - pv[2], UB[3] - pv[2]); F[1] = I[1] - tb[0]; a1 = tb[1]; a2 = -tb[1] - tb[2];   // V103A (Kathodenfolger)
    for (int q = 0; q < 5; q++) J[5 + q] -= a1 * dV[q] + a2 * dV[10 + q];
    Tube::grid(pv[0] - pv[2], gb); F[2] = I[2] - gb[0];
    for (int q = 0; q < 5; q++) J[10 + q] -= gb[1] * (dV[q] - dV[10 + q]);
    tubeAt(tubes[2], pv[3] - pv[5], pv[4] - pv[5]); F[3] = I[3] - tb[0]; a1 = tb[1]; a2 = tb[2]; a3 = -tb[1] - tb[2];   // V103B
    for (int q = 0; q < 5; q++) J[15 + q] -= a1 * dV[15 + q] + a2 * dV[20 + q] + a3 * dV[25 + q];
    Tube::grid(pv[3] - pv[5], gb); F[4] = I[4] - gb[0];
    for (int q = 0; q < 5; q++) J[20 + q] -= gb[1] * (dV[15 + q] - dV[25 + q]);
  }
  double evalF(Sec &C, double vg) {
    int nu = C.nu, npr = C.npr;
    for (int r = 0; r < npr; r++) { double s = C.w[r]; for (int u = 0; u < nu; u++) s += C.dV[r * nu + u] * C.I[u]; C.pv[r] = s; }
    residual(C, vg); double e = 0; for (int u = 0; u < nu; u++) e = std::max(e, std::fabs(C.F[u])); return e;
  }
  int solve(Sec &C, const double *Ih, const double *src, double known, double vg, int maxIt) {
    int nc = C.nc, ns = C.ns, ncol = C.ncol, npr = C.npr, nu = C.nu; double *J = C.J.data(), *F = C.F.data(), *I = C.I.data(), *D = Dn;
    for (int r = 0; r < npr; r++) { double s = 0; int o = r * ncol;
      if (Ih) for (int j = 0; j < nc; j++) s += C.Mp[o + j] * Ih[j];
      for (int j = 0; j < ns; j++) s += C.Mp[o + nc + j] * src[j];
      if (C.kn >= 0) s += C.Mp[o + nc + ns + C.kn] * known;
      C.w[r] = s; }
    int it = 0; bool ok = false; double err = evalF(C, vg);
    for (; it < maxIt; it++) {
      if (err < tol && it > 0) { ok = true; break; }
      for (int u = 0; u < nu; u++) D[u] = -F[u];
      for (int c = 0; c < nu; c++) { int p = c; double bb = -1; for (int r = c; r < nu; r++) { double m = std::fabs(J[r*nu+c]); if (m > bb) { bb = m; p = r; } }
        if (p != c) { for (int q = 0; q < nu; q++) std::swap(J[c*nu+q], J[p*nu+q]); std::swap(D[c], D[p]); }
        double d = J[c*nu+c]; for (int r = c + 1; r < nu; r++) { double f = J[r*nu+c] / d; if (f == 0) continue; for (int q = c; q < nu; q++) J[r*nu+q] -= f * J[c*nu+q]; D[r] -= f * D[c]; } }
      for (int r = nu - 1; r >= 0; r--) { double s = D[r]; for (int q = r + 1; q < nu; q++) s -= J[r*nu+q] * D[q]; D[r] = s / J[r*nu+r]; }
      for (int u = 0; u < nu; u++) Ib[u] = I[u];
      { double dm = 0; for (int u = 0; u < nu; u++) dm = std::max(dm, std::fabs(D[u])); if (dm < 1e-15) { ok = true; break; } }   // Schritt praktisch null: fertig (sonst Leerlauf in der Schrittweitensuche)
      double lam = 1; for (int u = 0; u < nu; u++) { double m = std::fabs(D[u]); if (m * lam > 5e-3) lam = 5e-3 / m; }
      double e2 = INFINITY;
      for (int ls = 0; ls < 12; ls++) { for (int u = 0; u < nu; u++) I[u] = Ib[u] + lam * D[u]; e2 = evalF(C, vg); if (e2 < err || e2 == 0) break; lam *= 0.5; }
      err = e2;
    }
    if (!ok && err < tol) ok = true;
    return ok ? it : -1;
  }
  void advance(Sec &C, AmpState &St, const double *src, double known) {
    int nc = C.nc, ns = C.ns, ncol = C.ncol, np = C.np, k = C.k; double *pI = C.pI.data(), *I = C.I.data();
    if (k == 0) pI[0] = I[0]; else if (k <= 3) { pI[0] = I[0]; pI[1] = I[1]; pI[2] = known; } else { for (int u = 0; u < 5; u++) pI[u] = I[u]; pI[5] = known; }
    for (int c = 0; c < nc; c++) { double s = 0; int o = c * ncol;
      for (int j = 0; j < nc; j++) s += C.Mc[o + j] * St.Ih[j];
      for (int j = 0; j < ns; j++) s += C.Mc[o + nc + j] * src[j];
      for (int j = 0; j < np; j++) s += C.Mc[o + nc + ns + j] * pI[j];
      St.vc[c] = s; }
    for (int c = 0; c < nc; c++) St.Ih[c] = C.G2[c] * St.vc[c] - St.Ih[c];
  }
  void blendNow() {
    if (!blending) return; double a = std::min(1.0, bpos / 512.0), b = 1 - a;
    for (int k = 0; k < 5; k++) { Sec &C = sec[k]; auto &f = bFrom[k]; auto &t = bTo[k];
      for (size_t i = 0; i < C.Mp.size(); i++) C.Mp[i] = b * f.Mp[i] + a * t.Mp[i];
      for (size_t i = 0; i < C.Mc.size(); i++) C.Mc[i] = b * f.Mc[i] + a * t.Mc[i];
      for (size_t i = 0; i < C.dV.size(); i++) C.dV[i] = b * f.dV[i] + a * t.dV[i]; }
    if (a >= 1) blending = false;
  }
  double run(double u, bool dc) {
    if (!dc && blending && ((++bpos) & 31) == 0) blendNow();
    dcMode = dc; double known = 0, vg = 0; std::vector<Sec> &secs = dc ? dcSec : sec; int maxIt = dc ? 300 : 60;
    for (int k = 0; k < 5; k++) {
      Sec &C = secs[k]; double src[2]; int ns = 1;
      if (k == 0) src[0] = low ? 0 : u; else if (k == 2) { src[0] = UB[1]; src[1] = low ? u : 0; ns = 2; } else src[0] = UB[k - 1];
      (void)ns;
      if (!dc) { if (C.Ip2.empty()) C.Ip2 = C.I; for (int q = 0; q < C.nu; q++) { double cur = C.I[q]; C.I[q] = 2 * cur - C.Ip2[q]; C.Ip2[q] = cur; } }
      solve(C, dc ? nullptr : st[k].Ih.data(), src, known, vg, maxIt);
      if (!dc) advance(C, st[k], src, known);
      if (k == 0) { vg = C.pv[0]; known = C.I[0]; } else if (k <= 3) { vg = C.pv[2]; known = C.I[1]; vkPrev[k - 1] = C.pv[1]; } else vkPrev[3] = C.pv[1];
    }
    auto &P0 = secs[0].pv, &P1 = secs[1].pv, &P2 = secs[2].pv, &P3 = secs[3].pv, &P4 = secs[4].pv;
    g0 = P0[0] - P1[1]; g1 = P1[2] - P2[1]; g2 = P2[2] - P3[1]; g3 = P3[2] - P4[1]; g4 = P4[3] - P4[5];
    return P4[6];
  }
  Snapshot snapshot() const { Snapshot s; s.st = st; for (auto &C : sec) s.I.push_back(C.I); std::memcpy(s.vkPrev, vkPrev, sizeof vkPrev); std::memcpy(s.vgkDC, vgkDC, sizeof vgkDC); s.w3DC = w3DC; return s; }
  void initFrom(const Parts &v, const Setting &s, const Snapshot &snap) {
    compileAll(v, s); st = snap.st; for (int k = 0; k < 5; k++) sec[k].I = snap.I[k];
    std::memcpy(vkPrev, snap.vkPrev, sizeof vkPrev); std::memcpy(vgkDC, snap.vgkDC, sizeof vgkDC); w3DC = snap.w3DC; for (double &p : peak) p = 0;
  }
  void retarget(const Parts &v, const Setting &s) {
    setTubes(s); auto D = defs(v, s); bFrom.resize(5); bTo.resize(5);
    for (int k = 0; k < 5; k++) { Sec fresh = compileSec(D[k], fs, false); prep(fresh, k);
      bFrom[k] = {sec[k].Mp, sec[k].Mc, sec[k].dV}; bTo[k] = {fresh.Mp, fresh.Mc, fresh.dV}; }
    blending = true; bpos = 0; blendNow();
  }
  void init(const Parts &v, const Setting &s) {
    compileAll(v, s); st.resize(5); for (int k = 0; k < 5; k++) { st[k].Ih.assign(sec[k].nc, 0); st[k].vc.assign(sec[k].nc, 0); }
    const std::vector<std::vector<double>> guess = {{0}, {0.6e-3, 0}, {0.6e-3, 0}, {0.25e-3, 0}, {1.1e-3, 1.5e-3, 0, 1.1e-3, 0}};
    for (int k = 0; k < 5; k++) { sec[k].I = guess[k]; dcSec[k].I = guess[k]; }
    vkPrev[0] = 1.6; vkPrev[1] = 1.7; vkPrev[2] = 2.5; vkPrev[3] = 0.9;
    if (stockTubes(s)) { for (int pass = 0; pass < 4; pass++) run(0, true); }
    else {   // Fortsetzung: Modellparameter in Schritten von der 12AX7 zur gewählten Röhre, jeweils vom vorigen Arbeitspunkt aus (exakte Kennlinie, ohne Tabelle)
      Tube tmp[3]; dcExact = true; const TubeModel &m0 = TUBE_MODELS[0]; const int NS = 8;
      for (int step = 1; step <= NS; step++) { double a = (double)step / NS;
        for (int i = 0; i < 3; i++) { const TubeModel &m1 = TUBE_MODELS[s.tube[i]]; Tube &T = tmp[i];
          T.MU = m0.mu + a * (m1.mu - m0.mu); T.EX = m0.ex + a * (m1.ex - m0.ex); T.KG1 = m0.kg1 + a * (m1.kg1 - m0.kg1); T.KP = m0.kp + a * (m1.kp - m0.kp); T.KVB = m0.kvb + a * (m1.kvb - m0.kvb); tubes[i] = &tmp[i]; }
        for (int pass = 0; pass < 3; pass++) run(0, true); }
      setTubes(s); for (int pass = 0; pass < 4; pass++) run(0, true); dcExact = false; }
    double known = 0;
    for (int k = 0; k < 5; k++) {
      Sec &D = dcSec[k], &C = sec[k]; double src = k == 0 ? 0 : UB[k - 1];
      std::vector<double> pI; if (k == 0) pI = {D.I[0]}; else if (k <= 3) pI = {D.I[0], D.I[1], known}; else { pI = D.I; pI.push_back(known); }
      std::vector<double> x(D.full[0].size(), 0.0);
      for (int j = 0; j < D.ns; j++) for (size_t q = 0; q < x.size(); q++) x[q] += D.full[j][q] * (j == 0 ? src : 0);
      for (int j = 0; j < D.np; j++) for (size_t q = 0; q < x.size(); q++) x[q] += D.full[D.ns + j][q] * pI[j];
      for (size_t ci = 0; ci < C.capsAll.size(); ci++) { int a = (int)C.capsAll[ci][0], b = (int)C.capsAll[ci][1]; double vc = (a >= 0 ? x[a] : 0) - (b >= 0 ? x[b] : 0); st[k].Ih[ci] = C.G2[ci] / 2 * vc; st[k].vc[ci] = vc; }
      C.I = D.I; known = k == 0 ? D.I[0] : D.I[1];
    }
    vgkDC[0] = g0; vgkDC[1] = g1; vgkDC[2] = g2; vgkDC[3] = g3; vgkDC[4] = g4;
    w3DC = dcSec[4].pv[6]; for (double &p : peak) p = 0;
  }
  void trackPeak() { double *dc = vgkDC; double g[5] = {g0, g1, g2, g3, g4}; for (int i = 0; i < 5; i++) peak[i] = std::max(peak[i], std::fabs(g[i] - dc[i])); }
  double step(double x) {
    if (os == 1) { double y = run(x, false) - w3DC; trackPeak(); return y; }
    xi = xi == 0 ? 31 : xi - 1; xb[xi] = xb[xi + 32] = x;
    for (int ph = 0; ph < os; ph++) { double u = 0; const double *hh = &hp[ph * 32]; for (int k = 0; k < 32; k++) u += hh[k] * xb[xi + k];
      double y = run(os * u, false) - w3DC; trackPeak(); yi = yi == 0 ? L - 1 : yi - 1; yb[yi] = yb[yi + L] = y; }
    double out = 0; for (int k = 0; k < L; k++) out += h[k] * yb[yi + k]; return out;
  }
};

inline std::vector<TriSS> smallSignal(const Parts &v, const Setting &s, double fs) {
  Amp a(fs, 1, &tubeOf(0)); a.low = s.low > 0.5; a.init(v, s); std::vector<TriSS> r(6); double o[3];
  auto at = [&](int ti, int sock, double vgk, double vpk) { const Tube &T = tubeOf(s.tube[sock]); T.tube(vgk, vpk, o); const TubeModel &m = TUBE_MODELS[s.tube[sock]];
    r[ti] = {o[1], 1 / std::max(o[2], 1e-12), m.cgk, m.cga}; };   // out[1] = dI/dVgk (Steilheit), out[2] = dI/dVpk (1/rp)
  auto &P0 = a.dcSec[0].pv, &P1 = a.dcSec[1].pv, &P2 = a.dcSec[2].pv, &P3 = a.dcSec[3].pv, &P4 = a.dcSec[4].pv;
  at(0, 0, P0[0] - P1[1], P1[0] - P1[1]); at(1, 0, P1[2] - P2[1], P2[0] - P2[1]); at(2, 1, P2[2] - P3[1], P3[0] - P3[1]);
  at(3, 1, P3[2] - P4[1], P4[0] - P4[1]); at(4, 2, P4[0] - P4[2], UB[3] - P4[2]); at(5, 2, P4[3] - P4[5], P4[4] - P4[5]);
  return r;
}
// Lautsprecher: Eminence Legend CB158 (Thiele-Small aus dem Datenblatt) im Bassreflexgehäuse der Portaflex-Größe (78 L, Abstimmung 50 Hz,
// Verlustgüte 7): akustisches Ersatzschaltbild → H(s) = s⁴Ts²Tb² / [(s²Ts²+sTs/Qts+1)(s²Tb²+sTb/Ql+1) + s²·α·Tb²], α = Vas/Vb.
// Oben: Schwingspuleninduktivität Le/Re (1. Ordnung, 3 kHz) und Membranaufbruch als 2. Ordnung bei 4 kHz (Q 0,7; Schätzung, Datenblatt: nutzbar bis 3 kHz).
struct SpkModel { double Fs = 34.2, Qts = 0.34, Vas = 336, Vb = 78, Fb = 50, Ql = 7, Re = 6.15, Le = 0.33e-3, fc = 4000, Qc = 0.7; };
// Bilineare Transformation eines reellen rationalen s-Polynoms (Koeffizienten aufsteigend, Grad ≤ 4) in z (absteigend, a[0] = 1)
inline void bilinear(const double *bs, const double *as, int n, double fs, double *bz, double *az) {
  double K = 2 * fs; double num[5] = {0,0,0,0,0}, den[5] = {0,0,0,0,0};
  // (1 - z^-1)^k (1 + z^-1)^(n-k) ausmultiplizieren
  for (int k = 0; k <= n; k++) { double poly[5] = {1,0,0,0,0}; int deg = 0;
    for (int i = 0; i < k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] -= poly[j-1]; deg++; }      // × (1 − z⁻¹)
    for (int i = 0; i < n - k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] += poly[j-1]; deg++; }  // × (1 + z⁻¹)
    double pk = std::pow(K, k); for (int j = 0; j <= n; j++) { num[j] += bs[k] * pk * poly[j]; den[j] += as[k] * pk * poly[j]; } }
  for (int j = 0; j <= n; j++) { bz[j] = num[j] / den[0]; az[j] = den[j] / den[0]; }
}
struct IIR { int n = 0; double b[5] = {0,0,0,0,0}, a[5] = {1,0,0,0,0}, z[5] = {0,0,0,0,0};
  double step(double x) { double y = b[0] * x + z[0]; for (int i = 1; i < n; i++) z[i-1] = b[i] * x + z[i] - a[i] * y; z[n-1] = b[n] * x - a[n] * y; return y; }
  void reset() { for (double &q : z) q = 0; } };
struct Speaker { IIR lo, le, cone; bool ready = false;
  void design(double fs, const SpkModel &m = SpkModel()) {
    double Ts = 1 / (2 * PI * m.Fs), Tb = 1 / (2 * PI * m.Fb), al = m.Vas / m.Vb;
    double bs[5] = {0, 0, 0, 0, Ts*Ts*Tb*Tb};
    double as[5] = {1, Ts/m.Qts + Tb/m.Ql, Ts*Ts + Tb*Tb + Ts*Tb/(m.Qts*m.Ql) + al*Tb*Tb, Ts*Ts*Tb/m.Ql + Tb*Tb*Ts/m.Qts, Ts*Ts*Tb*Tb};
    lo.n = 4; bilinear(bs, as, 4, fs, lo.b, lo.a);
    double tau = m.Le / m.Re; double b1[2] = {1, 0}, a1[2] = {1, tau}; le.n = 1; bilinear(b1, a1, 1, fs, le.b, le.a);
    double w0 = 2 * PI * m.fc; double b2[3] = {w0*w0, 0, 0}, a2[3] = {w0*w0, w0/m.Qc, 1}; cone.n = 2; bilinear(b2, a2, 2, fs, cone.b, cone.a);
    lo.reset(); le.reset(); cone.reset(); ready = true; }
  double step(double x) { return cone.step(le.step(lo.step(x))); } };
// Betrag des Lautsprecher-Frequenzgangs (für Diagramme), gleiche Zahlen wie Speaker::design
inline double spkMag(double f, const SpkModel &m = SpkModel()) {
  double Ts = 1 / (2 * PI * m.Fs), Tb = 1 / (2 * PI * m.Fb), al = m.Vas / m.Vb, w = 2 * PI * f;
  double dr = (1 - w*w*Ts*Ts) * (1 - w*w*Tb*Tb) - (w*Ts/m.Qts) * (w*Tb/m.Ql) - w*w*al*Tb*Tb, di = (1 - w*w*Ts*Ts) * (w*Tb/m.Ql) + (w*Ts/m.Qts) * (1 - w*w*Tb*Tb);
  double lo = (w*w*w*w*Ts*Ts*Tb*Tb) / std::hypot(dr, di), le = 1 / std::hypot(1.0, w * m.Le / m.Re), w0 = 2 * PI * m.fc, cone = w0*w0 / std::hypot(w0*w0 - w*w, w0*w/m.Qc);
  return lo * le * cone; }
// ---------- Zielzustand und Glättung (gemeinsam für linear und Röhre) ----------
struct Target { Parts v; Setting s; bool pa = false; int taper = 0; double vin = 0.1; bool nl = false; int os = 2; bool spk = false; };   // spk: Lautsprecher am Ausgang
struct Norms { double high1 = 35986.334852078886, high2 = 920.5260111823894, low1 = 29.12973364705375, low2 = 37.52073127705745, pa = 5.957573504094253; };

struct PaCore {
  double fs; LinRunner r; double cur = 0, tgt = 0; Parts v; bool hasV = false; int cnt = 0; double norm = 1;
  explicit PaCore(double fs_) : fs(fs_) {}
  void setTarget(const Parts &pv, double pres, double nrm) { tgt = pres; norm = nrm; bool changed = !hasV || !(pv == v);
    if (changed) { v = pv; hasV = true; if (r.sys.nc == 0) cur = pres; r.set(buildPA(v, cur, fs), 1, false); } }
  void tick() { if (++cnt < 256) return; cnt = 0; double d = tgt - cur; if (d == 0) return; double a = 1 - std::exp(-256 / (0.015 * fs));
    if (std::fabs(d) > 1e-5) cur += d * a; else cur = tgt; r.set(buildPA(v, cur, fs), 1, true); }
  double step(double u) { return r.step(u) / norm; }
};

struct CoreBase { virtual ~CoreBase() {} virtual void setTarget(const Target &t) = 0; virtual void tick() = 0; virtual double step(double u) = 0; virtual bool isNl() const = 0; virtual int osOf() const = 0; virtual bool lowOf() const = 0; virtual int chOf() const = 0; double peaks[5] = {0,0,0,0,0}; };

struct LinCore : CoreBase {
  double fs; LinRunner r; Setting cur, tgt; Parts v; bool hasV = false; int cnt = 0; bool pa = false; PaCore paC; Norms N; double vin = 0.1; std::vector<TriSS> ss; bool useSS = false;
  explicit LinCore(double fs_) : fs(fs_), paC(fs_) {}
  void updSS() { useSS = !stockTubes(cur); if (useSS) ss = smallSignal(v, cur, fs); }
  double normOf(const Setting &c) const { double nP1 = c.low ? N.low1 : N.high1, nP2 = c.low ? N.low2 : N.high2;
    double x = c.ch1, r0 = chanShape(v, 0), r1 = chanShape(v, 1); return nP2 * (chanShape(v, x) / r0) * std::pow(nP1 / (nP2 * r1 / r0), x); }
  void rebuild(bool keep) { r.set(buildLin(v, cur, fs, useSS ? &ss : nullptr), normOf(cur), keep); }
  void setTarget(const Target &t) override {
    Setting s = t.s; bool tubesChanged = hasV && (cur.tube[0] != s.tube[0] || cur.tube[1] != s.tube[1] || cur.tube[2] != s.tube[2]); bool partsChanged = !hasV || !(t.v == v) || tubesChanged; tgt = s; vin = t.vin;
    bool chChanged = hasV && cur.ch1 != s.ch1; if (chChanged) cur.ch1 = s.ch1;
    if (partsChanged || !hasV || chChanged) {
      bool had = hasV && partsChanged; std::vector<double> oldG = had ? r.sys.G2 : std::vector<double>();
      v = t.v; if (!hasV) { cur = s; hasV = true; } for (int i = 0; i < 3; i++) cur.tube[i] = s.tube[i]; updSS();
      rebuild(!had);
      if (had && oldG.size() == (size_t)r.sys.nc) { r.hasPrev = false; r.blend = 1; r.prevNorm = r.norm; for (size_t j = 0; j < oldG.size(); j++) r.Ih[j] += (r.sys.G2[j] - oldG[j]) / 2 * r.vc[j]; }
    }
    pa = t.pa; paC.setTarget(t.v, t.s.pres, N.pa);
  }
  void tick() override { if (++cnt < 256) return; cnt = 0; double a = 1 - std::exp(-256 / (0.015 * fs)); bool moved = false;
    for (int i = 0; i < 10; i++) { double &c = sfield(cur, i), d = sfield(tgt, i) - c; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-5) c += d * a; else c = sfield(tgt, i); }
    if (moved) rebuild(true); }
  double step(double u) override { double y = r.step(u, peaks); if (pa) { paC.tick(); y = paC.step(y); } return y; }
  bool isNl() const override { return false; } int osOf() const override { return 1; } bool lowOf() const override { return cur.low > 0.5; } int chOf() const override { return cur.ch1 > 0.5 ? 1 : 2; }
};

struct NlCore : CoreBase {
  double fs; int os; Amp amp; Setting cur, tgt; Parts v; bool hasV = false; int cnt = 0; double vin = 0.1; bool pa = false; PaCore paC; Norms N;
  static std::map<std::string, Snapshot> &cache() { static std::map<std::string, Snapshot> c; return c; }
  NlCore(double fs_, int os_, const Tube *t) : fs(fs_), os(os_), amp(fs_, os_, t), paC(fs_) {}
  void setTarget(const Target &t) override {
    Setting s = t.s; vin = t.vin; pa = t.pa; amp.low = s.low > 0.5;
    bool partsChanged = !hasV || !(t.v == v) || cur.tube[0] != s.tube[0] || cur.tube[1] != s.tube[1] || cur.tube[2] != s.tube[2];
    if (partsChanged) { v = t.v; hasV = true; cur = s; tgt = s;
      std::string key(reinterpret_cast<const char*>(&v), sizeof v); key += std::to_string(fs) + "|" + std::to_string(os) + "|" + std::to_string(s.tube[0]) + std::to_string(s.tube[1]) + std::to_string(s.tube[2]);
      auto &c = cache(); auto it = c.find(key);
      if (it == c.end()) { amp.init(v, cur); if (c.size() >= 32) c.clear(); c[key] = amp.snapshot(); } else amp.initFrom(v, cur, it->second);   // begrenzter Zwischenspeicher
    } else tgt = s;
    paC.setTarget(t.v, t.s.pres, N.pa);
  }
  void tick() override { if (++cnt < 256) return; cnt = 0; double a = 1 - std::exp(-256 / (0.015 * fs)); bool moved = false;
    for (int i = 0; i < 10; i++) { double &c = sfield(cur, i), d = sfield(tgt, i) - c; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-5) c += d * a; else c = sfield(tgt, i); }
    if (tgt.ch1 != cur.ch1) { cur.ch1 = tgt.ch1; moved = true; }
    if (moved) amp.retarget(v, cur); }
  double step(double u) override { double y = amp.step(u * vin) / 20; if (pa) { paC.tick(); y = paC.step(y); } for (int i = 0; i < 5; i++) peaks[i] = amp.peak[i]; return y; }
  bool isNl() const override { return true; } int osOf() const override { return os; } bool lowOf() const override { return cur.low > 0.5; } int chOf() const override { return cur.ch1 > 0.5 ? 1 : 2; }
};

// Moduswechsel (linear/Röhre, Überabtastung, Buchse, Kanal): neue Schaltung rechnet die letzten 150 ms nach und wird über 30 ms eingeblendet
struct Engine {
  double fs; Tube tube; CoreBase *a = nullptr, *b = nullptr; double xf = 0; std::vector<float> hist; int hpos = 0, hfill = 0; int qpos = 0, qlen = 0, qdone = 0; bool replay = false; Target last; bool hasLast = false;
  Speaker spk; bool spkOn = false;
  explicit Engine(double fs_) : fs(fs_) { tube.makeTab(); hist.assign((int)std::lround(0.15 * fs), 0); spk.design(fs_); }
  ~Engine() { delete a; delete b; }
  static std::string kindOf(bool nl, int os, bool low) { return (nl ? "nl" + std::to_string(os) : std::string("lin")) + (low ? "|low" : "|high"); }
  std::string kindOfCore(const CoreBase *c) const { return kindOf(c->isNl(), c->osOf(), c->lowOf()); }
  CoreBase *make(const Target &t) { CoreBase *c = t.nl ? (CoreBase*)new NlCore(fs, t.os, &tube) : (CoreBase*)new LinCore(fs); c->setTarget(t); return c; }
  void setTarget(const Target &t) {
    last = t; hasLast = true; if (t.spk != spkOn) { spkOn = t.spk; spk.lo.reset(); spk.le.reset(); spk.cone.reset(); }
    std::string kind = kindOf(t.nl, t.os, t.s.low > 0.5);
    if (!a) { a = make(t); return; }
    CoreBase *newest = b ? b : a;
    bool chChange = newest->chOf() != (t.s.ch1 > 0.5 ? 1 : 2);
    if (b && replay && (kind != kindOfCore(b) || chChange)) { delete b; b = nullptr; replay = false; }   // während des Nachrechnens erneut geändert: verwerfen
    if (!b && (kind != kindOfCore(newest) || chChange)) {
      b = make(t); qlen = std::min(hfill, (int)hist.size()); qpos = (hpos - qlen + (int)hist.size()) % (int)hist.size(); qdone = 0; replay = true; xf = 0;
    } else { a->setTarget(t); if (b) b->setTarget(t); }
  }
  void block(float *buf, int n) {
    if (!a) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    if (b && replay) { int todo = std::min(384, qlen - qdone); for (int i = 0; i < todo; i++) { b->tick(); b->step(hist[qpos]); qpos = (qpos + 1) % (int)hist.size(); qdone++; } if (qdone >= qlen) replay = false; }
    for (int i = 0; i < n; i++) {
      double x = buf[i]; hist[hpos] = (float)x; hpos = (hpos + 1) % (int)hist.size(); if (hfill < (int)hist.size()) hfill++;
      a->tick(); double y = a->step(x);
      if (b) { b->tick(); double yb = b->step(x);
        if (!replay) { xf += 1.0 / (0.03 * fs); if (xf >= 1) { delete a; a = b; b = nullptr; y = yb; } else y = y * (1 - xf) + yb * xf; } }
      if (spkOn) y = spk.step(y);
      buf[i] = (float)-y;   // Polarität wie am Eingang: der Vorverstärker (5 invertierende Stufen) dreht das Signal, für Aufnahme und Vergleich wird das hier aufgehoben
    }
  }
};
// Spitzenwerte der Gitter-Kathoden-Aussteuerung (relativ, 1 = Übersteuerungseinsatz): linear p·vin/Bezug, Röhre p/|Vgk0|
inline void takePeaksFrom(Engine *E, double *out) {
  for (int i = 0; i < 5; i++) out[i] = 0; if (!E || !E->a) return; CoreBase *c = E->b ? E->b : E->a;
  if (c->isNl()) { NlCore *n = (NlCore*)c; for (int i = 0; i < 5; i++) { out[i] = n->amp.peak[i] / std::max(1e-9, std::fabs(n->amp.vgkDC[i])); n->amp.peak[i] = 0; } }
  else { LinCore *l = (LinCore*)c; for (int i = 0; i < 5; i++) { out[i] = l->peaks[i] * l->vin / MONREF[i]; l->peaks[i] = 0; } }
}
} // namespace aor
