// C-Schnittstelle des Wisent-Echoreck-Kerns für WebAssembly (wisent_core.hpp): Eingang mono, Ausgang mono.
// Zwei Instanzen: A rechnet das Signal, P (48 kHz, nie mit Signal) liefert die Anzeigen (Frequenzgang, Echomuster; Kleinsignal im Arbeitspunkt).
#include "wisent_core.hpp"
using namespace wisent;
static Core *A = nullptr, *P = nullptr; static Setting curA; static float bufW[8192]; static double tgtW[24], freqW[512], respW[3 * 512], patW[256];
static double fsA = 48000, outG = 1;
// tgtW: 0 INPUT, 1 LENGTH OF SWELL, 2 VOLUME, 3 BASS, 4 TREBLE (0 … 1), 5 Betriebsart (0 ECHO, 1 REPEAT, 2 SWELL), 6–9 Wiedergabeköpfe 1–4, 10–13 Rückkopplungsköpfe 1–4 (0/1),
// 14 Volt Spitze bei 0 dBFS, 15 OUTPUT (dB), 16 Netzfrequenz (0 = 50 Hz, 1 = 60 Hz), 17 DRY (Trimmer TD, 0 … 1), 18 Varispeed (0,25 … 2), 19 WOBBLE (Taumeln, 0 … 1; ab 1.2)
static Setting fromT() { Setting s; auto k = [](double x, double d) { return std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : d; };
  s.input = k(tgtW[0], 0.25); s.swell = k(tgtW[1], 0.5); s.volume = k(tgtW[2], 1.0); s.dry = k(tgtW[17], 0.67); s.bass = k(tgtW[3], 0.5); s.treble = k(tgtW[4], 0.5);
  s.mode = std::isfinite(tgtW[5]) ? std::max(0, std::min(2, (int)std::lround(tgtW[5]))) : M_REPEAT;
  for (int i = 0; i < 4; i++) { s.out[i] = tgtW[6 + i] > 0.5; s.fb[i] = tgtW[10 + i] > 0.5; } return s; }
static double vinT() { double v = tgtW[14]; return std::isfinite(v) && v >= 0.001 && v <= 100 ? v : 0.2; }
static double hzT() { return tgtW[16] > 0.5 ? 60 : 50; }
static double vsT() { double v = tgtW[18]; return std::isfinite(v) ? std::min(2.0, std::max(0.25, v)) : 1.0; }
static double wobT() { double w = tgtW[19]; return std::isfinite(w) ? std::min(1.0, std::max(0.0, w)) : 0.0; }
static double gainT() { double d = tgtW[15]; return std::pow(10.0, (std::isfinite(d) ? std::min(24.0, std::max(-24.0, d)) : 0) / 20); }
static double okFs(double fs) { return std::isfinite(fs) && fs >= 8000 && fs <= 384000 ? fs : 48000; }
extern "C" {
float *ws_buffer() { return bufW; } double *ws_target_buffer() { return tgtW; } double *ws_freq_buffer() { return freqW; } double *ws_resp_buffer() { return respW; } double *ws_pattern_buffer() { return patW; }
// Gerät einrichten (Ruhezustand einschwingen, etwa 0,1 s): Abtastrate, Einstellung aus tgtW
void ws_init(double fs) { delete A; A = new Core(); fsA = okFs(fs); curA = fromT(); A->init(fsA, curA, hzT(), vinT(), vsT()); A->setWobble(wobT()); outG = gainT(); }
void ws_reset() { if (A) A->drum.clear(); }   // Trommel leeren (Neubeginn der Wiedergabe)
int ws_latency() { return A ? A->lat : 0; }
// n Abtastwerte in bufW durchrechnen (an Ort und Stelle); Regler gleiten wie im Plugin (alle 32 Abtastwerte bei 48 kHz um höchstens 0,01), Tasten sofort
void ws_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (!A) { for (int i = 0; i < n; i++) bufW[i] = 0; return; }
  const Setting t = fromT(); A->setSpeed(hzT(), vsT()); A->setWobble(wobT()); A->vin = vinT(); outG = gainT(); const int ck = (int)std::max(32L, std::lround(32 * fsA / 48000));
  for (int i0 = 0; i0 < n; i0 += ck) { if (glideTo(curA, t)) A->apply(curA); const int e = std::min(n, i0 + ck);
    for (int i = i0; i < e; i++) { double y = A->tick(bufW[i]) * outG; bufW[i] = std::isfinite(y) ? (float)y : 0.f; } } }
double ws_eye() { return A ? A->eye : 0; }   // Aufnahmepegel je Sättigung des Drahts (magisches Auge, geglättet)
double ws_tau1() { return A ? A->drum.tau1 : 0.0744; }   // Zeit zwischen zwei Köpfen (s, gleitet beim Umschalten 50/60 Hz und mit Varispeed)
// ---- Anzeigen ----
static void plotSet() { if (!P) { P = new Core(); P->init(48000, fromT(), hzT()); } P->apply(fromT()); P->hz = hzT(); P->vs = vsT(); P->drum.speed(P->hz, P->vs, true); }
// Frequenzgang für n Frequenzen aus freqW → respW (dB): [0, n) trocken, [n, 2n) erstes Echo über Kopf k, [2n, 3n) Schleife über Kopf j (je Volt am Eingang bzw. je Durchlauf)
// k = erster Wiedergabekopf (SWELL: Kopf 1), j = erster Rückkopplungskopf (−1 = keiner bzw. ECHO: nur trocken und Echo)
void ws_plot(int n, int k, int j) { if (n < 0) return; if (n > 512) n = 512; plotSet(); k = std::max(0, std::min(3, k));
  for (int i = 0; i < n; i++) { const double f = freqW[i]; if (!(std::isfinite(f) && f > 0)) { respW[i] = respW[n + i] = respW[2 * n + i] = -600; continue; }
    Core::Gains g = P->gains(f, k), gl = j >= 0 && j < 4 ? P->gains(f, j) : g; auto db = [](std::complex<double> z) { return 20 * std::log10(std::abs(z) + 1e-30); };
    respW[i] = db(g.dry); respW[n + i] = db(g.echo); respW[2 * n + i] = j >= 0 && j < 4 ? db(gl.loop) : -600; } }
// Echomuster für einen kurzen Ton der Frequenz f: Betrag des Ausgangs je Eingang zur Zeit m·τ1, m = 0 … N−1 → patW
void ws_pattern(double f, int N) { if (N < 1) return; if (N > 256) N = 256; plotSet(); P->pattern(std::isfinite(f) && f > 0 ? f : 1000, N, patW); }
}
