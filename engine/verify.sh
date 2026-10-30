#!/bin/bash
# Prüft, ob die frisch gebauten Programme mit den ausgelieferten übereinstimmen (Byte für Byte).
E="$(cd "$(dirname "$0")" && pwd)"; R="$(dirname "$E")"; cd "$E"
python3 - "$R" <<'PY'
import base64, re, sys, os
R = sys.argv[1]; B = os.path.join(R, 'engine', 'build')
def emb(f): return base64.b64decode(re.search(r'<script id="wasmcore" type="text/plain">([^<]*)</script>', open(os.path.join(R, f), encoding='utf-8').read()).group(1).strip())
for w, f in [('laney-core.wasm', 'web/tonestack/laney-aor50/index.html'), ('b100ii-core.wasm', 'web/tonestack/yamaha-b100ii/index.html')]:
    p = os.path.join(B, w)
    if not os.path.exists(p): continue
    if open(p, 'rb').read() == emb(f): print(f'{w}: identisch mit der Seite'); continue
    print(f'{w}: Bytes weichen von der Seite ab', end='')
    import subprocess, tempfile; tf = tempfile.NamedTemporaryFile(suffix='.wasm', delete=False); tf.write(emb(f)); tf.close()   # Klang vergleichen
    cmp = 'engine/tests/wasmcompare.js' if w == 'laney-core.wasm' else 'engine/tests/ywasmcompare.js'
    print(', Klangvergleich:'); print(subprocess.run(['node', os.path.join(R, cmp), p, tf.name], capture_output=True, text=True).stdout); os.unlink(tf.name)
for n in ['Laney_AOR50.clap', 'Yamaha_B100II.clap']:
    a = os.path.join(B, n); b = os.path.join(R, 'reaper-clap', n)
    if os.path.exists(a): print(n + ': ' + ('identisch mit reaper-clap/' if open(a, 'rb').read() == open(b, 'rb').read() else 'ABWEICHEND von reaper-clap/'))
for n in ['Laney_AOR50', 'Yamaha_B100II']:   # VST3 (nur wenn mit VST3=1 gebaut)
    a = os.path.join(B, 'vst3', n + '.vst3', 'Contents', 'x86_64-linux', n + '.so'); b = os.path.join(R, 'reaper-vst3', n + '.vst3', 'Contents', 'x86_64-linux', n + '.so')
    if os.path.exists(a) and os.path.exists(b): print(n + '.vst3: ' + ('identisch mit reaper-vst3/' if open(a, 'rb').read() == open(b, 'rb').read() else 'ABWEICHEND von reaper-vst3/'))
import zipfile, glob
for z in sorted(glob.glob(os.path.join(R, 'web/tonestack/download/*.zip'))):   # Download-Pakete der Übersichtsseite müssen die ausgelieferten Plugins enthalten
    with zipfile.ZipFile(z) as zf:
        for n in zf.namelist():
            if n.endswith('.clap'): print(os.path.basename(z) + ' / ' + n + ': ' + ('identisch mit reaper-clap/' if zf.read(n) == open(os.path.join(R, 'reaper-clap', n), 'rb').read() else 'ABWEICHEND von reaper-clap/'))
            if n.endswith('.so') and '.vst3/' in n: print(os.path.basename(z) + ' / ' + n.split('/')[0] + ': ' + ('identisch mit reaper-vst3/' if zf.read(n) == open(os.path.join(R, 'reaper-vst3', n), 'rb').read() else 'ABWEICHEND von reaper-vst3/'))
PY
