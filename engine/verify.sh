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
    if w == 'laney-core.wasm':   # gemeinsamer Kern hat die (vom Laney ungenutzte) Yamaha-Ergänzung: Klang vergleichen
        open('/tmp/_deployed.wasm', 'wb').write(emb(f)); import subprocess
        print(', Klangvergleich:'); print(subprocess.run(['node', os.path.join(R, 'engine/tests/wasmcompare.js'), p, '/tmp/_deployed.wasm'], capture_output=True, text=True).stdout)
    else: print()
a = os.path.join(B, 'Laney_AOR50.clap'); b = os.path.join(R, 'reaper-clap', 'Laney_AOR50.clap')
if os.path.exists(a): print('Laney_AOR50.clap: ' + ('identisch mit reaper-clap/' if open(a, 'rb').read() == open(b, 'rb').read() else 'ABWEICHEND von reaper-clap/'))
PY
