// Prüfstand AmpFett 76 (Kern u76_core.hpp, nach dem Schaltplan UREI 1176LN): u76test
// Prüft gegen den Plan und die Angaben im Handbuch (Universal Audio 1176LN): Gleichspannungen, „Q“-Bias (1,0 dB), Ruhe (kein Gleichanteil, keine Schwingung),
// Frequenzgang, Kennlinien der vier RATIO-Tasten und „alle Tasten“, Attack- und Release-Zeiten, sowie die Erweiterungen (stufenlose Ratio, Hochpass in
// der Seitenkette, zweite Stufe, DRY). Ende mit „alle Prüfungen bestanden“ und Rückgabewert 0, sonst „ABWEICHUNG“ und 1.
#include "u76_core.hpp"
#include <cstdio>
#include <functional>
#include <future>
#include <atomic>
#include <string>
#include <thread>
#include <vector>
using namespace u76;
static int fails = 0;
static void chk(std::string &o, int &nf, const char *name, double got, double lo, double hi, const char *unit) { char b[400]; bool ok = got >= lo && got <= hi;
  std::snprintf(b, sizeof b, "  %-58s %10.4g %-4s (Soll %g … %g)%s\n", name, got, unit, lo, hi, ok ? "" : "   ABWEICHUNG"); o += b; if (!ok) nf++; }
struct Res { std::string o; int nf = 0; void c(const char *n, double g, double lo, double hi, const char *u = "") { chk(o, nf, n, g, lo, hi, u); } void p(const char *f) { o += f; } template <class A0, class... A> void p(const char *f, A0 a0, A... a) { char b[600]; std::snprintf(b, sizeof b, f, a0, a...); o += b; } };
static const double FS = 48000, DBU = 0.775 * 1.41421356237;   // Spitze bei 0 dBu
static Setting base() { Setting s; s.input = 0.5; s.output = 0.5; s.attack = 0.5; s.release = 0.5; s.ratio = 0; return s; }
static Core mk(const Setting &s, const Parts &p = Parts(), double fs = FS) { Core c; c.init(fs, p, s); c.settleFast(12, 0.3); return c; }
static double grdb(const Parts &p, double vgs) { return 20 * std::log10(1 + p.r6 * fetG(p.q1, vgs)); }   // Dämpfung des Abschwächers R6/FET gegenüber gesperrtem FET
// Sinus f, Spitze a (Volt), Dauer T; ausgewertet werden die letzten 0,2 s: Grundwelle am Ausgang (dBu), Gleichanteil, Rest (Klirr + Störung), mittlere GR
struct Tone { double dbu, dc, rest, gr, hold; };
static Tone tone(Core c, double f, double a, double T) { c.fixDev(); double y[O_NO], cs = 0, sn = 0, ss = 0, sm = 0, g = 0; long n = (long)(T * c.fs), m = (long)(0.2 * c.fs);
  for (long i = 0; i < n; i++) { double ph = 2 * PI * f * i / c.fs; c.step(a * std::sin(ph), y); if (i >= n - m) { cs += y[O_OUT] * std::cos(ph); sn += y[O_OUT] * std::sin(ph); ss += y[O_OUT] * y[O_OUT]; sm += y[O_OUT]; g += grdb(c.v, y[O_GATE]); } }
  double amp = 2 * std::hypot(cs, sn) / m, dc = sm / m, ac2 = ss / m - dc * dc; Tone t; t.dbu = 20 * std::log10(std::max(amp, 1e-12) / DBU); t.dc = dc; t.rest = std::sqrt(std::max(0.0, ac2 - amp * amp / 2)) / std::max(amp / std::sqrt(2.0), 1e-12); t.gr = g / m; t.hold = y[O_HOLD]; return t; }

// 1. Gleichspannungen gegen die Angaben im Plan
static Res tDC() { Res r; r.p("Gleichspannungen (Plan → Modell, Volt)\n"); Core c = mk(base()); auto V = c.volts();
  const struct { const char *n; double v, tol; } plan[] = {{"V30", 30.3, 0.1}, {"P26", 25.8, 0.4}, {"B2", 1.02, 0.15}, {"C2", 1.76, 0.25}, {"E2", 0.51, 0.1}, {"C3", 11.98, 0.6}, {"O4", 11.5, 0.6},
    {"B5", 4.98, 0.3}, {"E5", 4.54, 0.3}, {"A", 28.6, 0.4}, {"Bq", 29.1, 0.4}, {"E7", 29.7, 0.4}, {"D7", 14.06, 0.8}, {"OUT", 13.40, 0.8}, {"e8", 13.47, 0.8}, {"E9", 12.77, 0.8}, {"e9", 13.35, 0.8},
    {"P28", 27.8, 0.4}, {"K12", 15.1, 0.8}, {"B12", 4.55, 0.3}, {"E12", 4.03, 0.3}, {"E13", 14.54, 0.8}, {"B14", 3.51, 0.3}, {"K14", 17.25, 0.8}, {"E15", 16.72, 0.8}};
  for (auto &q : plan) { char nm[64]; std::snprintf(nm, sizeof nm, "%-4s Plan %6.2f", q.n, q.v); r.c(nm, V[q.n], q.v - q.tol, q.v + q.tol, "V"); }
  r.p("  Halte (C27) %.4f V, „Q“-Bias-Knoten %.4f V, Kette A (AC) %.4f V, −10 V %.3f\n", V["B"], V["Q18"], V["AC"], V["VN"]); return r; }

// 2. „Q“-Bias und Ruhe
static Res tBias() { Res r; r.p("„Q“-Bias (Handbuch: FET-Dämpfung 1,0 dB; Gyraf: Attack OFF) und Ruhe\n");
  auto gain = [](double qb, double att, double rel) { Parts p; p.qbias = qb; Setting s = base(); s.attack = att; s.release = rel; Core c = mk(s, p); return 20 * std::log10(std::abs(c.response(1000))); };
  double ref = gain(1.0, -1, 1), q = Parts().qbias; r.p("  Verstärkung bei gesperrtem FET, INPUT/OUTPUT Mitte: %.2f dB\n", ref);
  r.c("Dämpfung, Attack OFF, Release schnell (Abgleichpunkt)", ref - gain(q, -1, 1), 0.95, 1.05, "dB");
  r.c("Dämpfung, Attack 4, Release 4 (Grundstellung)", ref - gain(q, 0.5, 0.5), 0.6, 1.1, "dB");
  r.c("Dämpfung, Attack 4, Release langsam", ref - gain(q, 0.5, 0), 0.5, 1.1, "dB");
  // Ruhe: 2 s Stille gleich nach dem Einschwingen – Gleichanteil, Wechsel von Abtastwert zu Abtastwert (Schwingung bei halber Abtastrate), Drift des Haltepunkts
  for (int hp = 0; hp < 2; hp++) { Setting s = base(); s.hpf = hp; s.hpfF = 0.5; Core c = mk(s); double y[O_NO], h0 = 0, dcmax = 0, alt = 0, pk = 0, prev = 0, pprev = 0;
    for (long i = 0; i < (long)(2 * FS); i++) { c.step(0, y); if (i == 0) h0 = y[O_HOLD]; pk = std::max(pk, std::fabs(y[O_OUT])); if (i > 1) alt = std::max(alt, std::fabs(y[O_PRE] - 2 * prev + pprev) / 4); pprev = prev; prev = y[O_PRE]; if (i % 4800 == 0) dcmax = std::max(dcmax, std::fabs(y[O_OUT])); }
    r.p("  %s:\n", hp ? "mit Hochpass in der Seitenkette" : "ohne Hochpass");
    r.c("  Ausgang in Stille, Spitze", 20 * std::log10(std::max(pk, 1e-12) / DBU), -200, -70, "dBu");
    r.c("  Wechsel am Vorverstärker (halbe Abtastrate)", alt * 1e3, 0, 0.3, "mV");
    r.c("  Drift des Haltepunkts in 2 s", (y[O_HOLD] - h0) * 1e3, -0.15, 0.15, "mV"); (void)dcmax; }
  return r; }

// 3. Verstärkung und Frequenzgang
static Res tGain() { Res r; r.p("Verstärkung und Frequenzgang (Kleinsignal)\n"); Setting s = base(); s.input = 1; s.output = 1; s.attack = -1; Core c = mk(s);
  double g1k = 20 * std::log10(std::abs(c.response(1000))); r.c("größte Verstärkung (INPUT, OUTPUT voll), 1 kHz", g1k, 35, 40, "dB");
  double lo = 1e9, hi = -1e9; for (double f = 20; f <= 20000.1; f *= std::pow(1000.0, 1 / 30.0)) { double g = 20 * std::log10(std::abs(c.response(f))) - g1k; lo = std::min(lo, g); hi = std::max(hi, g); }
  r.c("Frequenzgang 20 Hz … 20 kHz, tiefster Punkt (Handbuch ±1 dB)", lo, -1, 0, "dB"); r.c("Frequenzgang 20 Hz … 20 kHz, höchster Punkt", hi, 0, 1, "dB"); return r; }

// 4. Kennlinien (statisch): INPUT 7,5; 1 kHz; Eingang in dBu, stufenweise aufwärts, jede Stufe 2,5 s (das Gate bekommt die Steuerspannung über R7/R9/C5
//    zur Hälfte sofort, den Rest mit etwa 1 s; nach 0,6 s liegt der Ausgang noch bis 0,8 dB höher)
static Res tCurve(int ratio) { Res r; const char *nm[5] = {"4:1", "8:1", "12:1", "20:1", "alle Tasten"}; Setting s = base(); s.input = 0.75; s.ratio = ratio; Core c = mk(s);
  const int NL = 9; const double L[NL] = {-40, -30, -25, -20, -15, -10, -5, 0, 10}; double o[NL], gr[NL], rest[NL], y[O_NO]; long i = 0;
  for (int k = 0; k < NL; k++) { double a = DBU * std::pow(10, L[k] / 20), cs = 0, sn = 0, ss = 0, sm = 0, g = 0; long n = (long)(2.5 * FS), m = (long)(0.2 * FS);
    for (long q = 0; q < n; q++, i++) { double ph = 2 * PI * 1000 * i / FS; c.step(a * std::sin(ph), y); if (q >= n - m) { cs += y[O_OUT] * std::cos(ph); sn += y[O_OUT] * std::sin(ph); ss += y[O_OUT] * y[O_OUT]; sm += y[O_OUT]; g += grdb(c.v, y[O_GATE]); } }
    double amp = 2 * std::hypot(cs, sn) / m, dc = sm / m; o[k] = 20 * std::log10(amp / DBU); gr[k] = g / m; rest[k] = std::sqrt(std::max(0.0, ss / m - dc * dc - amp * amp / 2)) / (amp / std::sqrt(2.0)); }
  r.p("Kennlinie %s:  Eingang dBu", nm[ratio]); for (double l : L) r.p(" %7.0f", l); r.p("\n               Ausgang dBu"); for (double v : o) r.p(" %7.2f", v); r.p("\n               FET dB     "); for (double v : gr) r.p(" %7.2f", v);
  r.p("\n               Rest %     "); for (double v : rest) r.p(" %7.2f", 100 * v); r.p("\n");
  double lin = o[0] - L[0]; r.c("Verstärkung unter der Schwelle (−40 dBu)", lin, 11, 13.2, "dB");
  double thr = 0; for (int k = 1; k < NL; k++) if (o[k] - L[k] < lin - 1) { double a = (o[k - 1] - L[k - 1]) - (lin - 1), b = (lin - 1) - (o[k] - L[k]); thr = L[k - 1] + (L[k] - L[k - 1]) * a / (a + b); break; }
  double slope = 10 / std::max(1e-3, o[6] - o[4]);   // dB Eingang je dB Ausgang zwischen −15 und −5 dBu
  const double sLo[5] = {3, 6, 9, 15, 9}, sHi[5] = {5, 10, 15, 26, 60}, tLo[5] = {-33, -29, -26, -24, -27}, tHi[5] = {-25, -21, -18, -16, -19};
  r.c("Schwelle (1 dB Stauchung), Eingang", thr, tLo[ratio], tHi[ratio], "dBu"); r.c("Verhältnis zwischen −15 und −5 dBu Eingang", slope, sLo[ratio], sHi[ratio], ":1");
  r.c("Rest (Klirr) bei −5 dBu", 100 * rest[6], 0, 3, "%"); return r; }

// 5. Zeiten: Tonstoß 1 kHz, −15 dBu bei INPUT 7,5 (etwa 10 dB bei 4:1), 0,3 s lang; Spannung am Haltekondensator C27, 63 % der Änderung.
//    Zur Kenntnis die Erholung der FET-Dämpfung selbst nach 0,3 s und nach 2 s Ton (nach langer Kompression langsamer: C5 über R7 + R9, etwa 1 s).
static Res tTimes() { Res r; r.p("Zeiten am Haltekondensator (Handbuch: Attack 20 … 800 µs, Release 50 ms … 1,1 s)\n");
  auto run = [](double att, double rel, double burst, double lvl, double tail, double *ta, double *tr, double *trg, double *gr) { Setting s = base(); s.attack = att; s.release = rel; s.input = 0.75; const double fs = 96000; Core c = mk(s, Parts(), fs); double a = DBU * std::pow(10, lvl / 20.0), y[O_NO];
    int N1 = (int)(burst * fs), N2 = (int)(tail * fs); std::vector<float> h(N1 + N2), g(N1 + N2); double h0 = 0, g0 = 0; for (int i = 0; i < 2000; i++) { c.step(0, y); h0 = y[O_HOLD]; g0 = grdb(c.v, y[O_GATE]); }
    for (int i = 0; i < N1 + N2; i++) { c.step(i < N1 ? a * std::sin(2 * PI * 1000 * i / fs) : 0, y); h[i] = (float)y[O_HOLD]; g[i] = (float)grdb(c.v, y[O_GATE]); }
    double he = 0, ge = 0; for (int i = N1 - 960; i < N1; i++) { he += h[i] / 960; ge += g[i] / 960; } *ta = *tr = *trg = -1;
    for (int i = 0; i < N1; i++) if (h[i] >= h0 + 0.632 * (he - h0)) { *ta = i / fs; break; } for (int i = N1; i < N1 + N2; i++) if (h[i] <= he - 0.632 * (he - h0)) { *tr = (i - N1) / fs; break; }
    for (int i = N1; i < N1 + N2; i++) if (g[i] <= ge - 0.632 * (ge - g0)) { *trg = (i - N1) / fs; break; } *gr = ge - g0; };
  double A[8][4]; const double cfg[8][5] = {{1, 0.5, 0.3, -15, 0.05}, {0, 0.5, 0.3, -15, 0.05}, {0.5, 1, 0.3, -15, 3}, {0.5, 0, 0.3, -15, 4}, {0.5, 1, 2.0, -15, 4}, {0.5, 0.5, 0.3, -15, 0.05}, {1, 0.5, 0.3, 0, 0.05}, {0, 0.5, 0.3, 0, 0.05}}; std::vector<std::future<void>> f;
  for (int k = 0; k < 8; k++) f.push_back(std::async(std::launch::async, [&, k]() { run(cfg[k][0], cfg[k][1], cfg[k][2], cfg[k][3], cfg[k][4], &A[k][0], &A[k][1], &A[k][2], &A[k][3]); })); for (auto &x : f) x.get();
  r.p("  Das Handbuch nennt keine Messbedingung; bei Rückwärtsregelung wird die Attack-Zeit mit der Übersteuerung kürzer.\n");
  r.c("Attack 7 (schnellst), −15 dBu (12 dB Dämpfung)", A[0][0] * 1e6, 150, 400, "µs"); r.c("Attack 1 (langsamst), −15 dBu", A[1][0] * 1e6, 2000, 4500, "µs");
  r.c("Attack 7 (schnellst), 0 dBu (26 dB Dämpfung)", A[6][0] * 1e6, 10, 400, "µs"); r.c("Attack 1 (langsamst), 0 dBu", A[7][0] * 1e6, 200, 4500, "µs");
  r.c("Release 7 (schnellst)", A[2][1] * 1e3, 40, 80, "ms"); r.c("Release 1 (langsamst)", A[3][1] * 1e3, 800, 1500, "ms");
  r.c("FET-Dämpfung im Tonstoß (4:1, −15 dBu, Grundstellung)", A[5][3], 6, 16, "dB");
  r.c("Erholung der FET-Dämpfung, Release 7, nach 0,3 s Ton", A[2][2] * 1e3, 30, 400, "ms"); r.c("Erholung der FET-Dämpfung, Release 7, nach 2 s Ton", A[4][2] * 1e3, 100, 1500, "ms"); return r; }

// 6. Stufenlose Ratio: an den vier Tastenpunkten gleich der Taste; dazwischen monoton
static Res tVar() { Res r; r.p("Stufenlose Ratio (Erweiterung): Ausgang bei 0 dBu, INPUT 7,5\n"); double a = DBU, prev = 0;
  for (int k = 0; k < 4; k++) { Setting s = base(); s.input = 0.75; s.ratio = k; double ob = tone(mk(s), 1000, a, 1.0).dbu; s.ratio = 5; s.rvar = k / 3.0; double ov = tone(mk(s), 1000, a, 1.0).dbu;
    char nm[80]; std::snprintf(nm, sizeof nm, "Taste %d gegen stufenlos bei %d/3: Unterschied", k, k); r.c(nm, ov - ob, -0.05, 0.05, "dB"); }
  a = DBU * std::pow(10, -5 / 20.0);   // statisch (3 s) bei −5 dBu: mit steigender Ratio sinkt der Ausgang nicht unbedingt (die Schwelle steigt mit), aber das Verhältnis −15 → −5 dBu wächst
  for (int k = 0; k <= 6; k++) { Setting s = base(); s.input = 0.75; s.ratio = 5; s.rvar = k / 6.0; Core c = mk(s); double o1 = tone(c, 1000, a / 3.16227766, 3).dbu, o2 = tone(c, 1000, a, 3).dbu, sl = 10 / std::max(1e-3, o2 - o1); r.p("  stufenlos %.2f: Verhältnis %.2f : 1\n", k / 6.0, sl); if (k && sl < prev * 0.98) { r.p("  nicht monoton   ABWEICHUNG\n"); r.nf++; } prev = sl; }
  return r; }

// 7. Hochpass in der Seitenkette
static Res tHpf() { Res r; r.p("Hochpass in der Seitenkette (Erweiterung), 4:1, INPUT 7,5, −5 dBu\n"); double a = DBU * std::pow(10, -5 / 20.0); Setting s = base(); s.input = 0.75; Core c0 = mk(s);
  Setting h = s; h.hpf = true; h.hpfF = std::log(150.0 / 20) / std::log(30.0); Core c1 = mk(h);   // 150 Hz
  double g0lo = tone(c0, 50, a, 1.0).gr, g1lo = tone(c1, 50, a, 1.0).gr, g0hi = tone(c0, 2000, a, 1.0).gr, g1hi = tone(c1, 2000, a, 1.0).gr;
  r.p("  GR bei 50 Hz: ohne %.2f dB, mit Hochpass 150 Hz %.2f dB;  bei 2 kHz: ohne %.2f dB, mit %.2f dB\n", g0lo, g1lo, g0hi, g1hi);
  r.c("50 Hz: GR mit Hochpass kleiner um", g0lo - g1lo, 3, 40, "dB"); r.c("2 kHz: GR mit und ohne gleich, Unterschied", g1hi - g0hi, -0.5, 0.5, "dB");
  // Umschalten im Betrieb: Haltepunkt darf nicht springen (bis 1.0-Vorstand: Zustände vertauscht, Sprung auf +9 V)
  { Core c = c0; c.fixDev(); double y[O_NO], hb = 0, dmax = 0; long n = (long)(0.6 * FS); for (long i = 0; i < 3 * n; i++) { if (i == n) c.apply(h); if (i == 2 * n) c.apply(s);
      double before = y[O_HOLD]; c.step(a * std::sin(2 * PI * 2000 * i / FS), y); if (i == n || i == 2 * n) dmax = std::max(dmax, std::fabs(y[O_HOLD] - before)); hb = y[O_HOLD]; }
    r.c("Umschalten bei 2 kHz: größter Sprung des Haltepunkts", dmax * 1e3, 0, 20, "mV"); (void)hb; }
  return r; }

// 8. Ganzes Gerät (Amp): zweite Stufe, DRY, Start ohne Gleichanteil
static Res tAmp() { Res r; r.p("Gerät mit zweiter Stufe und DRY (Erweiterungen), Eingang 1 kHz, 0,3 V Spitze\n"); Setting s = base(); Amp a; a.init(FS, Parts(), s, s); a.ready = true; a.vin = 1;
  auto run = [&](Amp b, bool stack, double dry, double amp, double *gr1, double *gr2) { b.stack = stack; b.dry = dry; std::vector<float> buf(4800); double g[2], pk = 0, sq = 0; for (int k = 0; k < 12; k++) { for (int i = 0; i < 4800; i++) buf[i] = (float)(amp * std::sin(2 * PI * 1000 * (k * 4800 + i) / FS)); b.process(buf.data(), 4800); if (k == 10) b.takeGR(g); }
    b.takeGR(g); for (float v : buf) { pk = std::max(pk, (double)std::fabs(v)); sq += v * v; } *gr1 = g[0]; *gr2 = g[1]; return std::sqrt(sq / 4800) * std::sqrt(2.0); };
  double g1, g2, o1 = run(a, false, 0, 0.3, &g1, &g2); r.p("  eine Stufe: Ausgang %.3f V Spitze, GR %.2f dB\n", o1, g1); r.c("GR eine Stufe (Grundstellung, 0,3 V)", g1, 8, 14, "dB");
  double h1, h2, o2 = run(a, true, 0, 0.3, &h1, &h2); r.p("  zwei Stufen: Ausgang %.3f V Spitze, GR %.2f + %.2f dB\n", o2, h1, h2); r.c("GR der zweiten Stufe", h2, 3, 30, "dB"); r.c("GR der ersten Stufe unverändert, Unterschied", h1 - g1, -0.1, 0.1, "dB");
  double d1, d2, o3 = run(a, false, 1, 0.3, &d1, &d2); r.p("  DRY voll: Ausgang %.3f V Spitze\n", o3); r.c("DRY voll: Ausgang gegenüber ohne, mindestens Eingang dazu (Phase gleich)", o3 - o1, 0.25, 0.35, "V");
  { Amp b; b = a; std::vector<float> z(4800, 0.f); double pk = 0; for (int k = 0; k < 10; k++) { std::fill(z.begin(), z.end(), 0.f); b.process(z.data(), 4800); for (float v : z) pk = std::max(pk, (double)std::fabs(v)); } r.c("Ausgang in 1 s Stille nach dem Start, Spitze", pk * 1e3, 0, 0.2, "mV"); }
  return r; }

int main() { std::vector<std::function<Res()>> T = {tDC, tBias, tGain, [] { return tCurve(0); }, [] { return tCurve(1); }, [] { return tCurve(2); }, [] { return tCurve(3); }, [] { return tCurve(4); }, tTimes, tVar, tHpf, tAmp};
  std::vector<Res> R(T.size()); unsigned nt = std::max(1u, std::min(4u, std::thread::hardware_concurrency())); std::atomic<size_t> next{0}; std::vector<std::thread> th;
  for (unsigned t = 0; t < nt; t++) th.emplace_back([&]() { for (;;) { size_t k = next++; if (k >= T.size()) return; R[k] = T[k](); } }); for (auto &x : th) x.join();
  for (auto &r : R) { std::fputs(r.o.c_str(), stdout); fails += r.nf; }
  if (fails) { std::printf("ABWEICHUNG: %d Prüfung(en) außerhalb der Grenzen\n", fails); return 1; } std::printf("alle Prüfungen bestanden\n"); return 0; }
