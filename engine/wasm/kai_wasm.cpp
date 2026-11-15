// C-Schnittstelle des Kaiserin-Reverb-Kerns für WebAssembly: Eingang mono, Hall stereo; Tonkurve und Anzeigetexte wie im Plugin
#include "kaiserin_core.hpp"
using namespace kai;
static Reverb *R = nullptr; static float inB[8192], lB[8192], rB[8192]; static double tgtB[16], freqB[512], respB[512]; static char txtB[256];
// tgtB: 0 MODE (0 HALL, 1 PLATE, 2 SPRING, 3 ROOM, 4 MOD, 5 SPARKLE), 1 DECAY, 2 MIX, 3 OUTPUT, 4 LOW, 5 HI, 6 THING 1, 7 THING 2 (0 … 1, 12 Uhr = 0,5), 8 HOLD (0/1)
static Setting fromT() { Setting s; auto k = [](double x) { return std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; };
  s.mode = std::isfinite(tgtB[0]) ? std::max(0, std::min(NMODE - 1, (int)std::lround(tgtB[0]))) : 0; s.decay = k(tgtB[1]); s.mix = k(tgtB[2]); s.out = k(tgtB[3]); s.low = k(tgtB[4]); s.hi = k(tgtB[5]);
  s.t1 = k(tgtB[6]); s.t2 = k(tgtB[7]); s.hold = std::isfinite(tgtB[8]) && tgtB[8] > 0.5; return s; }
static double fsB = 48000;
extern "C" {
float *kai_in_buffer() { return inB; } float *kai_l_buffer() { return lB; } float *kai_r_buffer() { return rB; } double *kai_target_buffer() { return tgtB; }
double *kai_freq_buffer() { return freqB; } double *kai_resp_buffer() { return respB; } char *kai_text_buffer() { return txtB; }
void kai_init(double fs) { delete R; R = nullptr; fsB = std::isfinite(fs) && fs >= 8000 && fs <= 384000 ? fs : 48000; R = new Reverb(); R->setup(fsB); }
void kai_reset() { if (R) R->reset(); }
void kai_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (R) R->process(inB, nullptr, lB, rB, n, fromT()); else for (int i = 0; i < n; i++) lB[i] = rB[i] = 0; }
// n Frequenzen aus freqB → respB (dB, auf 1 kHz bezogen); ref 1: dieselbe Stellung mit LOW/HI in der Mitte
void kai_plot(int n, int ref) { if (n < 0) return; if (n > 512) n = 512; Setting s = fromT(); if (ref) { s.low = 0.5; s.hi = 0.5; } toneCurve(s, fsB, freqB, n, respB); }
// Anzeigetext zu einem Knopf (0 DECAY … 6 THING 2) in txtB
void kai_describe(int knob) { describe(fromT(), std::max(0, std::min(6, knob)), txtB, (int)sizeof txtB); }
}
