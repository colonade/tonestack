// C-Schnittstelle des Viereck-34-Kerns für WebAssembly: ein Kanal (linker Kanal mit seiner BALANCE-Stellung; Seite mono wie die Amps),
// Frequenzgang (Kleinsignal, wie im Plugin) und Aussteuerung der beiden OPV.
#include "viereck_core.hpp"
using namespace viereck;
static Unit *U = nullptr; static float bufB[8192]; static double tgtB[16], freqB[512], respB[512], peakB[2];
// tgtB: 0 TILT −3 … +3 (+ heller), 1 stufenlos (0 = 7 Rasten wie Original, 1 = stufenlos), 2 Drehpunkt Hz (≤ 0: Original), 3 BASS −3 … +3
// (−3 Step 300 Hz, −2 Step 170, −1 Step 100, 0 aus, +1/+2/+3 Lift +3/+6/+9 dB), 4 Filter 0/1/2, 5 SLOPE, 6 BALANCE −1 … +1, 7 LEVEL dB re Nennausgang
static Setting fromT() { Setting s; auto num = [](double x, double d) { return std::isfinite(x) ? x : d; };
  s.tilt = std::max(-3.0, std::min(3.0, num(tgtB[0], 0))); s.stepped = num(tgtB[1], 0) < 0.5; double p = num(tgtB[2], 0); s.pivot = p > 0 ? std::max(80.0, std::min(4000.0, p)) : 0;
  s.bass = std::max(-3, std::min(3, (int)std::lround(num(tgtB[3], 0)))); s.filter = std::max(0, std::min(2, (int)std::lround(num(tgtB[4], 0)))); s.slope = num(tgtB[5], 0) > 0.5;
  s.balance = balPos(num(tgtB[6], 0), 0); return s; }
static double vinT() { double l = std::isfinite(tgtB[7]) ? std::max(-30.0, std::min(10.0, tgtB[7])) : -10; return std::pow(10, l / 20) * 0.5 * std::sqrt(2.0) / gain0(); }   // 0 dBFS-Sinus gibt bei flacher Einstellung LEVEL dB re 0,5 V eff. an OUTPUT
extern "C" {
float *vr_buffer() { return bufB; } double *vr_target_buffer() { return tgtB; } double *vr_freq_buffer() { return freqB; } double *vr_resp_buffer() { return respB; } double *vr_peaks_buffer() { return peakB; }
static double okFs(double fs) { return std::isfinite(fs) && fs >= 8000 && fs <= 384000 ? fs : 48000; }
void vr_init(double fs) { delete U; U = new Unit(); U->vin = vinT(); U->init(okFs(fs), Parts(), fromT()); }
void vr_set_target() { if (U) { U->vin = vinT(); U->setTarget(fromT()); } }
void vr_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (U) U->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void vr_take_peaks() { if (U) U->takePeaks(peakB); else peakB[0] = peakB[1] = 0; }
// n Frequenzen aus freqB → respB in dB (bezogen auf flach bei BALANCE in der Mitte, 1 kHz = 0 dB ohne Klangregelung); ref = 1: Bezugskurve (flach, ohne Filter)
void vr_plot(int n, int ref) { if (n < 0) return; if (n > 512) n = 512; Setting s = fromT(); if (ref) { Setting r; r.balance = s.balance; s = r; } Parts v; double g0 = gain0();
  for (int i = 0; i < n; i++) { double f = freqB[i]; respB[i] = std::isfinite(f) && f > 0 ? 20 * std::log10(std::abs(response(v, s, f)) / g0 + 1e-30) : -600; } }
}
