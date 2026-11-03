// C-Schnittstelle des GK-800RB-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "gk_core.hpp"
using namespace gk;
static Amp *A = nullptr; static Core *P = nullptr; static float bufB[8192]; static double tgtB[24], peakB[8], freqB[512], respB[512];
// tgtB = 0…8 Regler vol, treb, himid, lomid, bass, boost, freq, hima, loma (0…1); 9 pad, 10 locut, 11 contour, 12 hiboost, 13 boostOn, 14 biamp (Schalter);
// 15 vin (V Spitze bei 0 dBFS), 16 spk, 17 power (Endstufen begrenzen), 18 direct (DIRECT OUT statt Lautsprecher)
static Setting fromT() { Setting s; for (int i = 0; i < NKNOB; i++) { double x = tgtB[i]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : 0.5; }
  s.pad = tgtB[9] > 0.5; s.locut = tgtB[10] > 0.5; s.contour = tgtB[11] > 0.5; s.hiboost = tgtB[12] > 0.5; s.boostOn = !(tgtB[13] < 0.5); s.biamp = tgtB[14] > 0.5; return s; }
static void switches() { double vin = tgtB[15]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; A->spk = tgtB[16] > 0.5; A->power = !(tgtB[17] < 0.5); A->direct = tgtB[18] > 0.5; }
extern "C" {
float *gk_buffer() { return bufB; } double *gk_target_buffer() { return tgtB; } double *gk_peaks_buffer() { return peakB; } double *gk_freq_buffer() { return freqB; } double *gk_resp_buffer() { return respB; }
void gk_init(double fs, int os) { delete A; A = new Amp(); Parts p; int o = (os == 1 || os == 2 || os == 4) ? os : 1; switches(); A->init(fs, o, p, fromT()); }   // Schalter vor init: Rampen starten in der richtigen Stellung
int gk_latency() { return A ? A->latency() : 0; }
void gk_set_target() { if (!A) return; A->setTarget(fromT()); switches(); }
void gk_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void gk_take_peaks() { if (!A) return; A->takePeaks(peakB); }   // U1a, EQ-Stufen (Anteil der OPV-Grenze), J113 (Anteil von 6 V Hub), 300-W- und 100-W-Endstufe (W an 4 bzw. 8 Ω)
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void gk_plot_init(double fs) { delete P; P = new Core(); Parts p; P->init(fs, p, fromT()); P->settle(0.5); }
// n Frequenzen aus freqB → respB in dB: Lautsprecherspannung je Volt Eingang (beide Endstufen, Summe), mit spk Schalldruck (bei 1 kHz gleich); direct: DIRECT OUT je Volt
void gk_plot(int n, int spk, int direct) { if (!P || n < 0) return; if (n > 512) n = 512; P->apply(fromT());
  for (int i = 0; i < n; i++) { double f = freqB[i]; if (!(std::isfinite(f) && f > 0)) { respB[i] = -600; continue; }
    double m = direct ? std::abs(P->response(f, O_DIRECT)) : std::abs(P->response(f, O_PA300) + P->response(f, O_PA100)) * std::abs(paResponse(f, P->v)) * (spk ? aor::spkMag(f) : 1);
    respB[i] = 20 * std::log10(m + 1e-30); } }
}
