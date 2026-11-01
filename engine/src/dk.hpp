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
// LU-Lösung eines kleinen Systems (in place), für Newton
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
  bool ok = false;
  static Mat tr(const Mat &a) { Mat t(a.c, a.r); for (int i = 0; i < a.r; i++) for (int j = 0; j < a.c; j++) t(j, i) = a(i, j); return t; }
  void finish() { tPbh = tr(Pbh); tPbu = tr(Pbu); tPbi = tr(Pbi); tPph = tr(Pph); tPpu = tr(Ppu); tK = tr(K); }
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
  Sys s; Vec h, vp, ip, vb, p, Jd, A, x, g, q, di, vp1; std::vector<Dev> dev; DevFn fn = nullptr; int maxIt = 12, maxPass = 4, gsPass = 6; double tol = 1e-6, stepLim = 30; int lastIt = 0, lastPass = 0, fails = 0;
  std::vector<std::vector<int>> grpDev, grpPort; std::vector<int> devGrp; std::vector<int> capOut; Vec capI;   // capOut: Kondensatoren, deren Strom (a→b) mit ausgegeben wird   // devGrp: Gruppe je Gerät (vom Aufrufer gesetzt, sonst alle in einer)
  void set(const Sys &ns) { bool first = h.empty() || (int)h.size() != ns.nh; s = ns; if (first) { h.assign(s.nh, 0.0); } if ((int)vp.size() != s.m) { vp.assign(s.m, 0.0); ip.assign(s.m, 0.0); }
    vb.assign(s.nh, 0.0); p.assign(s.m, 0.0); if (Jd.size() != (size_t)s.m * 16) Jd.assign((size_t)s.m * 16, 0.0); A.assign((size_t)s.m * s.m, 0.0); x.assign(s.m, 0.0); g.assign(s.m, 0.0); q.assign(s.m, 0.0); di.assign(s.m, 0.0); vp1 = vp;
    int ng = 0; for (size_t k = 0; k < dev.size(); k++) ng = std::max(ng, k < devGrp.size() ? devGrp[k] + 1 : 1);
    grpDev.assign(ng + 1, {}); grpPort.assign(ng + 1, {});   // letzte Gruppe: alle Geräte zusammen (Rückfall, wenn die Gruppen einander nicht schnell genug einig werden)
    for (size_t k = 0; k < dev.size(); k++) { int G = k < devGrp.size() ? devGrp[k] : 0; for (int GG : {G, ng}) { grpDev[GG].push_back((int)k); for (int q = 0; q < dev[k].np; q++) grpPort[GG].push_back(dev[k].p0 + q); } }
    memDev.clear(); for (size_t k = 0; k < dev.size(); k++) if (dev[k].mem) { dev[k].ht = s.T / 2; memDev.push_back((int)k); } }
  std::vector<int> memDev;   // Geräte mit Gedächtnis (Fluss), nach jedem Abtastwert fortgeschrieben
  // Gedächtnis aus einem früheren Geräteverzeichnis übernehmen (nach Neuaufbau des Netzes, gleiche Reihenfolge)
  static void keepMem(std::vector<Dev> &nw, const std::vector<Dev> &old) { for (size_t k = 0; k < nw.size() && k < old.size(); k++) if (nw[k].mem && old[k].mem) { nw[k].st = old[k].st; nw[k].vl = old[k].vl; for (int j = 0; j < 8; j++) nw[k].hs[j] = old[k].hs[j]; } }
  // Gerät k auswerten: Ströme nach ip, Ableitungsblock nach Jd[p0*16 ...] (Zeilen je Tor, bis 16 Spalten; Geräte also höchstens 4 Tore)
  void evalDev(int k) { const Dev &d = dev[k]; double j[16]; fn(d, &vp[d.p0], &ip[d.p0], j); for (int a = 0; a < d.np; a++) for (int b = 0; b < d.np; b++) Jd[(size_t)(d.p0 + a) * 16 + b] = j[a * d.np + b]; }
  // q = p + K·i wird mitgeführt: Änderungen der Ströme einer Gruppe gehen spaltenweise ein
  double resid(int r) { return vp[r] - q[r]; }
  void evalGroup(int G) { const std::vector<int> &P = grpPort[G]; for (int c : P) di[c] = ip[c]; for (int k : grpDev[G]) evalDev(k);
    for (int c : P) { double d = ip[c] - di[c]; if (d != 0) axpy(s.m, d, &s.tK.d[(size_t)c * s.m], q.data()); } }
  // Newton für Gruppe G bei festen Strömen der übrigen Gruppen; Rückgabe: größter Anfangsfehler
  double solveGroup(int G) {
    const std::vector<int> &P = grpPort[G]; const int n = (int)P.size(), m = s.m; double first = 0;
    for (int it = 0; it < maxIt; it++) {
      double err = 0; for (int i = 0; i < n; i++) { g[i] = resid(P[i]); err = std::max(err, std::fabs(g[i])); }
      if (it == 0) first = err;
      if (!(err < 1e30)) { fails++; return err; }   // NaN/Überlauf: nicht als konvergiert werten
      if (err < tol) break;
      // A = I − K_GG·J_GG (J blockdiagonal je Gerät)
      for (int ri = 0; ri < n; ri++) { const double *kr = &s.K.d[(size_t)P[ri] * m]; double *ar = &A[(size_t)ri * n]; int ci = 0;
        for (int dk_ : grpDev[G]) { const Dev &d = dev[dk_]; for (int b = 0; b < d.np; b++, ci++) { double a = (P[ri] == d.p0 + b) ? 1 : 0;
            for (int q = 0; q < d.np; q++) a -= kr[d.p0 + q] * Jd[(size_t)(d.p0 + q) * 16 + b]; ar[ci] = a; } }
        x[ri] = -g[ri]; }
      if (!solve(n, A.data(), x.data())) { fails++; break; }
      { double mx = 0; for (int i = 0; i < n; i++) mx = std::max(mx, std::fabs(x[i])); double sc = mx > stepLim ? stepLim / mx : 1; for (int i = 0; i < n; i++) vp[P[i]] += sc * x[i]; }
      evalGroup(G);
      lastIt++;
    }
    return first; }
  void step(const double *u, double *y) {
    const int m = s.m, nh = s.nh, nu = s.nu;
    std::fill(p.begin(), p.end(), 0.0); for (int j = 0; j < nh; j++) axpy(m, h[j], &s.tPph.d[(size_t)j * m], p.data()); for (int j = 0; j < nu; j++) axpy(m, u[j], &s.tPpu.d[(size_t)j * m], p.data());
    for (int k = 0; k < m; k++) { double v0 = vp[k]; vp[k] = 2 * v0 - vp1[k]; vp1[k] = v0; }   // Startwert: lineare Vorhersage aus den letzten beiden Abtastwerten
    for (size_t k = 0; k < dev.size(); k++) evalDev((int)k);
    q = p; for (int c = 0; c < m; c++) axpy(m, ip[c], &s.tK.d[(size_t)c * m], q.data());
    lastIt = 0; int pass = 0; const int ng = (int)grpPort.size() - 1; bool ok = false;
    // Gruppen nacheinander (Block-Gauß-Seidel); nach gsPass Durchgängen ohne Einigung Newton über alle Tore gemeinsam
    for (; pass < gsPass; pass++) { double e = 0; for (int G = 0; G < ng; G++) { double r = solveGroup(G); if (!(r <= e)) e = r; } if (e < tol) { ok = true; break; } }   // NaN bleibt NaN
    if (!ok) for (int k = 0; k < maxPass; k++, pass++) if (solveGroup(ng) < tol) { ok = true; break; }
    if (!ok) {   // Rückfall: vom Wert des letzten Abtastwerts aus, kleinere Schritte, mehr Iterationen
      for (int k = 0; k < m; k++) vp[k] = vp1[k]; for (size_t k = 0; k < dev.size(); k++) evalDev((int)k);
      q = p; for (int c = 0; c < m; c++) axpy(m, ip[c], &s.tK.d[(size_t)c * m], q.data());
      int mi = maxIt; double sl = stepLim; maxIt = 60; stepLim = 10; for (int k = 0; k < 4 && !ok; k++, pass++) if (solveGroup(ng) < tol) ok = true; maxIt = mi; stepLim = sl; }
    lastPass = pass; if (!ok) fails++;
    for (int k : memDev) { Dev &d = dev[k]; double v = vp[d.p0], dphi = d.ht * (d.vl + v);
      for (int j = 0; j < d.nhs; j++) d.hs[j] = std::max(-d.hr[j], std::min(d.hr[j], d.hs[j] + dphi));
      d.st += dphi; d.vl = v; }
    std::fill(vb.begin(), vb.end(), 0.0); for (int j = 0; j < nh; j++) axpy(nh, h[j], &s.tPbh.d[(size_t)j * nh], vb.data());
    for (int j = 0; j < nu; j++) axpy(nh, u[j], &s.tPbu.d[(size_t)j * nh], vb.data()); for (int j = 0; j < m; j++) axpy(nh, ip[j], &s.tPbi.d[(size_t)j * nh], vb.data());
    for (int o = 0; o < s.no; o++) { double a = 0; for (int j = 0; j < nh; j++) a += s.Ooh(o, j) * h[j]; for (int j = 0; j < nu; j++) a += s.Oou(o, j) * u[j]; for (int j = 0; j < m; j++) a += s.Ooi(o, j) * ip[j]; y[o] = a; }
    capI.resize(capOut.size()); for (size_t k = 0; k < capOut.size(); k++) { int c = capOut[k]; capI[k] = s.gc[c] * vb[c] + h[c]; }
    for (int k = 0; k < s.ncap; k++) h[k] = -(2 * s.gc[k] * vb[k] + h[k]);
    for (size_t gi = 0; gi < s.gl.size(); gi++) { const Mat &G = s.gl[gi]; int o = s.glOff[gi];
      double t[16]; for (int i = 0; i < G.r; i++) { double a = 0; for (int j = 0; j < G.c; j++) a += G(i, j) * vb[o + j]; t[i] = 2 * a + h[o + i]; }
      for (int i = 0; i < G.r; i++) h[o + i] = t[i]; }
  }
};
} // namespace dk
