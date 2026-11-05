# Röhrenmodelle (Koren-Form) an Datenpunkte angepasst bzw. geprüft: python3 engine/tools/tube_fits.py
# Triode: Ia = 2/kg1·(Ua/kp·ln(1 + exp(kp·(1/mu + Ug/√(kvb + Ua²)))))^ex   (core.hpp Tube, b15_core.hpp triode)
# Pentode (b15_core.hpp pentode): E1 = Ug2/kp·ln(1 + exp(kp·(1/mu + Ug1/Ug2))); Ia = 2·E1^ex/kg1·atan(Ua/kvb); Ig2 = 2·E1^ex/kg2·(1 + c2·(π/2 − atan(Ua/kvb)))
# Dempwolf/Zölzer (ab Laney 3.21 / AmpFett 1.6 für 12AX7 und 6SL7): Ia = G·(ln(1 + exp(C·(Ua/mu + Ug)))/C)^γ, γ = 1,26 wie bei Dempwolf gemessen
# Ab Laney 3.25 / AmpFett 1.10: alle Vorstufentypen in der Dempwolf-Form; Endröhren in der Derk-Form nach Reefman (EM4 Theory, 2016):
#   Ig2 = sg·I_P/kg2·(1 + αs·f + Ps), Ia = sa·I_P·((1/kg1 − 1/kg2)·(1 − f) + A·Ua/kg1 − Ps/kg2), f = 1/(1 + β·Ua), I_P = E1^ex (b15_core.hpp pentodeD)
# Ergebnisse stehen in core.hpp (TUBE_MODELS), laney_pa.hpp (T12AX7, DEL34), b15_core.hpp (T6SL7, D6L6) und docs/PROJEKTSTAND.md.
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

# ---------- Vorstufentypen in der Dempwolf-Form (ab Laney 3.25) ----------
# Datenblattpunkte (Ua, Ug, Ia mA, gm mA/V, rp kΩ); γ 1,26 wie bei Dempwolf, bei 12AT7/12AU7 frei (zwei Punkte), mu bei 12AU7 frei
TYP = {'12AT7': (60, True, [(250, -2, 10, 5.5, 10.9), (100, -1, 3.7, 4.0, 15)], [60, 1.35, 460, 300]),
       '12AU7': (None, True, [(250, -8.5, 10.5, 2.2, 7.7), (100, 0, 11.8, 3.1, 6.25)], [21.5, 1.3, 1180, 84]),
       '5751':  (70, False, [(250, -3, 1.0, 1.2, 58)], [70, 1.17, 1420, 377]),
       '12AY7': (44, False, [(250, -4, 3.0, 1.75, 25)], [44, 1.7, 3781, 206])}
def dwm(G, C, mu, ga): return lambda p, ug, ua, kvb=300: dw((G, C, mu, ga), ug, ua) * (1 - math.exp(-ua))
def pts_res(fn, pts): return with_model(fn, lambda: sum(([math.log(tri(None, ug, ua) / i), math.log(gm(None, ug, ua) / g), math.log(rp(None, ug, ua) / R)] for ua, ug, i, g, R in pts), []))
print('\nVorstufentypen, Dempwolf-Form (ab 3.25) gegen Koren (bis 3.24):')
for n, (mu0, gfree, pts, kp) in TYP.items():
    def unpack(q):
        G, C = q[0], q[1]; k = 2; ga = 1.26; mu = mu0
        if gfree: ga = q[k]; k += 1
        if mu0 is None: mu = q[k]
        return G, C, mu, ga
    x0 = [2e-3, 1.0] + ([1.3] if gfree else []) + ([19] if mu0 is None else []); lo = [1e-6, 0.1] + ([1.0] if gfree else []) + ([10] if mu0 is None else [])
    hi = [1, 30] + ([1.5] if gfree else []) + ([30] if mu0 is None else [])
    f = least_squares(lambda q: pts_res(dwm(*unpack(q)), pts), x0, bounds=(lo, hi)); G, C, mu, ga = unpack(f.x)
    print(f'  {n}: G {G:.6e} C {C:.6g} mu {mu:.4g} γ {ga:.3g}, rms-Fehler {np.sqrt(np.mean(np.square(f.fun))):.3f}')
    for ua, ug, i, g, R in pts:
        fn = dwm(G, C, mu, ga); print(f'     {ua} V/{ug} V: ' + with_model(fn, lambda: f'{tri(None, ug, ua):.2f} mA, {gm(None, ug, ua):.2f} mA/V, {rp(None, ug, ua):.1f} k') +
                                      f'  Koren {tri(kp, ug, ua):.2f} mA, {gm(kp, ug, ua):.2f} mA/V, {rp(kp, ug, ua):.1f} k  (Datenblatt {i}, {g}, {R})')

# ---------- Endröhren in der Derk-Form (ab Laney 3.25 / AmpFett 1.10) ----------
def derk(P, g1, g2, va):   # P = (mu, ex, kg1, kp, kvb, kg2, A, β, αs, S, ap, w, ν, λ, sa, sg) → (Ia, Ig2) in mA
    mu, ex, kg1, kp, kvb, kg2, A, be, als, S, ap, w, nu, lam, sa, sg = P
    q = kp * (1 / mu + g1 / math.sqrt(kvb + g2 * g2)); sp = math.log1p(math.exp(q)) if q < 35 else q; ip = (g2 / kp * sp) ** ex
    v = max(va, 0.0); f = 1 / (1 + be * v); ps = S * v * (1 + math.tanh(-ap * (v - (g2 / lam - nu * g1 - w)))) if S else 0
    return sa * ip * ((1 / kg1 - 1 / kg2) * (1 - f) + A * v / kg1 - ps / kg2) * 1e3, sg * ip / kg2 * (1 + als * f + ps) * 1e3
def pgm(F, g1, g2, va, d=0.05): return (F(g1 + d, g2, va)[0] - F(g1 - d, g2, va)[0]) / 2 / d
def pri(F, g1, g2, va, d=0.5): return 2 * d / (F(g1, g2, va + d)[0] - F(g1, g2, va - d)[0])
def pushpull(F, g1, g2, B, Raa, vss, N=64):   # zwei Röhren am idealen Übertrager (Anode–Anode Raa): mittlere Ströme beider Röhren, Ausgangsleistung
    R = Raa / 4; d = []; ia = []; ig = []
    for k in range(N):
        u = vss / 2 * math.sin(2 * math.pi * k / N); a, b = g1 + u, g1 - u
        x = brentq(lambda x: x - (F(a, g2, B - R * x * 1e-3)[0] - F(b, g2, B + R * x * 1e-3)[0]), -2000, 2000)
        i1, s1 = F(a, g2, B - R * x * 1e-3); i2, s2 = F(b, g2, B + R * x * 1e-3); d.append(x); ia.append(i1 + i2); ig.append(s1 + s2)
    c1 = 2 * np.mean(np.array(d) * np.sin(2 * np.pi * np.arange(N) / N)) * 1e-3; return np.mean(ia), np.mean(ig), (2 * R * c1) ** 2 / (2 * Raa)
def table(F, g2=250): return '\n'.join(f'     Ug1 {g1:4d}: ' + ' '.join(f'{F(g1, g2, va)[0]:5.0f}' for va in (10, 20, 30, 50, 75, 100, 150, 250)) for g1 in (0, -5, -10, -15, -20))
korF = lambda P: (lambda g1, g2, va: pen(P, g1, g2, va))
# EL34: uTracer-Messung (Reefman, ExtractModel, dos4ever.com/uTracer3/TubeLib.inc, 15-Dec-13; BTetrodeD: A/kg1 = 1,6e-6), Ströme auf Mullard 250/250/−12,2 V skaliert
E = [12.5, 1.363, 217.7, 50.5, 1282.7, 1950.2, 1.6e-6 * 217.7, 0.105, 6.09, 0.022, 0.033, 64, 2.91, 4.23, 1, 1]
a0, s0 = derk(E, -12.2, 250, 250); E[14], E[15] = 100 / a0, 14.9 / s0; FE = lambda g1, g2, va: derk(E, g1, g2, va)
print('\nEL34 Derk (ab Laney 3.25): Maßstab Ia %.10g, Ig2 %.10g' % (E[14], E[15]))
for g1, g2, va, ia, ig, g, R in [(-12.2, 250, 250, 100, 14.9, 11, 15), (-14.5, 245, 250, 70, 10, 11, 20), (-13.5, 265, 250, 100, 14.9, 12.5, 17)]:
    print(f'  {va}/{g2}/{g1} V: Derk {FE(g1, g2, va)[0]:.1f} mA, Ig2 {FE(g1, g2, va)[1]:.1f}, S {pgm(FE, g1, g2, va):.1f}, ri {pri(FE, g1, g2, va):.1f} k | Koren {pen(EL34, g1, g2, va)[0]:.1f}, {pen(EL34, g1, g2, va)[1]:.1f}, {pgm(korF(EL34), g1, g2, va):.1f}, {pri(korF(EL34), g1, g2, va):.1f} k | Datenblatt {ia}, {ig}, {g}, {R} k')
print('  Laney-Ruhe 400/395/−36 V: Derk %.1f mA, Koren %.1f mA' % (FE(-36, 395, 400)[0], pen(EL34, -36, 395, 400)[0]))
print('  Kennlinien Ug2 250 V, Ua 10/20/30/50/75/100/150/250 V (mA) – Derk:\n' + table(FE) + '\n  Koren:\n' + table(korF(EL34)))
# 6L6GC: Derk ohne Sekundäremission, αs = 10 fest, an Datenblatt angepasst (RCA/GE: Einzelröhre A1, Gegentakt A1/AB1/AB2)
Z6 = [(-14, 250, 250, 72, 5, 6.0, 22.5), (-12.5, 200, 300, 48, 2.5, 5.3, 35), (-18, 250, 350, 54, 2.5, 5.2, 33), (-16, 250, 250, 60, 5, 5.5, 24.5), (-17.5, 270, 270, 67, 5.5, 5.7, 23.5)]
Z6b = [(-22.5, 270, 360, 44, 2.5), (-18, 225, 360, 39, 1.75)]
PP6 = [(-16, 250, 250, 5000, 32, 140, 16, 14.5), (-17.5, 270, 270, 5000, 35, 155, 17, 17.5), (-22.5, 270, 360, 6600, 45, 132, 15, 26.5),
       (-22.5, 270, 360, 3800, 45, 140, 11, 18), (-18, 225, 360, 6000, 52, 142, 11, 31), (-22.5, 270, 360, 3800, 72, 205, 16, 47)]
def l6p(q): return [q[0], q[1], q[2], q[3], 300, q[4], q[5], q[6], 10.0, 0, 0, 0, 0, 1, 1, 1]
def res6l(q):
    F = lambda g1, g2, va: derk(l6p(q), g1, g2, va); r = []
    for g1, g2, va, ia, ig, g, R in Z6: a, s_ = F(g1, g2, va); r += [2 * math.log(a / ia), math.log(s_ / ig), math.log(pgm(F, g1, g2, va) / g), 0.7 * math.log(pri(F, g1, g2, va) / R)]
    for g1, g2, va, ia, ig in Z6b: a, s_ = F(g1, g2, va); r += [math.log(a / ia), 0.5 * math.log(s_ / ig)]
    for g1, g2, B, Raa, v, ia, ig, po in PP6: a, s_, p_ = pushpull(F, g1, g2, B, Raa, v); r += [0.7 * math.log(a / ia), 0.4 * math.log(s_ / ig), 0.7 * math.log(p_ / po)]
    return r
f6l = least_squares(res6l, [8.753, 1.358, 1500, 38.93, 15000, 500, 0.1], bounds=([5, 1.1, 300, 10, 1500, 0, 0.01], [15, 1.8, 8000, 200, 1e5, 1e5, 2]))
P6 = l6p(f6l.x); F6 = lambda g1, g2, va: derk(P6, g1, g2, va); K6 = korF(L6)
print('\n6L6GC Derk (ab AmpFett 1.10): ' + ', '.join(f'{x:.10g}' for x in P6) + f'   rms-Fehler {np.sqrt(np.mean(np.square(f6l.fun))):.3f}')
for g1, g2, va, ia, ig, g, R in Z6:
    print(f'  {va}/{g2}/{g1} V: Derk {F6(g1, g2, va)[0]:.1f} mA, Ig2 {F6(g1, g2, va)[1]:.2f}, S {pgm(F6, g1, g2, va):.2f}, ri {pri(F6, g1, g2, va):.1f} k | Koren {K6(g1, g2, va)[0]:.1f}, {K6(g1, g2, va)[1]:.2f}, {pgm(K6, g1, g2, va):.2f}, {pri(K6, g1, g2, va):.1f} k | Datenblatt {ia}, {ig}, {g}, {R} k')
for g1, g2, B, Raa, v, ia, ig, po in PP6:
    a, s_, p_ = pushpull(F6, g1, g2, B, Raa, v); ak, sk, pk = pushpull(K6, g1, g2, B, Raa, v)
    print(f'  Gegentakt {B}/{g2}/{g1} V, {Raa} Ω, {v} Vss: Derk Ia {a:.0f} mA, Ig2 {s_:.1f}, {p_:.1f} W | Koren {ak:.0f}, {sk:.1f}, {pk:.1f} W | Datenblatt {ia}, {ig}, {po} W')
print('  Kennlinien Ug2 250 V (mA) – Derk:\n' + table(F6) + '\n  Koren:\n' + table(K6))

# 6550 (ab AmpFett SVT-CL 1.0): Derk ohne Sekundäremission, Knie-Form (ex, β, αs) wie 6L6GC (D6L6 in b15_core.hpp) fest, kvb 300;
# an die Tung-Sol/GE-Datenblattwerte angepasst: 250/250/−14 V 140 mA, Ig2 12 mA, 11 mA/V, 15 kΩ; 400/225/−16,5 V 85–87 mA, Ig2 4–9 mA, etwa 11 mA/V (TAD 6550A);
# mu schwach Richtung 8 gezogen. Ergebnis in svt_core.hpp (D6550). Prüfung: Ruhestrom 24 mA bei 660/345 V und etwa −40 V (SVT-CL: 0,24 V an 10 Ω, „approx. −45 V“).
EX6, BE6 = 1.562959483, 0.06717976054
Z55 = [(-14, 250, 250, 140, 12, 11.0, 15.0), (-16.5, 225, 400, 86, 6.5, 11.0, None)]
def p55(q): return [q[0], EX6, q[1], q[2], 300, q[3], q[4], BE6, 10.0, 0, 0, 0, 0, 1, 1, 1]
def res55(q):
    F = lambda g1, g2, va: derk(p55(q), g1, g2, va); r = []
    for g1, g2, va, ia, ig, g, R in Z55:
        a, s_ = F(g1, g2, va); r += [2 * math.log(a / ia), 0.5 * math.log(s_ / ig), math.log(pgm(F, g1, g2, va) / g)]
        if R: r += [0.7 * math.log(pri(F, g1, g2, va) / R)]
    return r + [0.3 * math.log(q[0] / 8.0)]
f55 = least_squares(res55, [8.28, 600, 36.8, 13000, 4.75e-4], bounds=([5, 100, 5, 1000, 1e-6], [12, 8000, 200, 1e5, 1e-2]))
P55 = p55(f55.x); F55 = lambda g1, g2, va: derk(P55, g1, g2, va)
print('\n6550 Derk (ab AmpFett SVT-CL 1.0): ' + ', '.join(f'{x:.10g}' for x in P55) + f'   rms-Fehler {np.sqrt(np.mean(np.square(f55.fun))):.3f}')
for g1, g2, va, ia, ig, g, R in Z55: print(f'  {va}/{g2}/{g1} V: Ia {F55(g1, g2, va)[0]:.1f} mA, Ig2 {F55(g1, g2, va)[1]:.1f}, S {pgm(F55, g1, g2, va):.2f}, ri {pri(F55, g1, g2, va):.1f} k | Datenblatt {ia}, {ig}, {g}, {R}')
print('  SVT-CL 660/345 V: 24 mA bei %.1f V' % brentq(lambda g1: F55(g1, 345, 660)[0] - 24, -120, -5))
