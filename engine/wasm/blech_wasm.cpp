// C-Schnittstelle des Blech-140-Kerns für WebAssembly (blech_core.hpp): Eingang mono, Hall stereo.
// Zwei Rollen: (1) Hall rechnen (blech_init/process; neue Impulsantworten kommen über blech_offer von außen oder werden mit internal = 1 langsam selbst
// gerechnet), (2) Impulsantworten rechnen (blech_job_*, in einem eigenen Worker: etwa 0,7 s je Platte). Speicher wächst bei blech_init (etwa 45 MB).
#include "blech_core.hpp"
using namespace blech;
static Reverb *R = nullptr; static IRJob *J = nullptr; static float inB[8192], lB[8192], rB[8192]; static double tgtB[16]; static std::vector<float> offL, offR;
// tgtB: 0 DECAY (s bei 500 Hz, 1 … 3,8), 1 MIX (0 … 1), 2 OUTPUT (0 … 1, Mitte 0 dB, ±24 dB), 3 PRE-DELAY (0 … 1 → 0 … 200 ms quadratisch), 4 BASS CUT (0 … 3)
static Setting fromT() { Setting s; auto k = [](double x, double d) { return std::isfinite(x) ? std::min(1.0, std::max(0.0, x)) : d; };
  double t = std::isfinite(tgtB[0]) ? tgtB[0] : 2; s.decay = (std::min(T500HI, std::max(T500LO, t)) - T500LO) / (T500HI - T500LO); s.mix = k(tgtB[1], 0.3); s.out = k(tgtB[2], 0.5); s.pre = k(tgtB[3], 0);
  s.bass = std::isfinite(tgtB[4]) ? std::min(3.0, std::max(0.0, std::round(tgtB[4]))) : 0; return s; }
static double t01(double t) { t = std::isfinite(t) ? std::min(T500HI, std::max(T500LO, t)) : 2; return (t - T500LO) / (T500HI - T500LO); }
static double okFs(double fs) { return std::isfinite(fs) && fs >= 8000 && fs <= 384000 ? fs : 48000; }
extern "C" {
float *blech_in_buffer() { return inB; } float *blech_l_buffer() { return lB; } float *blech_r_buffer() { return rB; } double *blech_target_buffer() { return tgtB; }
// Hall einrichten: Abtastrate, DECAY (s) der ersten Platte; internal 1 = neue Platten selbst rechnen (langsam, ohne zweiten Worker)
void blech_init(double fs, double t500, int internal) { delete R; R = new Reverb(); R->internalJob = internal != 0; R->setup(okFs(fs), t01(t500)); offL.assign(R->job.L, 0.f); offR.assign(R->job.L, 0.f); }
void blech_reset() { if (R) R->reset(); }
void blech_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (R) R->process(inB, nullptr, lB, rB, n, fromT()); else for (int i = 0; i < n; i++) lB[i] = rB[i] = 0; }
int blech_ir_len() { return R ? R->job.L : 0; }
float *blech_offer_l() { return offL.data(); } float *blech_offer_r() { return offR.data(); }   // nach jedem blech_offer neu abfragen (Puffer werden getauscht)
void blech_offer(double t500) { if (R) R->offer(offL, offR, std::min(T500HI, std::max(T500LO, t500))); }
double blech_shown() { return R ? R->decayShown() : 2; }   // Nachhallzeit der eingebauten Platte (s bei 500 Hz; wandert beim Überblenden)
int blech_changing() { return R && R->changing() ? 1 : 0; }
void blech_set_internal(int on) { if (R) R->internalJob = on != 0; }   // ohne IR-Worker: neue Platten langsam selbst rechnen
double blech_fsi() { return R ? R->fsi : 0; }
// Impulsantwort für eine neue Platte (eigener Worker): fs = Abtastrate der Wiedergabe (intern fs/k wie im Hall)
void blech_job_init(double fs) { delete J; J = new IRJob(); fs = okFs(fs); J->setup(fs / std::min(16, factorOf(fs))); }
void blech_job_run(double t500) { if (J) { J->run(std::min(T500HI, std::max(T500LO, t500))); J->phase = IRJob::IDLE; } }
int blech_job_len() { return J ? J->L : 0; }
float *blech_job_l() { return J ? J->hL.data() : nullptr; } float *blech_job_r() { return J ? J->hR.data() : nullptr; }
// Modell für Anzeigen: Nachhallzeit bei f (Hz) für DECAY t500 (s bei 500 Hz)
double blech_t60(double f, double t500) { Plate p; return t60At(p, std::max(1.0, f), std::min(T500HI, std::max(T500LO, t500))); }
}
