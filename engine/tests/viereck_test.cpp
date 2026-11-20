// Prüfung des Viereck-34-Kerns (Klangteil des Quad 34) gegen die Quad-34-Service-Unterlagen und Messungen am Gerät.
//   viereck_test            alle Prüfungen, Rückgabe 1 bei Abweichung
// 1. TILT (Service Data Fig. 3): Stellungen ±1/±2/±3 bei 100 Hz und 10 kHz auf ±0,15 dB, Drehpunkt (gleich laut wie 0) 820 … 880 Hz.
// 2. BASS LIFT (Messung am Gerät, kenrockwell.com „Quad 34“): +3: 82,5 Hz +3,00 dB, 20,4 Hz +12,27 dB; +6: 82,5 Hz +6,03, 27,0 Hz +14,00; +9: 82,5 Hz +10,26, 36,7 Hz +15,26;
//    Toleranz ±0,5 dB (Bauteiltoleranzen am gemessenen Gerät). Höchstwerte wie Fig. 8(b): etwa +12/+14/+15,3 dB.
// 3. BASS STEP: Absenkung unten 5,5 dB (kenrockwell: „precise 5.5 dB“), alle drei Stellungen bei 10 Hz gegen 0 dB −5,5 ± 0,4 dB; Übergang 100 < 170 < 300 Hz.
// 4. Filter (Fig. 10, abgelesen): F1 −6,3 dB, F1 SLOPE −9, F2 −10 dB bei 20 kHz, F2 SLOPE −14 dB bei 18 kHz; ±1,5 dB (Ablesung; RV4 5 kΩ an Fig. 10 angepasst).
//    Ohne Filter unter 20 kHz flach (±0,3 dB gegen 1 kHz).
// 5. Drehpunkt-Regler: TILT +3 mit Drehpunkt 200 Hz bzw. 3,5 kHz ist um den Faktor verschoben (Kurve bei f·k gleich der Kurve des Originals bei f, 300 Hz … 3 kHz ±0,25 dB).
// 6. BALANCE (Service Data Prüfanweisung 3): aus der Mitte zum lauten Ende +2,5 dB, zum leisen Ende −65 dB (±0,1 dB); Verlauf stetig und
//    gleichsinnig; Mitte gleich laut in beiden Kanälen.
// 7. Zeitbereich (Unit, 48 kHz): flache Einstellung 1:1 (1 kHz Sinus −20 dBFS, Pegel ±0,05 dB, Phase gleich), Umschalten im Lauf ohne Sprung
//    (größter Schritt höchstens 2 × größter Schritt des Signals; TILT-Rasten gleiten 15 ms), Übersteuerung (+20 dB) endlich und begrenzt.
#include "viereck_core.hpp"
#include <cstdio>
using namespace viereck;
static int bad = 0;
static double db(const Setting &s, double f) { Parts v; Setting r; r.balance = s.balance; return 20 * std::log10(std::abs(response(v, s, f)) / std::abs(response(v, r, 1000))); }
static void chk(const char *what, double got, double want, double tol) { bool ok = std::fabs(got - want) <= tol; if (!ok) bad++; printf("%-52s %+7.2f (Soll %+6.2f ± %.2f)%s\n", what, got, want, tol, ok ? "" : "   ABWEICHUNG"); }
int main() {
  printf("Drehpunkt des Originals: %.2f Hz\n", pivot0()); { bool ok = pivot0() > 820 && pivot0() < 880; if (!ok) bad++; }
  // 1. TILT
  for (int t = -3; t <= 3; t++) { if (!t) continue; Setting s; s.tilt = t; char b[80];
    std::snprintf(b, 80, "TILT %+d bei 100 Hz", t); chk(b, db(s, 100), -t, 0.15); std::snprintf(b, 80, "TILT %+d bei 10 kHz", t); chk(b, db(s, 10000), t, 0.15); }
  // 2. BASS LIFT
  struct L { int b; double f, d; } lift[] = {{1, 82.5, 3.00}, {1, 20.4, 12.27}, {2, 82.5, 6.03}, {2, 27.0, 14.00}, {3, 82.5, 10.26}, {3, 36.7, 15.26}};
  for (auto &l : lift) { Setting s; s.bass = l.b; char b[80]; std::snprintf(b, 80, "BASS LIFT +%d dB bei %.1f Hz", 3 * l.b, l.f); chk(b, db(s, l.f), l.d, 0.5); }
  // 3. BASS STEP
  { double f3[3]; const int hz[3] = {300, 170, 100};
    for (int k = 0; k < 3; k++) { Setting s; s.bass = -3 + k; char b[80]; std::snprintf(b, 80, "BASS STEP %d Hz bei 10 Hz", hz[k]); Setting fl; chk(b, db(s, 10) - db(fl, 10), -5.5, 0.4);
      double lo = 10, hi = 2000; for (int i = 0; i < 60; i++) { double m = std::sqrt(lo * hi); if (db(s, m) < -3) lo = m; else hi = m; } f3[k] = lo; printf("    −3-dB-Punkt %.0f Hz\n", lo); }
    bool ok = f3[0] > f3[1] && f3[1] > f3[2]; if (!ok) bad++; printf("    Reihenfolge 300 > 170 > 100 Hz: %s\n", ok ? "ja" : "NEIN   ABWEICHUNG"); }
  // 4. Filter
  { struct F { int fl; bool sl; double f, d; const char *n; } fl[] = {{1, false, 20000, -6.3, "F1"}, {1, true, 20000, -9, "F1 SLOPE"}, {2, false, 20000, -10, "F2"}, {2, true, 18000, -14, "F2 SLOPE"}};
    for (auto &x : fl) { Setting s; s.filter = x.fl; s.slope = x.sl; char b[80]; std::snprintf(b, 80, "%s bei %.0f kHz", x.n, x.f / 1000); chk(b, db(s, x.f), x.d, 1.5); }
    Setting s; double mx = 0; for (double f = 20; f <= 20000; f *= 1.1) mx = std::max(mx, std::fabs(db(s, f))); chk("ohne Filter: größte Abweichung 20 Hz … 20 kHz", mx, 0, 0.3); }
  // 5. Drehpunkt-Regler
  for (double p : {200.0, 3500.0}) { Setting a, o; a.tilt = o.tilt = 3; a.pivot = p; double k = p / pivot0(), mx = 0;
    for (double f = 300; f <= 3000; f *= 1.05) mx = std::max(mx, std::fabs(db(a, f * k) - db(o, f))); char b[80]; std::snprintf(b, 80, "Drehpunkt %.0f Hz: Kurve verschoben, größte Abweichung", p); chk(b, mx, 0, 0.25); }
  // 6. BALANCE
  { Parts v; Setting m; double g = std::abs(response(v, m, 1000)); auto bdb = [&](double b, int ch) { Setting s; s.balance = balPos(b, ch); return 20 * std::log10(std::abs(response(v, s, 1000)) / g); };
    chk("BALANCE ganz rechts: rechter Kanal (dB gegen Mitte)", bdb(1, 1), 2.5, 0.1); chk("BALANCE ganz rechts: linker Kanal", bdb(1, 0), -65, 0.1);
    chk("BALANCE ganz links: linker Kanal", bdb(-1, 0), 2.5, 0.1); chk("BALANCE ganz links: rechter Kanal", bdb(-1, 1), -65, 0.1);
    chk("BALANCE Mitte: linker gegen rechten Kanal", bdb(0, 0) - bdb(0, 1), 0, 0.01);
    bool mono = true; double last = 1e9; for (int i = -100; i <= 100; i++) { double d = bdb(i / 100.0, 0); if (!(d <= last + 1e-9) || !std::isfinite(d)) mono = false; last = d; }
    if (!mono) bad++; printf("    Verlauf linker Kanal stetig fallend: %s;  Mitte RV4-Anteil %.3f, halb rechts %+.2f / %+.2f dB\n", mono ? "ja" : "NEIN   ABWEICHUNG", balC(), bdb(0.5, 1), bdb(0.5, 0)); }
  // 7. Zeitbereich
  { const double fs = 48000; Unit u; u.vin = 1.32; u.init(fs, Parts(), Setting()); int N = (int)fs; std::vector<float> x(N);
    for (int i = 0; i < N; i++) x[i] = (float)(0.1 * std::sin(2 * PI * 1000 * i / fs)); std::vector<float> y = x; u.process(y.data(), N);
    double sxy = 0, sxx = 0, syy = 0; for (int i = N / 2; i < N; i++) { sxy += x[i] * (double)y[i]; sxx += x[i] * (double)x[i]; syy += y[i] * (double)y[i]; }
    Setting fl; double ref = std::abs(response(Parts(), fl, 1000)) / gain0(); chk("Zeitbereich flach 1 kHz: Pegel (dB) gegen Kleinsignal", 10 * std::log10(syy / sxx) - 20 * std::log10(ref), 0, 0.05);
    chk("Zeitbereich flach 1 kHz: Korrelation mit dem Eingang", sxy / std::sqrt(sxx * syy), 1, 0.01);
    // Umschalten im Lauf: alle Schalter einmal, TILT-Rasten, Filter
    Unit w; w.vin = 1.32; w.init(fs, Parts(), Setting()); std::vector<float> z(3 * N); for (int i = 0; i < 3 * N; i++) z[i] = (float)(0.3 * std::sin(2 * PI * 82 * i / fs) + 0.1 * std::sin(2 * PI * 3100 * i / fs));
    double dsig = 0; for (int i = 1; i < 3 * N; i++) dsig = std::max(dsig, (double)std::fabs(z[i] - z[i - 1]));
    int blk = 4800, nb = 3 * N / blk; double dmax = 0; float last = 0; const Setting seq[] = {[] { Setting s; s.tilt = 3; return s; }(), [] { Setting s; s.tilt = -3; return s; }(), [] { Setting s; s.bass = 3; return s; }(),
      [] { Setting s; s.bass = -3; return s; }(), [] { Setting s; s.filter = 1; return s; }(), [] { Setting s; s.filter = 2; s.slope = true; return s; }(), [] { Setting s; s.stepped = false; s.tilt = 1.5; s.pivot = 300; return s; }(), Setting()};
    for (int b = 0; b < nb; b++) { if (b > 2 && b % 3 == 0) w.setTarget(seq[(b / 3) % 8]); w.process(z.data() + b * blk, blk);
      for (int i = 0; i < blk; i++) { float v = z[b * blk + i]; if (b || i) dmax = std::max(dmax, (double)std::fabs(v - last)); last = v; } }
    bool fin = true; for (float v : z) fin = fin && std::isfinite(v); chk("Umschalten im Lauf: größter Schritt / größter Schritt des Signals", dmax / dsig, 1, 1.0); if (!fin) { bad++; printf("   nicht endliche Werte\n"); }
    Unit h; h.vin = 1.32; h.init(fs, Parts(), Setting()); std::vector<float> g(N); for (int i = 0; i < N; i++) g[i] = (float)(10 * std::sin(2 * PI * 100 * i / fs)); h.process(g.data(), N); double mx = 0; bool f2 = true; for (float v : g) { mx = std::max(mx, (double)std::fabs(v)); f2 = f2 && std::isfinite(v); }
    printf("Übersteuerung (+20 dBFS, 100 Hz): Spitze am Ausgang %.2f (OPV-Grenze %.2f)%s\n", mx, Parts().op_vs / 1.32, f2 && mx < 2 * Parts().op_vs / 1.32 ? "" : "   ABWEICHUNG"); if (!(f2 && mx < 2 * Parts().op_vs / 1.32)) bad++; }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
