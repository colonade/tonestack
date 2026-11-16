// Prüfungen des Wisent-Echoreck-Kerns (wisent_core.hpp):
//   dc      Ruhespannungen gegen den Plan, Abgleichwerte
//   time    Echozeiten der vier Köpfe (50 und 60 Hz)
//   loop    Rückkopplung: Verhältnis aufeinanderfolgender Wiederholungen über LENGTH OF SWELL (REPEAT, Kopf 1)
//   level   Pegel trocken / Echo bei 1 kHz, Klirrfaktor über dem Eingangspegel
//   cpu     Rechenzeit je Abtastwert
//   wav <datei.f32> <aus.f32> [mode input swell volume heads(out) heads(fb)]   mono float32 roh durch das Gerät
#include "wisent_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace wisent;
static double rmsOf(const std::vector<double> &y, size_t a, size_t b) { double e = 0; for (size_t i = a; i < b && i < y.size(); i++) e += y[i] * y[i]; return std::sqrt(e / std::max<size_t>(1, b - a)); }
int main(int argc, char **argv) {
  std::string w = argc > 1 ? argv[1] : "all"; bool all = w == "all"; const double fs = 48000; int bad = 0;
  if (all || w == "dc") { Core c; Setting s; auto t0 = std::chrono::steady_clock::now(); c.init(fs, s); double ti = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::vector<double> u = {0, c.v.b1, c.v.b2, c.v.b3, c.g5b}, u2 = {c.dry, 0, 0, 0, 0}; dk::Vec V = dk::nodeVoltages(c.rec.net, c.fs2, c.rec.run.h, u, c.rec.run.ip), V2 = dk::nodeVoltages(c.pb.net, fs, c.pb.run.h, u2, c.pb.run.ip);
    auto at = [&](const char *n) { auto it = c.rec.net.idx.find(n); if (it != c.rec.net.idx.end()) return V[it->second]; return V2[c.pb.net.idx.at(n)]; };
    std::printf("Einrichten %.2f s. Ruhespannungen (Plan): V4A Anode %.1f (80), Mischer Anode %.1f (130), V5B Kathode %.2f (0,7), V6A Anode %.1f (130), V6A Kathode %.2f (≈4,8),\n"
      "  (V3A, V4B linear)\n", ti, at("p4"), at("pm"), at("k5b"), at("p6"), at("k6"));
    std::printf("Abgleich: Aufnahmestrom %.1f µA je V an der Anode V6A, Sättigung %.1f µA, Kopfempfindlichkeit %.3g, Ausgang ×%.3f\n", c.kRec * 1e6, c.drum.isat * 1e6, c.drum.kh, c.gOut);
    if (std::fabs(at("p4") - 80) > 15 || std::fabs(at("pm") - 130) > 20 || std::fabs(at("p6") - 130) > 25 ) bad++; }
  if (all || w == "time") for (double hz : {50.0, 60.0}) { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = false; s.volume = 1; c.init(fs, s, hz);
    std::printf("%.0f Hz: Echozeiten (Spitze der Hüllkurve, Klick)", hz);
    for (int k = 0; k < 4; k++) { Setting t = s; t.out[k] = true; c.apply(t); c.drum.clear(); std::vector<double> y((size_t)(0.5 * fs));
      for (size_t i = 0; i < y.size(); i++) { double x = i < 48 ? 0.3 * std::sin(2 * PI * 1000 * i / fs) : 0; y[i] = c.step(x); }
      size_t pk = 0; double m = 0; for (size_t i = (size_t)(0.03 * fs); i < y.size(); i++) if (std::fabs(y[i]) > m) { m = std::fabs(y[i]); pk = i; }
      std::printf("  Kopf %d: %.1f ms", k + 1, 1000.0 * pk / fs); double want = (k + 1) * 34e-3 / (0.457 * hz / 50); if (std::fabs(pk / fs - want) > 0.003) bad++; }
    std::printf("\n"); }
  if (all || w == "loop") { std::printf("REPEAT, Kopf 1 in Ausgang und Rückkopplung: Verhältnis 2./1. Wiederholung (dB) über LENGTH OF SWELL\n ");
    for (double sw : {0.0, 0.3, 0.5, 0.7, 0.8, 0.9, 1.0}) { Core c; Setting s; s.mode = M_REPEAT; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = k == 0; s.swell = sw; s.input = 0.5; c.init(fs, s);
      std::vector<double> y((size_t)(0.4 * fs)); for (size_t i = 0; i < y.size(); i++) { double x = i < 480 ? 0.02 * std::sin(2 * PI * 1000 * i / fs) * std::sin(PI * i / 480) : 0; y[i] = c.step(x); }
      double t1 = c.drum.tau1 * fs; double r1 = rmsOf(y, (size_t)(t1 - 0.004 * fs), (size_t)(t1 + 0.014 * fs)), r2 = rmsOf(y, (size_t)(2 * t1 - 0.004 * fs), (size_t)(2 * t1 + 0.014 * fs));
      std::printf("  %.1f: %+.1f", sw, 20 * std::log10(r2 / std::max(r1, 1e-30))); }
    std::printf("\n"); }
  if (all || w == "level") { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) { s.out[k] = k == 0; s.fb[k] = false; } c.init(fs, s);
    std::printf("ECHO, Kopf 1, INPUT %.1f, VOLUME %.1f: Eingang 1 kHz → trocken (vor dem Echo) / Echo (dB re Eingang)\n", s.input, s.volume);
    for (double db : {-40.0, -30.0, -20.0, -10.0, 0.0}) { double a = std::pow(10, db / 20) * std::sqrt(2.0); c.drum.clear(); std::vector<double> y((size_t)(0.25 * fs));
      for (size_t i = 0; i < y.size(); i++) y[i] = c.step(i < (size_t)(0.06 * fs) ? a * std::sin(2 * PI * 1000 * i / fs) : 0);
      double d = rmsOf(y, (size_t)(0.02 * fs), (size_t)(0.055 * fs)), t1 = c.drum.tau1 * fs, e = rmsOf(y, (size_t)(t1 + 0.07 * fs), (size_t)(t1 + 0.1 * fs));
      // Echo allein: Rest nach dem Ende des Eingangs (0,06 s … ) bei t1 + 0,02 … 0,05 s
      double e2 = rmsOf(y, (size_t)(0.065 * fs + 0.0 * fs), (size_t)(0.0 * fs + t1)); (void)e2;
      double eo = rmsOf(y, (size_t)(t1 + 0.02 * fs), (size_t)(t1 + 0.05 * fs));
      std::printf("  %+5.0f dBFS: trocken %+.1f dB, Echo %+.1f dB\n", db, 20 * std::log10(d / (a / std::sqrt(2.0))), 20 * std::log10(eo / (a / std::sqrt(2.0)))); (void)e; } }
  if (all || w == "cpu") { Core c; Setting s; c.init(fs, s); std::vector<double> x((size_t)(5 * fs)); uint32_t q = 1; for (auto &v : x) { q = q * 1664525u + 1013904223u; v = ((q >> 8) / 16777216.0 - 0.5) * 0.3; }
    auto t0 = std::chrono::steady_clock::now(); double acc = 0; for (double v : x) acc += c.step(v); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("Rechenzeit: %.0f ns je Abtastwert (%.1f %% eines Kerns bei 48 kHz), Newton-Fehlschläge %d %s\n", dt / x.size() * 1e9, dt / 5 * 100, c.rec.run.fails + c.pb.run.fails, std::isfinite(acc) ? "" : "NICHT ENDLICH"); }
  if (w == "wav") { FILE *f = std::fopen(argv[2], "rb"); std::vector<float> x; float b; while (std::fread(&b, 4, 1, f) == 1) x.push_back(b); std::fclose(f);
    Core c; Setting s; if (argc > 4) s.mode = std::atoi(argv[4]); if (argc > 5) s.input = std::atof(argv[5]); if (argc > 6) s.swell = std::atof(argv[6]); if (argc > 7) s.volume = std::atof(argv[7]);
    if (argc > 8) for (int k = 0; k < 4; k++) s.out[k] = argv[8][k] == '1'; if (argc > 9) for (int k = 0; k < 4; k++) s.fb[k] = argv[9][k] == '1'; c.init(fs, s);
    FILE *o = std::fopen(argv[3], "wb"); for (size_t i = 0; i < x.size() + (size_t)(3 * fs); i++) { float y = (float)c.step(i < x.size() ? x[i] : 0); std::fwrite(&y, 4, 1, o); } std::fclose(o); return 0; }
  std::printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
