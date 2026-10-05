// C-Schnittstelle des Rechenkerns für WebAssembly (AudioWorklet)
#include "core.hpp"
using namespace aor;
static Engine *E = nullptr; static double fsG = 48000; static Target T; static bool haveT = false;
static float bufG[8192]; static double partsG[64], tgtG[32], peaksG[8];
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
void aor_init(double fs) { delete E; E = new Engine(fs); fsG = fs; haveT = false; }
// Ziel setzen: partsG = Bauteile in PARTS_LIST-Reihenfolge; tgtG = v1,v2,t,m,b,mv,pl,pres(mit Kennlinie),mid,top,bass,ch1,low,pa,vin,nl,os
void aor_set_target() {
  if (!E) return; Parts p; double *q = partsG; int i = 0;
#define X(a, b, c) p.a = q[i++];
  PARTS_LIST(X)
#undef X
  T.v = p; T.s.v1 = tgtG[0]; T.s.v2 = tgtG[1]; T.s.t = tgtG[2]; T.s.m = tgtG[3]; T.s.b = tgtG[4]; T.s.mv = tgtG[5]; T.s.pl = tgtG[6]; T.s.pres = tgtG[7];
  T.s.mid = tgtG[8]; T.s.top = tgtG[9]; T.s.bass = tgtG[10]; T.s.ch1 = tgtG[11]; T.s.low = tgtG[12]; T.pa = tgtG[13] > 0.5; T.vin = tgtG[14]; T.nl = tgtG[15] > 0.5; T.os = (int)tgtG[16];
  E->setTarget(T); haveT = true;
}
void aor_process(int n) { if (n < 0) return; if (n > 8192) n = 8192;   // Puffergrenze
  if (E && haveT) E->block(bufG, n); else for (int i = 0; i < n; i++) bufG[i] = 0; }
// Spitzenwerte der Gitter-Kathoden-Aussteuerung (relativ): linear p·vin/Bezug, Röhre p/|Vgk0|
void aor_take_peaks() { takePeaksFrom(E, peaksG); }
}
