#!/bin/bash
# Prüft, ob die frisch gebauten Programme mit den ausgelieferten übereinstimmen (Byte für Byte): WebAssembly in den Seiten, Plugins in dist/*.zip.
E="$(cd "$(dirname "$0")" && pwd)"; R="$(dirname "$E")"; cd "$E"
python3 - "$R" <<'PY'
import base64, re, sys, os
R = sys.argv[1]; B = os.path.join(R, 'engine', 'build')
def emb(f): return base64.b64decode(re.search(r'<script id="wasmcore" type="text/plain">([^<]*)</script>', open(os.path.join(R, f), encoding='utf-8').read()).group(1).strip())
for w, f in [('laney-core.wasm', 'web/tonestack/laney-aor50/index.html'), ('b100ii-core.wasm', 'web/tonestack/yamaha-b100ii/index.html'), ('b15nf-core.wasm', 'web/tonestack/ampfett-b15nf/index.html'), ('gk800rb-core.wasm', 'web/tonestack/gk-800rb/index.html'), ('g100-core.wasm', 'web/tonestack/yamaha-g100/index.html'), ('b100iii-core.wasm', 'web/tonestack/yamaha-b100iii/index.html')]:
    p = os.path.join(B, w)
    if not os.path.exists(p): continue
    if open(p, 'rb').read() == emb(f): print(f'{w}: identisch mit der Seite'); continue
    print(f'{w}: Bytes weichen von der Seite ab', end='')
    import subprocess, tempfile; tf = tempfile.NamedTemporaryFile(suffix='.wasm', delete=False); tf.write(emb(f)); tf.close()   # Klang vergleichen
    if w in ('b15nf-core.wasm', 'gk800rb-core.wasm', 'g100-core.wasm', 'b100iii-core.wasm'): print(' (kein Klangvergleich für diesen Kern)'); os.unlink(tf.name); continue
    cmp = 'engine/tests/wasmcompare.js' if w == 'laney-core.wasm' else 'engine/tests/ywasmcompare.js'
    print(', Klangvergleich:'); print(subprocess.run(['node', os.path.join(R, cmp), p, tf.name], capture_output=True, text=True).stdout); os.unlink(tf.name)
import zipfile, glob   # Plugin-Pakete (dist/, aus scripts/pack-plugins.sh oder aus dem GitHub-Release) gegen den frischen Bau
zs = sorted(glob.glob(os.path.join(R, 'dist', '*.zip')))
if not zs: print('dist/: keine Plugin-Pakete (bash scripts/pack-plugins.sh oder gh release download)')
for z in zs:
    with zipfile.ZipFile(z) as zf:
        for n in zf.namelist():
            if n.endswith('.clap'): a = os.path.join(B, n)
            elif n.endswith('.so') and '.vst3/' in n: a = os.path.join(B, 'vst3', n)
            else: continue
            if not os.path.exists(a): print(os.path.basename(z) + ' / ' + n + ': nicht gebaut'); continue
            print(os.path.basename(z) + ' / ' + n.split('/')[0] + ': ' + ('identisch mit dem Bau' if zf.read(n) == open(a, 'rb').read() else 'ABWEICHEND vom Bau'))
PY
