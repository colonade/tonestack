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
  static YParts orig() { YParts p;   // Werte des Service Manuals (wie in der Web-Seite), für das Plugin
#define X(n, val) p.n = val;
    YPARTS_DEFAULTS(X)
#undef X
    return p; }
};
struct YSetting { double t = 1, b = 1, m = 1, vol = 1, e[5] = {0.5,0.5,0.5,0.5,0.5};
  double vin = 0.1, ing = 1; bool pwr = false, spk = false; };   // vin: Spitzenpegel an der Buchse (V), ing: Eingangsteiler HIGH/LOW, pwr: Endstufe (MA-Board) mit absolutem Pegel, spk: Lautsprecher
inline double &yfield(YSetting &s, int i) { double *f[] = {&s.t, &s.b, &s.m, &s.vol, &s.e[0], &s.e[1], &s.e[2], &s.e[3], &s.e[4]}; return *f[i]; }
inline LinNet netY(const YParts &v, const YSetting &s, double fs) {   // Netz des PA-Boards (Zeitbereich und, über responseAt, Frequenzgang)
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
    // G50K mit geerdeter Mittenanzapfung (Schaltplan): Bahn BA–Mitte–BB, Schleifer w auf der Hälfte, in der er steht; Mitte = Masse
    const double R = v.vreq;
    if (p >= 0.5) { o.R("BA","w"+K,std::max(R*(1-p),1.0)); o.R("w"+K,"0",std::max(R*(p-0.5),1.0)); o.R("0","BB",R/2); }
    else          { o.R("BA","0",R/2); o.R("0","w"+K,std::max(R*(0.5-p),1.0)); o.R("w"+K,"BB",std::max(R*p,1.0)); }
    o.C("w"+K,"n"+K,cs[k],fs); o.R("n"+K,"ge"+K,v.rg); o.C("n"+K,"gb"+K,cq[k],fs);
    o.R("gb"+K,"0",110e3); o.C("gb"+K,"ge"+K,v.cg,fs); o.R("ge"+K,"0",10e3);
    bjt("gb"+K,"0","ge"+K,1.44e-3); }
  o.RC("c5","po",10e3,4.7e-6,fs); o.R("po","0",100e3);
  o.R("po","0",10e3); o.C("po","0",3.3e-9,fs); o.C("po","o",1e-6,fs); o.R("o","0",100e3);   // Eingang MA-Board (Schaltplan): 10 kΩ und 3300 pF gegen Masse, 1 µF auf das Gate von TR301 (100 kΩ Vorspannung); o = Gate
  return o;
}
inline LinSys buildY(const YParts &v, const YSetting &s, double fs) { return netY(v, s, fs).compile("o", {}); }
// Eingangsteiler: HIGH 15 kΩ auf 330 kΩ (Bezug 1), LOW 47 kΩ auf 15 kΩ ∥ (1 kΩ + 330 kΩ): 0,245 = −12,2 dB (Service Manual Tabelle 3: 12 ± 3 dB)
constexpr double ING_LOW = (1 / (1 / 15e3 + 1 / 331e3)) / (47e3 + 1 / (1 / 15e3 + 1 / 331e3)) / (330e3 / 345e3);
// Kleinsignal-Betrag Eingang → Ausgang O (mit Eingangsteiler und Lautsprecher, ohne Endstufe), wie ENGINE.H der Web-Seite
inline double ymag(const LinNet &o, double f, const YSetting &s) { return std::abs(responseAt(o, f, "o")) * s.ing * (s.spk ? spkMag(f) : 1); }
// ---------- Endstufe (MA-Board) und Lautsprecher ----------
// Endstufe (MA-Board), nach Schaltplan: Eingang 10 kΩ und 3300 pF gegen Masse, 1 µF aufs Gate von TR301 (im Netz netY); nichtinvertierender Verstärker
// mit 1,8 MΩ Gegenkopplung gegen 100 kΩ ∥ (10 kΩ + 1 µF): ab etwa 16 Hz 1 + 1,8 MΩ/9,1 kΩ = 199-fach (46 dB), darunter fällt die Verstärkung auf 19.
// Eichung an Service Manual Tabelle 3 (0,775 mV an HIGH → 2,2 V am Lautsprecher, 69 dB bei Tabelle-1-Stellung): TR101 (Source-Stufe 2SK30A)
// mit 8,15-fach angesetzt (der FET-Ausgangswiderstand und Exemplarstreuung sind unbekannt; der Wert schließt die Kette auf 69 dB).
// Begrenzung: Mittenspannung 45 V (Service Manual), nutzbarer Hub etwa ±42 V (100 W an 8 Ω = 40 V Spitze sauber, 140 W bei 10 % Klirr);
// weiches Knie ab 38 V. Ausgangselko 2200 µF an 8 Ω: Hochpass 9 Hz. Vollaussteuerung der Ausgabe = 48 V (Reserve für die Kippung des Ausgangselkos).
constexpr double MA_RF = 1.8e6, MA_RB = 100e3, MA_RG = 10e3, MA_CG = 1e-6;
inline void maDesign(IIR &f, double fs) { const double g0 = 1 + MA_RF / MA_RB, tg = MA_RG * MA_CG;
  double b1[2] = {g0, g0 * tg + MA_RF * MA_CG}, a1[2] = {1, tg}; f.n = 1; bilinear(b1, a1, 1, fs, f.b, f.a); }
constexpr double G_TR101 = 8.15, V_KNEE = 38.0, V_LIM = 42.0, V_FS = 48.0;
inline double clipMA(double x) { double a = std::fabs(x); if (a <= V_KNEE) return x; double y = V_KNEE + (V_LIM - V_KNEE) * std::tanh((a - V_KNEE) / (V_LIM - V_KNEE)); return x < 0 ? -y : y; }
// Glättung alle 256 Abtastwerte mit Überblendung; Bauteiltausch sofort mit erhaltenen Kondensatorspannungen
struct YCore {
  double fs; LinRunner r; YSetting cur, tgt; YParts v; bool hasV = false; int cnt = 0;
  Speaker spk; IIR hpOut, ma; double ingCur = 1, pkPre = 0, pwrMix = -1, spkMix = -1, vinCur = -1;   // pwrMix/spkMix: Anteil von Endstufe/Lautsprecher, gleiten in 20 ms (kein Knacken beim Schalten); −1 = noch nicht gesetzt   // Endstufe/Lautsprecher: Zustände; pkPre = Spitze am MA-Ausgang vor der Begrenzung (V)
  explicit YCore(double fs_) : fs(fs_) { spk.design(fs_); double b1[2] = {0, 2200e-6 * 8}, a1[2] = {1, 2200e-6 * 8}; hpOut.n = 1; bilinear(b1, a1, 1, fs_, hpOut.b, hpOut.a); maDesign(ma, fs_); }
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
    const double ingT = tgt.ing, step = 1.0 / (0.02 * fs); const double pwrT = tgt.pwr ? 1 : 0, spkT = tgt.spk ? 1 : 0;   // Eingangsteiler mit fester Flanke (1,0 je 20 ms)
    if (pwrMix < 0) pwrMix = pwrT; if (spkMix < 0) spkMix = spkT;
    if (vinCur < 0) vinCur = tgt.vin; const double kv = 1 - std::exp(-1.0 / (0.02 * fs));   // Instrumentenpegel gleitet in 20 ms (kein Sprung im Endstufenpfad)
    for (int i = 0; i < n; i++) { tick(); if (!std::isfinite(buf[i])) buf[i] = 0;
      if (ingCur < ingT) ingCur = std::min(ingT, ingCur + step); else if (ingCur > ingT) ingCur = std::max(ingT, ingCur - step);
      if (pwrMix < pwrT) pwrMix = std::min(pwrT, pwrMix + step); else if (pwrMix > pwrT) pwrMix = std::max(pwrT, pwrMix - step);
      if (spkMix < spkT) spkMix = std::min(spkT, spkMix + step); else if (spkMix > spkT) spkMix = std::max(spkT, spkMix - step);
      vinCur += (tgt.vin - vinCur) * kv; double lin = r.step(buf[i]) * ingCur, y = lin;
      double vo = ma.step(lin * vinCur * G_TR101);   // linear: Ausgang · Pegel · TR101 · MA-Board, dann Begrenzung (MA-Filter läuft immer mit)
      if (pwrMix > 0) { double a = std::fabs(vo); if (a > pkPre) pkPre = a; double yp = hpOut.step(clipMA(vo)) / V_FS; y = pwrMix >= 1 ? yp : lin + pwrMix * (yp - lin); }
      else hpOut.step(0);   // Ausgangselko ohne Endstufe in Ruhe halten
      { double ys = spk.step(y); if (spkMix >= 1) y = ys; else if (spkMix > 0) y += spkMix * (ys - y); }   // Lautsprecherfilter läuft immer mit
      if (!std::isfinite(y)) { recover(); for (; i < n; i++) buf[i] = 0; return; }
      buf[i] = (float)-y; } }   // Polarität wie am Eingang (das PA-Board invertiert)
  // Neubeginn aus der Ruhe: Regler sofort am Ziel (kein Gleiten), alle Zustände leer (lineares Netz: Ruhe = 0)
  void restart() { if (!hasV) return; cur = tgt; r.set(buildY(v, cur, fs), 1, false); recover(); ingCur = tgt.ing; }
  void recover() { std::fill(r.Ih.begin(), r.Ih.end(), 0.0); std::fill(r.vc.begin(), r.vc.end(), 0.0); r.hasPrev = false; r.blend = 1; hpOut.reset(); ma.reset(); spk.lo.reset(); spk.le.reset(); spk.cone.reset(); pkPre = 0; pwrMix = spkMix = vinCur = -1; }
  double takePeak() { double p = pkPre; pkPre = 0; return p; }
};
} // namespace b100
