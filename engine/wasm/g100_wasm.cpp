// C-Schnittstelle des Yamaha-G100-212II-Kerns für WebAssembly: Verstärker (Wiedergabe) und eine zweite Instanz nur für den Frequenzgang
#include "g100_core.hpp"
using namespace g100;
static Amp *A = nullptr; static Core *P = nullptr; static float bufB[8192]; static double tgtB[24], peakB[8], freqB[512], respB[512];
// tgtB = 0…11 Regler vola, treba, mida, bassa, gainb, masterb, trebb, midb, bassb, level, q, freq (0…1); 12 fata, 13 brighta, 14 fatb, 15 brightb, 16 Kanal B, 17 LOW-Buchse (Schalter);
// 18 vin (V Spitze bei 0 dBFS), 19 power (Endstufe begrenzen), 20 taper der A-Potis (0 log 15 %, 1 log 10 %, 2 linear), 21 reverb (0…1, ab 1.1), 22 Stärke der Nichtlinearität des Federtanks 0 … 10 (0 = linear wie 1.1; ab 1.4, 1.2/1.3: > 0,5 = linear)
static Setting fromT() { Setting s; for (int i = 0; i < NKNOB; i++) { double x = tgtB[i < 12 ? i : 21]; sfield(s, i) = std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : i < 12 ? 0.5 : 0.0; }   // Regler 0…11, REVERB aus tgtB[21]
  s.fata = tgtB[12] > 0.5; s.brighta = tgtB[13] > 0.5; s.fatb = tgtB[14] > 0.5; s.brightb = tgtB[15] > 0.5; s.chanB = tgtB[16] > 0.5; s.low = tgtB[17] > 0.5; double tp = tgtB[20]; s.taper = std::isfinite(tp) ? std::max(0, std::min(2, (int)std::lround(tp))) : 0; { double a = tgtB[22]; s.tankAmt = std::isfinite(a) ? std::min(10.0, std::max(0.0, a)) : 5; s.tankLin = s.tankAmt <= 0; } return s; }
static void switches() { double vin = tgtB[18]; A->vin = std::isfinite(vin) && vin > 0 ? vin : 0.1; A->power = !(tgtB[19] < 0.5); }
extern "C" {
float *g100_buffer() { return bufB; } double *g100_target_buffer() { return tgtB; } double *g100_peaks_buffer() { return peakB; } double *g100_freq_buffer() { return freqB; } double *g100_resp_buffer() { return respB; }
void g100_init(double fs, int os) { delete A; A = new Amp(); Parts p; int o = (os == 1 || os == 2 || os == 4) ? os : 1; switches(); A->init(fs, o, p, fromT()); }   // Schalter vor init: Rampen starten in der richtigen Stellung
int g100_latency() { return A ? A->latency() : 0; }
void g100_set_target() { if (!A) return; A->setTarget(fromT()); switches(); }
void g100_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (A) A->process(bufB, n); else for (int i = 0; i < n; i++) bufB[i] = 0; }
void g100_take_peaks() { if (!A) return; A->takePeaks(peakB); }   // IC1, Kanal A, Dioden B, 2SK30A, EQ (Anteil der Grenze), Endstufe (W an 4 Ω)
// Frequenzgang (Kleinsignal im Arbeitspunkt): eigene Instanz, einmal eingeschwungen
void g100_plot_init(double fs) { delete P; P = new Core(); Parts p; P->init(fs, p, fromT()); P->settle(0.5); }
// n Frequenzen aus freqB → respB in dB: Lautsprecherspannung je Volt Eingang (Vorstufe als Schaltung, Endstufe mit Ausgangselko)
void g100_plot(int n) { if (!P || n < 0) return; if (n > 512) n = 512; P->apply(fromT());
  for (int i = 0; i < n; i++) { double f = freqB[i]; if (!(std::isfinite(f) && f > 0)) { respB[i] = -600; continue; }
    double m = std::abs(P->response(f, O_G)) * std::abs(paResponse(f, P->v)); respB[i] = 20 * std::log10(m + 1e-30); } }
}
