#!/usr/bin/env python3
# Windows-Seite der Prüfung (läuft im GitHub-Bau auf einem Windows-Rechner, nur Python-Standardbibliothek):
#   python wincheck.py ORDNER VALIDATOR BILDORDNER
# ORDNER: VST3/, CLAP/, plugtest.exe, wingui.exe, in.f32, ref/ (aus engine/win/mkrefs.py unter Linux), settings.txt
# 1. Steinberg-Validator auf jedes VST3-Bundle (alle Tests bestanden)
# 2. CLAP durchrechnen (plugtest.exe) und mit der Linux-Rechnung vergleichen: endlich, Pegel auf 0,1 dB gleich; Abweichung wird ausgegeben
# 3. Oberfläche im echten Fenster (wingui.exe): alle Prüfpunkte, Bild als PNG → BILDORDNER/win-<kurz>.png
# Rückgabe 1 bei einem Fehler; Fehler zusätzlich als GitHub-Hinweis (::error::).
import math, os, re, struct, subprocess, sys, zlib
SHORT = {"Laney_AOR50": "laney", "Lametta_B100II": "b100ii", "AmpFett_B15NF": "b15nf", "GK_800RB": "gk", "Lametta_G100": "g100", "Lametta_B100III": "b100iii",
         "AmpFett_SVTCL": "svtcl", "Wien_EQ": "wieneq", "AmpFett_76": "ampfett76", "Loldano_SLO100": "loldano", "Kaiserin_Reverb": "kaiserin",
         "Blech_140": "blech140", "Wisent_Echoreck": "wisent", "Viereck_34": "viereck"}
bad = []
def err(msg): bad.append(msg); print("::error::" + msg); sys.stdout.flush()
def run(cmd, t=600):
    try: r = subprocess.run(cmd, capture_output=True, text=True, timeout=t, errors="replace"); return r.returncode, r.stdout + r.stderr
    except subprocess.TimeoutExpired: return -1, "Zeitüberschreitung"
def readf32(p):
    with open(p, "rb") as f: d = f.read()
    return struct.unpack("<%df" % (len(d) // 4), d)
def bmp2png(src, dst):
    with open(src, "rb") as f: d = f.read()
    off = struct.unpack_from("<I", d, 10)[0]; w, h = struct.unpack_from("<ii", d, 18); top = h < 0; h = abs(h)
    rows = []
    for y in range(h):
        yy = y if top else h - 1 - y; r = d[off + yy * w * 4: off + (yy + 1) * w * 4]
        rows.append(b"\x00" + bytes(c for i in range(0, len(r), 4) for c in (r[i + 2], r[i + 1], r[i])))
    def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b) & 0xffffffff)
    with open(dst, "wb") as f: f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))
def main():
    z, validator, img = sys.argv[1], sys.argv[2], sys.argv[3]; os.makedirs(img, exist_ok=True)
    names = sorted(f[:-5] for f in os.listdir(os.path.join(z, "CLAP")) if f.endswith(".clap"))
    if sorted(names) != sorted(SHORT): err("Plugins: %s" % names)
    settings = open(os.path.join(z, "settings.txt")).read().split("\n")[:-1]
    print("== 1. VST3-Validator")
    for p in names:
        rc, out = run([validator, os.path.join(z, "VST3", p + ".vst3")])
        m = re.findall(r"Result: (\d+) tests passed, (\d+) tests failed", out)
        if m: ok, fail = sum(int(a) for a, b in m), sum(int(b) for a, b in m)
        else: ok, fail = 0, -1
        print("  %-18s %s bestanden %d, fehlgeschlagen %d (Rückgabe %d)" % (p, "ok    " if rc == 0 and fail == 0 and ok > 0 else "FEHLER", ok, fail, rc))
        if not (rc == 0 and fail == 0 and ok > 0): err("Validator %s: %d bestanden, %d fehlgeschlagen, Rückgabe %d" % (p, ok, fail, rc)); print("\n".join(l for l in out.splitlines() if "FAIL" in l.upper() or "ERROR" in l.upper())[:2000])
    print("== 2. Rechnung gegen Linux (gleiches Signal, gleiche Einstellung)")
    for p in names:
        for k, s in enumerate(settings):
            out = os.path.join(z, "win-%s-%d.f32" % (p, k)); rc, txt = run([os.path.join(z, "plugtest.exe"), os.path.join(z, "CLAP", p + ".clap"), "render", os.path.join(z, "in.f32"), out, s])
            if rc != 0 or not os.path.exists(out): err("Rechnung %s [%s]: Rückgabe %d %s" % (p, s, rc, txt[-200:])); continue
            a = readf32(out); b = readf32(os.path.join(z, "ref", "%s-%d.f32" % (p, k)))
            if len(a) != len(b): err("Rechnung %s [%s]: Länge %d statt %d" % (p, s, len(a), len(b))); continue
            fin = all(math.isfinite(v) for v in a); ea = sum(v * v for v in a); eb = sum(v * v for v in b); ed = sum((u - v) ** 2 for u, v in zip(a, b))
            lev = 10 * math.log10((ea + 1e-30) / (eb + 1e-30)); dif = 10 * math.log10((ed + 1e-30) / (eb + 1e-30))
            good = fin and abs(lev) < 0.1 and eb > 0
            print("  %-18s [%-11s] Pegel Windows/Linux %+.4f dB, Abweichung %7.1f dB %s" % (p, s or "Grund", lev, dif, "ok" if good else "FEHLER"))
            if not good: err("Rechnung %s [%s]: endlich=%s Pegel %+.3f dB" % (p, s, fin, lev))
    print("== 3. Oberfläche im Fenster")
    for p in names:
        bmp = os.path.join(img, "win-%s.bmp" % SHORT[p]); rc, out = run([os.path.join(z, "wingui.exe"), os.path.join(z, "CLAP", p + ".clap"), bmp], 300)
        print("  " + p); print("\n".join("    " + l for l in out.splitlines() if l.startswith(("RESULT", "Plugin"))))
        if rc != 0: err("Oberfläche %s: Rückgabe %d" % (p, rc))
        if os.path.exists(bmp): bmp2png(bmp, bmp[:-4] + ".png"); os.remove(bmp)
        else: err("Oberfläche %s: kein Bild" % p)
    print("FEHLER: %d" % len(bad) if bad else "alle Windows-Prüfungen bestanden"); return 1 if bad else 0
sys.exit(main())
