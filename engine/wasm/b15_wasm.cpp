// C-Schnittstelle des B-15NF-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "b15_core.hpp"
using namespace b15;
static Amp *A = nullptr; static Core *P = nullptr; static float bufB[8192]; static double tgtB[16], peakB[8], freqB[512], respB[512];
static Setting fromT() { Setting s; for (int i = 0; i < 6; i++) { double x = tgtB[i]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
  double j = tgtB[6]; s.jack = (std::isfinite(j) && j >= 0 && j <= 2) ? (int)j : 0; return s; }
extern "C" {
float *bf_buffer() { return bufB; } double *bf_target_buffer() { return tgtB; } double *bf_peaks_buffer() { return peakB; } double *bf_freq_buffer() { return freqB; } double *bf_resp_buffer() { return respB; }
// tgtB = vol1, treb1, bass1, vol2, treb2, bass2 (0…1), jack (0 Bass, 1 Guitar, 2 Inst.), vin (V Spitze bei 0 dBFS), spk, standby, power
void bf_init(double fs, int os) { delete A; A = new Amp(); Parts p; int o = (os == 1 || os == 2 || os == 4) ? os : 1; A->vin = 0.1;
  A->spk = tgtB[8] > 0.5; A->standby = tgtB[9] > 0.5; A->power = !(tgtB[10] < 0.5); A->init(fs, o, p, fromT()); }   // Schalter vor init: Rampen starten schon in der richtigen Stellung
int bf_latency() { return A ? A->latency() : 0; }
void bf_set_target() { if (!A) return; A->setTarget(fromT()); double vin = tgtB[7]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; A->spk = tgtB[8] > 0.5; A->standby = tgtB[9] > 0.5; A->power = !(tgtB[10] < 0.5); }
void bf_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void bf_take_peaks() { if (!A) return; A->takePeaks(peakB); peakB[5] = A->takeOut(); }
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void bf_plot_init(double fs) { delete P; P = new Core(); Parts p; P->init(fs, p, fromT()); P->settle(1.5); P->spkNorm = accNorm(p); }
// n Frequenzen aus freqB → respB in dB (Spannung am Lautsprecher je Volt Eingang; spk: Schalldruck, bei 1 kHz gleich)
void bf_plot(int n, int spk) { if (!P || n < 0) return; if (n > 512) n = 512; Setting s = fromT(); s.tap16 = false; P->apply(s); P->spkAc = spk != 0;
  for (int i = 0; i < n; i++) respB[i] = std::isfinite(freqB[i]) && freqB[i] > 0 ? 20 * std::log10(std::abs(P->response(freqB[i])) + 1e-30) : -600; }
}
