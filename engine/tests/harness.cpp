// Testprogramm: harness in.f32 out.f32 fs "t:idx=val,...;t:..." (Parameterindizes wie im Plugin; 19 = Presence 0..10)
#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <sstream>
using namespace aor;
struct Ev { long t; int idx; double val; };
int main(int argc, char **argv) {
  if (argc < 4) { std::fprintf(stderr, "harness in.f32 out.f32 fs [\"t:idx=val,...\"]\n"); return 1; }
  double fs = atof(argv[3]); FILE *fi = fopen(argv[1], "rb"); fseek(fi, 0, SEEK_END); long n = ftell(fi) / 4; fseek(fi, 0, SEEK_SET);
  std::vector<float> x(n); if (fread(x.data(), 4, n, fi) != (size_t)n) return 2; fclose(fi);
  double sl[23] = {0,0,5,5,5,5,5,5,5,0,0,0,0,0,0,0,1,100,2,5,0,0,0};   // Grundstellung wie im Plugin (alles 5, Verzerrung an, 4-fach)
  std::vector<Ev> evs; std::string spec = argc > 4 ? argv[4] : ""; std::stringstream ss(spec); std::string part;
  while (std::getline(ss, part, ';')) { if (part.empty()) continue; size_t c = part.find(':'); long t = (long)(atof(part.substr(0, c).c_str()) * fs); std::stringstream p2(part.substr(c + 1)); std::string kv;
    while (std::getline(p2, kv, ',')) { size_t e = kv.find('='); evs.push_back({t, atoi(kv.substr(0, e).c_str()), atof(kv.substr(e + 1).c_str())}); } }
  Engine eng(fs);
  auto target = [&]() { Target t; t.v = sl[12] > 0.5 ? Parts::umbau() : Parts::orig(); t.taper = (int)sl[14];
    { int tp = t.taper; t.s.v1 = taper(sl[2]/10, tp); t.s.v2 = taper(sl[3]/10, tp); t.s.pl = taper(sl[4]/10, tp); t.s.t = taper(sl[5]/10, tp); t.s.m = sl[6]/10; t.s.b = taper(sl[7]/10, tp); t.s.mv = taper(sl[8]/10, tp); t.s.pres = sl[19]/10; }
    t.s.mid = sl[9] > 0.5; t.s.top = sl[10] > 0.5; t.s.bass = sl[11] > 0.5; t.s.ch1 = sl[1] < 0.5 ? 1 : 0; t.s.low = sl[0] > 0.5; t.pa = sl[13] > 0.5; t.nl = sl[16] > 0.5;
    t.vin = t.nl ? sl[17] / 1000 : 1; int o = (int)std::lround(sl[18]); t.os = o == 0 ? 1 : o == 1 ? 2 : o == 2 ? 4 : 8; for (int i = 0; i < 3; i++) t.s.tube[i] = std::max(0, std::min(NTUBES - 1, (int)std::lround(sl[20 + i]))); return t; };
  const int B = 128; std::vector<float> out(n); double outg = 1;
  for (long pos = 0; pos < n; pos += B) {
    bool ch = false; for (auto &e : evs) if (e.t >= pos && e.t < pos + B) { sl[e.idx] = e.val; ch = true; }
    if (pos == 0 || ch) { eng.setTarget(target()); outg = std::pow(10, sl[15] / 20); }
    int m = (int)std::min<long>(B, n - pos); std::vector<float> buf(x.begin() + pos, x.begin() + pos + m);
    eng.block(buf.data(), m); for (int i = 0; i < m; i++) out[pos + i] = (float)(buf[i] * outg);
  }
  FILE *fo = fopen(argv[2], "wb"); fwrite(out.data(), 4, n, fo); fclose(fo); return 0;
}
