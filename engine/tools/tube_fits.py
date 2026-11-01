# Röhrenmodelle (Koren-Form) an Datenpunkte angepasst bzw. geprüft: python3 engine/tools/tube_fits.py
# Triode: Ia = 2/kg1·(Ua/kp·ln(1 + exp(kp·(1/mu + Ug/√(kvb + Ua²)))))^ex   (core.hpp Tube, b15_core.hpp triode)
# Pentode (b15_core.hpp pentode): E1 = Ug2/kp·ln(1 + exp(kp·(1/mu + Ug1/Ug2))); Ia = 2·E1^ex/kg1·atan(Ua/kvb); Ig2 = 2·E1^ex/kg2·(1 + c2·(π/2 − atan(Ua/kvb)))
# Dempwolf/Zölzer (ab Laney 3.21 / AmpFett 1.6 für 12AX7 und 6SL7): Ia = G·(ln(1 + exp(C·(Ua/mu + Ug)))/C)^γ, γ = 1,26 wie bei Dempwolf gemessen
# Ergebnisse stehen in core.hpp (TUBE_MODELS[0]), laney_pa.hpp (T12AX7, PEL34), b15_core.hpp (T6SL7, P6L6) und docs/PROJEKTSTAND.md.
import math
import numpy as np
from scipy.optimize import brentq, least_squares

def tri(p, ug, ua, kvb=300):
    mu, ex, kg1, kp = p
    s = math.sqrt(kvb + ua * ua); q = kp * (1 / mu + ug / s); sp = math.log1p(math.exp(q)) if q < 35 else q
    return 2 * (ua / kp * sp) ** ex / kg1 * 1e3   # mA
def gm(p, ug, ua, d=1e-3): return (tri(p, ug + d, ua) - tri(p, ug - d, ua)) / 2 / d
def rp(p, ug, ua, d=1e-2): return 2 * d / (tri(p, ug, ua + d) - tri(p, ug, ua - d))   # kΩ
def stage(p, B, Ra, Rk):   # Arbeitspunkt einer Stufe mit Kathodenwiderstand (Gitter an Masse): Anoden- und Kathodenspannung
    f = lambda I: I - tri(p, -I * 1e-3 * Rk, B - I * 1e-3 * (Ra + Rk))
    I = brentq(f, 1e-6, 0.999 * B / (Ra + Rk) * 1e3); return B - I * 1e-3 * Ra, I * 1e-3 * Rk

# ---------- 12AX7 / ECC83 ----------
# RCA-Datenblatt: (Ua, Ug, Ia mA, gm mA/V, rp kΩ)
RCA = [(100, -1, 0.5, 1.25, 80), (250, -2, 1.2, 1.6, 62.5)]
# Laney AOR 50, Plan DRG 1054: (Stufe, Ub, Ra, Rk, Anode laut Plan, Kathode laut Plan)
LANEY = [('V101A', 240, 100e3, 2.7e3, 178, 1.6), ('V101B', 252, 100e3, 2.7e3, 183, 1.7), ('V102A', 252, 100e3, 10e3, 221, None),
         ('V102B', 260, 100e3, 820, 150, 1.0), ('V103B', 260, 100e3, 820, 142, 1.0)]
PI = [(189, -1.13, 1.23), (173, -1.13, 1.17)]   # Phasenumkehr V104 (Anodenwiderstände 82 k / 100 k an 326 V, Anoden 225 / 209 V)
def res12(q):
    p = [100] + list(q); r = []
    for n, B, Ra, Rk, ua, uk in LANEY: a, k = stage(p, B, Ra, Rk); r.append(math.log((B - a) / (B - ua)))   # Anodenstrom relativ
    for ua, ug, i, g, R in RCA: r += [math.log(tri(p, ug, ua) / i), math.log(gm(p, ug, ua) / g), math.log(rp(p, ug, ua) / R)]
    for ua, ug, i in PI: r.append(math.log(tri(p, ug, ua) / i))
    return r
def show12(name, p, head=None):
    print(f'{name}: ' + (head or f'mu {p[0]:.1f} ex {p[1]:.3f} kg1 {p[2]:.1f} kp {p[3]:.1f}') + f'   rms log-Fehler {np.sqrt(np.mean(np.square(res12(p[1:])))):.3f}')
    print('   Laney-Stufen  ' + '  '.join(f'{n} {stage(p, B, Ra, Rk)[0]:.0f} V ({ua})' for n, B, Ra, Rk, ua, uk in LANEY))
    print('   RCA           ' + '  '.join(f'{ua} V/{ug} V: {tri(p, ug, ua):.2f} mA ({i}), {gm(p, ug, ua):.2f} mA/V ({g}), {rp(p, ug, ua):.0f} k ({R})' for ua, ug, i, g, R in RCA))
print('12AX7 (mu 100 fest, ex >= 1,2):')
show12('  Koren 1996 (bis Laney 3.19)', [100, 1.4, 1060, 600])
f = least_squares(res12, [1.4, 1060, 600], bounds=([1.2, 300, 150], [1.6, 4000, 1500]))
show12('  angepasst (ab 3.20)        ', [100] + list(f.x))

def dw(p, ug, ua):   # Dempwolf/Zölzer; p = (G, C, mu, gamma)
    G, C, mu, ga = p; x = C * (ua / mu + ug); sp = math.log1p(math.exp(x)) if x < 35 else x
    return G * (sp / C) ** ga * 1e3
def with_model(fn, body):   # rechnet eine Auswertung mit einer anderen Kennlinie (tri wird kurz ersetzt)
    global tri; old = tri; tri = fn
    try: return body()
    finally: tri = old
fd = least_squares(lambda q: with_model(lambda p, ug, ua, kvb=300: dw((q[0], q[1], 100, 1.26), ug, ua), lambda: res12([0, 0, 0])), [2.2e-3, 3.4], bounds=([1e-4, 0.5], [1e-2, 20]))
GD, CD = fd.x
with_model(lambda p, ug, ua, kvb=300: dw((GD, CD, 100, 1.26), ug, ua), lambda: show12('  Dempwolf (ab 3.21)         ', [100, 0, 0, 0], 'G %.6e C %.5f mu 100 gamma 1,26' % (GD, CD)))

# ---------- 6SL7GT (AmpFett) ----------
B15 = [('V1A', 350, 470e3, 5.6e3, 165, 2.2), ('V1B', 350, 220e3, 2.2e3, 175, 2.0)]   # Technical Data 1968
p6 = [70, 1.4, 1851.8, 281.0]
def res6(q):
    p = (q[0], q[1], 70, 1.26); r = []
    for n, B, Ra, Rk, ua, uk in B15: a, k = with_model(lambda pp, ug, ua_, kvb=300: dw(p, ug, ua_), lambda: stage(None, B, Ra, Rk)); r.append(math.log((B - a) / (B - ua)))
    t = lambda pp, ug, ua_, kvb=300: dw(p, ug, ua_)
    r += with_model(t, lambda: [math.log(tri(None, -2, 250) / 2.3), math.log(gm(None, -2, 250) / 1.6), math.log(rp(None, -2, 250) / 44)])
    return r
f6 = least_squares(res6, [1e-3, 2.0], bounds=([1e-5, 0.3], [1e-1, 30])); p6d = (f6.x[0], f6.x[1], 70, 1.26)
t6 = lambda pp, ug, ua, kvb=300: dw(p6d, ug, ua)
print('\n6SL7GT Dempwolf (ab AmpFett 1.6): G %.6e C %.5f, rms-Fehler %.3f' % (f6.x[0], f6.x[1], np.sqrt(np.mean(np.square(res6(f6.x))))),
      with_model(t6, lambda: '250 V/−2 V: %.2f mA, %.2f mA/V, %.0f k; ' % (tri(None, -2, 250), gm(None, -2, 250), rp(None, -2, 250)) + '  '.join(f'{n} Anode {stage(None, B, Ra, Rk)[0]:.0f} V ({ua})' for n, B, Ra, Rk, ua, uk in B15)))
print('6SL7GT Koren (bis 1.5): 250 V/−2 V: %.2f mA (2,3), %.2f mA/V (1,6), %.0f k (44)' % (tri(p6, -2, 250), gm(p6, -2, 250), rp(p6, -2, 250)),
      '  ' + '  '.join(f'{n} Anode {stage(p6, B, Ra, Rk)[0]:.0f} V ({ua}), Kathode {stage(p6, B, Ra, Rk)[1]:.2f} V ({uk})' for n, B, Ra, Rk, ua, uk in B15))

# ---------- Pentoden ----------
def pen(P, g1, g2, ua):
    mu, ex, kg1, kg2, kp, kvb, c2 = P; q = kp * (1 / mu + g1 / g2); sp = math.log1p(math.exp(q)) if q < 35 else q; e1 = g2 / kp * sp; pw = e1 ** ex
    at = math.atan(max(ua, 0) / kvb); return 2 * pw / kg1 * at * 1e3, 2 * pw / kg2 * (1 + c2 * (math.pi / 2 - at)) * 1e3
EL34 = [11, 1.35, 650, 4200, 60, 24, 1.0]   # Koren
ia, ig2 = pen(EL34, -12.2, 250, 250); EL34[2] *= ia / 100; EL34[3] *= ig2 / 14.9   # Datenblatt: Ua = Ug2 = 250 V, Ug1 −12,2 V: 100 mA, 14,9 mA
g = (pen(EL34, -12.1, 250, 250)[0] - pen(EL34, -12.3, 250, 250)[0]) / 0.2
print('\nEL34: Koren mit kg1 %.1f, kg2 %.1f: 250/250/−12,2 V → %.1f mA (100), Ig2 %.1f mA (14,9), %.1f mA/V (11)' % (EL34[2], EL34[3], *pen(EL34, -12.2, 250, 250), g))
print('      Laney-Ruhe (Ua 400 V, Ug2 395 V, Ug1 −36 V): Ia %.1f mA, Ig2 %.1f mA, Verlustleistung %.1f W von 25 W' % (*pen(EL34, -36, 395, 400), pen(EL34, -36, 395, 400)[0] * 0.4))
L6 = [8.753, 1.358, 1499.6, 26600, 38.93, 54.50, 1.0]; L6K = [8.7, 1.35, 1460, 4500, 48, 12, 1.0]
for ug1 in (-14, -20):
    print('6L6GC 250/250/%d V: unser Modell %.1f mA, Koren %.1f mA' % (ug1, pen(L6, ug1, 250, 250)[0], pen(L6K, ug1, 250, 250)[0]))
