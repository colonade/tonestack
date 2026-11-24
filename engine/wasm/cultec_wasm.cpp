// C-Schnittstelle des Cultec-EQ-1A-Kerns für WebAssembly: ein Kanal (Seite mono wie die anderen), Frequenzgang (Kleinsignal wie im Plugin), Anzeigen.
#include "cultec_core.hpp"
#include <cstring>
using namespace cultec;
static Unit *U = nullptr, *UO = nullptr; static long fadeN = 0, fadeLen = 1; static float oldB[8192]; static Core *PC[2] = {nullptr, nullptr}; static float bufB[8192]; static double tgtB[16], freqB[512], respB[512], peakB[3];
// tgtB: 0 LOW BOOST 0 … 10, 1 LOW ATTEN, 2 CPS 0 … 3 (20/30/60/100 Hz), 3 HIGH BOOST, 4 BANDWIDTH 0 … 10 (schmal … breit), 5 KCS 0 … 6 (3 … 16 kHz),
// 6 HIGH ATTEN, 7 ATTEN SEL 0 … 2 (5/10/20 kHz), 8 IN (1) / OUT (0), 9 LEVEL dBu für 0 dBFS; ab 1.1: 10 MODEL (0 EQP-1A, 1 EQP-1R),
// 11 CPS des EQP-1R 0 … 2 (30/60/100 Hz), 12 KCS des EQP-1R 0 … 4 (3/5/8/10/12 kHz); beim EQP-1R gelten 11/12 statt 2/5, ATTEN SEL entfällt
static int modelT() { return std::isfinite(tgtB[10]) && tgtB[10] > 0.5 ? 1 : 0; }
static int osReq = 2;
static Setting fromT() { Setting s; auto num = [](double x, double d) { return std::isfinite(x) ? x : d; }; auto k10 = [&](double x) { return std::max(0.0, std::min(10.0, num(x, 0))) / 10; };
  s.lb = k10(tgtB[0]); s.la = k10(tgtB[1]); s.hb = k10(tgtB[3]); s.bw = std::max(0.0, std::min(10.0, num(tgtB[4], 5))) / 10; s.hc = k10(tgtB[6]);
  s.lf = std::max(0, std::min(NLF - 1, (int)std::lround(num(tgtB[2], 2)))); s.hf = std::max(0, std::min(NHF - 1, (int)std::lround(num(tgtB[5], 2)))); s.hcf = std::max(0, std::min(NHC - 1, (int)std::lround(num(tgtB[7], 1))));
  if (modelT() == 1) { s.lf = std::max(0, std::min(NLF_R - 1, (int)std::lround(num(tgtB[11], 1)))); s.hf = std::max(0, std::min(NHF_R - 1, (int)std::lround(num(tgtB[12], 1)))); s.hcf = 0; }
  s.in = num(tgtB[8], 1) > 0.5; return s; }
static double vinT() { return dbuToVin(std::isfinite(tgtB[9]) ? std::max(0.0, std::min(30.0, tgtB[9])) : 18); }
extern "C" {
float *ct_buffer() { return bufB; } double *ct_target_buffer() { return tgtB; } double *ct_freq_buffer() { return freqB; } double *ct_resp_buffer() { return respB; } double *ct_peaks_buffer() { return peakB; }
// os: 1, 2 oder 4 (intern höchstens etwa 200 kHz)
static double okFs(double fs) { return std::isfinite(fs) && fs >= 8000 && fs <= 384000 ? fs : 48000; }
// Neubau bei laufendem Kern (andere Überabtastung, anderes Modell): der alte rechnet weiter, bis der neue nach seiner Latenz Ton gibt, dann 30 ms Überblendung
void ct_init(double fs, int os) { Unit *n = new Unit(); n->vin = vinT(); osReq = os; n->init(okFs(fs), os, partsFor(modelT()), fromT());
  if (U && U->ready) { delete UO; UO = U; fadeLen = std::max(1L, (long)(0.03 * n->fsB)); fadeN = fadeLen + Unit::LAT; } else { delete U; delete UO; UO = nullptr; } U = n; }
int ct_latency() { return Unit::LAT; }
void ct_set_target() { if (U && U->v.model != modelT()) { ct_init(U->fsB, osReq); return; } if (U) { U->vin = vinT(); U->setTarget(fromT()); } }   // anderes Modell: neu eingeschwungen
void ct_reset() { delete UO; UO = nullptr; if (U) { U->c = U->c0; U->setTarget(fromT()); U->snap(); } }
void ct_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (!U) { for (int i = 0; i < n; i++) bufB[i] = 0; return; }
  if (UO) std::memcpy(oldB, bufB, n * sizeof(float)); U->process(bufB, n);
  if (UO) { UO->process(oldB, n); for (int i = 0; i < n; i++) { double w = fadeN >= fadeLen ? 1 : fadeN > 0 ? std::sin(0.5 * PI * (double)fadeN / fadeLen) : 0, u = std::sqrt(std::max(0.0, 1 - w * w));
      if (fadeN > 0) fadeN--; bufB[i] = (float)(bufB[i] * u + oldB[i] * w); }
    if (fadeN <= 0) { delete UO; UO = nullptr; } } }
void ct_take_peaks() { if (UO) { double q[3]; UO->takePeaks(q); } if (U) U->takePeaks(peakB); else peakB[0] = peakB[1] = peakB[2] = 0; }   // Ausgang (V Spitze an 600 Ω), 12AU7, 12AX7 (Anteil der Vorspannung)
// n Frequenzen aus freqB → respB in dB gegen „alle Regler 0“ bei 1 kHz; ref = 1: Bezugskurve (alle Regler 0)
void ct_plot(int n, int ref) { if (n < 0) return; if (n > 512) n = 512;
  const int m = modelT(); Core *&pc = PC[m]; if (!pc) { pc = new Core(); pc->warp = false; pc->init(96000, partsFor(m), Setting()); pc->settle(0.2); }
  pc->cur = Setting(); double r0 = std::abs(pc->response(1000)); pc->cur = ref ? Setting() : fromT();
  for (int i = 0; i < n; i++) { double f = freqB[i]; respB[i] = std::isfinite(f) && f > 0 ? 20 * std::log10(std::abs(pc->response(f)) / r0 + 1e-30) : -600; } }
}
