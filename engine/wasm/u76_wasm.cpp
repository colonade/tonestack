// C-Schnittstelle des AmpFett-76-Kerns für WebAssembly: Gerät (Wiedergabe) und eine zweite Instanz nur für die Kennlinie
#include "u76_core.hpp"
using namespace u76;
static Amp *A = nullptr, *Q = nullptr; static float bufS[8192], tmpS[2048]; static double tgtS[16], peakS[8], lvlS[64], respS[64];
// tgtS wie die Plugin-Parameter: 0 INPUT, 1 OUTPUT (0…10), 2 ATTACK (0 = OFF, 1 langsam … 7 schnell), 3 RELEASE (1 … 7), 4 RATIO (0 = 4:1, 1 = 8:1, 2 = 12:1,
// 3 = 20:1, 4 = alle Tasten, 5 = stufenlos), 5 stufenlose Ratio (4 … 20), 6 DRY (0…10), 7 Hochpass der Seitenkette (0/1), 8 dessen Frequenz (20 … 600 Hz),
// 9 zweite Stufe (0/1), 10 THRU, 11 ATTACK 2, 12 RELEASE 2, 13 OUTPUT 2, 14 Kennlinie der Potis (0 log 15 %, 1 log 10 %, 2 linear), 15 vin (V Spitze bei 0 dBFS)
static double num(double x, double lo, double hi, double d) { return std::isfinite(x) ? std::max(lo, std::min(hi, x)) : d; }
static double attOf(double a) { return a < 0.5 ? -1 : std::max(0.0, std::min(1.0, (a - 1) / 6)); }
static double relOf(double r) { return std::max(0.0, std::min(1.0, (r - 1) / 6)); }
static double rvarOf(double q) { const double p[4] = {4, 8, 12, 20}; q = std::max(4.0, std::min(20.0, q)); for (int k = 0; k < 3; k++) if (q <= p[k + 1]) return (k + (q - p[k]) / (p[k + 1] - p[k])) / 3; return 1; }
static Setting fromT(int stage) { Setting s; s.input = num(tgtS[stage ? 10 : 0], 0, 10, 5) / 10; s.output = num(tgtS[stage ? 13 : 1], 0, 10, 5) / 10;
  s.attack = attOf(num(tgtS[stage ? 11 : 2], 0, 7, 4)); s.release = relOf(num(tgtS[stage ? 12 : 3], 1, 7, 4)); s.ratio = (int)std::lround(num(tgtS[4], 0, 5, 0)); s.rvar = rvarOf(num(tgtS[5], 4, 20, 8));
  s.hpf = tgtS[7] > 0.5; s.hpfF = std::log(num(tgtS[8], 20, 600, 100) / 20) / std::log(30.0); s.taper = (int)std::lround(num(tgtS[14], 0, 2, 0)); return s; }
static void applyTo(Amp *a, bool snap) { a->vin = num(tgtS[15], 1e-3, 30, 0.3); a->stack = tgtS[9] > 0.5; a->dry = potTaper(num(tgtS[6], 0, 10, 0) / 10, 0); a->setTarget(fromT(0), fromT(1)); if (snap) a->snap(); }
extern "C" {
float *u7_buffer() { return bufS; } double *u7_target_buffer() { return tgtS; } double *u7_peaks_buffer() { return peakS; } double *u7_level_buffer() { return lvlS; } double *u7_resp_buffer() { return respS; }
void u7_init(double fs) { delete A; A = new Amp(); Setting s0; s0.attack = attOf(4); s0.release = relOf(4); A->init(fs, Parts(), s0, s0); A->ready = true; applyTo(A, true); }
int u7_latency() { return 0; }
void u7_set_target() { if (A) applyTo(A, false); }
void u7_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufS, n); else for (int i = 0; i < n; i++) bufS[i] = 0; }
// peakS: 0/1 Gain Reduction Stufe 1/2 (dB, Spitze seit der letzten Abfrage), 2 Ausgang Spitze (V), 3 Ausgang effektiv (V), 4 Volt bei 0 dBFS, 5 zweite Stufe aktiv
void u7_take_peaks() { if (!A) return; double g[2]; A->takeGR(g); peakS[0] = g[0]; peakS[1] = g[1]; A->takeOut(&peakS[2], &peakS[3]); peakS[4] = A->vin; peakS[5] = A->stack ? 1 : 0; }
// Kennlinie: eigene Instanz bei fs (16 kHz genügt für 1 kHz), eingeschwungen mit den aktuellen Schalterstellungen (Ratio, Hochpass, ATTACK OFF, Kennlinie):
// nach dem Umschalten braucht die Schaltung wie das Gerät ein bis zwei Sekunden, bis der Haltepunkt wieder steht. 12 s grob sind nötig (mit 6 s lag er 17 mV daneben).
static double plotFs = 16000;
void u7_plot_init(double fs) { delete Q; Q = new Amp(); plotFs = fs; Q->init(fs, Parts(), fromT(0), fromT(1), nullptr, 12, 0.2); Q->ready = true; }
// n Eingangspegel aus lvlS (dBFS, aufsteigend; 0 dBFS = vin Volt Spitze) → respS: Ausgangspegel in dBFS (Grundwelle 1 kHz) nach je 0,12 s Ton,
// die Stufen folgen ohne Pause aufeinander (wie ein langsam lauter werdender Ton). respS[n] / respS[n + 1]: Gain Reduction der Stufen beim letzten Pegel.
void u7_plot(int n) { if (!Q || n < 0) return; if (n > 62) n = 62; Amp a; a = *Q; applyTo(&a, true); const int N = (int)(0.12 * plotFs), M = (int)(0.02 * plotFs); long t = 0; double g[2] = {0, 0};
  for (int i = 0; i < n; i++) { double amp = std::isfinite(lvlS[i]) ? std::pow(10, std::min(12.0, lvlS[i]) / 20) : 0, cs = 0, sn = 0; a.takeGR(g);
    for (int k = 0; k < N; k += 64) { int m = std::min(64, N - k); for (int j = 0; j < m; j++) tmpS[j] = (float)(amp * std::sin(2 * PI * 1000 * (t + j) / plotFs));
      a.process(tmpS, m); if (k + m > N - M) for (int j = 0; j < m; j++) if (k + j >= N - M) { double ph = 2 * PI * 1000 * (t + j) / plotFs; cs += tmpS[j] * std::cos(ph); sn += tmpS[j] * std::sin(ph); } t += m; }
    respS[i] = 20 * std::log10(2 * std::hypot(cs, sn) / M + 1e-30); }
  a.takeGR(g); respS[n] = g[0]; respS[n + 1] = g[1]; }
}
