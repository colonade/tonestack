// Prüft den WebAssembly-Kern des Wisent Echoreck (build/wisent-core.wasm) gegen den C++-Kern (wisenttest wav) und die Anzeigen gegen die Rechnung:
//   wisenttest wav in.f32 cpp.f32 1 0.25 0.5 0.8 0100 0100   (REPEAT, Kopf 2), dann
//   node engine/tests/wisentwasm.js build/wisent-core.wasm in.f32 cpp.f32
const fs = require('fs');
const [wasmPath, inPath, cppPath] = process.argv.slice(2);
const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
(async () => {
  const bin = fs.readFileSync(wasmPath); let bad = 0;
  const r = await WebAssembly.instantiate(bin, imports); const x = r.instance.exports; if (x._initialize) x._initialize();
  const FS = 48000, B = 128, tg = () => new Float64Array(x.memory.buffer, x.ws_target_buffer(), 24);
  { const t = tg(); t.fill(0); t[0] = 0.25; t[1] = 0.5; t[2] = 0.8; t[3] = 0.5; t[4] = 0.5; t[5] = 1; t[7] = 1; t[11] = 1; t[14] = 0.2; t[15] = 0; t[16] = 0; t[17] = 0.67; t[18] = 1; }
  let t0 = Date.now(); x.ws_init(FS); const tInit = (Date.now() - t0) / 1000;
  const inp = new Float32Array(fs.readFileSync(inPath).buffer.slice(0)), N = inp.length + 3 * FS, y = new Float32Array(N);
  t0 = Date.now();
  for (let p = 0; p < N; p += B) { const n = Math.min(B, N - p), b = new Float32Array(x.memory.buffer, x.ws_buffer(), n); for (let i = 0; i < n; i++) b[i] = p + i < inp.length ? inp[p + i] : 0; x.ws_process(n); y.set(new Float32Array(x.memory.buffer, x.ws_buffer(), n), p); }
  const tRun = (Date.now() - t0) / 1000;
  const cpp = new Float32Array(fs.readFileSync(cppPath).buffer.slice(0)); let e = 0, ref = 0; if (cpp.length !== N) { console.log(`Länge ${cpp.length} statt ${N}`); bad++; }
  for (let i = 0; i < Math.min(N, cpp.length); i++) { const d = y[i] - cpp[i]; e += d * d; ref += cpp[i] * cpp[i]; }
  const db = 10 * Math.log10(e / ref + 1e-300); console.log(`Einrichten ${tInit.toFixed(2)} s, ${(N / FS).toFixed(1)} s Signal in ${tRun.toFixed(2)} s (${(tRun / (N / FS) * 100).toFixed(1)} % Echtzeit); WebAssembly gegen C++: ${db.toFixed(1)} dB`);
  if (!(db < -90)) bad++;   // Unterschiede der Mathematik-Bibliotheken (exp, tanh) laufen durch Newton und Rückkopplung
  console.log(`Latenz ${x.ws_latency()} (Soll 3), Kopfabstand ${(x.ws_tau1() * 1000).toFixed(2)} ms (Soll 74,40), Auge ${x.ws_eye().toFixed(3)}`); if (x.ws_latency() !== 3 || Math.abs(x.ws_tau1() - 0.0744) > 1e-4) bad++;
  // Echomuster: REPEAT Kopf 2 → Echos bei 2, 4, 6 … (·τ1), dazwischen nur Übersprechen der abgeschalteten Köpfe über die Swell-Schiene W (TSW offen: etwa 36 dB unter dem Echo);
  // Verhältnis aufeinanderfolgender Wiederholungen ≈ Schleifenverstärkung bei 1 kHz
  x.ws_pattern(1000, 12); const P = new Float64Array(x.memory.buffer, x.ws_pattern_buffer(), 12); const pd = Array.from(P, v => (20 * Math.log10(v + 1e-30)).toFixed(1));
  console.log('Echomuster (dB): ' + pd.join(' ')); if (!(P[1] < 0.03 * P[2] && P[3] < 0.03 * P[2] && P[2] > 0.05 && P[4] < P[2])) bad++;
  const fr = new Float64Array(x.memory.buffer, x.ws_freq_buffer(), 3); fr[0] = 100; fr[1] = 1000; fr[2] = 8000; x.ws_plot(3, 1, 1); const R = new Float64Array(x.memory.buffer, x.ws_resp_buffer(), 9);
  const loop1k = R[7], ratio = 20 * Math.log10(P[4] / P[2]); console.log(`Frequenzgang (dB) trocken ${R[0].toFixed(1)}/${R[1].toFixed(1)}/${R[2].toFixed(1)}, Echo ${R[3].toFixed(1)}/${R[4].toFixed(1)}/${R[5].toFixed(1)}, Schleife ${R[6].toFixed(1)}/${R[7].toFixed(1)}/${R[8].toFixed(1)} (100 Hz/1 kHz/8 kHz); Schleife 1 kHz gegen Muster ${loop1k.toFixed(2)} / ${ratio.toFixed(2)}`);
  if (Math.abs(loop1k - ratio) > 0.2 || Math.abs(R[4] - 20 * Math.log10(P[2])) > 0.01 || Math.abs(R[1]) > 0.05) bad++;
  console.log(bad ? `FEHLER: ${bad}` : 'alle Prüfungen bestanden'); process.exit(bad ? 1 : 0);
})();
