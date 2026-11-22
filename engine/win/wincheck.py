#!/usr/bin/env python3
# Windows-Seite der Prüfung (läuft im GitHub-Bau auf einem Windows-Rechner, nur Python-Standardbibliothek):
#   python wincheck.py ORDNER VALIDATOR BILDORDNER EDITORHOST
# ORDNER: VST3/, CLAP/, plugtest.exe, wingui.exe, in.f32, ref/ (aus engine/win/mkrefs.py unter Linux), settings.txt, plugins.txt (scripts/plugins.txt)
# 1. Steinberg-Validator auf jedes VST3-Bundle (alle Tests bestanden)
# 2. CLAP durchrechnen (plugtest.exe) und mit der Linux-Rechnung vergleichen: endlich, Pegel auf 0,1 dB gleich, Abweichung höchstens −50 dB
#    (andere Mathematikbibliothek: msvcrt statt glibc; gemessen −70 … −340 dB)
# 3. Oberfläche im echten Fenster (wingui.exe): alle Prüfpunkte, Bild als PNG → BILDORDNER/win-<kurz>.png
# 4. VST3-Oberfläche in einem echten VST3-Host (editorhost aus dem VST3-SDK; so lädt auch Cubase): Fenster mit der Ansicht, Größe passt,
#    Größe des Host-Fensters ändern (onSize, Seitenverhältnis), 80 Mausaktionen, Schließen; Fehlermeldungen des Hosts (Dialog) und Abstürze gelten als Fehler
# Rückgabe 1 bei einem Fehler; Fehler zusätzlich als GitHub-Hinweis (::error::).
import math, os, re, struct, subprocess, sys, zlib
import ctypes, random, time
def plugin_list(path):   # scripts/plugins.txt: Name|Versionsvariable|README|Kurzname|Anzeigename
    out = {}
    for l in open(path, encoding="utf-8"):
        l = l.strip()
        if l and not l.startswith("#"): f = l.split("|"); out[f[0]] = f[3]
    return out
MAXDIF = -50.0
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
# ---- 4. VST3-Oberfläche in editorhost (Fenster eines anderen Prozesses, über user32) ----
def vst3host(ehost, bundle, name):
    from ctypes import wintypes as W
    u = ctypes.WinDLL("user32", use_last_error=True)
    PROC = ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
    u.EnumWindows.argtypes = [PROC, W.LPARAM]; u.EnumChildWindows.argtypes = [W.HWND, PROC, W.LPARAM]
    u.GetWindowThreadProcessId.argtypes = [W.HWND, ctypes.POINTER(W.DWORD)]; u.GetClassNameW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]
    u.GetWindowTextW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]; u.GetClientRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
    u.GetWindowRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]; u.IsWindowVisible.argtypes = [W.HWND]; u.IsWindow.argtypes = [W.HWND]
    u.GetSystemMetrics.argtypes = [ctypes.c_int]; u.SetWindowPos.argtypes = [W.HWND, W.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, W.UINT]
    u.PostMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]; u.ClientToScreen.argtypes = [W.HWND, ctypes.POINTER(W.POINT)]
    def wins(pid):
        res = []
        def cb(h, l):
            q = W.DWORD(); u.GetWindowThreadProcessId(h, ctypes.byref(q))
            if q.value == pid: res.append(h)
            return True
        u.EnumWindows(PROC(cb), 0); return res
    def kids(h):
        res = []
        def cb(c, l): res.append(c); return True
        u.EnumChildWindows(h, PROC(cb), 0); return res
    def cls(h): b = ctypes.create_unicode_buffer(256); u.GetClassNameW(h, b, 256); return b.value
    def txt(h): b = ctypes.create_unicode_buffer(1024); u.GetWindowTextW(h, b, 1024); return b.value
    def csize(h): r = W.RECT(); u.GetClientRect(h, ctypes.byref(r)); return r.right - r.left, r.bottom - r.top
    def res(what, ok, detail=""):
        print("    RESULT %-28s %s %s" % (what, "ok" if ok else "FEHLER", detail)); sys.stdout.flush()
        if not ok: err("VST3-Host %s: %s %s" % (name, what, detail))
        return ok
    print("  " + name); sys.stdout.flush()
    pr = subprocess.Popen([ehost, bundle]); top = view = None; t0 = time.time(); why = "keine Ansicht nach 30 s"
    try:
        while time.time() - t0 < 30:
            ws = [h for h in wins(pr.pid) if u.IsWindowVisible(h)]
            dlg = [h for h in ws if cls(h) == "#32770"]
            if dlg: why = "Meldung des Hosts: " + " / ".join(t for t in (txt(k) for d in dlg for k in kids(d)) if t); break
            for h in ws:
                if cls(h) == "VSTSDK WindowClass":
                    v = [k for k in kids(h) if cls(k).startswith("tonestack_view_")]
                    if v: top, view = h, v[0]
            if view: break
            if pr.poll() is not None: why = "Host beendet (Rückgabe %s)" % pr.returncode; break
            time.sleep(0.2)
        if not res("fenster mit ansicht", view is not None, "" if view else why): return
        time.sleep(1.0)
        tw, th = csize(top); vw, vh = csize(view)
        r = W.RECT(); u.GetWindowRect(top, ctypes.byref(r)); ow, oh = r.right - r.left, r.bottom - r.top
        sw, sh = u.GetSystemMetrics(0), u.GetSystemMetrics(1)   # SM_CXSCREEN/SM_CYSCREEN: Windows begrenzt Hauptfenster auf etwa die Bildschirmgröße (GitHub-Rechner 1024 × 768)
        clipped = tw < vw and ow >= sw; clippedH = th < vh and oh >= sh - 40   # Höhe: Taskleiste und Rahmen abgezogen (Cultec 1400 × 893 auf 768 Zeilen)
        res("groesse", vw >= 300 and vh >= 100 and (abs(vh - th) <= 1 or clippedH) and (abs(vw - tw) <= 1 or clipped),
            "Ansicht %dx%d, Host-Fenster %dx%d%s" % (vw, vh, tw, th, " (vom Bildschirm begrenzt)" if (clipped and abs(vw - tw) > 1) or (clippedH and abs(vh - th) > 1) else ""))
        # Host-Fenster auf 900 Pixel Innenbreite (passt auf jeden Bildschirm, über jeder Untergrenze): onSize → adjust_size → set_size
        want = 900; wh = int(want * vh / vw) + 40
        u.SetWindowPos(top, None, 0, 0, want + (ow - tw), wh + (oh - th), 0x0002 | 0x0004 | 0x0010)   # SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
        time.sleep(1.0); tw2, th2 = csize(top); vw2, vh2 = csize(view)
        res("groesse aendern (onSize)", tw2 == want and abs(vw2 - want) <= 2 and abs(vw2 * vh / vw - vh2) <= 3 and vh2 <= th2 + 2,
            "Host-Fenster %dx%d, Ansicht %dx%d (vorher %dx%d)" % (tw2, th2, vw2, vh2, vw, vh))
        rnd = random.Random(7); vw, vh = csize(view)
        for k in range(80):
            x, y = rnd.randrange(vw), rnd.randrange(vh); lp = (y << 16) | x; kind = k % 3
            if kind == 0: u.PostMessageW(view, 0x0201, 1, lp); u.PostMessageW(view, 0x0202, 0, lp)   # Klick
            elif kind == 1:
                u.PostMessageW(view, 0x0201, 1, lp)
                for s in range(1, 6): u.PostMessageW(view, 0x0200, 1, (max(0, y - 10 * s) << 16) | x)
                u.PostMessageW(view, 0x0202, 0, (max(0, y - 50) << 16) | x)
            else:
                pt = W.POINT(x, y); u.ClientToScreen(view, ctypes.byref(pt))
                u.PostMessageW(view, 0x020A, ((120 if rnd.random() < 0.5 else -120) & 0xffff) << 16, ((pt.y & 0xffff) << 16) | (pt.x & 0xffff))
            if k % 10 == 9: time.sleep(0.05)
        time.sleep(1.0)
        dlg = [h for h in wins(pr.pid) if u.IsWindowVisible(h) and cls(h) == "#32770"]
        res("maus", pr.poll() is None and not dlg and u.IsWindow(view), "Host laeuft" if pr.poll() is None else "Host beendet (Rückgabe %s)" % pr.returncode)
        u.PostMessageW(top, 0x0010, 0, 0)   # WM_CLOSE
        try: rc = pr.wait(15)
        except subprocess.TimeoutExpired: rc = None
        res("schliessen", rc == 0, "Rückgabe %s" % rc)
    finally:
        if pr.poll() is None: pr.kill(); pr.wait()
def main():
    z, validator, img, ehost = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]; os.makedirs(img, exist_ok=True)
    global SHORT; SHORT = plugin_list(os.path.join(z, "plugins.txt"))
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
            good = fin and abs(lev) < 0.1 and eb > 0 and dif <= MAXDIF
            print("  %-18s [%-11s] Pegel Windows/Linux %+.4f dB, Abweichung %7.1f dB %s" % (p, s or "Grund", lev, dif, "ok" if good else "FEHLER"))
            if not good: err("Rechnung %s [%s]: endlich=%s Pegel %+.3f dB Abweichung %.1f dB" % (p, s, fin, lev, dif))
    print("== 3. Oberfläche im Fenster")
    for p in names:
        bmp = os.path.join(img, "win-%s.bmp" % SHORT[p]); rc, out = run([os.path.join(z, "wingui.exe"), os.path.join(z, "CLAP", p + ".clap"), bmp], 300)
        print("  " + p); print("\n".join("    " + l for l in out.splitlines() if l.startswith(("RESULT", "Plugin"))))
        if rc != 0: err("Oberfläche %s: Rückgabe %d" % (p, rc))
        if os.path.exists(bmp): bmp2png(bmp, bmp[:-4] + ".png"); os.remove(bmp)
        else: err("Oberfläche %s: kein Bild" % p)
    print("== 4. VST3-Oberfläche im VST3-Host (editorhost)")
    for p in names: vst3host(ehost, os.path.join(z, "VST3", p + ".vst3"), p)
    print("FEHLER: %d" % len(bad) if bad else "alle Windows-Prüfungen bestanden"); return 1 if bad else 0
sys.exit(main())
