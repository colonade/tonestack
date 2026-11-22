// C-Schnittstelle des Cultec-EQ-1A-Kerns für WebAssembly: ein Kanal (Seite mono wie die anderen), Frequenzgang (Kleinsignal wie im Plugin), Anzeigen.
#include "cultec_core.hpp"
using namespace cultec;
static Unit *U = nullptr; static Core *PC = nullptr; static float bufB[8192]; static double tgtB[16], freqB[512], respB[512], peakB[3];
// tgtB: 0 LOW BOOST 0 … 10, 1 LOW ATTEN, 2 CPS 0 … 3 (20/30/60/100 Hz), 3 HIGH BOOST, 4 BANDWIDTH 0 … 10 (schmal … breit), 5 KCS 0 … 6 (3 … 16 kHz),
// 6 HIGH ATTEN, 7 ATTEN SEL 0 … 2 (5/10/20 kHz), 8 IN (1) / OUT (0), 9 LEVEL dBu für 0 dBFS
static Setting fromT() { Setting s; auto num = [](double x, double d) { return std::isfinite(x) ? x : d; }; auto k10 = [&](double x) { return std::max(0.0, std::min(10.0, num(x, 0))) / 10; };
  s.lb = k10(tgtB[0]); s.la = k10(tgtB[1]); s.hb = k10(tgtB[3]); s.bw = std::max(0.0, std::min(10.0, num(tgtB[4], 5))) / 10; s.hc = k10(tgtB[6]);
  s.lf = std::max(0, std::min(NLF - 1, (int)std::lround(num(tgtB[2], 2)))); s.hf = std::max(0, std::min(NHF - 1, (int)std::lround(num(tgtB[5], 2)))); s.hcf = std::max(0, std::min(NHC - 1, (int)std::lround(num(tgtB[7], 1))));
  s.in = num(tgtB[8], 1) > 0.5; return s; }
static double vinT() { return dbuToVin(std::isfinite(tgtB[9]) ? std::max(0.0, std::min(30.0, tgtB[9])) : 18); }
extern "C" {
float *ct_buffer() { return bufB; } double *ct_target_buffer() { return tgtB; } double *ct_freq_buffer() { return freqB; } double *ct_resp_buffer() { return respB; } double *ct_peaks_buffer() { return peakB; }
// os: 1, 2 oder 4 (intern höchstens etwa 200 kHz)
void ct_init(double fs, int os) { delete U; U = new Unit(); U->vin = vinT(); U->init(fs, os, Parts(), fromT()); }
int ct_latency() { return Unit::LAT; }
void ct_set_target() { if (U) { U->vin = vinT(); U->setTarget(fromT()); } }
void ct_reset() { if (U) { U->c = U->c0; U->setTarget(fromT()); U->snap(); } }
void ct_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (U) U->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void ct_take_peaks() { if (U) U->takePeaks(peakB); else peakB[0] = peakB[1] = peakB[2] = 0; }   // Ausgang (V Spitze an 600 Ω), 12AU7, 12AX7 (Anteil der Vorspannung)
// n Frequenzen aus freqB → respB in dB gegen „alle Regler 0“ bei 1 kHz; ref = 1: Bezugskurve (alle Regler 0)
void ct_plot(int n, int ref) { if (n < 0) return; if (n > 512) n = 512;
  if (!PC) { PC = new Core(); PC->warp = false; PC->init(96000, Parts(), Setting()); PC->settle(0.2); }
  PC->cur = Setting(); double r0 = std::abs(PC->response(1000)); PC->cur = ref ? Setting() : fromT();
  for (int i = 0; i < n; i++) { double f = freqB[i]; respB[i] = std::isfinite(f) && f > 0 ? 20 * std::log10(std::abs(PC->response(f)) / r0 + 1e-30) : -600; } }
}
