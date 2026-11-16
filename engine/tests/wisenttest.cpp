// Prüfungen des Wisent-Echoreck-Kerns (wisent_core.hpp):
//   dc      Ruhespannungen gegen den Plan, Abgleichwerte
//   time    Echozeiten der vier Köpfe (50 und 60 Hz)
//   loop    Rückkopplung: Verhältnis aufeinanderfolgender Wiederholungen über LENGTH OF SWELL (REPEAT, Kopf 1)
//   pattern Kleinsignal-Echomuster gegen die Rechnung im Zeitbereich, Vorzeichen der Wiedergabe (Rampe)
//   thr     Schwelle der Rückkopplung über der Geschwindigkeit (Varispeed)
//   tone    Richtung der Klangregelung
//   lat     Latenz über die Schaltung hinaus je Abtastrate gegen die gemeldete
//   level   Pegel trocken / Echo bei 1 kHz, Klirrfaktor über dem Eingangspegel
//   cpu     Rechenzeit je Abtastwert
//   (Umgebung FS=Abtastrate, Standard 48000)
//   wav <datei.f32> <aus.f32> [mode input swell volume heads(out) heads(fb)]   mono float32 roh durch das Gerät
#include "wisent_core.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace wisent;
static double rmsOf(const std::vector<double> &y, size_t a, size_t b) { double e = 0; for (size_t i = a; i < b && i < y.size(); i++) e += y[i] * y[i]; return std::sqrt(e / std::max<size_t>(1, b - a)); }
int main(int argc, char **argv) {
  std::string w = argc > 1 ? argv[1] : "all"; bool all = w == "all"; const double fs = std::getenv("FS") ? std::atof(std::getenv("FS")) : 48000; int bad = 0;   // FS=…: andere Abtastrate
  if (all || w == "dc") { Core c; Setting s; auto t0 = std::chrono::steady_clock::now(); c.init(fs, s); double ti = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::vector<double> u = {0, c.v.b1, c.v.b2, c.v.b3, c.g5b}, u2 = {c.dry, 0, 0, 0, 0, c.v.b2}; dk::Vec V = dk::nodeVoltages(c.rec.net, c.fs2, c.rec.run.h, u, c.rec.run.ip), V2 = dk::nodeVoltages(c.pb.net, fs, c.pb.run.h, u2, c.pb.run.ip);
    auto at = [&](const char *n) { auto it = c.rec.net.idx.find(n); if (it != c.rec.net.idx.end()) return V[it->second]; return V2[c.pb.net.idx.at(n)]; };
    std::printf("Einrichten %.2f s. Ruhespannungen (Plan): V4A Anode %.1f (80), Mischer Anode %.1f (130), V5B Kathode %.2f (0,7), V6A Anode %.1f (130), V6A Kathode %.2f (≈4,8),\n"
      "  V3A Anode %.1f, Kathode %.2f (1,4) (V4B linear)\n", ti, at("p4"), at("pm"), at("k5b"), at("p6"), at("k6"), at("p3"), at("k3"));
    std::printf("Abgleich: Aufnahmestrom %.1f µA je V an der Anode V6A, Sättigung %.1f µA, Kopfempfindlichkeit %.3g, Ausgang ×%.3f\n", c.kRec * 1e6, c.drum.isat * 1e6, c.drum.kh, c.gOut);
    if (std::fabs(at("p4") - 80) > 15 || std::fabs(at("pm") - 130) > 20 || std::fabs(at("p6") - 130) > 25 ) bad++; }
  if (all || w == "time") for (double hz : {50.0, 60.0}) { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = false; s.volume = 1; c.init(fs, s, hz);
    std::printf("%.0f Hz: Echozeiten (Spitze der Hüllkurve, Klick)", hz);
    for (int k = 0; k < 4; k++) { Setting t = s; t.out[k] = true; c.apply(t); c.drum.clear(); std::vector<double> y((size_t)(0.5 * fs));
      for (size_t i = 0; i < y.size(); i++) { double x = i < 48 ? 0.3 * std::sin(2 * PI * 1000 * i / fs) : 0; y[i] = c.tick(x); }
      size_t pk = 0; double m = 0; for (size_t i = (size_t)(0.03 * fs); i < y.size(); i++) if (std::fabs(y[i]) > m) { m = std::fabs(y[i]); pk = i; }
      std::printf("  Kopf %d: %.1f ms", k + 1, 1000.0 * pk / fs); double want = (k + 1) * 34e-3 / (0.457 * hz / 50); if (std::fabs(pk / fs - want) > 0.003) bad++; }
    std::printf("\n"); }
  if (all || w == "loop") { std::printf("REPEAT, Kopf 1 in Ausgang und Rückkopplung: Verhältnis 2./1. Wiederholung (dB) über LENGTH OF SWELL\n ");
    for (double sw : {0.0, 0.3, 0.5, 0.7, 0.8, 0.9, 1.0}) { Core c; Setting s; s.mode = M_REPEAT; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = k == 0; s.swell = sw; s.input = 0.5; c.init(fs, s);
      std::vector<double> y((size_t)(0.4 * fs)); for (size_t i = 0; i < y.size(); i++) { double x = i < 480 ? 0.02 * std::sin(2 * PI * 1000 * i / fs) * std::sin(PI * i / 480) : 0; y[i] = c.tick(x); }
      double t1 = c.drum.tau1 * fs; double r1 = rmsOf(y, (size_t)(t1 - 0.004 * fs), (size_t)(t1 + 0.014 * fs)), r2 = rmsOf(y, (size_t)(2 * t1 - 0.004 * fs), (size_t)(2 * t1 + 0.014 * fs));
      std::printf("  %.1f: %+.1f", sw, 20 * std::log10(r2 / std::max(r1, 1e-30))); }
    std::printf("\n"); }
  if (all || w == "pattern") {   // Kleinsignal-Echomuster (Anzeige der Seite) gegen die Rechnung im Zeitbereich; Trommel: Vorzeichen der Ableitung (Rampe)
    std::printf("Echomuster (Kleinsignal, 1 kHz) gegen Zeitbereich, REPEAT Kopf 1: Verhältnis 2./1. Wiederholung (dB)\n ");
    for (double sw : {0.0, 0.5, 0.9}) { Core c; Setting s; s.mode = M_REPEAT; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = k == 0; s.swell = sw; s.input = 0.5; c.init(fs, s);
      double o[4]; c.pattern(1000, 4, o); double pr = 20 * std::log10(o[2] / o[1]);
      std::vector<double> y((size_t)(0.4 * fs)); for (size_t i = 0; i < y.size(); i++) { double x = i < 480 ? 0.02 * std::sin(2 * PI * 1000 * i / fs) * std::sin(PI * i / 480) : 0; y[i] = c.tick(x); }
      double t1 = c.drum.tau1 * fs; double r1 = rmsOf(y, (size_t)(t1 - 0.004 * fs), (size_t)(t1 + 0.014 * fs)), r2 = rmsOf(y, (size_t)(2 * t1 - 0.004 * fs), (size_t)(2 * t1 + 0.014 * fs)), me = 20 * std::log10(r2 / r1);
      std::printf("  %.1f: Muster %+.2f, Zeitbereich %+.2f", sw, pr, me); if (std::fabs(pr - me) > 0.5) bad++; }
    std::printf("\n");
    { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) { s.out[k] = k == 0; s.fb[k] = false; } c.init(fs, s);   // erstes Echo: Pegel aus dem Muster gegen die Messung (1 kHz, −40 dBFS)
      double o[2]; c.pattern(1000, 2, o); double a = 0.01 * std::sqrt(2.0); c.drum.clear(); std::vector<double> y((size_t)(0.25 * fs));
      for (size_t i = 0; i < y.size(); i++) y[i] = c.tick(i < (size_t)(0.06 * fs) ? a * std::sin(2 * PI * 1000 * i / fs) : 0);
      double t1 = c.drum.tau1 * fs, eo = rmsOf(y, (size_t)(t1 + 0.02 * fs), (size_t)(t1 + 0.05 * fs)) / (a / std::sqrt(2.0)), d = rmsOf(y, (size_t)(0.02 * fs), (size_t)(0.055 * fs)) / (a / std::sqrt(2.0));
      std::printf("ECHO Kopf 1, −40 dBFS: trocken Muster %+.2f / gemessen %+.2f dB, Echo Muster %+.2f / gemessen %+.2f dB\n", 20 * std::log10(o[0]), 20 * std::log10(d), 20 * std::log10(o[1]), 20 * std::log10(eo));
      if (std::fabs(20 * std::log10(o[1] / eo)) > 0.5 || std::fabs(20 * std::log10(o[0] / d)) > 0.5) bad++; }
    { Drum d; d.setup(2 * fs, 50); d.kh = 1; d.isat = 1e9; d.eraseRes = 0; d.wowA = 0; double e[4] = {}, last = 0; const long n = (long)(0.2 * 2 * fs);   // Rampe: 1 A/s steigend
      for (long i = 0; i < n; i++) { d.read(e); d.write((double)i / (2 * fs)); last = e[0]; }
      std::printf("Trommel: Rampe 1 A/s (nach dem Aufnahme-Tiefpass) → Kopf 1 %+.4f V (Soll ≈ +1,38: positive Ableitung, Tiefen +2,8 dB über 1 kHz)\n", last); if (last < 1 || last > 1.6) bad++; }
    for (double f2 : {2 * fs, fs < 88000 ? 4 * fs : 2 * fs}) { Drum d; d.setup(f2, 50); std::printf("Wiedergabe-FIR bei %.0f Hz (%d Abgriffe), Abweichung vom Ziel (dB, Bezug 1 kHz bei 50 Hz):", f2, d.nf); double wmax = 0;
      auto ideal = [&](double f, double v) { double u = PI * d.gpb * f / v; return 2 * PI * f * std::exp(-2 * PI * d.dsp * f / v) * (u > 1e-9 ? std::sin(u) / u : 1); }; const double ref = ideal(1000, d.v50) / (2 * PI * 1000);
      for (double hz : {50.0, 60.0}) { d.speed(hz, 1, true); if (hz == 60) std::printf(" | 60 Hz:");
        for (double f : {50.0, 100.0, 300.0, 1000.0, 3000.0, 8000.0, 12000.0}) { double e = 20 * std::log10(std::abs(d.firH(f)) / ideal(f, d.v) * ref);
          std::printf(" %.0f:%+.2f", f, e); if (f >= 300) wmax = std::max(wmax, std::fabs(e)); else wmax = std::max(wmax, std::fabs(e) - 0.6); } }
      std::printf("\n"); if (wmax > 0.2) bad++; } }
  if (all || w == "thr") {   // Schwelle der Schleife über der Geschwindigkeit gegen loopThreshold (Varispeed)
    std::printf("Schwelle der Schleife (REPEAT, ein Kopf), LENGTH OF SWELL: gerechnet / Tabelle\n ");
    for (double r : {0.8, 1.0, 1.4, 2.0, 2.4}) { double thr = -1; for (double sw = 0.1; sw <= 1.0001 && thr < 0; sw += 0.01) { Core c; Setting s; s.mode = M_REPEAT; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = k == 0; s.swell = sw;
        c.init(fs, s, r > 2 ? 60 : 50, 0.2, r > 2 ? r / 1.2 : r); double mx = -1e9; for (double f = 30; f < 15000; f *= 1.03) mx = std::max(mx, 20 * std::log10(std::abs(c.gains(f, 0).loop))); if (mx >= 0) thr = sw; }
      std::printf("  %.1f×: %.2f / %.2f", r, thr, loopThreshold(r)); if (std::fabs(thr - loopThreshold(r)) > 0.025) bad++; }
    std::printf("\n"); }
  if (all || w == "tone") {   // Klangregelung: Rechtsanschlag = mehr Tiefen bzw. Höhen (Echo je Aufnahmestrom und Trommel, also nur Wiedergabeseite)
    auto e = [&](double ba, double tr, double f) { Core c; Setting s; s.mode = M_ECHO; s.bass = ba; s.treble = tr; for (int k = 0; k < 4; k++) { s.out[k] = k == 0; s.fb[k] = false; } c.init(fs, s);
      auto g = c.gains(f, 0); return 20 * std::log10(std::abs(g.echo / g.rec / c.drumH(f))); };
    double b0 = e(0, 0.5, 100), b1 = e(1, 0.5, 100), t0 = e(0.5, 0, 7000), t1 = e(0.5, 1, 7000);
    std::printf("Klangregelung (Echo, Wiedergabeseite): BASS 0/1 bei 100 Hz %+.1f / %+.1f dB, TREBLE 0/1 bei 7 kHz %+.1f / %+.1f dB\n", b0, b1, t0, t1); if (b1 - b0 < 10 || t1 - t0 < 10) bad++; }
  if (all || w == "wob") {   // Taumeln (ab 1.2): Poisson-Kern gegen e^(−2π·τ·f); Hüllkurve des Echos über eine Umdrehung gegen den größten Zusatzverlust
    Core c; Setting s0; c.init(fs, s0); double mx = 0; std::printf("Taumeln, Kern gegen e^(-2pi tau f) (dB, Breite in Abtastwerten der Trommelrate):\n ");
    for (double ts : {0.5, 3.0, 12.0, 40.0}) for (double f : {500.0, 2000.0, 5000.0}) { const double want = -2 * PI * ts / c.drum.fs * f * 20 / std::log(10.0); if (want < -30) continue;
      const double got = 20 * std::log10(std::fabs(c.drum.wobGain(ts, f))); mx = std::max(mx, std::fabs(got - want)); std::printf(" %.1f/%.0f Hz: %+.2f (%+.2f)", ts, f, got, want); }
    std::printf("\n  größte Abweichung %.2f dB (erwartet < 0,3)\n", mx); if (mx > 0.3) bad++;
    for (int k = 0; k < 2; k++) { const double wb = k ? 1.0 : 0.0, f = 1000; Core e; Setting s; s.mode = M_ECHO; s.dry = 0; s.input = 0.25; for (int j = 0; j < 4; j++) { s.out[j] = j == 0; s.fb[j] = false; } e.init(fs, s); e.setWobble(wb);
      const long n0 = (long)(1.0 * fs), nr = (long)(e.drum.trev * fs), win = (long)(0.02 * fs); double lo = 1e9, hi = 0, acc = 0; long cnt = 0;
      for (long i = 0; i < n0 + nr; i++) { const double y = e.tick(0.002 * std::sin(2 * PI * f * i / fs)); if (i < n0) continue; acc += y * y; if (++cnt == win) { const double r = std::sqrt(acc / win); lo = std::min(lo, r); hi = std::max(hi, r); acc = 0; cnt = 0; } }
      const double want = k ? 2 * PI * 2 * Core::WOBMAX * f / e.drum.v * 20 / std::log(10.0) : 0, got = 20 * std::log10(hi / lo);
      std::printf("Echo über eine Umdrehung (1 kHz, Kopf 1), WOBBLE %.0f: Schwankung %.2f dB (erwartet etwa %.2f)\n", wb * 10, got, want); if (std::fabs(got - want) > (k ? 1.0 : 0.3)) bad++; } }
  if (all || w == "lat") {   // Laufzeit über die Schaltung hinaus (Halbbänder, Sammeln bei dec > 1) aus der Phase des trockenen Wegs bei 1 kHz, gegen die gemeldete Latenz
    std::printf("Latenz (Abtastwerte der Plugin-Rate): gemessen / gemeldet\n ");
    for (double f2 : {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0, 352800.0, 384000.0}) { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) s.out[k] = s.fb[k] = false; c.init(f2, s);
      const double f = 1000; const long n = (long)(0.3 * f2), n0 = (long)(0.2 * f2); std::complex<double> acc = 0;
      for (long i = 0; i < n; i++) { double y = c.tick(0.01 * std::sin(2 * PI * f * i / f2)); if (i >= n0) acc += y * std::polar(1.0, -2 * PI * f * i / f2); }
      double dphi = std::arg(acc / std::complex<double>(0, -1)) - std::arg(c.gains(f, 0).dry); while (dphi > PI) dphi -= 2 * PI; while (dphi < -PI) dphi += 2 * PI;
      double d = -dphi / (2 * PI * f) * f2; std::printf("  %.1f kHz: %.2f / %d", f2 / 1000, d, c.lat); if (std::fabs(d - c.lat) > 0.6) bad++; }
    std::printf("\n"); }
  if (all || w == "level") { Core c; Setting s; s.mode = M_ECHO; for (int k = 0; k < 4; k++) { s.out[k] = k == 0; s.fb[k] = false; } c.init(fs, s);
    std::printf("ECHO, Kopf 1, INPUT %.2f, VOLUME %.2f: Eingang 1 kHz → trocken (vor dem Echo) / Echo (dB re Eingang)\n", s.input, s.volume);
    for (double db : {-40.0, -30.0, -20.0, -10.0, 0.0}) { double a = std::pow(10, db / 20) * std::sqrt(2.0); c.drum.clear(); std::vector<double> y((size_t)(0.25 * fs));
      for (size_t i = 0; i < y.size(); i++) y[i] = c.tick(i < (size_t)(0.06 * fs) ? a * std::sin(2 * PI * 1000 * i / fs) : 0);
      double d = rmsOf(y, (size_t)(0.02 * fs), (size_t)(0.055 * fs)), t1 = c.drum.tau1 * fs, e = rmsOf(y, (size_t)(t1 + 0.07 * fs), (size_t)(t1 + 0.1 * fs));
      // Echo allein: Rest nach dem Ende des Eingangs (0,06 s … ) bei t1 + 0,02 … 0,05 s
      double e2 = rmsOf(y, (size_t)(0.065 * fs + 0.0 * fs), (size_t)(0.0 * fs + t1)); (void)e2;
      double eo = rmsOf(y, (size_t)(t1 + 0.02 * fs), (size_t)(t1 + 0.05 * fs));
      std::printf("  %+5.0f dBFS: trocken %+.1f dB, Echo %+.1f dB\n", db, 20 * std::log10(d / (a / std::sqrt(2.0))), 20 * std::log10(eo / (a / std::sqrt(2.0)))); (void)e; } }
  if (all || w == "cpu") { Core c; Setting s; c.init(fs, s); std::vector<double> x((size_t)(5 * fs)); uint32_t q = 1; for (auto &v : x) { q = q * 1664525u + 1013904223u; v = ((q >> 8) / 16777216.0 - 0.5) * 0.3; }
    auto t0 = std::chrono::steady_clock::now(); double acc = 0; for (double v : x) acc += c.tick(v); double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("Rechenzeit: %.0f ns je Abtastwert (%.1f %% eines Kerns bei %.1f kHz), Newton-Fehlschläge %d %s\n", dt / x.size() * 1e9, dt / 5 * 100, fs / 1000, c.rec.run.fails + c.pb.run.fails, std::isfinite(acc) ? "" : "NICHT ENDLICH"); }
  if (w == "wav") { FILE *f = std::fopen(argv[2], "rb"); std::vector<float> x; float b; while (std::fread(&b, 4, 1, f) == 1) x.push_back(b); std::fclose(f);
    Core c; Setting s; if (argc > 4) s.mode = std::atoi(argv[4]); if (argc > 5) s.input = std::atof(argv[5]); if (argc > 6) s.swell = std::atof(argv[6]); if (argc > 7) s.volume = std::atof(argv[7]);
    if (argc > 8) for (int k = 0; k < 4; k++) s.out[k] = argv[8][k] == '1'; if (argc > 9) for (int k = 0; k < 4; k++) s.fb[k] = argv[9][k] == '1'; c.init(fs, s); if (argc > 10) c.setWobble(std::atof(argv[10]));   // 10: WOBBLE 0 … 1 (ab 1.2)
    FILE *o = std::fopen(argv[3], "wb"); for (size_t i = 0; i < x.size() + (size_t)(3 * fs); i++) { float y = (float)c.tick(i < x.size() ? x[i] : 0); std::fwrite(&y, 4, 1, o); } std::fclose(o); return 0; }
  std::printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
