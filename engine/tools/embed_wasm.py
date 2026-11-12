# Bettet frisch gebaute WebAssembly-Kerne in die Web-Seiten ein: python3 engine/tools/embed_wasm.py [repo]
import base64, os, re, sys
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
B = os.path.join(R, 'engine', 'build')
targets = {'laney-core.wasm': ['web/tonestack/laney-aor50/index.html'],
           'b100ii-core.wasm': ['web/tonestack/lametta-b100ii/index.html'],
           'b15nf-core.wasm': ['web/tonestack/ampfett-b15nf/index.html'],
           'gk800rb-core.wasm': ['web/tonestack/gk-800rb/index.html'],
           'g100-core.wasm': ['web/tonestack/lametta-g100/index.html'],
           'b100iii-core.wasm': ['web/tonestack/lametta-b100iii/index.html'],
           'svtcl-core.wasm': ['web/tonestack/ampfett-svtcl/index.html'],
           'wieneq-core.wasm': ['web/tonestack/wien-eq/index.html'],
           'ampfett76-core.wasm': ['web/tonestack/ampfett-76/index.html']}
for wasm, files in targets.items():
    if not os.path.exists(os.path.join(B, wasm)): sys.exit('fehlt: engine/build/' + wasm + ' (erst bash engine/build.sh mit wasi-sdk)')
    b64 = base64.b64encode(open(os.path.join(B, wasm), 'rb').read()).decode()
    for f in files:
        p = os.path.join(R, f); h = open(p, encoding='utf-8').read()
        h2, n = re.subn(r'(<script id="wasmcore" type="text/plain">)[^<]*(</script>)', lambda m: m.group(1) + b64 + m.group(2), h)
        if n != 1: print('nicht gefunden in', f); continue
        open(p, 'w', encoding='utf-8').write(h2); print(wasm, '→', f)
