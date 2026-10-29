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
struct YSetting { double t = 1, b = 1, m = 1, vol = 1, e[5] = {0.5,0.5,0.5,0.5,0.5};
  double vin = 0.1, ing = 1; bool pwr = false, spk = false; };   // vin: Spitzenpegel an der Buchse (V), ing: Eingangsteiler HIGH/LOW, pwr: Endstufe (MA-Board) mit absolutem Pegel, spk: Lautsprecher
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
// ---------- Endstufe (MA-Board) und Lautsprecher ----------
// Verstärkungskette, geeicht an Service Manual Tabelle 3 (0,775 mV an HIGH → 2,2 V am Lautsprecher, 69 dB bei Tabelle-1-Stellung):
// TR101 (Source-Stufe 2SK30A, 5,6 kΩ) 8,7-fach, PA-Board laut Netz (+11,8 dB bei Tabelle 1), MA-Board 1 + 820 kΩ/10 kΩ = 83-fach (38,4 dB).
// Begrenzung: Mittenspannung 45 V (Service Manual), nutzbarer Hub etwa ±42 V (100 W an 8 Ω = 40 V Spitze sauber, 140 W bei 10 % Klirr);
// weiches Knie ab 38 V. Ausgangselko 2200 µF an 8 Ω: Hochpass 9 Hz. Vollaussteuerung der Ausgabe = 48 V (Reserve für die Kippung des Ausgangselkos).
constexpr double G_TR101 = 8.7, G_MA = 83.0, V_KNEE = 38.0, V_LIM = 42.0, V_FS = 48.0;
inline double clipMA(double x) { double a = std::fabs(x); if (a <= V_KNEE) return x; double y = V_KNEE + (V_LIM - V_KNEE) * std::tanh((a - V_KNEE) / (V_LIM - V_KNEE)); return x < 0 ? -y : y; }
// Lautsprecher: Eminence Legend CB158 (Thiele-Small aus dem Datenblatt) im Bassreflexgehäuse der Portaflex-Größe (78 L, Abstimmung 50 Hz,
// Verlustgüte 7): akustisches Ersatzschaltbild → H(s) = s⁴Ts²Tb² / [(s²Ts²+sTs/Qts+1)(s²Tb²+sTb/Ql+1) + s²·α·Tb²], α = Vas/Vb.
// Oben: Schwingspuleninduktivität Le/Re (1. Ordnung, 3 kHz) und Membranaufbruch als 2. Ordnung bei 4 kHz (Q 0,7; Schätzung, Datenblatt: nutzbar bis 3 kHz).
struct SpkModel { double Fs = 34.2, Qts = 0.34, Vas = 336, Vb = 78, Fb = 50, Ql = 7, Re = 6.15, Le = 0.33e-3, fc = 4000, Qc = 0.7; };
// Bilineare Transformation eines reellen rationalen s-Polynoms (Koeffizienten aufsteigend, Grad ≤ 4) in z (absteigend, a[0] = 1)
inline void bilinear(const double *bs, const double *as, int n, double fs, double *bz, double *az) {
  double K = 2 * fs; double num[5] = {0,0,0,0,0}, den[5] = {0,0,0,0,0};
  // (1 - z^-1)^k (1 + z^-1)^(n-k) ausmultiplizieren
  for (int k = 0; k <= n; k++) { double poly[5] = {1,0,0,0,0}; int deg = 0;
    for (int i = 0; i < k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] -= poly[j-1]; deg++; }      // × (1 − z⁻¹)
    for (int i = 0; i < n - k; i++) { for (int j = deg + 1; j >= 1; j--) poly[j] += poly[j-1]; deg++; }  // × (1 + z⁻¹)
    double pk = std::pow(K, k); for (int j = 0; j <= n; j++) { num[j] += bs[k] * pk * poly[j]; den[j] += as[k] * pk * poly[j]; } }
  for (int j = 0; j <= n; j++) { bz[j] = num[j] / den[0]; az[j] = den[j] / den[0]; }
}
struct IIR { int n = 0; double b[5] = {0,0,0,0,0}, a[5] = {1,0,0,0,0}, z[5] = {0,0,0,0,0};
  double step(double x) { double y = b[0] * x + z[0]; for (int i = 1; i < n; i++) z[i-1] = b[i] * x + z[i] - a[i] * y; z[n-1] = b[n] * x - a[n] * y; return y; }
  void reset() { for (double &q : z) q = 0; } };
struct Speaker { IIR lo, le, cone; bool ready = false;
  void design(double fs, const SpkModel &m = SpkModel()) {
    double Ts = 1 / (2 * PI * m.Fs), Tb = 1 / (2 * PI * m.Fb), al = m.Vas / m.Vb;
    double bs[5] = {0, 0, 0, 0, Ts*Ts*Tb*Tb};
    double as[5] = {1, Ts/m.Qts + Tb/m.Ql, Ts*Ts + Tb*Tb + Ts*Tb/(m.Qts*m.Ql) + al*Tb*Tb, Ts*Ts*Tb/m.Ql + Tb*Tb*Ts/m.Qts, Ts*Ts*Tb*Tb};
    lo.n = 4; bilinear(bs, as, 4, fs, lo.b, lo.a);
    double tau = m.Le / m.Re; double b1[2] = {1, 0}, a1[2] = {1, tau}; le.n = 1; bilinear(b1, a1, 1, fs, le.b, le.a);
    double w0 = 2 * PI * m.fc; double b2[3] = {w0*w0, 0, 0}, a2[3] = {w0*w0, w0/m.Qc, 1}; cone.n = 2; bilinear(b2, a2, 2, fs, cone.b, cone.a);
    lo.reset(); le.reset(); cone.reset(); ready = true; }
  double step(double x) { return cone.step(le.step(lo.step(x))); } };
// Glättung alle 256 Abtastwerte mit Überblendung; Bauteiltausch sofort mit erhaltenen Kondensatorspannungen
struct YCore {
  double fs; LinRunner r; YSetting cur, tgt; YParts v; bool hasV = false; int cnt = 0;
  Speaker spk; IIR hpOut; double ingCur = 1, pkPre = 0; bool spkWas = false, pwrWas = false;   // Endstufe/Lautsprecher: Zustände; pkPre = Spitze am MA-Ausgang vor der Begrenzung (V)
  explicit YCore(double fs_) : fs(fs_) { spk.design(fs_); double b1[2] = {0, 2200e-6 * 8}, a1[2] = {1, 2200e-6 * 8}; hpOut.n = 1; bilinear(b1, a1, 1, fs_, hpOut.b, hpOut.a); }
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
  void block(float *buf, int n) { if (!hasV) { for (int i = 0; i < n; i++) buf[i] = 0; return; }
    const double ingT = tgt.ing, step = 1.0 / (0.02 * fs); const bool pwr = tgt.pwr, sp = tgt.spk;   // Eingangsteiler mit fester Flanke (1,0 je 20 ms)
    if (pwr != pwrWas) { hpOut.reset(); pwrWas = pwr; } if (sp != spkWas) { spk.lo.reset(); spk.le.reset(); spk.cone.reset(); spkWas = sp; }
    for (int i = 0; i < n; i++) { tick();
      if (ingCur < ingT) ingCur = std::min(ingT, ingCur + step); else if (ingCur > ingT) ingCur = std::max(ingT, ingCur - step);
      double y;
      if (pwr) { double vo = r.step(buf[i] * tgt.vin * ingCur * G_TR101) * G_MA; double a = std::fabs(vo); if (a > pkPre) pkPre = a; y = hpOut.step(clipMA(vo)) / V_FS; }
      else y = r.step(buf[i]) * ingCur;
      if (sp) y = spk.step(y);
      buf[i] = (float)-y; } }   // Polarität wie am Eingang (das PA-Board invertiert)
  double takePeak() { double p = pkPre; pkPre = 0; return p; }
};
} // namespace b100
