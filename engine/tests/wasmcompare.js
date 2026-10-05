// Vergleicht zwei Laney-WebAssembly-Kerne im Klang: node wasmcompare.js a.wasm b.wasm
const fs = require('fs'), path = require('path');
const parts = [...fs.readFileSync(path.join(__dirname, '..', 'gen', 'parts_gen.h'), 'utf8').matchAll(/X\((\w+), ([^,]+), ([^)]+)\)/g)].map(m => +m[2]);
const load = f => { const x = new WebAssembly.Instance(new WebAssembly.Module(fs.readFileSync(f)), { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } }).exports; if (x._initialize) x._initialize(); return x; };
const taper = x => { const s = 1 / 0.15 - 1, b = s * s; return (Math.pow(b, x) - 1) / (b - 1); };
function run(f, t, N) { const x = load(f), fsr = 48000; x.aor_init(fsr);
  new Float64Array(x.memory.buffer, x.aor_parts_buffer(), parts.length).set(parts);
  new Float64Array(x.memory.buffer, x.aor_target_buffer(), 17).set(t); x.aor_set_target();
  const out = new Float32Array(N), B = 128;
  for (let p = 0; p < N; p += B) { const b = new Float32Array(x.memory.buffer, x.aor_buffer(), B); for (let i = 0; i < B; i++) b[i] = 0.4 * Math.sin(2 * Math.PI * 82.4 * (p + i) / fsr) * (1 + 0.5 * Math.sin((p + i) / 9000));
    x.aor_process(B); out.set(new Float32Array(x.memory.buffer, x.aor_buffer(), B), p); }
  return out; }
const base = [0.5, 0.5, 0.5, taper(0.5), 0.5, 0.5, 0.5, taper(0.5), 0, 0, 0, 1, 0, 0];
const cases = [['linear', [...base, 1, 0, 2]], ['Röhre 2-fach, 300 mV', [...base, 0.3, 1, 2]], ['Röhre 4-fach, 1 V, Endstufe', [...base.slice(0, 13), 1, 1, 1, 4]], ['Röhre 8-fach, Preamp 2, Boosts', [0.5,0.5,0.5,taper(0.5),0.5,0.5,0.5,taper(0.8),1,1,1,0,0,1, 0.3, 1, 8]]];
for (const [name, t] of cases) { const a = run(process.argv[2], t, 96000), b = run(process.argv[3], t, 96000); let d = 0, p = 0; for (let i = 0; i < a.length; i++) { d = Math.max(d, Math.abs(a[i] - b[i])); p = Math.max(p, Math.abs(b[i])); }
  console.log(name.padEnd(34) + (d === 0 ? 'bitgleich' : (20 * Math.log10(d / p)).toFixed(0) + ' dB') + ' (Spitze ' + p.toFixed(3) + ')'); }
