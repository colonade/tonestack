// C-Schnittstelle des Loldano-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "slo_core.hpp"
using namespace slo;
static Amp *A = nullptr; static Core *P = nullptr; static float bufS[8192]; static double tgtS[16], peakS[12], freqS[512], respS[512];
// tgtS = 0 preamp overdrive, 1 preamp normal, 2 bass, 3 middle, 4 treble, 5 master overdrive, 6 master normal, 7 presence (0…1),
// 8 Kanal (1 = OVERDRIVE), 9 Crunch (0 = Clean), 10 Bright, 11 vin (V Spitze bei 0 dBFS)
static Setting fromT() { Setting s; for (int i = 0; i < 8; i++) { double x = tgtS[i]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
  s.od = !(tgtS[8] < 0.5); s.crunch = tgtS[9] > 0.5; s.bright = tgtS[10] > 0.5; return s; }
extern "C" {
float *lo_buffer() { return bufS; } double *lo_target_buffer() { return tgtS; } double *lo_peaks_buffer() { return peakS; } double *lo_freq_buffer() { return freqS; } double *lo_resp_buffer() { return respS; }
void lo_init(double fs, int os) { delete A; A = new Amp(); int o = (os == 1 || os == 2 || os == 4) ? os : 1; A->vin = 0.1; A->init(fs, o, Parts(), fromT()); }
int lo_latency() { return A ? A->latency() : 0; }
void lo_set_target() { if (!A) return; A->setTarget(fromT()); double vin = tgtS[11]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; }
void lo_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufS, n); else for (int i = 0; i < n; i++) bufS[i] = 0; }
// peakS: 0 V1B, 1 V2A (OVERDRIVE) bzw. V1A (NORMAL), 2 V3B, 3 V4B, 4 Phasenumkehr, 5/6 6L6GC Seite A/B (Gitterhub, 1 = Gitter erreicht die Kathode),
// 7 Spitzenspannung am Lautsprecher (V), 8 Volt bei 0 dBFS, 9 Leistung (Effektivspannung² / 8 Ω, W)
void lo_take_peaks() { if (!A) return; A->takePeaks(peakS); peakS[7] = A->takeOut(); peakS[8] = VFS; peakS[9] = A->takePow(); }
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void lo_plot_init(double fs) { delete P; P = new Core(); P->init(fs, Parts(), fromT()); P->settle(SETTLE); }
// n Frequenzen aus freqS → respS in dB (Lautsprecherspannung je Volt Eingang)
void lo_plot(int n) { if (!P || n < 0) return; if (n > 512) n = 512; P->apply(fromT());
  for (int i = 0; i < n; i++) respS[i] = std::isfinite(freqS[i]) && freqS[i] > 0 ? 20 * std::log10(std::abs(P->response(freqS[i])) + 1e-30) : -600; }
}
