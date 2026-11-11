// C-Schnittstelle des Wien-EQ-Kerns für WebAssembly: ein Kanal (Wiedergabe, mono wie die Seiten der Amps) und der Frequenzgang (analog, wie im Plugin)
#include "weq_core.hpp"
using namespace weq;
static Channel *C = nullptr; static float bufB[8192]; static double tgtB[24], freqB[512], respB[512];
// tgtB = 0 … 4 GAIN LOW, MID 1, MID 2, MID 3, HIGH (Knopf 0 … 1, Mitte 0,5 = 0 dB); 5 … 7 Q MID 1 … 3 (0 … 1); 8 … 10 FREQ MID 1 … 3 (0 … 1);
// 11 LOW-Tasten (0 keine = 40 Hz, 1 = 60, 2 = 100, 3 beide = 160); 12 HIGH-Tasten (0 keine = Kuhschwanz, 1 = 12k, 2 = 8k, 3 beide = 4k); 13 EQ ein; 14 Regelbereich (0 ±6, 1 ±12, 2 ±20 dB)
static Setting fromT() { Setting s; auto num = [](double x, double d) { return std::isfinite(x) ? x : d; };
  for (int i = 0; i < NB; i++) s.gain[i] = 2 * std::min(1.0, std::max(0.0, num(tgtB[i], 0.5))) - 1;
  for (int k = 0; k < 3; k++) { s.q[k] = std::min(1.0, std::max(0.0, num(tgtB[5 + k], 0.5))); s.f[k] = std::min(1.0, std::max(0.0, num(tgtB[8 + k], 0.5))); }
  s.lowSel = std::max(0, std::min(3, (int)std::lround(num(tgtB[11], 0)))); s.highSel = std::max(0, std::min(3, (int)std::lround(num(tgtB[12], 0))));
  s.on = !(num(tgtB[13], 1) < 0.5); s.range = std::max(0, std::min(2, (int)std::lround(num(tgtB[14], 1)))); return s; }
extern "C" {
float *weq_buffer() { return bufB; } double *weq_target_buffer() { return tgtB; } double *weq_freq_buffer() { return freqB; } double *weq_resp_buffer() { return respB; }
void weq_init(double fs) { delete C; C = new Channel(); C->setup(fs); C->snap(fromT()); }
int weq_latency() { return C ? C->latency() : 0; }
void weq_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (C) C->process(bufB, n, fromT()); else for (int i = 0; i < n; i++) bufB[i] = 0; }
// n Frequenzen aus freqB → respB in dB; band −1 = ganzer EQ, 0 … 4 = nur dieses Band (LOW, MID 1 … 3, HIGH)
void weq_plot(int n, int band) { if (n < 0) return; if (n > 512) n = 512; Setting s = fromT(); Band b[NB]; bands(s, b);
  for (int i = 0; i < n; i++) { double f = freqB[i]; if (!(std::isfinite(f) && f > 0)) { respB[i] = -600; continue; }
    std::complex<double> h = band < 0 ? response(f, s) : (s.on && band < NB ? bandS(std::complex<double>(0, 2 * PI * f), b[band]) : std::complex<double>(1, 0));
    respB[i] = 20 * std::log10(std::abs(h) + 1e-30); } }
}
