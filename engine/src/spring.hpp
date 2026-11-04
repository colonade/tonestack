// Federhall (Accutronics-Tank) als physikalisches Modell: dispersive Wellenleiter je Feder nach Abel/Berners/Costello/Smith (AES 2006) und
// Välimäki/Parker/Abel (JAES 2010): unterhalb der Übergangsfrequenz fC laufen Torsionswellen auf der Schraubenfeder dispersiv – tiefe
// Frequenzen kommen zuerst an, hohe später –, jedes Echo ist ein aufwärts gleitender Chirp, der mit jedem Umlauf länger wird. Nachgebildet mit
// einer Kette gestreckter Allpässe erster Ordnung (Streckung K legt fC = fs/(2K) fest) in einer Rückkopplungsschleife mit der Laufzeit Td der
// Feder, Verlusten (T60) und Tiefpass; oberhalb fC ein zweiter, kaum dispersiver Weg mit kürzerem Nachhall und geringerem Pegel.
// Eingang: Strom in der Treiberspule (A), Ausgang: Leerlaufspannung der Abnehmerspule (V). Alle Kennwerte sind Schätzungen (siehe Parts im Kern).
// Ab G100 1.2 nichtlinear (Tank::nl; nl = false rechnet Bit für Bit wie 1.1): der Zustand der schon schwingenden Feder ändert, wie ein neuer Ton wirkt.
//   1. Wandler: Treiberspule mit Kernsättigung (isat), Magnetperle im Luftspalt – je weiter sie gerade ausgelenkt ist, desto schwächer die Kopplung
//      (B(m) = 1 / (1 + (m/m0)²)), gleich an der Abnehmerspule (Leerlaufspannung ∝ B(Auslenkung)).
//   2. Feder: bei großer Verdrillung steifer (Wellen schneller: Schleifenlaufzeit bis kHard kürzer), stärker gedämpft (Schleifenverstärkung bis kDamp
//      kleiner) und begrenzt (weiche Grenze mLim der Verdrillung) – gesteuert von der Hüllkurve der Federbewegung.
//   3. Rückwirkung: die bewegten Perlen an der Treiberseite induzieren eine Spannung in die Treiberspule (als Quelle im Netz des Kerns). Der lineare Anteil
//      (Nenn-Kopplung) steckt schon in den Kennwerten des linearen Tanks (Nachhall, Impedanz); gerechnet wird nur die Abweichung durch die Auslenkung:
//      rMot · m · (B(m) − 1). Ausgelenkte Perlen dämpfen die Feder über die Spule weniger, und der Spulenstrom des nächsten Tons ändert sich.
//   Bewegung in „Strom-Äquivalent“ (A): die Wellengrößen des Modells, die bei kleinen Signalen dem Spulenstrom entsprechen.
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
namespace springrev {
struct Params {
  int nSprings = 2; double td[3] = {0.033, 0.037, 0.041};   // Laufzeiten je Feder (s)
  double t60 = 2.75;        // Nachhallzeit (s) bei tiefen Frequenzen
  double fC = 4200;         // Übergangsfrequenz (Hz): bis hierhin dispersiv (Chirps)
  int M = 100; double a1 = 0.6;   // Allpass-Kette: Stufen und Koeffizient (bestimmt die Chirp-Länge: bei a1 = 0,6 vom 0,25- bis 4-Fachen von K·M Abtastwerten)
  double fLoopLp = 6000, fLoopHp = 40;   // Verluste in der Schleife: Tiefpass (Höhen klingen schneller ab), Hochpass (kein Gleichanteil)
  double fInHp = 120;       // Wandler: unterhalb etwa 120 Hz überträgt der Tank kaum
  double hfLevel = 0.25, hfT60 = 1.2, hfM = 8, hfA = -0.35;   // Weg oberhalb fC: Pegel, Nachhall, kleine Allpass-Kette
  double gain = 16.0;       // Übertragung Spulenstrom → Leerlaufspannung (V/A), Effektivwert im Mittel über die Frequenz (geschätzt, siehe Kern)
  // Nichtlinear (ab 1.2, alles geschätzt): Typ-B-Eingang (150 Ω) Nennstrom etwa 6,5 mA eff. / 19 mA Spitze; Sättigung beim 8-Ω-Typ bei etwa dem Zehnfachen
  // des Nennstroms gemessen (sound-au.com, „Reverb Tanks“), auf 150 Ω umgerechnet etwa 80 mA Spitze
  double isat = 0.080;      // Kernsättigung der Treiberspule (A Spitze): Antrieb = isat·tanh(i/isat)
  double m0 = 0.019;        // Auslenkungsmaßstab der Magnetperlen und der Feder (A-Äquivalent): Kopplung 1/(1 + (m/m0)²); ab G100 1.3 / Laney 3.36 die Nennstrom-Spitze
                            // des B-Eingangs (19 mA), vorher 80 mA (Sättigungsbereich) – damit war der Unterschied zum linearen Tank bei normalem Spiel unhörbar
  double kHard = 0.03, kDamp = 0.10, mLim = 0.038;   // Feder: Laufzeit bis 3 % kürzer und Schleifenverstärkung bis 10 % kleiner bei Hüllkurve ≥ m0; weiche Grenze der Verdrillung (2·m0)
  double envAtt = 0.001, envRel = 0.020;   // Hüllkurve der Federbewegung: Anstieg 1 ms, Abfall 20 ms
  double rMot = 2;          // Rückwirkung auf die Treiberspule (Ω je A-Äquivalent, geschätzt): als volle Rückwirkung gerechnet verkürzte sie den Nachhall um 150–600 Hz
                            // um 3–4 dB nach 1–2 s (Prüfung im Kern); als Abweichung verlängert sie ihn höchstens um so viel (Perlen ganz ausgelenkt)
};
struct OnePole { double b = 1, a = 0, z = 0; void lp(double f, double fs) { double x = std::exp(-2 * 3.14159265358979323846 * f / fs); b = 1 - x; a = x; } double step(double u) { z = b * u + a * z; return z; } void reset() { z = 0; } };
struct Hp1 { double k = 0, x1 = 0, y1 = 0; void set(double f, double fs) { k = std::exp(-2 * 3.14159265358979323846 * f / fs); } double step(double u) { double y = k * (y1 + u - x1); x1 = u; y1 = y; return y; } void reset() { x1 = y1 = 0; } };
// Gestreckter Allpass erster Ordnung: y[n] = a·x[n] + x[n−K] − a·y[n−K]
struct SAP { int K = 1; double a = 0; std::vector<double> x, y; int i = 0;
  void init(int K_, double a_) { K = std::max(1, K_); a = a_; x.assign(K, 0.0); y.assign(K, 0.0); i = 0; }
  inline double step(double u) { double xo = x[i], yo = y[i]; double v = a * u + xo - a * yo; x[i] = u; y[i] = v; if (++i == K) i = 0; return v; }
  void reset() { std::fill(x.begin(), x.end(), 0.0); std::fill(y.begin(), y.end(), 0.0); i = 0; } };
struct Delay { std::vector<double> b; int n = 1, w = 0;
  void init(int len) { n = std::max(1, len); b.assign(n, 0.0); w = 0; }
  inline double read() const { return b[w]; }   // ältester Wert = n Abtastwerte zurück
  inline void write(double v) { b[w] = v; if (++w == n) w = 0; }
  inline double readFrac(double L) const { L = std::min((double)n, std::max(1.0, L)); int i = (int)L; double f = L - i;   // Wert von vor L Abtastwerten (linear interpoliert)
    int a = w - i; if (a < 0) a += n; double va = b[a == n ? 0 : a]; if (f == 0) return va; int c = a - 1; if (c < 0) c += n; return va + f * (b[c] - va); }
  void reset() { std::fill(b.begin(), b.end(), 0.0); w = 0; } };
struct Spring {
  std::vector<SAP> ap, aph; Delay dl, dlh; OnePole lp, lpIn; Hp1 hpLoop, hpIn, hpHf; double g = 0, gh = 0, hfLevel = 0;
  double env = 0, ea = 0, er = 0, back = 0, mDev = 0, m0 = 1, kHard = 0, kDamp = 0, mLim = 1; int dlN = 1;   // mDev: Abweichung der Rückwirkung (m·(B(m) − 1))   // nichtlinear: Hüllkurve, ihre Koeffizienten, zurücklaufende Welle an der Treiberseite
  void design(double fs, const Params &p, int k) {
    int K = std::max(1, (int)std::lround(fs / (2 * p.fC)));
    ap.assign(p.M, SAP()); for (auto &s : ap) s.init(K, p.a1);
    aph.assign((size_t)std::max(0, (int)p.hfM), SAP()); for (auto &s : aph) s.init(1, p.hfA);
    // Gruppenlaufzeit der Kette bei tiefen Frequenzen: M·K·(1−a)/(1+a); die Schleife bekommt so viel Verzögerung, dass der Chirp-Anfang nach Td kommt
    double tau0 = p.M * K * (1 - p.a1) / (1 + p.a1); double td = p.td[k];
    dl.init(std::max(8, (int)std::lround(td * fs - tau0))); dlh.init(std::max(8, (int)std::lround(td * fs - p.hfM * (1 - p.hfA) / (1 + p.hfA))));
    g = std::pow(10.0, -3 * td / p.t60); gh = std::pow(10.0, -3 * td / p.hfT60); hfLevel = p.hfLevel;
    lp.lp(p.fLoopLp, fs); lpIn.lp(p.fC, fs); hpLoop.set(p.fLoopHp, fs); hpIn.set(p.fInHp, fs); hpHf.set(p.fC, fs);
    dlN = dl.n; ea = std::exp(-1 / (p.envAtt * fs)); er = std::exp(-1 / (p.envRel * fs)); m0 = p.m0; kHard = p.kHard; kDamp = p.kDamp; mLim = p.mLim; env = 0; back = 0; mDev = 0; }
  // Abnehmer am anderen Ende der Feder: Ausgang ist das Signal nach einem Durchlauf (Allpass-Kette + Verzögerung), davon ein Teil zurück zum Anfang
  inline double step(double u) {
    double x = hpIn.step(u);
    double d = dl.read(); double s = lpIn.step(x) + g * hpLoop.step(lp.step(d)); for (auto &a : ap) s = a.step(s); dl.write(s);
    double dh = dlh.read(); double h = hpHf.step(x) + gh * dh; for (auto &a : aph) h = a.step(h); dlh.write(h);
    return d + hfLevel * dh; }
  static inline double coup(double m, double m0) { double r = m / m0; return 1 / (1 + r * r); }   // Kopplung einer Magnetperle bei Auslenkung m
  // nichtlinear: u = Antrieb nach der Kernsättigung (A); Kopplung an beiden Enden nach der Auslenkung der Perle, Steifigkeit/Dämpfung/Grenze nach der Hüllkurve
  inline double stepNl(double u) {
    double q = std::min(1.0, (env / m0) * (env / m0));   // Aussteuerung der Feder 0 … 1
    double d = dl.readFrac(dlN * (1 - kHard * q));   // steifer → schneller → kürzere Laufzeit
    double mIn = lpIn.z + back;   // Bewegung an der Treiberseite im letzten Abtastwert (eingespeist + zurückgelaufen)
    double cIn = coup(mIn, m0); mDev = mIn * (cIn - 1); double x = hpIn.step(u) * cIn;
    back = g * (1 - kDamp * q) * hpLoop.step(lp.step(d));
    double s = lpIn.step(x) + back; s = mLim * std::tanh(s / mLim); for (auto &a : ap) s = a.step(s); dl.write(s);
    double dh = dlh.read(); double h = hpHf.step(x) + gh * dh; for (auto &a : aph) h = a.step(h); dlh.write(h);
    double ad = std::fabs(d); env = ad > env ? ea * env + (1 - ea) * ad : er * env + (1 - er) * ad;
    return d * coup(d, m0) + hfLevel * dh; }
  void reset() { for (auto &a : ap) a.reset(); for (auto &a : aph) a.reset(); dl.reset(); dlh.reset(); lp.reset(); lpIn.reset(); hpLoop.reset(); hpIn.reset(); hpHf.reset(); env = 0; back = 0; mDev = 0; }
};
struct Tank { std::vector<Spring> sp; double scale = 1; Params p; bool nl = false; double vb = 0;   // nl: nichtlinear (ab 1.2); vb: Rückwirkungsspannung in die Treiberspule (V)
  // scale: Übertragung im Mittel über die Frequenz = p.gain; eine Schleife mit Verstärkung g hat den Effektivwert-Faktor 1/√(1−g²) (Kammfilter, über die Zähne gemittelt)
  void design(double fs, const Params &pp) { p = pp; sp.assign(std::max(1, std::min(3, pp.nSprings)), Spring()); double sum = 0;
    for (size_t k = 0; k < sp.size(); k++) { sp[k].design(fs, pp, (int)k); sum += 1 / std::sqrt(1 - sp[k].g * sp[k].g); } scale = pp.gain / sum; }
  inline double step(double i) { if (nl) return stepNl(i); double y = 0; for (auto &s : sp) y += s.step(i); return scale * y; }
  inline double stepNl(double i) { double u = p.isat * std::tanh(i / p.isat), y = 0, b = 0; for (auto &s : sp) { y += s.stepNl(u); b += s.mDev; } vb = p.rMot * b; return scale * y; }
  void reset() { for (auto &s : sp) s.reset(); vb = 0; }
};
} // namespace springrev
