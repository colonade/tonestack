// Yamaha B100-115II: PA-Board (Klangregler, TR102, Volume, TR103/104, TR105 + fünf Gyrator-Bänder) als lineares Zeitbereichsnetz
#pragma once
#include "core.hpp"
#include "yparts_gen.h"
namespace b100 {
using namespace aor;
constexpr double VT = 0.02585;
struct YParts {
#define X(n) double n = 0;
  YPARTS_LIST(X)
#undef X
  bool operator==(const YParts &o) const { return std::memcmp(this, &o, sizeof(YParts)) == 0; }
};
struct YSetting { double t = 1, b = 1, m = 1, vol = 1, e[5] = {0.5,0.5,0.5,0.5,0.5}; };
inline double &yfield(YSetting &s, int i) { double *f[] = {&s.t, &s.b, &s.m, &s.vol, &s.e[0], &s.e[1], &s.e[2], &s.e[3], &s.e[4]}; return *f[i]; }
inline LinSys buildY(const YParts &v, const YSetting &s, double fs) {
  LinNet o; o.N("in");
  auto bjt = [&](const std::string &b, const std::string &c, const std::string &e, double ic) { o.bjt(b, c, e, ic / VT, v.hfe); };
  o.R("in","src",v.rs);
  o.RC("src","T",v.rt*(1-s.t),v.ct,fs);
  o.R("T","W",v.rt*s.t); o.R("src","A",v.rsl);
  o.C("A","W",v.cb,fs); o.C("A","M",v.cm,fs);
  o.R("W","M",v.rb*s.b); o.R("M","0",v.rm*s.m);
  o.RC("T","b2",v.rs2,v.cl,fs);
  o.R("b2","0",v.rbias + 1/(1/39e3 + 1/4.7e3));
  o.R("e2","0",v.re); o.R("c2","0",v.rc); o.C("b2","c2",v.ccb,fs);
  bjt("b2","c2","e2",1.40e-3);
  o.C("c2","vt",2.2e-6,fs);
  o.R("vt","vw",v.vol*(1-s.vol)); o.R("vw","0",v.vol*s.vol);
  o.RC("vw","b3",560,1e-6,fs);
  o.R("b3","0",100e3 + 1/(1/39e3 + 1/3.9e3));
  o.R("e3","0",3.3e3); o.R("c3","0",15e3); o.C("b3","c3",100e-12,fs);
  bjt("b3","c3","e3",0.588e-3);
  o.R("c3","b4",1e-6); o.R("e4","0",10e3); o.C("b4","e4",1000e-12,fs);
  bjt("b4","0","e4",2.06e-3);
  o.C("e4","x1",10e-6,fs); o.R("x1","0",100e3); o.C("x1","b5",3.3e-6,fs);
  o.R("b5","0",56e3 + 1/(1/33e3 + 1/10e3));
  o.R("e5","0",4.7e3); o.R("c5","0",4.7e3); o.C("b5","c5",100e-12,fs);
  bjt("b5","c5","e5",1.306e-3);
  o.C("c5","BA",47e-6,fs); o.C("e5","BB",47e-6,fs);
  const double cs[5] = {v.cs0, v.cs1, v.cs2, v.cs3, v.cs4}, cq[5] = {v.cq0, v.cq1, v.cq2, v.cq3, v.cq4};
  for (int k = 0; k < 5; k++) { std::string K = std::to_string(k); double p = s.e[k];
    o.R("BA","w"+K,std::max(v.vreq*(1-p),1.0)); o.R("w"+K,"BB",std::max(v.vreq*p,1.0));
    o.C("w"+K,"n"+K,cs[k],fs); o.R("n"+K,"ge"+K,v.rg); o.C("n"+K,"gb"+K,cq[k],fs);
    o.R("gb"+K,"0",110e3); o.C("gb"+K,"ge"+K,v.cg,fs); o.R("ge"+K,"0",10e3);
    bjt("gb"+K,"0","ge"+K,1.44e-3); }
  o.RC("c5","o",10e3,4.7e-6,fs); o.R("o","0",100e3);
  return o.compile("o", {});
}
// Glättung alle 256 Abtastwerte mit Überblendung; Bauteiltausch sofort mit erhaltenen Kondensatorspannungen
struct YCore {
  double fs; LinRunner r; YSetting cur, tgt; YParts v; bool hasV = false; int cnt = 0;
  explicit YCore(double fs_) : fs(fs_) {}
  void setTarget(const YParts &pv, const YSetting &s) {
    bool partsChanged = !hasV || !(pv == v); tgt = s;
    if (partsChanged) { bool had = hasV; std::vector<double> oldG = had ? r.sys.G2 : std::vector<double>();
      v = pv; if (!hasV) { cur = s; hasV = true; }
      r.set(buildY(v, cur, fs), 1, !had);
      if (had && oldG.size() == (size_t)r.sys.nc) { r.hasPrev = false; r.blend = 1; for (size_t j = 0; j < oldG.size(); j++) r.Ih[j] += (r.sys.G2[j] - oldG[j]) / 2 * r.vc[j]; } }
  }
  void tick() { if (!hasV || ++cnt < 256) return; cnt = 0; double a = 1 - std::exp(-256 / (0.015 * fs)); bool moved = false;
    for (int i = 0; i < 9; i++) { double &c = yfield(cur, i), d = yfield(tgt, i) - c; if (d == 0) continue; moved = true; if (std::fabs(d) > 1e-5) c += d * a; else c = yfield(tgt, i); }
    if (moved) r.set(buildY(v, cur, fs), 1, true); }
  void block(float *buf, int n) { if (!hasV) { for (int i = 0; i < n; i++) buf[i] = 0; return; } for (int i = 0; i < n; i++) { tick(); buf[i] = (float)r.step(buf[i]); } }
};
} // namespace b100
