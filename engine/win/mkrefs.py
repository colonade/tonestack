#!/usr/bin/env python3
# Linux-Seite der Windows-Prüfung: Prüfsignal erzeugen und mit den Linux-CLAPs durchrechnen (Bezug für engine/win/wincheck.py).
#   python3 engine/win/mkrefs.py ZIELORDNER
# ZIELORDNER/in.f32 (3 s, 48 kHz, gezupfte Basstöne mit Obertönen, Spitze 0,6) und ZIELORDNER/ref/<Plugin>-<n>.f32 je Einstellung.
import math, os, struct, subprocess, sys
PLUGINS = ["Laney_AOR50", "Lametta_B100II", "AmpFett_B15NF", "GK_800RB", "Lametta_G100", "Lametta_B100III", "AmpFett_SVTCL",
           "Wien_EQ", "AmpFett_76", "Loldano_SLO100", "Kaiserin_Reverb", "Blech_140", "Wisent_Echoreck", "Viereck_34"]
SETTINGS = ["", "0=3,1=7,2=2"]   # Grundstellung und eine andere Stellung (wie beim Vergleich mit dem Release)
def main():
    z = sys.argv[1]; e = os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.makedirs(os.path.join(z, "ref"), exist_ok=True)
    fs = 48000; n = 3 * fs; notes = [41.2, 55, 73.4, 98, 61.7, 82.4]; ph = 0.0; x = []
    for i in range(n):
        t = i / fs; k = int(t / 0.5); f = notes[k % len(notes)]; env = math.exp(-(t - k * 0.5) * 4); ph += 2 * math.pi * f / fs
        x.append(0.6 * env * sum(math.sin(h * ph) / h for h in range(1, 7)))
    with open(os.path.join(z, "in.f32"), "wb") as fo: fo.write(struct.pack("<%df" % n, *x))
    for p in PLUGINS:
        for k, s in enumerate(SETTINGS):
            out = os.path.join(z, "ref", "%s-%d.f32" % (p, k))
            r = subprocess.run([os.path.join(e, "build", "plugtest"), os.path.join(e, "build", p + ".clap"), "render", os.path.join(z, "in.f32"), out, s], capture_output=True, text=True)
            if r.returncode != 0 or not os.path.exists(out): print("FEHLER", p, s, r.stdout[-300:], r.stderr[-300:]); sys.exit(1)
    with open(os.path.join(z, "settings.txt"), "w") as fo: fo.write("\n".join(SETTINGS) + "\n")
    print("Bezug: %d Plugins x %d Einstellungen in %s" % (len(PLUGINS), len(SETTINGS), z))
main()
