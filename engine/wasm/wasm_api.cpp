// C-Schnittstelle des Rechenkerns für WebAssembly (AudioWorklet)
#define AOR_REVERB 1   // Federhall im Effektweg (ab 3.35, siehe laney_rev.hpp)
#include "core.hpp"
using namespace aor;
static Engine *E = nullptr; static double fsG = 48000; static Target T; static bool haveT = false;
static float bufG[8192]; static double partsG[64], tgtG[32], peaksG[8], ssG[24];
extern "C" {
float *aor_buffer() { return bufG; }
double *aor_parts_buffer() { return partsG; }
double *aor_target_buffer() { return tgtG; }
double *aor_peaks_buffer() { return peaksG; }
int aor_parts_count() { int n = 0;
#define X(a, b, c) n++;
  PARTS_LIST(X)
#undef X
  return n; }
void aor_init(double fs) { delete E; E = new Engine(fs); E->pad = 31; fsG = fs; haveT = false; }   // feste Latenz 31 in jedem Modus, wie im Plugin
int aor_latency() { return 31; }
// Ziel setzen: partsG = Bauteile in PARTS_LIST-Reihenfolge; tgtG = v1,v2,t,m,b,mv,pl,pres(mit Kennlinie),mid,top,bass,ch1,low,pa,vin,nl,os,tube1,tube2,tube3,spk,mis,eco,rev,tankLin (25 Werte; mis = Streuung der Endröhren 0…0,1; eco = Endstufe bei 2x mit Grundrate; rev = REVERB 0…1 mit Kennlinie, ab 3.35; tank = Stärke des Federtanks 0…10, 0 = linear, ab 3.37)
void aor_set_target() {
  if (!E) return; Parts p; double *q = partsG; int i = 0;
  const Parts d = Parts::orig();
#define X(a, b, c) p.a = std::isfinite(q[i]) && q[i] > 0 ? q[i] : d.a; i++;   // unsinnige Werte (NaN, ∞, ≤ 0) → Gerätewert
  PARTS_LIST(X)
#undef X
  T.v = p; T.s.v1 = tgtG[0]; T.s.v2 = tgtG[1]; T.s.t = tgtG[2]; T.s.m = tgtG[3]; T.s.b = tgtG[4]; T.s.mv = tgtG[5]; T.s.pl = tgtG[6]; T.s.pres = tgtG[7];
  T.s.mid = tgtG[8]; T.s.top = tgtG[9]; T.s.bass = tgtG[10]; T.s.ch1 = tgtG[11]; T.s.low = tgtG[12]; T.pa = tgtG[13] > 0.5; T.vin = tgtG[14]; T.nl = tgtG[15] > 0.5; { int o = std::isfinite(tgtG[16]) ? (int)tgtG[16] : 2; T.os = (o == 1 || o == 2 || o == 4 || o == 8) ? o : 2; } for (int i = 0; i < 3; i++) { int t = (int)tgtG[17 + i]; T.s.tube[i] = (t >= 0 && t < NTUBES) ? t : 0; } T.spk = tgtG[20] > 0.5; T.mis = std::isfinite(tgtG[21]) ? std::min(0.1, std::max(0.0, tgtG[21])) : 0; T.eco = tgtG[22] > 0.5; T.rev = std::isfinite(tgtG[23]) ? std::min(1.0, std::max(0.0, tgtG[23])) : 0; T.tankAmt = std::isfinite(tgtG[24]) ? std::min(10.0, std::max(0.0, tgtG[24])) : 5; T.tankLin = T.tankAmt <= 0;   /* ab 3.35: REVERB (mit Kennlinie), Federtank linear */ T.s.late = p.rpad > 0 ? 1 : 0; T.v.rpad = 2.2e6; { const Parts O = Parts::orig(), U = Parts::umbau(); auto eq = [](double a, double b) { return std::fabs(a - b) <= 1e-6 * std::fabs(b); };   // Bass-Umbau und „Sanft“ der Seite als gleitende Schalter (wie Plugin 3.30), nur bei Plan- bzw. Umbauwerten
    bool soft = T.v.c3 <= 1.0001e-9, bm = eq(T.v.c2, U.c2) && eq(T.v.c14, U.c14) && (soft || eq(T.v.c3, U.c3)), bo = eq(T.v.c2, O.c2) && eq(T.v.c14, O.c14) && (soft || eq(T.v.c3, O.c3));
    if (bm || bo) { T.s.bmod = bm ? 1 : 0; T.s.soft = soft ? 1 : 0; T.v.c2 = O.c2; T.v.c14 = O.c14; T.v.c3 = O.c3; } }   // „Später“ ab 3.29 gleitend (Setting.late) statt Bauteilwechsel
  E->setTarget(T); haveT = true;
}
void aor_process(int n) { if (n < 0) return; if (n > 8192) n = 8192;   // Puffergrenze
  if (E && haveT) E->block(bufG, n); else for (int i = 0; i < n; i++) bufG[i] = 0; }
// Spitzenwerte der Gitter-Kathoden-Aussteuerung (relativ): linear p·vin/Bezug, Röhre p/|Vgk0|
void aor_take_peaks() { takePeaksFrom(E, peaksG); }
double *aor_ss_buffer() { return ssG; }
// Kleinsignalwerte der sechs Triodensysteme für die aktuelle Bestückung (nach aor_set_target): gm, rp, cgk, cga je System
void aor_small_signal() { if (!haveT) return; auto r = smallSignal(T.v, T.s, fsG); for (int i = 0; i < 6; i++) { ssG[4*i] = r[i].gm; ssG[4*i+1] = r[i].rp; ssG[4*i+2] = r[i].cgk; ssG[4*i+3] = r[i].cga; } }
}
