// C-Schnittstelle des SVT-CL-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "svt_core.hpp"
using namespace svt;
static Amp *A = nullptr; static Core *P = nullptr; static float bufS[8192]; static double tgtS[16], peakS[8], freqS[512], respS[512];
// tgtS = 0 gain, 1 bass, 2 mid, 3 treble, 4 master (0…1), 5 frequency (0…4 = 220 Hz … 3 kHz), 6 ultra lo, 7 ultra hi, 8 Buchse (0 = 0 dB, 1 = −15 dB),
// 9 Abgriff (0 = 4 Ω, 1 = 2 Ω), 10 Kennlinie der A-Potis (0 log 15 %, 1 log 10 %, 2 linear), 11 vin (V Spitze bei 0 dBFS), 12 power (0 = stumm)
static int clampI(double x, int lo, int hi, int d) { return std::isfinite(x) ? std::max(lo, std::min(hi, (int)std::lround(x))) : d; }
static Setting fromT() { Setting s; for (int i = 0; i < 5; i++) { double x = tgtS[i]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
  s.freq = clampI(tgtS[5], 0, 4, 2); s.ulo = tgtS[6] > 0.5; s.uhi = tgtS[7] > 0.5; s.jack = clampI(tgtS[8], 0, 1, 0); s.tap = clampI(tgtS[9], 0, 1, 0); s.taper = clampI(tgtS[10], 0, 2, 0); return s; }
extern "C" {
float *sv_buffer() { return bufS; } double *sv_target_buffer() { return tgtS; } double *sv_peaks_buffer() { return peakS; } double *sv_freq_buffer() { return freqS; } double *sv_resp_buffer() { return respS; }
void sv_init(double fs, int os) { delete A; A = new Amp(); int o = (os == 1 || os == 2 || os == 4) ? os : 1; A->vin = 0.1; A->power = !(tgtS[12] < 0.5); A->init(fs, o, Parts(), fromT()); }
int sv_latency() { return A ? A->latency() : 0; }
void sv_set_target() { if (!A) return; A->setTarget(fromT()); double vin = tgtS[11]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; A->power = !(tgtS[12] < 0.5); }
void sv_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufS, n); else for (int i = 0; i < n; i++) bufS[i] = 0; }
// peakS: 0 V1B, 1 V1A, 2 V2B, 3 Endstufe V1A, 4/5 6550 Seite A/B (Gitterhub, 1 = Gitter erreicht 0 V), 6 Spitzenspannung am Abgriff (V), 7 Volt bei 0 dBFS
void sv_take_peaks() { if (!A) return; A->takePeaks(peakS); peakS[6] = A->takeOut(); peakS[7] = A->vfs(); }
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void sv_plot_init(double fs) { delete P; P = new Core(); P->init(fs, Parts(), fromT()); P->settle(1.5); }
// n Frequenzen aus freqS → respS in dB (Spannung am Abgriff je Volt Eingang)
void sv_plot(int n) { if (!P || n < 0) return; if (n > 512) n = 512; P->apply(fromT());
  for (int i = 0; i < n; i++) respS[i] = std::isfinite(freqS[i]) && freqS[i] > 0 ? 20 * std::log10(std::abs(P->response(freqS[i])) + 1e-30) : -600; }
}
