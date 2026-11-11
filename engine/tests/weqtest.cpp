// Prüfstand Wien-EQ (engine/src/weq_core.hpp): Angaben von adt-audio, digitale Bänder gegen analog, ganzer Kanal (Impulsantwort) gegen analog.
//   g++ -O3 -std=c++17 -Isrc tests/weqtest.cpp -o build/weqtest && build/weqtest
// Ausgabe je Prüfung eine Zeile „Abweichung …“ (innerhalb der Grenze) oder „ABWEICHUNG …“ (Grenze überschritten, Rückgabewert 1).
#include "weq_core.hpp"
#include <cstdio>
#include <random>
using namespace weq;
static int fails = 0;
static void line(const char *what, double v, double lim, const char *unit) {
  bool ok = std::fabs(v) <= lim; if (!ok) fails++;
  std::printf("%s %s: %+.4f %s (Grenze ±%g)\n", ok ? "Abweichung" : "ABWEICHUNG", what, v, unit, lim); }
static double qOct(double q) { return 2 * std::asinh(1 / (2 * q)) / std::log(2.0); }
int main() {
  // 1. Angaben (Beschreibung TM205): Bereiche der Mittenbänder, Bandbreite 5 Oktaven … Terz, LOW/HIGH-Frequenzen, Regelbereich
  const double lo[3] = {20, 60, 250}, hi[3] = {2000, 7000, 25000};
  double e = 0; for (int k = 0; k < 3; k++) { e = std::max(e, std::fabs(midFreq(k, 0) / lo[k] - 1)); e = std::max(e, std::fabs(midFreq(k, 1) / hi[k] - 1)); }
  line("Mittenfrequenz an den Anschlägen (relativ)", e, 1e-9, "");
  line("Bandbreite Q links (Soll 5 Oktaven)", qOct(midQ(0)) - 5, 1e-6, "Okt.");
  line("Bandbreite Q rechts (Soll 1/3 Oktave)", qOct(midQ(1)) - 1.0 / 3, 1e-6, "Okt.");
  // MID: voller Pegel an der Mittenfrequenz. LOW/HIGH (Kuhschwanz): voller Pegel am Rand (0 Hz bzw. weit oben), halber an der genannten Frequenz.
  { double w = 0, wh = 0; for (int r = 0; r < 3; r++) { Setting s; s.range = r; for (int b = 0; b < NB; b++) { s.gain[b] = 1; Band x[NB]; bands(s, x);
        if (b == 0 || b == 4) { for (int sg = 1; sg >= -1; sg -= 2) { s.gain[b] = sg; bands(s, x); double fe = b == 0 ? 1e-4 : 1e9;
            w = std::max(w, std::fabs(20 * std::log10(std::abs(bandS(std::complex<double>(0, 2 * PI * fe), x[b]))) - sg * RANGE_DB[r]));
            wh = std::max(wh, std::fabs(20 * std::log10(std::abs(bandS(std::complex<double>(0, 2 * PI * x[b].f), x[b]))) - sg * RANGE_DB[r] / 2));
            w = std::max(w, std::fabs(20 * std::log10(std::abs(bandS(std::complex<double>(0, 2 * PI * (b == 0 ? 1e9 : 1e-4)), x[b])))));   // am anderen Rand keine Wirkung
            Band y[NB]; s.gain[b] = -sg; bands(s, y); w = std::max(w, std::fabs(std::abs(bandS({0, 2 * PI * 777.0}, x[b]) * bandS({0, 2 * PI * 777.0}, y[b])) - 1)); } s.gain[b] = 0; continue; }
        double f = x[b].f, m = 20 * std::log10(std::abs(bandS(std::complex<double>(0, 2 * PI * f), x[b]))); w = std::max(w, std::fabs(m - RANGE_DB[r]));
        s.gain[b] = -1; bands(s, x); m = 20 * std::log10(std::abs(bandS(std::complex<double>(0, 2 * PI * f), x[b]))); w = std::max(w, std::fabs(m + RANGE_DB[r])); s.gain[b] = 0; } }
    line("Pegel bei Anschlag (±6/±12/±20 dB): MID an der Mittenfrequenz, LOW/HIGH am Rand", w, 1e-6, "dB");
    line("LOW/HIGH: halber Pegel an der genannten Frequenz", wh, 1e-9, "dB"); }
  line("HIGH ohne Taste (Soll 20 kHz)", HIGH_F[0] - 20000, 0, "Hz");
  // 2. Ein Band digital (Rechenrate) gegen analog: Raster über Frequenz, Güte, Pegel; danach die Kuhschwanzfilter LOW (40 … 160 Hz) und HIGH (4 … 20 kHz)
  for (double fsI : {88200.0, 96000.0, 176400.0, 192000.0}) { double worst = 0;
    for (int a = 0; a < 120; a++) for (int qi = 0; qi < 9; qi++) for (double db : {0.3, 1.0, 3.0, 6.0, 12.0, 20.0, -6.0, -20.0}) {
      double fc = 20 * std::pow(1250.0, a / 119.0), q = QMIN * std::pow(QMAX / QMIN, qi / 8.0); Band b{fc, q, db}; Biquad d; design(d, b, fsI);
      for (int i = 0; i < 200; i++) { double f = 20 * std::pow(1000.0, i / 199.0); std::complex<double> zi = std::exp(std::complex<double>(0, -2 * PI * f / fsI));
        std::complex<double> H = (d.b0 + d.b1 * zi + d.b2 * zi * zi) / (1.0 + d.a1 * zi + d.a2 * zi * zi); double x = 20 * std::log10(std::abs(H) / std::abs(bandS({0, 2 * PI * f}, b)));
        if (!std::isfinite(x)) x = 99; if (std::fabs(x) > std::fabs(worst)) worst = x; } }
    char t[96]; std::snprintf(t, sizeof t, "ein Band digital gegen analog, Rechenrate %.1f kHz", fsI / 1000); line(t, worst, 0.02, "dB");
    double ws = 0; for (int kind : {LOSHELF, HISHELF}) for (int a = 0; a < 40; a++) for (double db : {0.001, 0.05, 0.3, 1.0, 3.0, 6.0, 12.0, 20.0, -0.3, -6.0, -20.0}) {
      double fc = kind == LOSHELF ? 40 * std::pow(4.0, a / 39.0) : 4000 * std::pow(5.0, a / 39.0); Band b{fc, 1, db, kind}; Biquad d; design(d, b, fsI);
      for (int i = 0; i < 200; i++) { double f = 20 * std::pow(1000.0, i / 199.0); std::complex<double> zi = std::exp(std::complex<double>(0, -2 * PI * f / fsI));
        std::complex<double> H = (d.b0 + d.b1 * zi) / (1.0 + d.a1 * zi); double x = 20 * std::log10(std::abs(H) / std::abs(bandS({0, 2 * PI * f}, b)));
        if (!std::isfinite(x)) x = 99; if (std::fabs(x) > std::fabs(ws)) ws = x; } }
    std::snprintf(t, sizeof t, "Kuhschwanz LOW/HIGH digital gegen analog, Rechenrate %.1f kHz", fsI / 1000); line(t, ws, 0.02, "dB"); }
  // 3. Ganzer Kanal (Halbbandfilter, fünf Bänder, Latenz): Impulsantwort, Betrag per DFT gegen analog bis 20 kHz
  for (double fs : {44100.0, 48000.0, 96000.0}) { double worst = 0; int latBad = 0; std::mt19937 rng(7); std::uniform_real_distribution<double> U(0, 1);
    for (int n = 0; n < 12; n++) { Setting s; for (int i = 0; i < NB; i++) s.gain[i] = 2 * U(rng) - 1; for (int k = 0; k < 3; k++) { s.q[k] = U(rng); s.f[k] = U(rng); }
      s.lowSel = rng() % 4; s.highSel = rng() % 4; s.range = rng() % 3; if (n == 0) { s = Setting(); s.gain[3] = 1; s.f[2] = 1; s.q[2] = 1; s.range = 2; } if (n == 1) { s = Setting(); s.gain[4] = 1; s.range = 2; }
      Channel c; c.setup(fs); c.snap(s); const int N = 1 << 15; std::vector<float> y(N, 0.f); y[0] = 1; c.process(y.data(), N, s); int L = c.latency();
      if (L != (fs < 80000 ? HB_LAT : 0)) latBad++;
      for (int i = 0; i < 60; i++) { double f = 20 * std::pow(1000.0, i / 59.0); if (f > 20000) break; std::complex<double> acc = 0, w = std::exp(std::complex<double>(0, -2 * PI * f / fs)), z = 1;
        for (int k = 0; k < N; k++) { acc += (double)y[k] * z; z *= w; if ((k & 1023) == 1023) z /= std::abs(z); }
        double x = 20 * std::log10(std::abs(acc) / std::abs(response(f, s))); if (std::fabs(x) > std::fabs(worst)) worst = x; } }
    char t[96]; std::snprintf(t, sizeof t, "Kanal (Impulsantwort) gegen analog, %.1f kHz, 12 Einstellungen", fs / 1000); line(t, worst, 0.03, "dB");
    std::snprintf(t, sizeof t, "Latenz falsch gemeldet, %.1f kHz", fs / 1000); line(t, latBad, 0, "Fälle"); }
  std::printf(fails ? "Wien-EQ: %d Prüfungen außerhalb der Grenze\n" : "Wien-EQ: alle Prüfungen innerhalb der Grenzen\n", fails);
  return fails ? 1 : 0; }
