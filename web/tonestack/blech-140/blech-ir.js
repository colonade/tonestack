// IR-Worker des Blech 140: rechnet die Impulsantwort einer neuen Platte aus den Moden (etwa 0,7 s; WebAssembly-Kern, blech_job_*).
// {wasm, fs} lädt den Kern; {run: t} (DECAY in s) rechnet, Antwort {ir: {L, R, t}} (Float32Array, übertragen). Kommen während der Rechnung neue
// Wünsche, wird danach nur der letzte gerechnet.
const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
let x = null, want = null, done = null, timer = 0;
function work() { timer = 0; if (!x || want === null || want === done) return; const t = want;
  x.blech_job_run(t); const n = x.blech_job_len(), L = new Float32Array(x.memory.buffer, x.blech_job_l(), n).slice(), R = new Float32Array(x.memory.buffer, x.blech_job_r(), n).slice();
  done = t; postMessage({ ir: { L, R, t } }, [L.buffer, R.buffer]); if (want !== done && !timer) timer = setTimeout(work, 0); }
onmessage = async e => { const m = e.data || {};
  if (m.wasm) { try { const r = await WebAssembly.instantiate(m.wasm, imports); x = (r.instance || r).exports; if (x._initialize) x._initialize(); x.blech_job_init(m.fs || 48000); if ('done' in m) done = m.done; postMessage({ ready: true }); }
    catch (err) { postMessage({ ready: false, error: String(err) }); } }
  if ('run' in m && Number.isFinite(+m.run)) { want = Math.min(3.8, Math.max(1, +m.run)); if (!timer) timer = setTimeout(work, 0); } };
