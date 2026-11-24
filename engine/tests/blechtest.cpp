// Prüfungen des Blech-140-Kerns (blech_core.hpp):
//   rt      Nachhallzeit je Terz aus der Impulsantwort (Schroeder, T30) gegen das Modell t60At, DECAY 1 / 2 / 3,8 s
//   conv    Faltung gegen direkte Rechnung an Stichproben (Rauschen), Latenz 0
//   switch  DECAY während der Wiedergabe drehen: Dauer bis zur neuen Platte, längster Block, endlich, kein Einbruch
//   cpu     Rechenzeit je Abtastwert (Ruhe) und Einrichtzeit
//   rate    96 und 192 kHz (Hall intern bei 48 kHz): Pegel gegen 48 kHz, Spiegelanteile über 26 kHz, Nachhallzeit, Rechenzeit
//   ir <decay01> <fs> <datei>   Impulsantwort stereo (float32 roh)
#include "blech_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace blech;
static double t30(const std::vector<float> &L, const std::vector<float> &R, double fc, double fs) {
  auto bq = [&](int type, double f) { struct B { double b0, b1, b2, a1, a2, z1 = 0, z2 = 0; double s(double x) { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } } q;
    double w = 2 * PI * f / fs, c = std::cos(w), al = std::sin(w) / (2 * 0.7071), a0 = 1 + al; if (type == 0) { q.b0 = (1 + c) / 2 / a0; q.b1 = -(1 + c) / a0; q.b2 = q.b0; } else { q.b0 = (1 - c) / 2 / a0; q.b1 = (1 - c) / a0; q.b2 = q.b0; }
    q.a1 = -2 * c / a0; q.a2 = (1 - al) / a0; return q; };
  const double bw = 1.1225; auto a = bq(0, fc / bw), b = bq(0, fc / bw), c = bq(1, fc * bw), d = bq(1, fc * bw), a2 = a, b2 = b, c2 = c, d2 = d; size_t n = L.size(); std::vector<double> e(n);
  for (size_t i = 0; i < n; i++) { double y = d.s(c.s(b.s(a.s(L[i])))), z = d2.s(c2.s(b2.s(a2.s(R[i])))); e[i] = y * y + z * z; }
  for (size_t i = n - 1; i-- > 0;) e[i] += e[i + 1]; double e0 = e[0]; double sx = 0, sy = 0, sxx = 0, sxy = 0; int k = 0;
  for (size_t i = 0; i < n; i++) { double db = 10 * std::log10(e[i] / e0 + 1e-300); if (db > -5) continue; if (db < -35) break; double t = i / fs; sx += t; sy += db; sxx += t * t; sxy += t * db; k++; }
  if (k < 10) return -1; return -60 / ((k * sxy - sx * sy) / (k * sxx - sx * sx)); }
// erwartete Nachhallzeit derselben Auswertung aus dem Modell: Energie je Mode × Filter, Abklingkurve Σ w·e^{−σt}/σ, gleiche Geradenanpassung
static double t30Model(const IRJob &j, double t500, double fc, double fs) { const double bw = 1.1225, s = damperFor(j.pl, t500);
  auto mag2 = [&](double f) { std::complex<double> z = std::polar(1.0, 2 * PI * f / fs), h = 1;
    for (int k = 0; k < 4; k++) { double fq = k < 2 ? fc / bw : fc * bw, w = 2 * PI * fq / fs, c = std::cos(w), al = std::sin(w) / (2 * 0.7071), a0 = 1 + al, b0, b1, b2;
      if (k < 2) { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = b0; } else { b0 = (1 - c) / 2; b1 = 1 - c; b2 = b0; }
      std::complex<double> zi = 1.0 / z; h *= (b0 + b1 * zi + b2 * zi * zi) / (a0 + (-2 * c) * zi + (1 - al) * zi * zi); } return std::norm(h); };
  std::vector<double> w(j.md.size()), sg(j.md.size()); for (size_t i = 0; i < j.md.size(); i++) { w[i] = 0.5 * (j.md[i].aL * j.md[i].aL + j.md[i].aR * j.md[i].aR) * mag2(j.md[i].f); sg[i] = sigmaAt(j.pl, j.md[i].f, s); }
  auto edc = [&](double t) { double e = 0; for (size_t i = 0; i < w.size(); i++) e += w[i] * std::exp(-sg[i] * t) / sg[i]; return e; };   // Energie ∝ e^{−σt} (σ auf die Energie bezogen)
  double e0 = edc(0), sx = 0, sy = 0, sxx = 0, sxy = 0; int k = 0; for (double t = 0; t < 8; t += 0.005) { double db = 10 * std::log10(edc(t) / e0); if (db > -5) continue; if (db < -35) break; sx += t; sy += db; sxx += t * t; sxy += t * db; k++; }
  return k < 10 ? -1 : -60 / ((k * sxy - sx * sy) / (k * sxx - sx * sx)); }
static void run(Reverb &r, const std::vector<float> &x, std::vector<float> &L, std::vector<float> &R, const Setting &s, int blk = 256) {
  size_t n = x.size(); L.assign(n, 0); R.assign(n, 0); for (size_t i = 0; i < n; i += blk) { int m = (int)std::min<size_t>(blk, n - i); r.process(&x[i], nullptr, &L[i], &R[i], m, s); } }
int main(int argc, char **argv) {
  std::string w = argc > 1 ? argv[1] : "all"; double fs = 48000; bool all = w == "all"; int bad = 0;
  if (w == "ir") { double d = std::atof(argv[2]); fs = std::atof(argv[3]); Reverb r; r.setup(fs, d); Setting s; s.decay = d; s.mix = 1; std::vector<float> x((size_t)(8.5 * fs), 0.f), L, R; x[0] = 1; run(r, x, L, R, s);
    FILE *f = std::fopen(argv[4], "wb"); for (size_t i = 0; i < L.size(); i++) { std::fwrite(&L[i], 4, 1, f); std::fwrite(&R[i], 4, 1, f); } std::fclose(f); return 0; }
  if (all || w == "cpu") { auto t0 = std::chrono::steady_clock::now(); Reverb r; r.setup(fs); double ts = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("== Einrichten (Moden %zu, Impulsantwort %d Abtastwerte, FFT %d): %.2f s\n", r.job.md.size(), r.job.L, r.job.N, ts);
    std::vector<float> x((size_t)(10 * fs)), L, R; { uint32_t q = 1; for (auto &v : x) { q = q * 1664525u + 1013904223u; v = (float)(((q >> 8) / 16777216.0 - 0.5) * 0.5); } }
    Setting s; s.mix = 0.5; auto t1 = std::chrono::steady_clock::now(); run(r, x, L, R, s); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count();
    std::printf("== Rechenzeit Faltung: %.1f ns je Abtastwert (%.2f %% eines Kerns bei 48 kHz)\n", dt / x.size() * 1e9, dt / 10 * 100);
    auto t2 = std::chrono::steady_clock::now(); r.job.run(3.0); double tj = std::chrono::duration<double>(std::chrono::steady_clock::now() - t2).count(); std::printf("== eine neue Impulsantwort: %.2f s Rechenzeit\n", tj); }
  if (all || w == "rt") { const double fc[7] = {125, 250, 500, 1000, 2000, 4000, 8000};
    for (double d : {0.0, 0.357, 1.0}) { Reverb r; r.setup(fs, d); Setting s; s.decay = d; s.mix = 1; std::vector<float> x((size_t)(8.5 * fs), 0.f), L, R; x[0] = 1; run(r, x, L, R, s);
      std::printf("DECAY %.2f s bei 500 Hz (gemessen/erwartet aus dem Modell, in Klammern Modell bei der Mittenfrequenz):\n ", t500Of(d)); for (int b = 0; b < 7; b++) { double t = t30(L, R, fc[b], fs), m = t60At(r.job.pl, fc[b], t500Of(d)), e = t30Model(r.job, t500Of(d), fc[b], fs); std::printf("  %4.0f %5.2f/%5.2f (%5.2f)", fc[b], t, e, m); if (std::fabs(t / e - 1) > (e > 6 ? 0.12 : 0.05)) bad++; }   /* über 6 s schneidet das Ausblenden ab 7 s mit */ std::printf("\n"); } }
  if (all || w == "conv") { Reverb r; r.setup(fs); Setting s; s.mix = 1; std::vector<float> x((size_t)(3 * fs)), L, R; uint32_t q = 7; for (auto &v : x) { q = q * 1664525u + 1013904223u; v = (float)(((q >> 8) / 16777216.0 - 0.5)); }
    run(r, x, L, R, s, 97);   // ungerade Blockgröße
    std::vector<float> hl = r.part.hL, hr = r.part.hR; double emax = 0, ref = 0;   // part.hL/hR = installierte Impulsantwort (setup)
    // Hochpass am Ausgang (20 Hz, 1. Ordnung) mitrechnen: Vergleich an den Stichproben über die Impulsantwort mit Hochpass
    std::vector<double> hh(hl.size()); { double k = std::exp(-2 * PI * 20 / fs), y = 0, xp = 0; for (size_t i = 0; i < hl.size(); i++) { y = k * (y + hl[i] - xp); xp = hl[i]; hh[i] = y; } }
    for (int n : {5, 255, 256, 257, 4000, 8191, 8192, 8193, 20000, 50000, 100000, 143999}) { double v = 0; for (int k = 0; k <= n && k < (int)hh.size(); k++) v += hh[k] * x[n - k]; emax = std::max(emax, std::fabs(v - L[n])); ref = std::max(ref, std::fabs(v)); }
    std::printf("== Faltung gegen direkte Rechnung (12 Stichproben, Blockgröße 97): größte Abweichung %.2e bei Pegel %.2e (%.0f dB)\n", emax, ref, 20 * std::log10(emax / ref)); if (emax / ref > 1e-4) bad++;
    std::vector<float> y((size_t)fs, 0.f), L2, R2; y[1000] = 1; Reverb r2; r2.setup(fs); run(r2, y, L2, R2, s); int first = -1; for (int i = 0; i < (int)L2.size(); i++) if (std::fabs(L2[i]) > 1e-9 || std::fabs(R2[i]) > 1e-9) { first = i; break; }
    std::printf("== Latenz: erster Ausgang bei %d (Impuls bei 1000)\n", first); if (first != 1000) bad++; }
  if (all || w == "switch") { Reverb r; r.setup(fs); r.internalJob = false; IRJob ext; ext.setup(fs); bool given = false;   /* wie im Plugin: Moden im Hilfsfaden (hier vorab), Zerlegung im Audio-Faden */ std::vector<float> x((size_t)(8 * fs)), L, R; uint32_t q = 3; for (auto &v : x) { q = q * 1664525u + 1013904223u; v = (float)(((q >> 8) / 16777216.0 - 0.5) * 0.3); }
    Setting s; s.mix = 1; L.assign(x.size(), 0); R.assign(x.size(), 0); double worst = 0; int sw = -1; bool fin = true;
    for (size_t i = 0; i < x.size(); i += 128) { if (i >= (size_t)fs) s.decay = 1.0; if (i >= (size_t)fs && !given) { ext.run(3.8); r.offer(ext.hL, ext.hR, 3.8); given = true; } auto t0 = std::chrono::steady_clock::now(); r.process(&x[i], nullptr, &L[i], &R[i], 128, s); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      worst = std::max(worst, dt / (128 / fs)); if (sw < 0 && r.t500Act > 3.7) sw = (int)i; for (int j = 0; j < 128; j++) if (!std::isfinite(L[i + j])) fin = false; }
    auto rms = [&](double a, double b) { double e = 0; for (size_t i = (size_t)(a * fs); i < (size_t)(b * fs); i++) e += L[i] * L[i]; return 10 * std::log10(e / ((b - a) * fs)); };
    double m = 1e9; for (double t = 1.0; t < 3.9; t += 0.05) m = std::min(m, rms(t, t + 0.05));
    std::printf("== DECAY 2,0 → 3,8 s bei 1 s (Impulsantwort fertig angeboten): neue Platte nach %.2f s, längster Block %.0f %% der Blockdauer, endlich %s; Pegel 0,5–1 s %.1f dB, kleinster 50-ms-Pegel während des Wechsels %.1f dB, nach 5 s %.1f dB\n",
      sw / fs - 1, worst * 100, fin ? "ja" : "NEIN", rms(0.5, 1.0), m, rms(5, 6)); if (!fin || m < rms(0.5, 1.0) - 3) bad++; }
  if (all || w == "rate") {
    auto tone = [](double f0, size_t n, double fsx) { std::vector<float> x(n); const double fr[5] = {200, 1000, 3100, 7300, 12100}; for (size_t i = 0; i < n; i++) { double v = 0; for (double f : fr) v += 0.1 * std::sin(2 * PI * f * i / fsx); x[i] = (float)v; } (void)f0; return x; };
    auto wetDb = [&](double fsx) { Reverb r; r.setup(fsx); Setting s; s.mix = 1; std::vector<float> x = tone(0, (size_t)(4 * fsx), fsx), L, R; run(r, x, L, R, s); double e = 0; size_t a = (size_t)(2 * fsx);
      for (size_t i = a; i < L.size(); i++) e += 0.5 * (L[i] * L[i] + R[i] * R[i]); return 10 * std::log10(e / (L.size() - a)); };
    const double ref = wetDb(48000);
    for (double fsx : {22050.0, 32000.0, 44100.0}) { IRJob j; j.setup(fsx); const double fm = j.md.empty() ? 0 : j.md.back().f;   // ab 1.5: keine Mode über 0,45·fs
      std::printf("   %.0f Hz: %zu Moden, höchste %.0f Hz (Grenze %.0f Hz)\n", fsx, j.md.size(), fm, std::min(18000.0, 0.45 * fsx)); if (fm > std::min(18000.0, 0.45 * fsx)) bad++; }
    for (double fsx : {96000.0, 192000.0}) { Reverb r; auto t0 = std::chrono::steady_clock::now(); r.setup(fsx); double ts = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      std::printf("== %.0f kHz: Hall intern bei %.0f kHz (Faktor %d, Filter %d Abgriffe, Laufzeit im Hallweg %.2f ms), Einrichten %.2f s\n", fsx / 1000, r.fsi / 1000, r.K, r.rs.N, (r.rs.N - 1) / fsx * 1e3, ts);
      double lv = wetDb(fsx); std::printf("   Hallpegel bei fünf Tönen 200 Hz … 12,1 kHz: %+.2f dB gegen 48 kHz\n", lv - ref); if (std::fabs(lv - ref) > 0.3) bad++;
      Setting s; s.mix = 1; std::vector<float> x((size_t)(8.5 * fsx), 0.f), L, R; x[0] = 1; run(r, x, L, R, s, 128);
      for (double fc : {500.0, 2000.0}) { double t = t30(L, R, fc, fsx), e = t30Model(r.job, 2.0, fc, fsx); std::printf("   T30 bei %4.0f Hz: %.2f s (Modell %.2f s)\n", fc, t, e); if (std::fabs(t / e - 1) > 0.05) bad++; }
      const int NF = 1 << 19; FFT f; f.init(NF); std::vector<cf> X(NF); for (int i = 0; i < NF; i++) X[i] = cf(L[i], R[i]); f.run(X.data(), false); double eHi = 0, eAll = 0;
      for (int k = 1; k < NF / 2; k++) { double e = std::norm(X[k]) + std::norm(X[NF - k]); eAll += e; if (k * fsx / NF > 26000) eHi += e; }
      std::printf("   Anteil über 26 kHz an der Impulsantwort: %.1f dB\n", 10 * std::log10(eHi / eAll + 1e-30)); if (eHi / eAll > 1e-7) bad++;
      std::vector<float> y((size_t)(5 * fsx)), L2, R2; { uint32_t q = 5; for (auto &v : y) { q = q * 1664525u + 1013904223u; v = (float)(((q >> 8) / 16777216.0 - 0.5) * 0.3); } } Setting s2; s2.mix = 0.5;
      auto t1 = std::chrono::steady_clock::now(); run(r, y, L2, R2, s2, 128); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count(); std::printf("   Rechenzeit %.2f %% eines Kerns\n", dt / 5 * 100); } }
  if (all || w == "exact") {   // Impulsantwort des Auftrags gegen die exakte Summe aller Moden im Zeitbereich (vor der Normierung: job.gain herausrechnen)
    Reverb r; r.setup(fs); IRJob &j = r.job; j.run(2.0); const int n1 = (int)(4.2 * fs); std::vector<double> xl(n1, 0.0);
    for (const Mode &m : j.md) { double sg = 0.5 * sigmaAt(j.pl, m.f, j.s), rr = std::exp(-sg / fs), th = 2 * PI * m.f / fs, c1 = 2 * rr * std::cos(th), c2 = -rr * rr, y2 = 1, y1 = rr * std::cos(th);
      xl[0] += m.aL; xl[1] += m.aL * y1; for (int n = 2; n < n1; n++) { double y = c1 * y1 + c2 * y2; y2 = y1; y1 = y; xl[n] += m.aL * y; } }
    for (double t0 : {0.0, 0.105, 0.2, 1.0, 2.0, 3.0, 4.0}) { int a0 = (int)(t0 * fs), a1 = std::min(n1, a0 + (int)(0.02 * fs)); double e = 0, ref = 0;
      for (int n = a0; n < a1; n++) { double h = j.hL[n] / j.gain; e += (h - xl[n]) * (h - xl[n]); ref += xl[n] * xl[n]; }
      std::printf("exakt %.2f … %.2f s: Abweichung %.1f dB\n", t0, t0 + 0.02, 10 * std::log10(e / ref)); if (10 * std::log10(e / ref) > -30) bad++; } }
  std::printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
