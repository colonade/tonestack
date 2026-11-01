#!/usr/bin/env python3
# Ampeg B-15NF: Röhrenmodelle (Koren-Form) an Datenblattwerte angepasst; gibt die Parameter und die Abweichungen aus.
#   6SL7GT (RCA): Klasse A, je System: Ua 250 V, Ug -2 V, Ia 2,3 mA, mu 70, ri 44 kΩ, S 1,6 mA/V
#   6L6GC: Klasse-A1- und Gegentakt-Ruhepunkte aus dem Datenblatt (Ei, 6L6), siehe PTS
import math
from scipy.optimize import least_squares

def triode(vg, vp, mu, ex, kg1, kp, kvb):
    if vp <= 0: return 0.0
    q = kp * (1/mu + vg / math.sqrt(kvb + vp*vp))
    e1 = vp / kp * (q if q > 35 else math.log1p(math.exp(q)))
    return 2 * e1**ex / kg1 if e1 > 0 else 0.0

def fit6sl7():
    mu, ex, kvb = 70.0, 1.4, 300.0
    def res(p):
        kg1, kp = p
        i = triode(-2, 250, mu, ex, kg1, kp, kvb); d = 1e-3
        gm = (triode(-2+d, 250, mu, ex, kg1, kp, kvb) - triode(-2-d, 250, mu, ex, kg1, kp, kvb)) / (2*d)
        return [(i - 2.3e-3)/2.3e-3, (gm - 1.6e-3)/1.6e-3]
    r = least_squares(res, [1060, 600], bounds=([100, 50], [20000, 5000]))
    kg1, kp = r.x
    i = triode(-2, 250, mu, ex, kg1, kp, kvb); d = 1e-3
    gm = (triode(-1.999, 250, mu, ex, kg1, kp, kvb) - triode(-2.001, 250, mu, ex, kg1, kp, kvb)) / 2e-3
    ri = 2e-3 / (triode(-2, 251, mu, ex, kg1, kp, kvb) - triode(-2, 249, mu, ex, kg1, kp, kvb))
    print(f"6SL7: mu {mu} ex {ex} kg1 {kg1:.1f} kp {kp:.1f} kvb {kvb}  ->  Ia {i*1e3:.3f} mA, S {gm*1e3:.3f} mA/V, ri {ri/1e3:.1f} kΩ")
    return dict(mu=mu, ex=ex, kg1=kg1, kp=kp, kvb=kvb)

# Pentode (Koren 1996): Ia = E1^ex/kg1*(1+sgn E1)*atan(Ua/kvb), E1 = Ug2/kp*ln(1+exp(kp*(1/mu+Ug1/Ug2))); Ig2 = (Ug1+Ug2/mu)^1.5/kg2
def pentode(vg1, vg2, vp, mu, ex, kg1, kg2, kp, kvb):
    q = kp * (1/mu + vg1/vg2); e1 = vg2/kp * (q if q > 35 else math.log1p(math.exp(q)))
    ia = 2 * e1**ex / kg1 * math.atan(vp/kvb) if e1 > 0 else 0.0
    x = vg1 + vg2/mu; ig2 = x**1.5 / kg2 if x > 0 else 0.0
    return ia, ig2

# (Ua, Ug2, Ug1, Ia mA, Ig2 mA) je Röhre, Ruhewerte
PTS = [(250, 250, -14, 72, 5), (300, 200, -12.5, 48, 2.5), (350, 250, -18, 54, 2.5),
       (250, 250, -16, 60, 5), (270, 270, -17.5, 67, 5.5), (360, 270, -22.5, 44, 2.5), (360, 225, -18, 39, 1.75)]
def fit6l6():
    def res(p):
        mu, ex, kg1, kp, kvb, kg2 = p; out = []
        for vp, vg2, vg1, ia, ig2 in PTS:
            a, g = pentode(vg1, vg2, vp, mu, ex, kg1, kg2, kp, kvb)
            out += [(a*1e3 - ia)/ia, 0.5*(g*1e3 - ig2)/ig2]
        # Steilheit 6 mA/V und ri 22,5 kΩ bei 250/250/-14
        d = 0.01; a1, _ = pentode(-14+d, 250, 250, mu, ex, kg1, kg2, kp, kvb); a0, _ = pentode(-14-d, 250, 250, mu, ex, kg1, kg2, kp, kvb)
        out.append(((a1-a0)/(2*d)*1e3 - 6.0)/6.0)
        b1, _ = pentode(-14, 250, 251, mu, ex, kg1, kg2, kp, kvb); b0, _ = pentode(-14, 250, 249, mu, ex, kg1, kg2, kp, kvb)
        out.append(0.5*((2/(b1-b0))/1e3 - 22.5)/22.5)
        return out
    r = least_squares(res, [8.7, 1.35, 1460, 48, 12, 4500], bounds=([3, 1.0, 100, 5, 1, 500], [20, 2.0, 10000, 300, 100, 50000]))
    mu, ex, kg1, kp, kvb, kg2 = r.x
    print(f"6L6GC: mu {mu:.3f} ex {ex:.3f} kg1 {kg1:.1f} kg2 {kg2:.1f} kp {kp:.2f} kvb {kvb:.2f}")
    for vp, vg2, vg1, ia, ig2 in PTS:
        a, g = pentode(vg1, vg2, vp, *r.x[:3], kg2, kp, kvb)
        print(f"  {vp}/{vg2}/{vg1}: Ia {a*1e3:.1f} (Datenblatt {ia}) mA, Ig2 {g*1e3:.2f} ({ig2}) mA")
    a, g = pentode(-50, 425, 430, mu, ex, kg1, kg2, kp, kvb)
    print(f"  B-15NF Ruhepunkt 430/425/-50: Ia {a*1e3:.1f} mA, Ig2 {g*1e3:.2f} mA, Pa {a*430:.1f} W")
    return dict(mu=mu, ex=ex, kg1=kg1, kg2=kg2, kp=kp, kvb=kvb)

if __name__ == '__main__':
    fit6sl7(); fit6l6()
