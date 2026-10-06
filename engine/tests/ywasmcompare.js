// Vergleicht zwei Yamaha-WebAssembly-Kerne im Klang: node ywasmcompare.js a.wasm b.wasm (Bauteilwerte aus der Web-Seite)
const fs = require('fs'), path = require('path'), vm = require('vm');
const R = path.join(__dirname, '..', '..'); const h = fs.readFileSync(path.join(R, 'web/tonestack/yamaha-b100ii/index.html'), 'utf8');
const js = h.split('<script>').slice(1).map(p => p.split('</script>')[0]).sort((a, b) => b.length - a.length)[0];
const i = js.indexOf('const PARTS = ['); const PARTS = vm.runInNewContext(js.slice(i + 14, js.indexOf('];', i) + 1)); const parts = PARTS.map(p => p[4] * p[3]);
const load = f => { const x = new WebAssembly.Instance(new WebAssembly.Module(fs.readFileSync(f)), { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } }).exports; if (x._initialize) x._initialize(); return x; };
function run(f, t, N) { const x = load(f), fsr = 48000; x.y_init(fsr); new Float64Array(x.memory.buffer, x.y_parts_buffer(), parts.length).set(parts); new Float64Array(x.memory.buffer, x.y_target_buffer(), 9).set(t); x.y_set_target();
  const out = new Float32Array(N), B = 128; for (let p = 0; p < N; p += B) { const b = new Float32Array(x.memory.buffer, x.y_buffer(), B); for (let k = 0; k < B; k++) b[k] = 0.4 * Math.sin(2 * Math.PI * 82.4 * (p + k) / fsr); x.y_process(B); out.set(new Float32Array(x.memory.buffer, x.y_buffer(), B), p); } return out; }
for (const [name, t] of [['Grundstellung', [0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5]], ['Treble 10, Bass 2, EQ', [1,0.2,0.5,0.8,0.9,0.1,0.5,0.7,0.2]]]) { const a = run(process.argv[2], t, 96000), b = run(process.argv[3], t, 96000); let d = 0, pk = 0; for (let k = 0; k < a.length; k++) { d = Math.max(d, Math.abs(a[k] - b[k])); pk = Math.max(pk, Math.abs(b[k])); }
  console.log(name.padEnd(24) + (d === 0 ? 'bitgleich' : (20 * Math.log10(d / pk)).toFixed(0) + ' dB') + ' (Spitze ' + pk.toFixed(3) + ')'); }
