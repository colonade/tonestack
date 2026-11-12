#!/usr/bin/env python3
# Vergleicht die Röhrenmodelle der Rechenkerne mit Modellen, die an Kennlinienmessungen einzelner Röhren angepasst wurden.
#   python3 scripts/tube_compare.py
# Quelle der Messmodelle: TubeLib.inc von Derk Reefman (Fassung 30.7.2017; Röhren mit dem uTracer vermessen, Parameter mit ExtractModel angepasst;
# Formeln TriodeK und BTetrodeDE aus derselben Datei). Die Zahlen unten sind daraus abgeschrieben; die Datei selbst liegt nicht im Repository.
# Gemessen wurde nur bei negativem Gitter: über Gitterstrom und über den Bereich Ug > 0 sagen diese Modelle nichts.
# Unsere Modelle: Dempwolf-Form (Trioden), Derk-Form D (Endröhren), Parameter aus engine/src/core.hpp, b15_core.hpp, svt_core.hpp, laney_pa.hpp.
import math, numpy as np
from scipy.optimize import brentq
def kor(p, vg, va):                      # TriodeK: Ia = E1^ex / kg1
    mu, ex, kg1, kp, kvb = p; q = kp*(1/mu + vg/math.sqrt(kvb + va*va)); sp = math.log1p(math.exp(q)) if q < 35 else q
    e1 = va/kp*sp; return e1**ex/kg1 if e1 > 0 else 0.0
def dem(p, vg, va):                      # Dempwolf/Zölzer: Ia = G·(ln(1 + e^(C·(Ua/mu + Ug)))/C)^γ
    G, C, mu, ga = p; x = C*(va/mu + vg); sp = math.log1p(math.exp(x)) if x < 35 else x
    return G*(sp/C)**ga*(1 - math.exp(-max(va, 0)))
LIB = {'12AX7': (108.93, .988, 389.8, 677.7, 10751), '12AT7': (63.24, 1.338, 306.7, 208.5, 2363), '12AU7': (20.73, 1.350, 751.3, 85.3, 992),
       '12AY7': (47.59, 1.135, 565.6, 244.2, 3024), '6SL7': (76.12, 1.169, 587.3, 527.8, 5197)}
OUR = {'12AX7': (2.019059e-3, 2.02928, 100, 1.26), '12AT7': (4.684130e-3, 1.98586, 60, 1.09), '12AU7': (9.627442e-4, 0.350357, 19.74, 1.5),
       '12AY7': (1.395767e-3, 1.04151, 44, 1.26), '6SL7': (1.215197e-3, 1.66175, 70, 1.26)}
GR = {'12AX7': [0, -0.5, -1, -1.5, -2, -3], '12AT7': [0, -1, -2, -3, -4], '12AU7': [0, -2, -4, -8, -12], '12AY7': [0, -1, -2, -3, -4], '6SL7': [0, -0.5, -1, -2, -3]}
for t in LIB:
    print(f'== {t}: Anodenstrom mA bei Ua = 50 … 300 V, gemessene Röhre / unser Modell'); errs = []
    for vg in GR[t]:
        row = []
        for va in (50, 100, 150, 200, 250, 300):
            a, b = kor(LIB[t], vg, va)*1e3, dem(OUR[t], vg, va)*1e3; row.append(f'{a:6.2f}/{b:6.2f}')
            if a > 0.05: errs.append(math.log(b/a))
        print(f'  Ug {vg:5.1f} V: ' + '  '.join(row))
    print(f'  unser Modell im Mittel {100*(math.exp(np.mean(errs)) - 1):+.0f} %, Streuung (rms log) {np.std(errs):.2f}')
def stage(f, B, Ra, Rk):
    g = lambda I: I - f(-I*Rk, B - I*(Ra + Rk)); I = brentq(g, 1e-9, 0.999*B/(Ra + Rk)); return B - I*Ra
print('Laney-Vorstufen, Anodenspannung: Plan | gemessene ECC83 | unser Modell')
for n, B, Ra, Rk, ua in [('V101A', 240, 100e3, 2.7e3, 178), ('V101B', 252, 100e3, 2.7e3, 183), ('V102A', 252, 100e3, 10e3, 221), ('V102B', 260, 100e3, 820, 150), ('V103B', 260, 100e3, 820, 142)]:
    print(f'  {n}: {ua} | {stage(lambda g, a: kor(LIB["12AX7"], g, a), B, Ra, Rk):.0f} | {stage(lambda g, a: dem(OUR["12AX7"], g, a), B, Ra, Rk):.0f}')
# ---------- 6L6GC ----------
def lib6l6(g1, g2, va):                  # BTetrodeDE, Knie exp(−(β·Ua)^1,5), Sekundäremission
    MU, EX, kG1, KP, kVB, kG2, Sc, ap, w, nu, lam, O, Ao, al, be, als = 9.41, 1.306, 446.6, 45.2, 3205.1, 6672.5, .031, .017, -5, 3.08, 14.66, .209e-2, .11e-5, .209e-2, .069, 8.10
    e1 = g2/KP*math.log1p(math.exp(KP*(1/MU + g1/math.sqrt(kVB + g2*g2)))); ip = e1**EX; f = math.exp(-(be*va)**1.5)
    return ip*(O + Ao*va - al*f - Sc/kG2*va*(1 + math.tanh(-ap*(va - g2/lam + w + nu*g1)))), ip/kG2*(1 + als*f)
def our6l6(g1, g2, va):                  # Derk-Form D, Knie 1/(1 + β·Ua), ohne Sekundäremission (b15_core.hpp D6L6)
    mu, ex, kg1, kp, kvb, kg2, A, be, als = 8.281661842, 1.562959483, 1150.834177, 36.84207058, 300, 26080.39458, 4.751966359e-4, 0.06717976054, 10
    e1 = g2/kp*math.log1p(math.exp(kp*(1/mu + g1/math.sqrt(kvb + g2*g2)))); ip = e1**ex; f = 1/(1 + be*va)
    return ip*((1/kg1 - 1/kg2)*(1 - f) + A*va/kg1), ip*(1 + als*f)/kg2
for g2 in (250, 350, 430):
    print(f'== 6L6GC, Ug2 {g2} V: Ia mA gemessene Röhre / unser Modell (Ig2), Ua = 20, 50, 100, 200, 300, 430 V')
    for g1 in (0, -10, -20, -30, -40, -50):
        print(f'  Ug1 {g1:4d}:' + ''.join('  %5.0f/%5.0f (%3.0f/%3.0f)' % (lib6l6(g1, g2, va)[0]*1e3, our6l6(g1, g2, va)[0]*1e3, lib6l6(g1, g2, va)[1]*1e3, our6l6(g1, g2, va)[1]*1e3) for va in (20, 50, 100, 200, 300, 430)))
