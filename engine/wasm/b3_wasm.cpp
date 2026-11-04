// C-Schnittstelle des Yamaha-B100-III-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "b3_core.hpp"
using namespace b3;
static Amp *A = nullptr; static Core *P = nullptr; static float bufB[8192]; static double tgtB[24], peakB[8], freqB[512], respB[512];
// tgtB = 0…11 Regler vol, treb, mid, bass, lev1, q1, f1, pre1, lev2, q2, f2, pre2 (0…1, Mitte 0,5); 12 EQ 1 ein, 13 EQ 2 ein (Fußschalter);
// 14 vin (V Spitze bei 0 dBFS), 15 power (Endstufe begrenzen), 16 taper der A-Potis (0 log 15 %, 1 log 10 %, 2 linear)
static Setting fromT() { Setting s; for (int i = 0; i < NKNOB; i++) { double x = tgtB[i]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
  s.eq1 = !(tgtB[12] < 0.5); s.eq2 = !(tgtB[13] < 0.5); double tp = tgtB[16]; s.taper = std::isfinite(tp) ? std::max(0, std::min(2, (int)std::lround(tp))) : 0; return s; }
static void switches() { double vin = tgtB[14]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; A->power = !(tgtB[15] < 0.5); }
extern "C" {
float *b3_buffer() { return bufB; } double *b3_target_buffer() { return tgtB; } double *b3_peaks_buffer() { return peakB; } double *b3_freq_buffer() { return freqB; } double *b3_resp_buffer() { return respB; }
void b3_init(double fs, int os) { delete A; A = new Amp(); Parts p; int o = os == 4 ? 4 : 2; switches(); A->init(fs, o, p, fromT()); }   // nur 2x und 4x (Amp::init); Schalter vor init
int b3_latency() { return A ? A->latency() : 0; }
void b3_set_target() { if (!A) return; A->setTarget(fromT()); switches(); }
void b3_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void b3_take_peaks() { if (!A) return; A->takePeaks(peakB); }   // IC101 1/2, Vorstufe, PEQ 1, PEQ 2 (Anteil der Grenze), Endstufe (W an 8 Ω)
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void b3_plot_init(double fs) { delete P; P = new Core(); Parts p; P->init(fs, p, fromT()); P->settle(0.3); }
// n Frequenzen aus freqB → respB in dB: Lautsprecherspannung je Volt Eingang (Vorstufe als Schaltung, Endstufe als Filter)
void b3_plot(int n) { if (!P || n < 0) return; if (n > 512) n = 512; P->apply(fromT());
  for (int i = 0; i < n; i++) { double f = freqB[i]; if (!(std::isfinite(f) && f > 0)) { respB[i] = -600; continue; }
    double m = std::abs(P->response(f, O_G)) * std::abs(paResponse(f, P->v)); respB[i] = 20 * std::log10(m + 1e-30); } }
}
