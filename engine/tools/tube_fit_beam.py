# Endröhren mit dem Knie der Strahltetrode (ab AmpFett B-15NF 1.24 / SVT-CL 1.10): python3 engine/tools/tube_fit_beam.py
# 6L6GC: Knie, Sekundäremission und kvb von einer vermessenen Röhre (Reefman, TubeLib.inc), Pegel-Parameter an die Datenblattwerte (wie tube_fits.py).
# 6550: keine Messung verfügbar; Knie-Form und Exponent vom 6L6GC übernommen (Annahme), Pegel an die Datenblattwerte.
import math, numpy as np
from scipy.optimize import brentq, least_squares
def derk(P, g1, g2, va, knee):   # knee 0: 1/(1+βUa) (Pentode), 1: exp(−(βUa)^1,5) (Strahltetrode)
    mu, ex, kg1, kp, kvb, kg2, A, be, als, S, ap, w, nu, lam, sa, sg = P
    q = kp*(1/mu + g1/math.sqrt(kvb + g2*g2)); sp = math.log1p(math.exp(q)) if q < 35 else q; ip = (g2/kp*sp)**ex
    v = max(va, 0.0); f = math.exp(-(be*v)**1.5) if knee else 1/(1 + be*v); ps = S*v*(1 + math.tanh(-ap*(v - (g2/lam - nu*g1 - w)))) if S else 0
    return sa*ip*((1/kg1 - 1/kg2)*(1 - f) + A*v/kg1 - ps/kg2)*1e3, sg*ip/kg2*(1 + als*f + ps)*1e3
def pgm(F, g1, g2, va, d=0.05): return (F(g1 + d, g2, va)[0] - F(g1 - d, g2, va)[0])/2/d
def pri(F, g1, g2, va, d=0.5): return 2*d/(F(g1, g2, va + d)[0] - F(g1, g2, va - d)[0])
def pushpull(F, g1, g2, B, Raa, vss, N=64):
    R = Raa/4; d = []; ia = []; ig = []
    for k in range(N):
        u = vss/2*math.sin(2*math.pi*k/N); a, b = g1 + u, g1 - u
        x = brentq(lambda x: x - (F(a, g2, B - R*x*1e-3)[0] - F(b, g2, B + R*x*1e-3)[0]), -3000, 3000)
        i1, s1 = F(a, g2, B - R*x*1e-3); i2, s2 = F(b, g2, B + R*x*1e-3); d.append(x); ia.append(i1 + i2); ig.append(s1 + s2)
    c1 = 2*np.mean(np.array(d)*np.sin(2*np.pi*np.arange(N)/N))*1e-3; return np.mean(ia), np.mean(ig), (2*R*c1)**2/(2*Raa)
Z6 = [(-14, 250, 250, 72, 5, 6.0, 22.5), (-12.5, 200, 300, 48, 2.5, 5.3, 35), (-18, 250, 350, 54, 2.5, 5.2, 33), (-16, 250, 250, 60, 5, 5.5, 24.5), (-17.5, 270, 270, 67, 5.5, 5.7, 23.5)]
Z6b = [(-22.5, 270, 360, 44, 2.5), (-18, 225, 360, 39, 1.75)]
PP6 = [(-16, 250, 250, 5000, 32, 140, 16, 14.5), (-17.5, 270, 270, 5000, 35, 155, 17, 17.5), (-22.5, 270, 360, 6600, 45, 132, 15, 26.5),
       (-22.5, 270, 360, 3800, 45, 140, 11, 18), (-18, 225, 360, 6000, 52, 142, 11, 31), (-22.5, 270, 360, 3800, 72, 205, 16, 47)]
def res(P, knee, detail=False):
    F = lambda g1, g2, va: derk(P, g1, g2, va, knee); r = []
    for g1, g2, va, ia, ig, g, R in Z6: a, s_ = F(g1, g2, va); r += [2*math.log(a/ia), math.log(s_/ig), math.log(pgm(F, g1, g2, va)/g), 0.7*math.log(max(pri(F, g1, g2, va), 1e-3)/R)]
    for g1, g2, va, ia, ig in Z6b: a, s_ = F(g1, g2, va); r += [math.log(a/ia), 0.5*math.log(s_/ig)]
    pp = []
    for g1, g2, B, Raa, v, ia, ig, po in PP6: a, s_, p_ = pushpull(F, g1, g2, B, Raa, v); r += [0.7*math.log(a/ia), 0.4*math.log(s_/ig), 0.7*math.log(p_/po)]; pp.append((a, s_, p_))
    return (r, pp) if detail else r
OLD = [8.281661842, 1.562959483, 1150.834177, 36.84207058, 300, 26080.39458, 4.751966359e-4, 0.06717976054, 10, 0, 0, 0, 0, 1, 1, 1]
LIB = [9.41, 1.306, 446.6, 45.2, 3205.1, 6672.5, 1.1e-6*446.6, 0.069, 8.10, 0.031, 0.017, -5, 3.08, 14.66, 1, 1]
def show(nm, P, knee):
    r, pp = res(P, knee, True); print(f'{nm}: rms {np.sqrt(np.mean(np.square(r))):.3f}')
    print('   Ruhepunkte Ia/Ig2 (Datenblatt): ' + '  '.join('%.0f/%.1f (%g/%g)' % (*derk(P, g1, g2, va, knee), ia, ig) for g1, g2, va, ia, ig, g, R in Z6))
    print('   Gegentakt Ia/Ig2/P (Datenblatt): ' + '  '.join('%.0f/%.0f/%.1f W (%g/%g/%g)' % (a, s_, p_, ia, ig, po) for (a, s_, p_), (g1, g2, B, Raa, v, ia, ig, po) in zip(pp, PP6)))
show('bisher (D-Form, Datenblatt)', OLD, 0); show('vermessene Röhre (TubeLib, DE-Form)', LIB, 1)
# neu: Knie und Sekundäremission von der vermessenen Röhre, Pegel-Parameter an das Datenblatt
def mk(q): return [q[0], q[1], q[2], q[3], LIB[4], q[4], q[5], LIB[7], LIB[8], LIB[9], LIB[10], LIB[11], LIB[12], LIB[13], 1, 1]
f = least_squares(lambda q: res(mk(q), 1), [LIB[0], LIB[1], LIB[2], LIB[3], LIB[5], LIB[6]], bounds=([5, 1.1, 100, 10, 1500, 0], [15, 1.8, 8000, 200, 1e5, 2])); NEW = mk(f.x)
show('neu A: Knie/Knick gemessen, Pegel an Datenblatt', NEW, 1); print('   Parameter: ' + ', '.join(f'{x:.10g}' for x in NEW))
def mk2(q): return [q[0], q[1], q[2], q[3], LIB[4], q[4], q[5], q[6], q[7], LIB[9], LIB[10], LIB[11], LIB[12], LIB[13], 1, 1]
f2 = least_squares(lambda q: res(mk2(q), 1), list(f.x) + [LIB[7], LIB[8]], bounds=([5, 1.1, 100, 10, 1500, 0, 0.02, 1], [15, 1.8, 8000, 200, 1e5, 2, 0.3, 30])); NEW2 = mk2(f2.x)
show('neu B: wie A, zusätzlich β und αs frei', NEW2, 1); print('   Parameter: ' + ', '.join(f'{x:.10g}' for x in NEW2))
for nm, P, k in (('bisher', OLD, 0), ('TubeLib', LIB, 1), ('neu A', NEW, 1)):
    print(f'{nm}: B-15NF-Ruhe 430/430/−49,5 V: Ia %.1f mA, Ig2 %.2f mA | Kennlinie Ug2 250, Ug1 0: ' % derk(P, -49.5, 430, 430, k) + ' '.join('%.0f' % derk(P, 0, 250, va, k)[0] for va in (10, 20, 30, 50, 75, 100, 150, 250)))

EX6, BE6, ALS6, KVB6 = 1.606514837, 0.069, 8.1, 3205.1
Z55 = [(-14, 250, 250, 140, 12, 11.0, 15.0), (-16.5, 225, 400, 86, 6.5, 11.0, None)]
OLD = [7.584690749, 1.562959483, 671.7245705, 200, 300, 12377.82231, 0.0002564468176, 0.06717976054, 10, 0, 0, 0, 0, 1, 1, 1]
def p55(q): return [q[0], EX6, q[1], q[2], KVB6, q[3], q[4], BE6, ALS6, 0, 0, 0, 0, 1, 1, 1]
def res55(q, P=None, knee=1):
    PP = P or p55(q); F = lambda g1, g2, va: derk(PP, g1, g2, va, knee); r = []
    for g1, g2, va, ia, ig, g, R in Z55:
        a, s_ = F(g1, g2, va); r += [2*math.log(a/ia), 0.5*math.log(s_/ig), math.log(pgm(F, g1, g2, va)/g)]
        if R: r += [0.7*math.log(pri(F, g1, g2, va)/R)]
    return r + ([0.3*math.log(q[0]/8.0)] if P is None else [])
f = least_squares(res55, [8.0, 800, 40, 13000, 5e-4], bounds=([5, 100, 5, 1000, 1e-6], [12, 8000, 300, 1e5, 1e-2])); P = p55(f.x)
print('6550 neu: ' + ', '.join(f'{x:.10g}' for x in P) + f'  rms {np.sqrt(np.mean(np.square(f.fun))):.3f} (bisher {np.sqrt(np.mean(np.square(res55(None, OLD, 0)))):.3f})')
for nm, PP, k in (('bisher', OLD, 0), ('neu', P, 1)):
    F = lambda g1, g2, va: derk(PP, g1, g2, va, k)
    print(f' {nm}: ' + '; '.join(f'{va}/{g2}/{g1}: Ia {F(g1,g2,va)[0]:.1f} ({ia}), Ig2 {F(g1,g2,va)[1]:.1f} ({ig}), S {pgm(F,g1,g2,va):.1f} ({g})' for g1, g2, va, ia, ig, g, R in Z55) + ' | SVT 660/345 V: 24 mA bei %.1f V' % brentq(lambda g1: F(g1, 345, 660)[0] - 24, -120, -5))
    print('    Ug2 345, Ug1 0, Ua 20/50/100/200/400/660: ' + ' '.join('%.0f(%.0f)' % F(0, 345, va) for va in (20, 50, 100, 200, 400, 660)))
