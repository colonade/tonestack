#!/usr/bin/env python3
# Vergleicht die Rechnung des Kerns (kern.txt) mit ngspice (spice.txt) Knoten für Knoten.
#   vergleich.py ORDNER [t0]      t0: Auswertung ab dieser Zeit (s), Vorgabe 0
# Je Knoten: Ruhespannung zu Beginn (beide), Wechselanteil (effektiv), Abweichung (effektiv und Spitze) absolut und bezogen auf den Wechselanteil in dB.
import sys, numpy as np
d = sys.argv[1]; t0 = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0
def rd(f):
    with open(f) as h: names = h.readline().split()
    a = np.loadtxt(f, skiprows=1); return [n.replace('v(', '').replace(')', '').lower() for n in names], a
kn, K = rd(d + '/kern.txt'); sn, S = rd(d + '/spice.txt')
tk = K[:, 0]; ts = S[:, 0]; m = (tk >= t0) & (tk <= ts[-1]); worst = 0
print('%-6s %10s %10s %9s | %10s %10s %8s | %9s' % ('Knoten', 'Ruhe Kern', 'Ruhe SPICE', 'Differenz', 'Wechsel V', 'Abw. eff', 'dB', 'Abw. Sp.'))
for j, n in enumerate(kn[1:], 1):
    if n not in sn: continue
    a = K[:, j]; b = np.interp(tk, ts, S[:, sn.index(n)]); ac = b[m] - b[m].mean(); e = a[m] - b[m]
    rms = np.sqrt((ac ** 2).mean()); er = np.sqrt((e ** 2).mean()); db = 20 * np.log10(max(er, 1e-12) / max(rms, 1e-12)); worst = max(worst, db)
    print('%-6s %10.4f %10.4f %+9.4f | %10.4g %10.3g %+8.1f | %9.3g' % (n, a[0], b[0], a[0] - b[0], rms, er, db, np.abs(e).max()))
print('größte Abweichung bezogen auf den Wechselanteil: %+.1f dB' % worst)
