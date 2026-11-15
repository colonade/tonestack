// Prüft den WebAssembly-Kern des Blech 140 (build/blech-core.wasm) gegen den C++-Kern und die Übergabe einer fremd gerechneten Platte:
//   node engine/tests/blechwasm.js build/blech-core.wasm ir_cpp.raw   (ir_cpp.raw aus: blechtest ir 0.357142857142857 48000 ir_cpp.raw, DECAY 2 s)
const fs = require('fs');
const [wasmPath, rawPath] = process.argv.slice(2);
const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
(async () => {
  const bin = fs.readFileSync(wasmPath); let bad = 0;
  const mk = async () => { const r = await WebAssembly.instantiate(bin, imports); const x = r.instance.exports; if (x._initialize) x._initialize(); return x; };
  const a = await mk(), FS = 48000, B = 128;
  let t0 = Date.now(); a.blech_init(FS, 2, 0); const tInit = (Date.now() - t0) / 1000;
  const tg = () => new Float64Array(a.memory.buffer, a.blech_target_buffer(), 16); { const t = tg(); t[0] = 2; t[1] = 1; t[2] = 0.5; t[3] = 0; t[4] = 0; }
  const N = Math.round(8.5 * FS), L = new Float32Array(N), R = new Float32Array(N);
  t0 = Date.now();
  for (let p = 0; p < N; p += B) { const n = Math.min(B, N - p), inp = new Float32Array(a.memory.buffer, a.blech_in_buffer(), n); inp.fill(0); if (p === 0) inp[0] = 1;
    a.blech_process(n); L.set(new Float32Array(a.memory.buffer, a.blech_l_buffer(), n), p); R.set(new Float32Array(a.memory.buffer, a.blech_r_buffer(), n), p); }
  const tRun = (Date.now() - t0) / 1000;
  const raw = new Float32Array(fs.readFileSync(rawPath).buffer.slice(0)); let e = 0, ref = 0;
  for (let i = 0; i < N; i++) { const dl = L[i] - raw[2 * i], dr = R[i] - raw[2 * i + 1]; e += dl * dl + dr * dr; ref += raw[2 * i] ** 2 + raw[2 * i + 1] ** 2; }
  const db = 10 * Math.log10(e / ref + 1e-300); console.log(`Einrichten ${tInit.toFixed(2)} s, Impulsantwort 8,5 s in ${tRun.toFixed(2)} s (${(tRun / 8.5 * 100).toFixed(1)} % Echtzeit); WebAssembly gegen C++: ${db.toFixed(1)} dB`); if (!(db < -100)) bad++;
  // zweite Instanz rechnet die Platte DECAY 3,8 s (wie der IR-Worker der Seite), Übergabe an die erste
  const b = await mk(); t0 = Date.now(); b.blech_job_init(FS); b.blech_job_run(3.8); const tJob = (Date.now() - t0) / 1000; const len = b.blech_job_len();
  if (len !== a.blech_ir_len()) { console.log('Länge verschieden', len, a.blech_ir_len()); bad++; }
  new Float32Array(a.memory.buffer, a.blech_offer_l(), len).set(new Float32Array(b.memory.buffer, b.blech_job_l(), len));
  new Float32Array(a.memory.buffer, a.blech_offer_r(), len).set(new Float32Array(b.memory.buffer, b.blech_job_r(), len));
  a.blech_offer(3.8); { const t = tg(); t[0] = 3.8; } let k = 0, seenChange = false; const inp = () => new Float32Array(a.memory.buffer, a.blech_in_buffer(), B);
  for (; k < 2 * FS / B; k++) { const x = inp(); for (let i = 0; i < B; i++) x[i] = Math.random() - 0.5; a.blech_process(B); if (a.blech_changing()) seenChange = true; if (a.blech_shown() > 3.79 && !a.blech_changing()) break; }
  console.log(`IR-Worker: Platte 3,8 s in ${tJob.toFixed(2)} s gerechnet; eingebaut nach ${(k * B / FS).toFixed(2)} s, Umbau sichtbar ${seenChange}, Zeiger ${a.blech_shown().toFixed(2)} s`);
  if (!seenChange || a.blech_shown() < 3.79 || k * B / FS > 1) bad++;
  // dieselbe Platte direkt im C++-Modell der Anzeige
  console.log(`Modell: T60(125 Hz, 2 s) = ${a.blech_t60(125, 2).toFixed(3)} s, T60(4 kHz, 2 s) = ${a.blech_t60(4000, 2).toFixed(3)} s`);
  console.log(bad ? 'FEHLER' : 'fertig'); process.exit(bad ? 1 : 0);
})();
