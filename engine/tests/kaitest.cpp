// Prüfungen des Kaiserin-Reverb-Kerns (kaiserin_core.hpp):
//   rt      Nachhallzeit je Oktave (Schroeder-Rückwärtsintegral, T30 von −5 bis −35 dB) gegen die Sollwerte aus den Reglerkennlinien
//   level   Pegel des Halls (nur Hall, MIX rechts) bei weißem Rauschen gegen den Eingang, je Modus → CAL
//   stable  alle Regler rechts (DECAY, THING 1/2, LOW, HI), 2 s Rauschen, dann 30 s Stille: Spitze, Endpegel, endlich?; HOLD 30 s: Pegel bleibt
//   stereo  Korrelation L/R des Halls
//   cpu     Rechenzeit je Abtastwert und Modus
//   sweep   Stellungen durchprobieren (Moduswechsel, Halten, Extremwerte, NaN am Eingang): alles endlich, Spitze begrenzt
//   ir <mode> <fs> <datei>   Impulsantwort (Stereo, float32 roh) für eigene Auswertung
#include "kaiserin_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace kai;
static double fsG = 48000;
static void run(Reverb &r, const std::vector<float> &x, std::vector<float> &L, std::vector<float> &R, const Setting &s) {
  size_t n = x.size(); L.assign(n, 0); R.assign(n, 0); for (size_t i = 0; i < n; i += 256) { int m = (int)std::min<size_t>(256, n - i); r.process(&x[i], nullptr, &L[i], &R[i], m, s); } }
static std::vector<float> noise(size_t n, uint32_t seed = 1) { Rng g(seed); std::vector<float> v(n); for (auto &x : v) x = (float)(0.5 * (g.next() * 2 - 1)); return v; }
static Setting wetOnly(int m) { Setting s; s.mode = m; s.mix = 1; return s; }
// Oktavband-Nachhallzeit aus der Impulsantwort (beide Kanäle)
static double t30(const std::vector<float> &L, const std::vector<float> &R, double fc, double fs) {
  Biquad a, b, c, d; a.set(0, fc / 1.414, fs); b.set(0, fc / 1.414, fs); c.set(1, fc * 1.414, fs); d.set(1, fc * 1.414, fs);
  Biquad a2 = a, b2 = b, c2 = c, d2 = d; size_t n = L.size(); std::vector<double> e(n);
  for (size_t i = 0; i < n; i++) { double y = d.step(c.step(b.step(a.step(L[i])))), z = d2.step(c2.step(b2.step(a2.step(R[i])))); e[i] = y * y + z * z; }
  for (size_t i = n - 1; i-- > 0;) e[i] += e[i + 1]; double e0 = e[0]; if (!(e0 > 0)) return -1;
  // lineare Regression von 10·log10(E/E0) zwischen −5 und −35 dB
  double sx = 0, sy = 0, sxx = 0, sxy = 0; int k = 0; for (size_t i = 0; i < n; i++) { double db = 10 * std::log10(e[i] / e0 + 1e-300); if (db > -5) continue; if (db < -35) break; double t = i / fs; sx += t; sy += db; sxx += t * t; sxy += t * db; k++; }
  if (k < 10) return -1; double sl = (k * sxy - sx * sy) / (k * sxx - sx * sx); return -60 / sl; }
static double target(int m, const Setting &s, double f) { double tm = t60Mid(m, s.decay), tl = tm * lowRatio(s.low), th = tm * hiRatioMode(m, s);
  if (f <= XLOW[m] / 2) return tl; if (f >= XHIGH[m] * 1.5) return th; if (f < 1000) return tm; return std::min(tm, th * 2); }
int main(int argc, char **argv) {
  std::string what = argc > 1 ? argv[1] : "all"; if (argc > 2 && what != "ir") fsG = std::atof(argv[2]); double fs = fsG; bool all = what == "all";
  if (what == "ir") { int m = std::atoi(argv[2]); fs = std::atof(argv[3]); Reverb r; r.setup(fs); std::vector<float> x((size_t)(fs * 12), 0.f), L, R; x[0] = 1; run(r, x, L, R, wetOnly(m));
    FILE *f = std::fopen(argv[4], "wb"); for (size_t i = 0; i < L.size(); i++) { std::fwrite(&L[i], 4, 1, f); std::fwrite(&R[i], 4, 1, f); } std::fclose(f); return 0; }
  int bad = 0;
  if (all || what == "rt") { std::printf("== Nachhallzeit je Oktave (s), gemessen / Soll, fs %.0f\n", fs); const double fc[6] = {125, 250, 500, 1000, 2000, 4000};
    for (int m = 0; m < NMODE; m++) for (double dec : {0.0, 0.5, 1.0}) { Setting s = wetOnly(m); s.decay = dec; Reverb r; r.setup(fs); double T = std::max(4.0, 2.2 * t60Mid(m, dec) * 1.2) + 0.5;
        std::vector<float> x((size_t)(fs * T), 0.f), L, R; x[0] = 1; run(r, x, L, R, s); std::printf("%-9s decay %.1f:", MODE_NAME[m], dec);
        for (int b = 0; b < 6; b++) { double t = t30(L, R, fc[b], fs), tg = target(m, s, fc[b]); std::printf("  %4.0f %5.2f/%5.2f", fc[b], t, tg); } std::printf("\n"); } }
  if (all || what == "level") { std::printf("== Pegel Hall/Eingang (dB) bei Rauschen, nur Hall, alle Regler 12 Uhr (Messung 4…8 s)\n");
    for (int m = 0; m < NMODE; m++) { Reverb r; r.setup(fs); auto x = noise((size_t)(fs * 8)); std::vector<float> L, R; run(r, x, L, R, wetOnly(m)); double ex = 0, ey = 0; size_t a = (size_t)(fs * 4);
      for (size_t i = a; i < x.size(); i++) { ex += x[i] * x[i]; ey += 0.5 * (L[i] * L[i] + R[i] * R[i]); } std::printf("%-9s %+6.2f dB\n", MODE_NAME[m], 10 * std::log10(ey / ex)); } }
  if (all || what == "stereo") { std::printf("== Korrelation L/R des Halls (Rauschen)\n");
    for (int m = 0; m < NMODE; m++) { Reverb r; r.setup(fs); auto x = noise((size_t)(fs * 6)); std::vector<float> L, R; run(r, x, L, R, wetOnly(m)); double sl = 0, sr = 0, sx = 0;
      for (size_t i = (size_t)(fs * 2); i < x.size(); i++) { sl += L[i] * L[i]; sr += R[i] * R[i]; sx += L[i] * R[i]; } std::printf("%-9s %+.3f\n", MODE_NAME[m], sx / std::sqrt(sl * sr)); } }
  if (all || what == "stable") { std::printf("== Stabilität: alle Regler rechts, 2 s Rauschen, 30 s Stille; HOLD 30 s\n");
    for (int m = 0; m < NMODE; m++) for (int h = 0; h < 2; h++) { Setting s = wetOnly(m); s.decay = 1; s.t1 = 1; s.t2 = 1; s.low = 1; s.hi = 1; s.out = 0.5; Reverb r; r.setup(fs);
        auto x = noise((size_t)(fs * 32)); for (size_t i = (size_t)(fs * 2); i < x.size(); i++) x[i] = 0; std::vector<float> L((size_t)(fs * 32)), R(L.size());
        double pk = 0, pk2 = 0, last = 0, mid = 0; bool fin = true;
        for (size_t i = 0; i < x.size(); i += 256) { int k = (int)std::min<size_t>(256, x.size() - i); Setting t = s; t.hold = h && i > (size_t)(fs * 1.9); r.process(&x[i], nullptr, &L[i], &R[i], k, t);
          for (int j = 0; j < k; j++) { double v = std::max(std::fabs(L[i + j]), std::fabs(R[i + j])); if (!std::isfinite(v)) fin = false; pk = std::max(pk, v); if (i > (size_t)(fs * 3)) pk2 = std::max(pk2, v); } }
        auto rms = [&](double t0) { double e = 0; size_t a = (size_t)(t0 * fs), b = std::min(L.size(), a + (size_t)fs); for (size_t i = a; i < b; i++) e += 0.5 * (L[i] * L[i] + R[i] * R[i]); return 10 * std::log10(e / (b - a) + 1e-30); };
        mid = rms(3); last = rms(30.5);
        bool ok = fin && pk < 8 && (h ? (last > mid - 12 && last < mid + 3) : last < mid);
        std::printf("%-9s %s: Spitze %.2f, nach 3 s %.1f dB, nach 30 s %.1f dB, endlich %s %s\n", MODE_NAME[m], h ? "HOLD " : "frei ", pk, mid, last, fin ? "ja" : "NEIN", ok ? "ok" : "FEHLER"); if (!ok) bad++; } }
  if (all || what == "cpu") { std::printf("== Rechenzeit (ns je Abtastwert, Stereo-Ausgang)\n");
    for (int m = 0; m < NMODE; m++) { Reverb r; r.setup(fs); auto x = noise((size_t)(fs * 10)); std::vector<float> L, R; Setting s = wetOnly(m); s.mix = 0.5; auto t0 = std::chrono::steady_clock::now(); run(r, x, L, R, s);
      double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); std::printf("%-9s %6.1f ns  (%.2f %% eines Kerns bei %.0f Hz)\n", MODE_NAME[m], dt / x.size() * 1e9, dt / 10 * 100, fs); } }
  if (all || what == "sweep") { std::printf("== Stellungen durchprobieren\n"); Reverb r; r.setup(fs); Rng g(7); auto x = noise((size_t)(fs * 60), 3); std::vector<float> L(x.size()), R(x.size()); double pk = 0; bool fin = true; Setting s;
    for (size_t i = 0; i < x.size(); i += 128) { if (i % (size_t)(fs * 0.3) < 128) { s.mode = (int)(g.next() * NMODE); s.decay = g.next(); s.mix = g.next(); s.out = g.next(); s.low = g.next(); s.hi = g.next(); s.t1 = g.next(); s.t2 = g.next(); s.hold = g.next() < 0.15; }
      if (i > (size_t)(fs * 50) && i < (size_t)(fs * 50.01)) x[i] = NAN;   // ein ungültiger Wert am Eingang
      int k = (int)std::min<size_t>(128, x.size() - i); r.process(&x[i], nullptr, &L[i], &R[i], k, s); for (int j = 0; j < k; j++) { double v = std::max(std::fabs(L[i + j]), std::fabs(R[i + j])); if (!std::isfinite(v)) { fin = false; } else pk = std::max(pk, v); } }
    bool ok = pk < 60; double end = 0; for (size_t i = x.size() - (size_t)fs; i < x.size(); i++) end = std::max(end, (double)std::fabs(L[i])); if (!std::isfinite(end)) ok = false;
    std::printf("Spitze %.2f (OUTPUT bis +20 dB), nicht endliche Werte: %s, nach dem NaN wieder endlich: %s %s\n", pk, fin ? "keine" : "ja", std::isfinite(end) ? "ja" : "NEIN", ok ? "ok" : "FEHLER"); if (!ok) bad++; }
  std::printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
