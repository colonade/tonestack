// Allgemeiner Schaltungslöser im Zeitbereich (nichtlineare Zustandsraum-Methode, "DK-Methode"):
// Lineares Netz aus Widerständen, Kondensatoren, gekoppelten Spulen und Quellen (Norton: Spannung hinter Widerstand),
// dazu nichtlineare Tore (Spannung zwischen zwei Knoten, Strom fließt zwischen denselben Knoten). Trapezregel.
// Je Abtastwert: Torspannungen v = p + K·i(v) per Newton lösen, danach Zweigspannungen und Zustände fortschreiben.
// Das lineare Netz wird bei Änderungen (Potis) neu übersetzt; die Zustände (Kondensatorspannungen, Spulenströme) bleiben erhalten.
#pragma once
#include <cmath>
#include <complex>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
namespace dk {
using Vec = std::vector<double>;
struct Mat { int r = 0, c = 0; Vec d; Mat() {} Mat(int r_, int c_) : r(r_), c(c_), d((size_t)r_ * c_, 0.0) {}
  double &operator()(int i, int j) { return d[(size_t)i * c + j]; } double operator()(int i, int j) const { return d[(size_t)i * c + j]; } };
// Inverse per Gauß-Jordan mit Spaltenpivot; false bei singulärer Matrix
inline bool invert(Mat &a) { int n = a.r; Mat b(n, n); for (int i = 0; i < n; i++) b(i, i) = 1;
  for (int k = 0; k < n; k++) { int p = k; double m = std::fabs(a(k, k)); for (int i = k + 1; i < n; i++) if (std::fabs(a(i, k)) > m) { m = std::fabs(a(i, k)); p = i; }
    if (m < 1e-300) return false;
    if (p != k) for (int j = 0; j < n; j++) { std::swap(a(k, j), a(p, j)); std::swap(b(k, j), b(p, j)); }
    double iv = 1 / a(k, k); for (int j = 0; j < n; j++) { a(k, j) *= iv; b(k, j) *= iv; }
    for (int i = 0; i < n; i++) if (i != k) { double f = a(i, k); if (f == 0) continue; for (int j = 0; j < n; j++) { a(i, j) -= f * a(k, j); b(i, j) -= f * b(k, j); } } }
  a = b; return true; }
// LU-Lösung eines kleinen Systems (in place), für Newton. solveN: dieselbe Rechnung mit fester Größe (der Übersetzer entrollt die Schleifen;
// Ergebnis bitgleich zu solve), Auswahl über solveAny.
template <int n> inline bool solveN(double *A, double *x) {
  for (int k = 0; k < n; k++) { int p = k; double m = std::fabs(A[k * n + k]); for (int i = k + 1; i < n; i++) if (std::fabs(A[i * n + k]) > m) { m = std::fabs(A[i * n + k]); p = i; }
    if (m < 1e-300) return false;
    if (p != k) { for (int j = 0; j < n; j++) std::swap(A[k * n + j], A[p * n + j]); std::swap(x[k], x[p]); }
    double iv = 1 / A[k * n + k];
    for (int i = k + 1; i < n; i++) { double f = A[i * n + k] * iv; if (f == 0) continue; for (int j = k + 1; j < n; j++) A[i * n + j] -= f * A[k * n + j]; x[i] -= f * x[k]; } }
  for (int k = n - 1; k >= 0; k--) { double s = x[k]; for (int j = k + 1; j < n; j++) s -= A[k * n + j] * x[j]; x[k] = s / A[k * n + k]; }
  return true; }
inline bool solve(int n, double *A, double *x);
// LU-Zerlegung mit Zeilentausch, Faktoren bleiben in A (L unter, U auf und über der Diagonale), Tausch in pv; luSolve wendet sie auf x an.
// Für Newton mit wiederverwendeter Jacobi-Matrix (Runner::reuseJ). luN: feste Größe (entrollt), luAny wählt.
template <int n> inline bool luN(double *A, int *pv) {
  for (int k = 0; k < n; k++) { int p = k; double m = std::fabs(A[k * n + k]); for (int i = k + 1; i < n; i++) if (std::fabs(A[i * n + k]) > m) { m = std::fabs(A[i * n + k]); p = i; }
    if (m < 1e-300) return false;
    pv[k] = p; if (p != k) for (int j = 0; j < n; j++) std::swap(A[k * n + j], A[p * n + j]);
    double iv = 1 / A[k * n + k];
    for (int i = k + 1; i < n; i++) { double f = A[i * n + k] * iv; A[i * n + k] = f; if (f == 0) continue; for (int j = k + 1; j < n; j++) A[i * n + j] -= f * A[k * n + j]; } }
  return true; }
template <int n> inline void lusN(const double *A, const int *pv, double *x) {
  for (int k = 0; k < n; k++) if (pv[k] != k) std::swap(x[k], x[pv[k]]);
  for (int k = 0; k < n; k++) { double xk = x[k]; for (int i = k + 1; i < n; i++) x[i] -= A[i * n + k] * xk; }
  for (int k = n - 1; k >= 0; k--) { double s = x[k]; for (int j = k + 1; j < n; j++) s -= A[k * n + j] * x[j]; x[k] = s / A[k * n + k]; } }
inline bool luG(int n, double *A, int *pv) {
  for (int k = 0; k < n; k++) { int p = k; double m = std::fabs(A[k * n + k]); for (int i = k + 1; i < n; i++) if (std::fabs(A[i * n + k]) > m) { m = std::fabs(A[i * n + k]); p = i; }
    if (m < 1e-300) return false;
    pv[k] = p; if (p != k) for (int j = 0; j < n; j++) std::swap(A[k * n + j], A[p * n + j]);
    double iv = 1 / A[k * n + k];
    for (int i = k + 1; i < n; i++) { double f = A[i * n + k] * iv; A[i * n + k] = f; if (f == 0) continue; for (int j = k + 1; j < n; j++) A[i * n + j] -= f * A[k * n + j]; } }
  return true; }
inline void lusG(int n, const double *A, const int *pv, double *x) {
  for (int k = 0; k < n; k++) if (pv[k] != k) std::swap(x[k], x[pv[k]]);
  for (int k = 0; k < n; k++) { double xk = x[k]; for (int i = k + 1; i < n; i++) x[i] -= A[i * n + k] * xk; }
  for (int k = n - 1; k >= 0; k--) { double s = x[k]; for (int j = k + 1; j < n; j++) s -= A[k * n + j] * x[j]; x[k] = s / A[k * n + k]; } }
#define DK_LU(N) case N: return luN<N>(A, pv);
#define DK_LS(N) case N: lusN<N>(A, pv, x); return;
inline bool luAny(int n, double *A, int *pv) {
  switch (n) { DK_LU(1) DK_LU(2) DK_LU(3) DK_LU(4) DK_LU(5) DK_LU(6) DK_LU(7) DK_LU(8) DK_LU(9) DK_LU(10) DK_LU(11) DK_LU(12) DK_LU(13) DK_LU(14) DK_LU(15) DK_LU(16)
  DK_LU(17) DK_LU(18) DK_LU(19) DK_LU(20) DK_LU(21) DK_LU(22) DK_LU(23) DK_LU(24) DK_LU(25) DK_LU(26) DK_LU(27) DK_LU(28) DK_LU(29) DK_LU(30) DK_LU(31) DK_LU(32) default: return luG(n, A, pv); } }
inline void lusAny(int n, const double *A, const int *pv, double *x) {
  switch (n) { DK_LS(1) DK_LS(2) DK_LS(3) DK_LS(4) DK_LS(5) DK_LS(6) DK_LS(7) DK_LS(8) DK_LS(9) DK_LS(10) DK_LS(11) DK_LS(12) DK_LS(13) DK_LS(14) DK_LS(15) DK_LS(16)
  DK_LS(17) DK_LS(18) DK_LS(19) DK_LS(20) DK_LS(21) DK_LS(22) DK_LS(23) DK_LS(24) DK_LS(25) DK_LS(26) DK_LS(27) DK_LS(28) DK_LS(29) DK_LS(30) DK_LS(31) DK_LS(32) default: lusG(n, A, pv, x); } }
#undef DK_LU
#undef DK_LS
inline bool solveAny(int n, double *A, double *x) {
  switch (n) { case 1: return solveN<1>(A, x); case 2: return solveN<2>(A, x); case 3: return solveN<3>(A, x); case 4: return solveN<4>(A, x);
    case 5: return solveN<5>(A, x); case 6: return solveN<6>(A, x); case 7: return solveN<7>(A, x); case 8: return solveN<8>(A, x);
    case 9: return solveN<9>(A, x); case 10: return solveN<10>(A, x); case 11: return solveN<11>(A, x); case 12: return solveN<12>(A, x);
    case 13: return solveN<13>(A, x); case 14: return solveN<14>(A, x); case 15: return solveN<15>(A, x); case 16: return solveN<16>(A, x);
#ifdef DK_BIGN   // nur wo definiert (SVT-CL ab 1.1): entrollte Löser bis 32 Tore (gleiche Rechnung wie solve, etwa 10 % schneller); andere Plugins bleiben bitgleich
    case 17: return solveN<17>(A, x); case 18: return solveN<18>(A, x); case 19: return solveN<19>(A, x); case 20: return solveN<20>(A, x); case 21: return solveN<21>(A, x); case 22: return solveN<22>(A, x); case 23: return solveN<23>(A, x); case 24: return solveN<24>(A, x); case 25: return solveN<25>(A, x); case 26: return solveN<26>(A, x); case 27: return solveN<27>(A, x); case 28: return solveN<28>(A, x); case 29: return solveN<29>(A, x); case 30: return solveN<30>(A, x); case 31: return solveN<31>(A, x); case 32: return solveN<32>(A, x);
#endif
    default: return solve(n, A, x); } }
inline bool solve(int n, double *A, double *x) {
  for (int k = 0; k < n; k++) { int p = k; double m = std::fabs(A[k * n + k]); for (int i = k + 1; i < n; i++) if (std::fabs(A[i * n + k]) > m) { m = std::fabs(A[i * n + k]); p = i; }
    if (m < 1e-300) return false;
    if (p != k) { for (int j = 0; j < n; j++) std::swap(A[k * n + j], A[p * n + j]); std::swap(x[k], x[p]); }
    double iv = 1 / A[k * n + k];
    for (int i = k + 1; i < n; i++) { double f = A[i * n + k] * iv; if (f == 0) continue; for (int j = k + 1; j < n; j++) A[i * n + j] -= f * A[k * n + j]; x[i] -= f * x[k]; } }
  for (int k = n - 1; k >= 0; k--) { double s = x[k]; for (int j = k + 1; j < n; j++) s -= A[k * n + j] * x[j]; x[k] = s / A[k * n + k]; }
  return true; }

// Netzliste. Knoten 0 = Masse; Namen werden bei Bedarf angelegt.
struct Net {
  std::map<std::string, int> idx; int nn = 0;
  struct R2 { int a, b; double v; }; std::vector<R2> R, C;
  struct Cpl { std::vector<std::pair<int,int>> w; Mat L; }; std::vector<Cpl> L;    // gekoppelte Wicklungen: Induktivitätsmatrix (H)
  struct Src { int a; double r; int in; }; std::vector<Src> S;                     // Eingang in: Spannung hinter r gegen Masse
  struct Port { int a, b; }; std::vector<Port> P;
  struct Out { int a, b; }; std::vector<Out> O;                                    // Ausgänge: Spannung a − b
  struct VC { int a, b, c, d; double g; }; std::vector<VC> G;                       // gesteuerte Stromquelle: g·(v_c − v_d) fließt von a nach b durch das Element
  struct IS { int a, b, in; double k; }; std::vector<IS> I;                         // Stromquelle k·u[in] von a nach b durch das Element
  int n(const std::string &s) { if (s == "0") return 0; auto it = idx.find(s); if (it != idx.end()) return it->second; return idx[s] = ++nn; }
  void r(const std::string &a, const std::string &b, double v) { R.push_back({n(a), n(b), std::max(v, 1e-3)}); }
  void c(const std::string &a, const std::string &b, double v) { C.push_back({n(a), n(b), v}); }
  void src(const std::string &a, double rs, int in) { S.push_back({n(a), rs, in}); }
  int port(const std::string &a, const std::string &b) { P.push_back({n(a), n(b)}); return (int)P.size() - 1; }
  void vccs(const std::string &a, const std::string &b, const std::string &c, const std::string &d, double g) { G.push_back({n(a), n(b), n(c), n(d), g}); }
  void isrc(const std::string &a, const std::string &b, int in, double k) { I.push_back({n(a), n(b), in, k}); }
  int out(const std::string &a, const std::string &b = "0") { O.push_back({n(a), n(b)}); return (int)O.size() - 1; }
  void coupled(const std::vector<std::pair<std::string,std::string>> &w, const Mat &Lm) { Cpl k; for (auto &x : w) k.w.push_back({n(x.first), n(x.second)}); k.L = Lm; L.push_back(k); }
};

// Übersetztes System. Zweige: zuerst alle Kondensatoren, dann alle Wicklungen (in Reihenfolge der Kopplungsgruppen).
struct Sys {
  int nn = 0, nh = 0, m = 0, nu = 0, no = 0, ncap = 0; double T = 0;
  Mat Pbh, Pbu, Pbi, Pph, Ppu, K, Ooh, Oou, Ooi;      // Zweigspannungen, Torspannungen, Ausgänge aus (h, u, i)
  Vec gc;                                             // Kondensatoren: 2C/T
  std::vector<Mat> gl;                                // Wicklungsgruppen: T/2·L⁻¹
  std::vector<int> glOff;
  Mat tPbh, tPbu, tPbi, tPph, tPpu, tK;               // spaltenweise Kopien (Index [Spalte][Zeile]) für vektorisierbare Produkte
  // Dünn besetzte Zeilen (nur Einträge ≠ 0) der zusammengesetzten Produkte über z = [h, u, i] (ab Laney 3.26 / AmpFett 1.11; vorher viele kurze axpy):
  // cP: p = [Pph Ppu]·[h u], cK: K·i, cB: vb = [Pbh Pbu Pbi]·z, cO: y = [Ooh Oou Ooi]·z
  // Dicht besetzte Produkte (ab 2026-11-07, wenn mindestens 60 % der Einträge ≠ 0 sind): Zeilen zusammenhängend, Skalarprodukt mit zwei 2er-Vektoren
  // (SSE2/NEON), ohne Indexzugriff; deutlich schneller als die dünne Form bei den meist fast vollen Matrizen cB, cO. Summationsreihenfolge anders →
  // Ergebnis nicht bitgleich (Rundung, etwa 1e-16 relativ). dense = false erzwingt die dünne Form.
  struct Csr { std::vector<int> st, col; Vec val; int nc = 0; bool dn = false; Vec D;
    void add(const Mat &a, int off, int row) { for (int j = 0; j < a.c; j++) { double v = a(row, j); if (v != 0) { col.push_back(off + j); val.push_back(v); } } }
    void fin(int ncols, bool dense) { nc = ncols; const int nr = (int)st.size() - 1; dn = dense && nr > 0 && nc >= 8 && val.size() * 10 >= (size_t)nr * nc * 6; D.clear();
      if (dn) { D.assign((size_t)nr * nc, 0.0); for (int r = 0; r < nr; r++) for (int k = st[r]; k < st[r + 1]; k++) D[(size_t)r * nc + col[k]] = val[k]; } }
    void apply(const double *__restrict z, double *__restrict y) const { const int nr = (int)st.size() - 1;
      if (dn) { typedef double v2 __attribute__((vector_size(16))); const double *__restrict M = D.data(); const int n4 = nc & ~3;
        for (int r = 0; r < nr; r++, M += nc) { v2 s0 = {0, 0}, s1 = {0, 0}; int j = 0;
          for (; j < n4; j += 4) { v2 a0, a1, z0, z1; std::memcpy(&a0, M + j, 16); std::memcpy(&a1, M + j + 2, 16); std::memcpy(&z0, z + j, 16); std::memcpy(&z1, z + j + 2, 16); s0 += a0 * z0; s1 += a1 * z1; }
          double t = (s0[0] + s1[0]) + (s0[1] + s1[1]); for (; j < nc; j++) t += M[j] * z[j]; y[r] = t; }
        return; }
      const int *__restrict S = st.data(), *__restrict C = col.data(); const double *__restrict V = val.data();
      for (int r = 0; r < nr; r++) { double a0 = 0, a1 = 0; int k = S[r]; const int e = S[r + 1];
        for (; k + 1 < e; k += 2) { a0 += V[k] * z[C[k]]; a1 += V[k + 1] * z[C[k + 1]]; } if (k < e) a0 += V[k] * z[C[k]]; y[r] = a0 + a1; } } };
  Csr cP, cK, cB, cO;
  bool ok = false;
  static Mat tr(const Mat &a) { Mat t(a.c, a.r); for (int i = 0; i < a.r; i++) for (int j = 0; j < a.c; j++) t(j, i) = a(i, j); return t; }
  void finish() { tPbh = tr(Pbh); tPbu = tr(Pbu); tPbi = tr(Pbi); tPph = tr(Pph); tPpu = tr(Ppu); tK = tr(K);
    cP = cK = cB = cO = Csr();
    for (int r = 0; r < m; r++) { cP.st.push_back((int)cP.col.size()); cP.add(Pph, 0, r); cP.add(Ppu, nh, r); } cP.st.push_back((int)cP.col.size());
    for (int r = 0; r < m; r++) { cK.st.push_back((int)cK.col.size()); cK.add(K, 0, r); } cK.st.push_back((int)cK.col.size());
    for (int r = 0; r < nh; r++) { cB.st.push_back((int)cB.col.size()); cB.add(Pbh, 0, r); cB.add(Pbu, nh, r); cB.add(Pbi, nh + nu, r); } cB.st.push_back((int)cB.col.size());
    for (int r = 0; r < no; r++) { cO.st.push_back((int)cO.col.size()); cO.add(Ooh, 0, r); cO.add(Oou, nh, r); cO.add(Ooi, nh + nu, r); } cO.st.push_back((int)cO.col.size());
    cP.fin(nh + nu, false); cK.fin(m, false); cB.fin(nh + nu + m, false); cO.fin(nh + nu + m, false); }
  void setDense(bool d) { cP.fin(cP.nc, d); cK.fin(cK.nc, d); cB.fin(cB.nc, d); cO.fin(cO.nc, d); }
};
inline Sys compile(const Net &net, double fs) {
  Sys s; s.T = 1 / fs; s.nn = net.nn; s.m = (int)net.P.size(); s.no = (int)net.O.size(); s.ncap = (int)net.C.size();
  for (auto &x : net.S) s.nu = std::max(s.nu, x.in + 1);
  for (auto &x : net.I) s.nu = std::max(s.nu, x.in + 1);
  std::vector<std::pair<int,int>> br; for (auto &c : net.C) br.push_back({c.a, c.b});
  for (auto &g : net.L) { s.glOff.push_back((int)br.size()); for (auto &w : g.w) br.push_back(w); }
  s.nh = (int)br.size(); int N = s.nn;
  Mat G(N, N);
  auto stamp = [&](int a, int b, double g) { if (a) G(a-1, a-1) += g; if (b) G(b-1, b-1) += g; if (a && b) { G(a-1, b-1) -= g; G(b-1, a-1) -= g; } };
  for (auto &x : net.R) stamp(x.a, x.b, 1 / x.v);
  for (auto &x : net.S) stamp(x.a, 0, 1 / x.r);
  for (auto &x : net.G) { auto add = [&](int r_, int c_, double v) { if (r_ && c_) G(r_-1, c_-1) += v; }; add(x.a, x.c, x.g); add(x.a, x.d, -x.g); add(x.b, x.c, -x.g); add(x.b, x.d, x.g); }
  s.gc.resize(s.ncap); for (int k = 0; k < s.ncap; k++) { s.gc[k] = 2 * net.C[k].v / s.T; stamp(net.C[k].a, net.C[k].b, s.gc[k]); }
  for (size_t g = 0; g < net.L.size(); g++) { Mat Li = net.L[g].L; invert(Li); for (auto &v : Li.d) v *= s.T / 2; s.gl.push_back(Li);
    int k = Li.r; for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) { double v = Li(i, j);   // Zweigleitwert-Matrix in die Knoten einprägen
      int ai = net.L[g].w[i].first, bi = net.L[g].w[i].second, aj = net.L[g].w[j].first, bj = net.L[g].w[j].second;
      auto add = [&](int r_, int c_, double x) { if (r_ && c_) G(r_-1, c_-1) += x; };
      add(ai, aj, v); add(ai, bj, -v); add(bi, aj, -v); add(bi, bj, v); } }
  for (int i = 0; i < N; i++) if (G(i, i) == 0) G(i, i) = 1e-12;   // schwebende Knoten (sollte nicht vorkommen)
  Mat M = G; if (!invert(M)) return s;
  // Spalten: Zweig k prägt −h in a und +h in b ein (Strom h fließt a→b durch den Zweig)
  auto colH = [&](int k, int row) { int a = br[k].first, b = br[k].second; double v = 0; if (a) v -= M(row, a-1); if (b) v += M(row, b-1); return v; };   // (M·(−S))[row][k]
  auto colU = [&](int u, int row) { double v = 0; for (auto &x : net.S) if (x.in == u && x.a) v += M(row, x.a-1) / x.r;
    for (auto &x : net.I) if (x.in == u) { if (x.a) v -= x.k * M(row, x.a-1); if (x.b) v += x.k * M(row, x.b-1); } return v; };
  auto colI = [&](int p, int row) { int a = net.P[p].a, b = net.P[p].b; double v = 0; if (a) v -= M(row, a-1); if (b) v += M(row, b-1); return v; };   // Torstrom fließt a→b durch das Bauteil: verlässt a
  auto rowD = [&](int a, int b, auto col, int j) { double v = 0; if (a) v += col(j, a-1); if (b) v -= col(j, b-1); return v; };
  s.Pbh = Mat(s.nh, s.nh); s.Pbu = Mat(s.nh, s.nu); s.Pbi = Mat(s.nh, s.m);
  for (int k = 0; k < s.nh; k++) { int a = br[k].first, b = br[k].second;
    for (int j = 0; j < s.nh; j++) s.Pbh(k, j) = rowD(a, b, colH, j);
    for (int j = 0; j < s.nu; j++) s.Pbu(k, j) = rowD(a, b, colU, j);
    for (int j = 0; j < s.m; j++) s.Pbi(k, j) = rowD(a, b, colI, j); }
  s.Pph = Mat(s.m, s.nh); s.Ppu = Mat(s.m, s.nu); s.K = Mat(s.m, s.m);
  for (int k = 0; k < s.m; k++) { int a = net.P[k].a, b = net.P[k].b;
    for (int j = 0; j < s.nh; j++) s.Pph(k, j) = rowD(a, b, colH, j);
    for (int j = 0; j < s.nu; j++) s.Ppu(k, j) = rowD(a, b, colU, j);
    for (int j = 0; j < s.m; j++) s.K(k, j) = rowD(a, b, colI, j); }
  s.Ooh = Mat(s.no, s.nh); s.Oou = Mat(s.no, s.nu); s.Ooi = Mat(s.no, s.m);
  for (int k = 0; k < s.no; k++) { int a = net.O[k].a, b = net.O[k].b;
    for (int j = 0; j < s.nh; j++) s.Ooh(k, j) = rowD(a, b, colH, j);
    for (int j = 0; j < s.nu; j++) s.Oou(k, j) = rowD(a, b, colU, j);
    for (int j = 0; j < s.m; j++) s.Ooi(k, j) = rowD(a, b, colI, j); }
  s.finish(); s.ok = true; return s;
}
// Knotenspannungen aus dem Zustand (für Prüfungen/Anzeige, nicht im Audiopfad): v = M(−S h + B u + Nᵀ i)
inline Vec nodeVoltages(const Net &net, double fs, const Vec &h, const Vec &u, const Vec &i) {
  Net tmp = net; for (int k = 1; k <= net.nn; k++) tmp.O.push_back({k, 0});
  Sys s = compile(tmp, fs); Vec v(net.nn + 1, 0.0);
  for (int k = 0; k < net.nn; k++) { int o = (int)net.O.size() + k; double y = 0;
    for (int j = 0; j < s.nh; j++) y += s.Ooh(o, j) * h[j]; for (int j = 0; j < s.nu; j++) y += s.Oou(o, j) * u[j]; for (int j = 0; j < s.m; j++) y += s.Ooi(o, j) * i[j];
    v[k + 1] = y; }
  return v; }

// Kleinsignal-Frequenzgang: Netz mit linearisierten Geräten (Ableitungsblöcke J je Gerät im Arbeitspunkt), Eingang `in` mit 1 V,
// Ausgang = Spannung zwischen a und b. Komplexe Knotenanalyse.
inline std::complex<double> acResponse(const Net &net, const std::vector<std::vector<double>> &Jdev, const std::vector<std::pair<int,int>> &devPorts, double f, int in, int oa, int ob) {
  typedef std::complex<double> C; int N = net.nn; std::vector<C> Y((size_t)N * N, C(0, 0)), b(N, C(0, 0)); double w = 2 * 3.14159265358979323846 * f;
  auto add = [&](int r_, int c_, C v) { if (r_ && c_) Y[(size_t)(r_-1) * N + c_-1] += v; };
  auto stamp = [&](int a, int bb, C g) { add(a, a, g); add(bb, bb, g); add(a, bb, -g); add(bb, a, -g); };
  for (auto &x : net.R) stamp(x.a, x.b, 1 / x.v);
  for (auto &x : net.C) stamp(x.a, x.b, C(0, w * x.v));
  for (auto &x : net.S) { stamp(x.a, 0, 1 / x.r); if (x.in == in && x.a) b[x.a-1] += 1 / x.r; }
  for (auto &x : net.G) { add(x.a, x.c, x.g); add(x.a, x.d, -x.g); add(x.b, x.c, -x.g); add(x.b, x.d, x.g); }
  for (auto &g : net.L) { int k = g.L.r; std::vector<C> Z((size_t)k * k); for (int i = 0; i < k * k; i++) Z[i] = C(0, w) * g.L.d[i];
    // Z invertieren (klein)
    std::vector<C> I((size_t)k * k, C(0, 0)); for (int i = 0; i < k; i++) I[i * k + i] = 1;
    for (int c = 0; c < k; c++) { int p = c; for (int i = c + 1; i < k; i++) if (std::abs(Z[i * k + c]) > std::abs(Z[p * k + c])) p = i;
      if (p != c) for (int j = 0; j < k; j++) { std::swap(Z[c * k + j], Z[p * k + j]); std::swap(I[c * k + j], I[p * k + j]); }
      C iv = C(1, 0) / Z[c * k + c]; for (int j = 0; j < k; j++) { Z[c * k + j] *= iv; I[c * k + j] *= iv; }
      for (int i = 0; i < k; i++) if (i != c) { C fct = Z[i * k + c]; for (int j = 0; j < k; j++) { Z[i * k + j] -= fct * Z[c * k + j]; I[i * k + j] -= fct * I[c * k + j]; } } }
    for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) { C v = I[i * k + j]; int ai = g.w[i].first, bi = g.w[i].second, aj = g.w[j].first, bj = g.w[j].second; add(ai, aj, v); add(ai, bj, -v); add(bi, aj, -v); add(bi, bj, v); } }
  for (size_t d = 0; d < devPorts.size(); d++) { int p0 = devPorts[d].first, np = devPorts[d].second; const std::vector<double> &J = Jdev[d];
    for (int a = 0; a < np; a++) for (int bb = 0; bb < np; bb++) { double v = J[a * np + bb]; if (v == 0) continue; int ra = net.P[p0 + a].a, rb = net.P[p0 + a].b, ca = net.P[p0 + bb].a, cb = net.P[p0 + bb].b;
      add(ra, ca, v); add(ra, cb, -v); add(rb, ca, -v); add(rb, cb, v); } }
  for (int i = 0; i < N; i++) if (Y[(size_t)i * N + i] == C(0, 0)) Y[(size_t)i * N + i] = 1e-12;
  for (int c = 0; c < N; c++) { int p = c; double mx = std::abs(Y[(size_t)c * N + c]); for (int i = c + 1; i < N; i++) if (std::abs(Y[(size_t)i * N + c]) > mx) { mx = std::abs(Y[(size_t)i * N + c]); p = i; }
    if (p != c) { for (int j = 0; j < N; j++) std::swap(Y[(size_t)c * N + j], Y[(size_t)p * N + j]); std::swap(b[c], b[p]); }
    C iv = C(1, 0) / Y[(size_t)c * N + c];
    for (int i = c + 1; i < N; i++) { C fct = Y[(size_t)i * N + c] * iv; if (fct == C(0, 0)) continue; for (int j = c; j < N; j++) Y[(size_t)i * N + j] -= fct * Y[(size_t)c * N + j]; b[i] -= fct * b[c]; } }
  for (int c = N - 1; c >= 0; c--) { C sum = b[c]; for (int j = c + 1; j < N; j++) sum -= Y[(size_t)c * N + j] * b[j]; b[c] = sum / Y[(size_t)c * N + c]; }
  return (oa ? b[oa-1] : C(0, 0)) - (ob ? b[ob-1] : C(0, 0)); }

// Laufzeit: Zustand h (Verlaufsströme), letzte Torspannungen als Newton-Startwert.
// Gerät: wertet seine Tore aus (Ströme und Ableitungen dI/dV innerhalb des Geräts, Blockdiagonale).
// sc: Streuung (Faktor auf die Ströme, z. B. ungleiche Endröhren); mem: Gerät mit Gedächtnis (Kernfluss eines Übertragers):
// st = Fluss bis zum letzten Abtastwert, vl = letzte Torspannung, ht = T/2 (vom Runner gesetzt). Das Gerät rechnet φ = st + ht·(vl + v).
// Hysterese (Übertragerkern): bis 8 „Stop“-Glieder mit Radius hr[k]; ihr Zustand hs[k] folgt dem Fluss, auf ±hr[k] begrenzt (Prandtl-Ishlinskii).
struct Dev { int kind; int p0; int np; const void *model; double sc = 1; bool mem = false; double st = 0, vl = 0, ht = 0; int nhs = 0; double hs[8] = {}, hr[8] = {}; };
using DevFn = void (*)(const Dev &, const double *v, double *i, double *J);   // J: np×np, Zeilen = Ströme, Spalten = Spannungen
// y[0..n) += a·x[0..n)
static inline void axpy(int n, double a, const double *__restrict x, double *__restrict y) { if (a == 0) return; for (int i = 0; i < n; i++) y[i] += a * x[i]; }
// Geräte werden in Gruppen gelöst (Block-Gauß-Seidel über Gruppen, Newton innerhalb jeder Gruppe), bis alle Torgleichungen
// gemeinsam erfüllt sind. Das Ergebnis ist dasselbe wie mit einem Newton über alle Tore; die Gruppen sind nur schwach gekoppelt
// (z. B. Vorstufe, Endstufe, Netzteil), daher genügen meist ein bis zwei Durchgänge, und die Gleichungssysteme bleiben klein.
struct Runner {
  Sys s; Vec h, vp, ip, vb, p, Jd, A, x, g, q, di, vp1, v0; bool damp = false; std::vector<Dev> dev; DevFn fn = nullptr; int maxIt = 12, maxPass = 4, gsPass = 6; double tol = 1e-5, stepLim = 30; int lastIt = 0, lastPass = 0, fails = 0;
  // reuseJ (ab 2026-11-07, je Plugin zuschaltbar): vereinfachtes Newton je Gruppe – die zerlegte Matrix A = I − K·J wird über Iterationen und Abtastwerte
  // weiterverwendet und erst neu aufgebaut, wenn der Fehler je Schritt nicht mindestens auf reuseRho fällt (oder nach Netzänderung, Fehlschlag, Rückfall).
  // Gleiches Ziel (Fehler < tol), anderer Weg: Abweichung in der Größenordnung von tol. Aus = bitgleich zur bisherigen Rechnung.
  bool dense = true;    // dichte Produkte (Sys::Csr), siehe dort; ab 2026-11-07 für alle Plugins (Unterschied nur Rundung)
  bool reuseStep = false; int reuseMinN = 8;   // wie reuseJ, aber Zerlegung nur innerhalb eines Abtastwerts (Iterationen und Durchgänge), je Abtastwert neu; nur Gruppen ab reuseMinN Toren
  bool dampFull = false;   // gemeinsamer Solve gleich mit Schrittweitensteuerung (wie im Rückfall), statt erst nach maxPass Fehlversuchen
  bool reuseJ = false; double reuseRho = 0.25; std::vector<Vec> luA; std::vector<std::vector<int>> luP; std::vector<char> luOK; long luFact = 0, luReuse = 0;   // tol bis Laney 3.25 / AmpFett 1.10: 1e-6 V (jetzt 15–20 % weniger Newton-Schritte, Abweichung leise unter −100 dB)
  std::vector<std::vector<int>> grpDev, grpPort; std::vector<int> devGrp; std::vector<int> capOut; Vec capI;   // capOut: Kondensatoren, deren Strom (a→b) mit ausgegeben wird   // devGrp: Gruppe je Gerät (vom Aufrufer gesetzt, sonst alle in einer)
  Vec z, kq;   // z = [h, u, i] für die dünnen Produkte; kq = K·i
  struct Blk { int p0, np; }; std::vector<std::vector<Blk>> grpBlk;   // Geräte der Gruppe in Reihenfolge der Tore (für buildA)
  void set(const Sys &ns) { bool first = h.empty() || (int)h.size() != ns.nh; s = ns; if (dense) s.setDense(true); if (first) { h.assign(s.nh, 0.0); } if ((int)vp.size() != s.m) { vp.assign(s.m, 0.0); ip.assign(s.m, 0.0); }
    vb.assign(s.nh, 0.0); p.assign(s.m, 0.0); if (Jd.size() != (size_t)s.m * 16) Jd.assign((size_t)s.m * 16, 0.0); A.assign((size_t)s.m * s.m, 0.0); x.assign(s.m, 0.0); v0.assign(s.m, 0.0); g.assign(s.m, 0.0); q.assign(s.m, 0.0); di.assign(s.m, 0.0); vp1 = vp;
    int ng = 0; for (size_t k = 0; k < dev.size(); k++) ng = std::max(ng, k < devGrp.size() ? devGrp[k] + 1 : 1);
    grpDev.assign(ng + 1, {}); grpPort.assign(ng + 1, {});   // letzte Gruppe: alle Geräte zusammen (Rückfall, wenn die Gruppen einander nicht schnell genug einig werden)
    for (size_t k = 0; k < dev.size(); k++) { int G = k < devGrp.size() ? devGrp[k] : 0; for (int GG : {G, ng}) { grpDev[GG].push_back((int)k); for (int q = 0; q < dev[k].np; q++) grpPort[GG].push_back(dev[k].p0 + q); } }
    memDev.clear(); for (size_t k = 0; k < dev.size(); k++) if (dev[k].mem) { dev[k].ht = s.T / 2; memDev.push_back((int)k); }
    z.assign((size_t)s.nh + s.nu + s.m, 0.0); kq.assign(s.m, 0.0);
    luA.assign(ng + 1, {}); luP.assign(ng + 1, {}); luOK.assign(ng + 1, 0); for (int G = 0; G <= ng; G++) { luA[G].assign(grpPort[G].size() * grpPort[G].size(), 0.0); luP[G].assign(grpPort[G].size(), 0); }
    grpBlk.assign(ng + 1, {}); for (int G = 0; G <= ng; G++) for (int dk_ : grpDev[G]) grpBlk[G].push_back({dev[dk_].p0, dev[dk_].np});
  }
  std::vector<int> memDev;   // Geräte mit Gedächtnis (Fluss), nach jedem Abtastwert fortgeschrieben
  // Gedächtnis aus einem früheren Geräteverzeichnis übernehmen (nach Neuaufbau des Netzes, gleiche Reihenfolge)
  static void keepMem(std::vector<Dev> &nw, const std::vector<Dev> &old) { for (size_t k = 0; k < nw.size() && k < old.size(); k++) if (nw[k].mem && old[k].mem) { nw[k].st = old[k].st; nw[k].vl = old[k].vl; for (int j = 0; j < 8; j++) nw[k].hs[j] = old[k].hs[j]; } }
  // Gerät k auswerten: Ströme nach ip, Ableitungsblock nach Jd[p0*16 ...] (Zeilen je Tor, bis 16 Spalten; Geräte also höchstens 4 Tore)
  void evalDev(int k) { const Dev &d = dev[k]; double j[16]; fn(d, &vp[d.p0], &ip[d.p0], j); for (int a = 0; a < d.np; a++) for (int b = 0; b < d.np; b++) Jd[(size_t)(d.p0 + a) * 16 + b] = j[a * d.np + b]; }
  // q = p + K·i wird mitgeführt: Änderungen der Ströme einer Gruppe gehen spaltenweise ein
  double resid(int r) { return vp[r] - q[r]; }
  void evalGroup(int G) { const std::vector<int> &P = grpPort[G]; for (int c : P) di[c] = ip[c]; for (int k : grpDev[G]) evalDev(k);
    for (int c : P) { double d = ip[c] - di[c]; if (d != 0) axpy(s.m, d, &s.tK.d[(size_t)c * s.m], q.data()); } }
  // A = I − K_GG·J_GG zeilenweise je Gerät (J blockdiagonal): Zeile von K über die Tore des Geräts mal dessen Block von J. Ab 2026-11-07 statt
  // Spaltenliste mit Indizes; gleiche Rechenreihenfolge je Element (δ, dann −K·J in Torreihenfolge), also bitgleich, aber ohne Umweg über Indizes.
  void buildA(int G, double *__restrict AA) const {
    const std::vector<int> &P = grpPort[G]; const int n = (int)P.size(); const Blk *bl = grpBlk[G].data(); const int nb = (int)grpBlk[G].size(); const double *JD = Jd.data();
    for (int ri = 0; ri < n; ri++) { const int pr = P[ri]; const double *kr = &s.K.d[(size_t)pr * s.m]; double *ar = AA + (size_t)ri * n;
      for (int bi = 0; bi < nb; bi++) { const int p0 = bl[bi].p0, np = bl[bi].np; const double *kv = kr + p0, *J = JD + (size_t)p0 * 16;
        switch (np) {
          case 1: ar[0] = (pr == p0 ? 1.0 : 0.0) - kv[0] * J[0]; break;
          case 2: { const double k0 = kv[0], k1 = kv[1];
            ar[0] = ((pr == p0 ? 1.0 : 0.0) - k0 * J[0]) - k1 * J[16]; ar[1] = ((pr == p0 + 1 ? 1.0 : 0.0) - k0 * J[1]) - k1 * J[17]; break; }
          case 3: { const double k0 = kv[0], k1 = kv[1], k2 = kv[2];
            for (int b = 0; b < 3; b++) ar[b] = (((pr == p0 + b ? 1.0 : 0.0) - k0 * J[b]) - k1 * J[16 + b]) - k2 * J[32 + b]; break; }
          default: for (int b = 0; b < np; b++) { double a = (pr == p0 + b) ? 1 : 0; for (int q = 0; q < np; q++) a -= kv[q] * J[q * 16 + b]; ar[b] = a; } }
        ar += np; } } }
  // wie solveGroup, aber mit weiterverwendeter Zerlegung (reuseJ); ohne Dämpfung (der gedämpfte Rückfall rechnet immer frisch)
  double solveGroupR(int G) {
    const std::vector<int> &P = grpPort[G]; const int n = (int)P.size(); double first = 0, prev = 0; bool lastFresh = false; double *LU = luA[G].data(); int *PV = luP[G].data();
    for (int it = 0; it < maxIt; it++) {
      double err = 0; for (int i = 0; i < n; i++) { g[i] = resid(P[i]); err = std::max(err, std::fabs(g[i])); }
      if (it == 0) first = err;
      if (!(err < 1e30)) { fails++; luOK[G] = 0; return err; }
      if (err < tol) break;
      if (it > 0 && !lastFresh && !(err <= reuseRho * prev)) luOK[G] = 0;   // alte Matrix zieht nicht mehr genug: neu aufbauen
      lastFresh = false;
      if (!luOK[G]) {
        buildA(G, LU);
        if (!luAny(n, LU, PV)) { fails++; break; }
        luOK[G] = 1; lastFresh = true; luFact++; } else luReuse++;
      for (int i = 0; i < n; i++) x[i] = -g[i];
      lusAny(n, LU, PV, x.data());
      { double mx = 0; for (int i = 0; i < n; i++) mx = std::max(mx, std::fabs(x[i])); double sc = mx > stepLim ? stepLim / mx : 1;
        for (int i = 0; i < n; i++) vp[P[i]] += sc * x[i]; evalGroup(G); }
      prev = err; lastIt++;
    }
    return first; }
  // Newton für Gruppe G bei festen Strömen der übrigen Gruppen; Rückgabe: größter Anfangsfehler
  double solveGroup(int G) {
    if (!damp && (reuseJ || (reuseStep && (int)grpPort[G].size() >= reuseMinN))) return solveGroupR(G);
    const std::vector<int> &P = grpPort[G]; const int n = (int)P.size(); double first = 0;
    for (int it = 0; it < maxIt; it++) {
      double err = 0; for (int i = 0; i < n; i++) { g[i] = resid(P[i]); err = std::max(err, std::fabs(g[i])); }
      if (it == 0) first = err;
      if (!(err < 1e30)) { fails++; return err; }   // NaN/Überlauf: nicht als konvergiert werten
      if (err < tol) break;
      // A = I − K_GG·J_GG (J blockdiagonal je Gerät); gleiche Rechenreihenfolge wie bis 3.25
      buildA(G, A.data()); for (int ri = 0; ri < n; ri++) x[ri] = -g[ri];
      if (!solveAny(n, A.data(), x.data())) { fails++; break; }
      { double mx = 0; for (int i = 0; i < n; i++) mx = std::max(mx, std::fabs(x[i])); double sc = mx > stepLim ? stepLim / mx : 1;
        if (!damp) { for (int i = 0; i < n; i++) vp[P[i]] += sc * x[i]; evalGroup(G); }
        else {   // nur im Rückfall (ab Laney 3.33 / AmpFett 1.14): Schritt halbieren, solange der Fehler wächst. Knicke in den Kennlinien (Hysterese des
                 // Ausgangsübertragers) ließen Newton sonst zwischen zwei Abschnitten pendeln (AmpFett STANDBY bei kräftigem Eingang: bis 20-fache Echtzeit je Block)
          for (int i = 0; i < n; i++) v0[i] = vp[P[i]]; double a = 1;
          for (int ls = 0; ls < 6; ls++, a *= 0.5) { for (int i = 0; i < n; i++) vp[P[i]] = v0[i] + a * sc * x[i]; evalGroup(G);
            double e = 0; for (int i = 0; i < n; i++) e = std::max(e, std::fabs(resid(P[i]))); if (e < err) break; } } }
      lastIt++;
    }
    return first; }
  void step(const double *u, double *y) {
    const int m = s.m, nh = s.nh, nu = s.nu;
    if (reuseStep) std::fill(luOK.begin(), luOK.end(), (char)0);
    double *zz = z.data(); for (int j = 0; j < nh; j++) zz[j] = h[j]; for (int j = 0; j < nu; j++) zz[nh + j] = u[j];
    s.cP.apply(zz, p.data());
    for (int k = 0; k < m; k++) { double v0 = vp[k]; vp[k] = 2 * v0 - vp1[k]; vp1[k] = v0; }   // Startwert: lineare Vorhersage aus den letzten beiden Abtastwerten
    for (size_t k = 0; k < dev.size(); k++) evalDev((int)k);
    s.cK.apply(ip.data(), kq.data()); for (int k = 0; k < m; k++) q[k] = p[k] + kq[k];
    lastIt = 0; int pass = 0; const int ng = (int)grpPort.size() - 1; bool ok = false;
    // Gruppen nacheinander (Block-Gauß-Seidel); nach gsPass Durchgängen ohne Einigung Newton über alle Tore gemeinsam
    for (; pass < gsPass; pass++) { double e = 0; for (int G = 0; G < ng; G++) { double r = solveGroup(G); if (!(r <= e)) e = r; } if (e < tol) { ok = true; break; } }   // NaN bleibt NaN
    if (!ok) { damp = dampFull; for (int k = 0; k < maxPass; k++, pass++) if (solveGroup(ng) < tol) { ok = true; break; } damp = false; }
    if (!ok) {   // Rückfall: vom Wert des letzten Abtastwerts aus, kleinere Schritte, mehr Iterationen
      for (int k = 0; k < m; k++) vp[k] = vp1[k]; for (size_t k = 0; k < dev.size(); k++) evalDev((int)k);
      s.cK.apply(ip.data(), kq.data()); for (int k = 0; k < m; k++) q[k] = p[k] + kq[k];
      int mi = maxIt; double sl = stepLim; maxIt = 60; stepLim = 10; damp = true; for (int k = 0; k < 4 && !ok; k++, pass++) if (solveGroup(ng) < tol) ok = true; damp = false; maxIt = mi; stepLim = sl; }
    lastPass = pass; if (!ok) fails++;
    for (int k : memDev) { Dev &d = dev[k]; double v = vp[d.p0], dphi = d.ht * (d.vl + v);
      for (int j = 0; j < d.nhs; j++) d.hs[j] = std::max(-d.hr[j], std::min(d.hr[j], d.hs[j] + dphi));
      d.st += dphi; d.vl = v; }
    for (int j = 0; j < m; j++) zz[nh + nu + j] = ip[j];
    s.cB.apply(zz, vb.data()); s.cO.apply(zz, y);
    capI.resize(capOut.size()); for (size_t k = 0; k < capOut.size(); k++) { int c = capOut[k]; capI[k] = s.gc[c] * vb[c] + h[c]; }
    for (int k = 0; k < s.ncap; k++) h[k] = -(2 * s.gc[k] * vb[k] + h[k]);
    for (size_t gi = 0; gi < s.gl.size(); gi++) { const Mat &G = s.gl[gi]; int o = s.glOff[gi];
      double t[16]; for (int i = 0; i < G.r; i++) { double a = 0; for (int j = 0; j < G.c; j++) a += G(i, j) * vb[o + j]; t[i] = 2 * a + h[o + i]; }
      for (int i = 0; i < G.r; i++) h[o + i] = t[i]; }
  }
};
} // namespace dk
