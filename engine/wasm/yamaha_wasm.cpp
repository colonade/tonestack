// C-Schnittstelle des Yamaha-Kerns für WebAssembly
#include "yamaha_core.hpp"
using namespace b100;
static YCore *Y = nullptr; static float bufY[8192]; static double partsY[64], tgtY[16];
extern "C" {
float *y_buffer() { return bufY; } double *y_parts_buffer() { return partsY; } double *y_target_buffer() { return tgtY; }
int y_parts_count() { int n = 0;
#define X(a) n++;
  YPARTS_LIST(X)
#undef X
  return n; }
void y_init(double fs) { delete Y; Y = new YCore(fs); }
// tgtY = t, b, m, vol, e0..e4 (Anteile 0..1, Kennlinie schon angewandt), vin (V), ing, pwr, spk
void y_set_target() { if (!Y) return; YParts p; int i = 0;
#define X(a) p.a = partsY[i++];
  YPARTS_LIST(X)
#undef X
  YSetting s; s.t = tgtY[0]; s.b = tgtY[1]; s.m = tgtY[2]; s.vol = tgtY[3]; for (int k = 0; k < 5; k++) s.e[k] = tgtY[4 + k]; s.vin = tgtY[9]; s.ing = tgtY[10] > 0 ? tgtY[10] : 1; s.pwr = tgtY[11] > 0.5; s.spk = tgtY[12] > 0.5; Y->setTarget(p, s); }
double y_take_peak() { return Y ? Y->takePeak() : 0; }
void y_process(int n) { if (n < 0) return; if (n > 8192) n = 8192; if (Y) Y->block(bufY, n); else for (int i = 0; i < n; i++) bufY[i] = 0; }
}
