// Prüfung Cultec EQ-1A (cultec_core.hpp): Ruhespannungen gegen den Werksplan, Verstärkung, Frequenzgänge gegen die Werkskurven, Klirr, Aussteuerung.
//   build/cultec_test [dc|gain|curves|flat|thd|all] (ohne Argument: all)
#include "cultec_core.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
using namespace cultec;
static int bad = 0;
static void chk(const char *what, double v, double lo, double hi) { bool ok = v >= lo && v <= hi; if (!ok) bad++; std::printf("  %-44s %10.4g  [%g … %g] %s\n", what, v, lo, hi, ok ? "" : "ABWEICHUNG"); }
static double db(double x) { return 20 * std::log10(x); }

// ---- Werkskurven (docs/cultec/werkskurven.json): kleiner Leser für genau diese Datei ----
#include <fstream>
#include <sstream>
struct Curve { std::string key, sub; std::vector<std::pair<double,double>> pts; };
static std::vector<Curve> loadCurves(const char *path, double &pkB, double &pkS) {
  std::ifstream f(path); std::stringstream ss; ss << f.rdbuf(); std::string t = ss.str(); std::vector<Curve> out; size_t i = 0; pkB = pkS = 0;
  auto str = [&]() { size_t a = t.find('"', i), b = t.find('"', a + 1); i = b + 1; return t.substr(a + 1, b - a - 1); };
  for (const char *k : {"bass_boost", "bass_atten", "hoehen_atten", "hoehen_boost_3k"}) {
    i = t.find(std::string("\"") + k + "\""); i = t.find('{', i) + 1; int depth = 1;
    while (depth > 0) { while (i < t.size() && std::isspace((unsigned char)t[i])) i++; if (t[i] == '}') { depth--; i++; continue; } if (t[i] == ',') { i++; continue; }
      Curve c; c.key = k; c.sub = str(); i = t.find('[', i) + 1;
      while (true) { while (std::isspace((unsigned char)t[i]) || t[i] == ',') i++; if (t[i] == ']') { i++; break; }
        i = t.find('[', i) + 1; char *e; double x = std::strtod(t.c_str() + i, &e); i = e - t.c_str(); i = t.find(',', i) + 1; double y = std::strtod(t.c_str() + i, &e); i = e - t.c_str(); i = t.find(']', i) + 1; c.pts.push_back({x, y}); }
      out.push_back(c); } }
  size_t a = t.find("\"breit_dB\""); if (a != std::string::npos) pkB = std::strtod(t.c_str() + t.find(':', a) + 1, nullptr);
  a = t.find("\"schmal_dB\""); if (a != std::string::npos) pkS = std::strtod(t.c_str() + t.find(':', a) + 1, nullptr);
  return out; }
// Einstellung zu einer Werkskurve (Maximalkurven: Regler auf 10)
static const char *curvesPath() { for (const char *c : {"docs/cultec/werkskurven.json", "../docs/cultec/werkskurven.json", "../../docs/cultec/werkskurven.json"}) { std::ifstream f(c); if (f) return c; } return "docs/cultec/werkskurven.json"; }
static Setting curveSetting(const Curve &c) { Setting s; s.lb = s.la = s.hb = s.hc = 0; s.in = true;
  auto idx = [](const double *t, int n, double v) { for (int k = 0; k < n; k++) if (std::fabs(t[k] - v) < 1e-9) return k; return -1; };
  if (c.key == "bass_boost") { s.lb = 1; s.lf = idx(LF_HZ, NLF, std::atof(c.sub.c_str())); }
  else if (c.key == "bass_atten") { s.la = 1; s.lf = idx(LF_HZ, NLF, std::atof(c.sub.c_str())); }
  else if (c.key == "hoehen_atten") { s.hc = 1; s.hcf = idx(HC_KHZ, NHC, std::atof(c.sub.c_str())); }
  else { s.hb = 1; s.hf = 0; s.bw = c.sub == "breit" ? 1 : 0; }
  return s; }
static Core settled(const Parts &p, const Setting &s, double fs = 48000, double sec = 30) { Core c; c.init(fs, p, s); c.settle(sec); return c; }
static void dc(const Parts &p, bool print) {
  Core c = settled(p, Setting());
  std::vector<double> u = {0, p.vdc}; dk::Vec V = dk::nodeVoltages(c.net, c.fs, c.run.h, u, c.run.ip);
  auto at = [&](const char *n) { return V[c.net.idx.at(n)]; };
  std::printf("Ruhespannungen (Plan):\n");
  chk("+345 V", at("B345"), 335, 355); chk("+325 V", at("B325"), 315, 335); chk("+290 V", at("B290"), 280, 300); chk("+250 V", at("B250"), 240, 260);
  chk("+31 V (Gitterableitung 12AU7)", at("B31"), 29, 33);
  chk("Anode 12AX7 +140 V", at("A1"), 130, 150); chk("Kathode 12AX7 +1,3 V", at("K1"), 1.1, 1.5); chk("Abgriff S +1,1 V", at("S"), 0.9, 1.3);
  chk("Kathode 12AU7 +40 V", at("KA"), 36, 44);
  if (print) { std::printf("#define CULTEC_REST {"); bool first = true; for (auto &kv : c.net.idx) { std::printf("%s{\"%s\", %.4f}", first ? "" : ", ", kv.first.c_str(), V[kv.second]); first = false; } std::printf("}\n"); } }

int main(int argc, char **argv) {
  const char *m = argc > 1 ? argv[1] : "all"; bool all = !std::strcmp(m, "all");
  Parts p;
  // Überschreiben einzelner Bauteile: name=wert (nur zum Ausprobieren)
  for (int a = 2; a < argc; a++) { const char *e = std::strchr(argv[a], '='); if (!e) continue; std::string k(argv[a], e - argv[a]); double x = std::atof(e + 1);
    std::map<std::string, double*> f = {{"in_lp", &p.in_lp}, {"in_k", &p.in_k}, {"in_cw", &p.in_cw}, {"is_lp", &p.is_lp}, {"is_k", &p.is_k}, {"is_cs", &p.is_cs}, {"is_rc", &p.is_rc},
      {"ot_lp", &p.ot_lp}, {"ot_kpp", &p.ot_kpp}, {"ot_kps", &p.ot_kps}, {"ot_cpp", &p.ot_cpp}, {"ot_nf", &p.ot_nf}, {"plb", &p.plb}, {"pla", &p.pla}};
    if (f.count(k)) *f[k] = x; else { std::printf("unbekannt: %s\n", k.c_str()); return 2; } }
  if (all || !std::strcmp(m, "dc") || !std::strcmp(m, "rest")) dc(p, !std::strcmp(m, "rest"));
  if (all || !std::strcmp(m, "gain")) {
    Core c = settled(p, Setting());
    for (double f : {10., 20., 50., 100., 1000., 5000., 10000., 20000., 40000.}) { auto o = c.response(f, O_OUT), i = c.response(f, O_IN);
      std::printf("  %6.0f Hz: Ausgang/Eingangsklemme %+6.2f dB, Eingangsklemme/Quelle %+6.2f dB\n", f, db(std::abs(o / i)), db(std::abs(i))); }
    // Schleifenverstärkung grob: Gegenkopplung aus (Wicklung 0 Windungen) gegen an
    Parts q = p; q.ot_nf = 1e-6; Core d = settled(q, Setting(), 48000, 5);
    std::printf("  Gegenkopplung: %.2f dB weniger Verstärkung (%s)\n", db(std::abs(d.response(1000) / c.response(1000))), std::abs(d.response(1000)) > std::abs(c.response(1000)) ? "gegenkoppelnd" : "MITKOPPELND");
  }
  if (all || !std::strcmp(m, "curves") || !std::strcmp(m, "flat")) {   // Frequenzgänge gegen die Werkskurven (dB gegen 1 kHz)
    Core c = settled(p, Setting(), 48000, 3);
    const std::complex<double> r0 = c.response(1000);   // Bezug: Regler 0 (flach) bei 1 kHz, wie in den Werksdiagrammen
    auto rel = [&](const Setting &s, double f) { Core d = c; d.cur = s; return db(std::abs(d.response(f) / r0)); };
    std::printf("Flach (Regler 0), dB gegen 1 kHz:"); double fmin = 0, fmax = 0; for (double f : {10., 20., 30., 50., 100., 2e3, 5e3, 7e3, 1e4, 1.3e4, 1.5e4, 2e4, 3e4}) { double r = rel(Setting(), f); std::printf(" %g:%+.2f", f, r); if (f >= 20 && f <= 20000) { fmin = std::min(fmin, r); fmax = std::max(fmax, r); } } std::printf("\n");
    chk("flach 20 Hz–20 kHz: tiefster Wert (Werk −1 dB)", fmin, -1.0, 0); chk("flach 20 Hz–20 kHz: höchster Wert (Werk +0 dB)", fmax, 0, 0.3);
    if (std::strcmp(m, "flat")) {
    double pkB, pkS; auto cv = loadCurves(curvesPath(), pkB, pkS); double se = 0; int ne = 0; if (cv.size() != 13) { std::printf("Werkskurven nicht gelesen (%s)\n", curvesPath()); bad++; }
    for (auto &k : cv) { Setting s = curveSetting(k); double e2 = 0; std::printf("  %-15s %-6s", k.key.c_str(), k.sub.c_str());
      for (auto &pt : k.pts) { double r = rel(s, pt.first), e = r - pt.second; e2 += e * e; se += e * e; ne++; std::printf(" %g:%+.1f/%+.1f", pt.first, r, pt.second); }
      std::printf("  | %.2f dB\n", std::sqrt(e2 / k.pts.size())); }
    std::printf("  Effektivfehler aller Kurven %.2f dB (%d Punkte)\n", std::sqrt(se / ne), ne); chk("Effektivfehler gegen die Werkskurven (dB)", std::sqrt(se / ne), 0, 1.0);
    double pkd = 0, fd = 0; for (int bw = 0; bw <= 1; bw++) { std::printf("  Höhenboost %s Spitze (Werk %+.1f):", bw ? "breit " : "schmal", bw ? pkB : pkS);
      for (int k = 0; k < NHF; k++) { Setting s; s.hb = 1; s.hf = k; s.bw = bw; double best = -99, fb = 0; for (double f = HF_KHZ[k] * 500; f < HF_KHZ[k] * 2000; f *= 1.005) { double r = rel(s, f); if (r > best) { best = r; fb = f; } }
        std::printf(" %g k %+.1f@%.2fk", HF_KHZ[k], best, fb / 1000); pkd = std::max(pkd, std::fabs(best - (bw ? pkB : pkS))); fd = std::max(fd, std::fabs(std::log2(fb / (HF_KHZ[k] * 1000)))); } std::printf("\n"); }
    chk("Höhenboost: Spitze gegen Werk (dB, größte Abw.)", pkd, 0, 1.0); chk("Höhenboost schmal/breit: Lage (Oktaven)", fd, 0, 0.12); } }
  if (!std::strcmp(m, "fit")) {   // Anpassung geschätzter Größen an die Werkskurven (Nelder-Mead in log): HS-29 Lp und Eisenverluste, HS-56 Lp, Spulengüte
    double pkB, pkS; auto cv = loadCurves(curvesPath(), pkB, pkS); Core c = settled(p, Setting(), 48000, 3);
    auto put = [&](Parts &q, const std::vector<double> &x) { q.is_lp = std::exp(x[0]); q.is_rc = std::exp(x[1]); q.in_lp = std::exp(x[2]); for (int k = 0; k < NHF; k++) q.qHF[k] = std::exp(x[3]); if (x.size() > 4) { q.lHF[0] = p.lHF[0] * std::exp(x[4]); q.cHF[0] = p.cHF[0] / std::exp(x[4]); } };
    auto cost = [&](const std::vector<double> &x) { Parts q = p; put(q, x); Core d = c; d.v = q; d.cur = Setting(); std::complex<double> r0 = d.response(1000); double e = 0;
      auto rel = [&](const Setting &s, double f) { d.cur = s; return db(std::abs(d.response(f) / r0)); };
      for (auto &k : cv) { Setting s = curveSetting(k); for (auto &pt : k.pts) { double r = rel(s, pt.first) - pt.second; e += r * r; } }
      for (int bw = 0; bw <= 1; bw++) for (int k : {0, 3, 6}) { Setting s; s.hb = 1; s.hf = k; s.bw = bw; double a = HF_KHZ[k] * 700, b = HF_KHZ[k] * 1400;
        for (int it = 0; it < 25; it++) { double m1 = a * std::pow(b / a, 0.382), m2 = a * std::pow(b / a, 0.618); if (rel(s, m1) > rel(s, m2)) b = m2; else a = m1; }
        double r = rel(s, std::sqrt(a * b)) - (bw ? pkB : pkS); e += 3 * r * r; }
      return e; };
    std::vector<double> x0 = {std::log(p.is_lp), std::log(p.is_rc), std::log(p.in_lp), std::log(p.qHF[0]), 0.0}; int n = (int)x0.size();
    std::vector<std::vector<double>> S(n + 1, x0); for (int i = 0; i < n; i++) S[i + 1][i] += 0.5; std::vector<double> F(n + 1); for (int i = 0; i <= n; i++) F[i] = cost(S[i]);
    for (int it = 0; it < 150; it++) { std::vector<int> o(n + 1); for (int i = 0; i <= n; i++) o[i] = i; std::sort(o.begin(), o.end(), [&](int a, int b) { return F[a] < F[b]; });
      std::vector<std::vector<double>> S2; std::vector<double> F2; for (int i : o) { S2.push_back(S[i]); F2.push_back(F[i]); } S = S2; F = F2;
      if (it % 10 == 0) std::printf("  %3d: %.3f  Lp %.4g  Rc %.4g  inLp %.4g  Q %.4g\n", it, std::sqrt(F[0] / 111), std::exp(S[0][0]), std::exp(S[0][1]), std::exp(S[0][2]), std::exp(S[0][3]));
      std::vector<double> cen(n, 0); for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) cen[j] += S[i][j] / n;
      auto mix = [&](double t) { std::vector<double> r(n); for (int j = 0; j < n; j++) r[j] = cen[j] + t * (S[n][j] - cen[j]); return r; };
      auto xr = mix(-1); double fr = cost(xr);
      if (fr < F[0]) { auto xe = mix(-2); double fe = cost(xe); if (fe < fr) { S[n] = xe; F[n] = fe; } else { S[n] = xr; F[n] = fr; } }
      else if (fr < F[n - 1]) { S[n] = xr; F[n] = fr; }
      else { auto xc = mix(0.5); double fc = cost(xc); if (fc < F[n]) { S[n] = xc; F[n] = fc; } else for (int i = 1; i <= n; i++) { for (int j = 0; j < n; j++) S[i][j] = S[0][j] + 0.5 * (S[i][j] - S[0][j]); F[i] = cost(S[i]); } } }
    std::printf("  Ergebnis: is_lp %.5g is_rc %.5g in_lp %.5g qHF %.5g, 3 kHz: L %.5g C %.5g (Faktor %.4f)\n", std::exp(S[0][0]), std::exp(S[0][1]), std::exp(S[0][2]), std::exp(S[0][3]), p.lHF[0] * std::exp(S[0][4]), p.cHF[0] / std::exp(S[0][4]), std::exp(S[0][4])); }
  if (all || !std::strcmp(m, "start")) {   // Start aus den Ruhespannungen: Ausgang in Stille (V an 600 Ω)
    for (double fs : {44100., 96000., 192000.}) { Core c; c.init(fs, p, Setting()); double y[O_NO + 4], mx[4] = {0, 0, 0, 0}; long n = (long)fs;
      for (long i = 0; i < n; i++) { c.step(0, y); int w = i < fs * 0.05 ? 0 : i < fs * 0.2 ? 1 : i < fs * 0.5 ? 2 : 3; mx[w] = std::max(mx[w], std::fabs(y[O_OUT])); }
      std::printf("  %6.0f Hz: größter Ausgang 0–50 ms %.2e V, –200 ms %.2e, –500 ms %.2e, –1 s %.2e\n", fs, mx[0], mx[1], mx[2], mx[3]);
      chk("Start: Ausgang nach 200 ms (dBu)", db(std::max(mx[2], 1e-12) / 0.7746), -200, -80); } }
  if (all || !std::strcmp(m, "unit")) {   // Gerät (Überabtastung, Pegel): Sinus durch Unit gegen die analoge Rechnung (Kleinsignal, −30 dBFS)
    for (double fsB : {44100., 48000., 96000.}) for (int os : {1, 2, 4}) for (int cs = 0; cs < 3; cs++) {
      Setting st; if (cs == 1) { st.hb = 1; st.hf = 6; st.bw = 0; } if (cs == 2) { st.hc = 1; st.hcf = 2; st.lb = 1; st.lf = 3; }
      Unit u; u.init(fsB, os, p, st); Core a = u.c; std::complex<double> r0; { Core f = a; f.cur = Setting(); r0 = f.response(1000); }
      std::printf("  %3.0f kHz %dx→%dx %-10s", fsB / 1000, os, u.os, cs == 0 ? "flach" : cs == 1 ? "16k schmal" : "Atten20k+B");
      double worst = 0;
      for (double f : {100., 1000., 5000., 10000., 14000., 16000., 18000., 20000.}) { if (f > 0.45 * fsB) continue;
        int n = (int)(0.25 * fsB); std::vector<float> b(n); for (int i = 0; i < n; i++) b[i] = (float)(0.0316 * std::sin(2 * PI * f * i / fsB)); Unit w = u; w.process(b.data(), n);
        double re = 0, im = 0; int m0 = n / 2; for (int i = m0; i < n; i++) { double ph = 2 * PI * f * (i - Unit::LAT) / fsB; re += b[i] * std::sin(ph); im += b[i] * std::cos(ph); }
        double g = 2 * std::hypot(re, im) / (n - m0) / 0.0316, ref = std::abs(a.response(f) / r0), d = db(g) - db(ref); worst = std::max(worst, std::fabs(d));
        std::printf(" %gk:%+.2f", f / 1000, d); }
      std::printf("  | %.2f dB\n", worst);
      double fi = fsB * u.os; if (fi > 170000) chk("Abweichung zur analogen Rechnung (dB), intern ≥ 176 kHz", worst, 0, 0.5); else if (fi > 85000) chk("Abweichung (dB), intern 88–96 kHz", worst, 0, 2.0); } }
  if (!std::strcmp(m, "nf")) {   // Windungsverhältnis der Gegenkopplung für 0 dB Gesamtverstärkung (1 kHz, Regler 0, Ausgang/Eingangsklemme)
    double lo = 0.001, hi = 0.2; for (int it = 0; it < 30; it++) { Parts q = p; q.ot_nf = std::sqrt(lo * hi); Core c = settled(q, Setting(), 48000, 3);
      double g = db(std::abs(c.response(1000, O_OUT) / c.response(1000, O_IN))); if (g > 0) lo = q.ot_nf; else hi = q.ot_nf; }
    std::printf("  ot_nf = %.6f\n", std::sqrt(lo * hi)); }
  if (all || !std::strcmp(m, "thd")) {   // Klirr bei 1 kHz (und 50 Hz) über dem Ausgangspegel (dBm an 600 Ω)
    Core c0 = settled(p, Setting(), 96000, 20); double g = std::abs(c0.response(1000, O_OUT));
    for (double f : {1000., 50.}) for (double dbm : {0., 10., 15., 20., 21., 22., 24.}) {
      double vo = 0.7746 * std::pow(10, dbm / 20) * std::sqrt(2), a = vo / (g * std::abs(c0.response(f, O_OUT)) / g); Core c = c0;
      int per = (int)std::lround(c.fs / f), n = per * (int)std::ceil(0.2 * f), skip = per * (int)std::ceil(0.3 * f); std::vector<double> y; double yy[O_NO + 4];
      for (int i = 0; i < skip + n; i++) { c.step(a * std::sin(2 * PI * f * i / c.fs), yy); if (i >= skip) y.push_back(yy[O_OUT]); }
      auto harm = [&](int k) { double re = 0, im = 0; for (int i = 0; i < n; i++) { double ph = 2 * PI * k * i / per; re += y[i] * std::cos(ph); im += y[i] * std::sin(ph); } return 2 * std::hypot(re, im) / n; };
      double h1 = harm(1), hs = 0; for (int k = 2; k <= 9; k++) hs += harm(k) * harm(k);
      std::printf("  %5.0f Hz %+5.1f dBm: Ausgang %+6.2f dBm, Klirr %.3f %% (k2 %.1f, k3 %.1f dB)\n", f, dbm, db(h1 / std::sqrt(2) / 0.7746), 100 * std::sqrt(hs) / h1, db(harm(2) / h1), db(harm(3) / h1)); } }
  std::printf("%s (%d Abweichungen)\n", bad ? "FEHLER" : "fertig", bad); return bad ? 1 : 0; }
