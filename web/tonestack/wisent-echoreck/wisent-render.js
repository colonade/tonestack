// Rechen-Thread des Wisent Echoreck: WebAssembly-Kern außerhalb des Audio-Threads.
// Betrieb „link“: der Audio-Thread schickt jeden Eingangsblock {n, b} (b mono, Platz für 2·n), zurück kommt b mit links (0 … n−1) und rechts (n … 2n−1, gleich);
// alle 30 ms geht der Aufnahmepegel {eye} (magisches Auge) an die Seite.
// Betrieb „render“: eine ganze Datei durchrechnen (Download), mit Ausklang (render.tail Sekunden Stille dahinter) und Fortschrittsmeldungen.
importScripts('wisent-worklet.js');
const { WisentWasm, imports } = self.WISENTDSP;
let eng = null, last = null, fs = 48000, eyN = 0;
onmessage = async e => {
  const m = e.data;
  if (m.params) { last = m.params; if (eng) eng.setTarget(m.params); }
  if (m.wasm) { fs = m.fs || fs;
    try { const r = await WebAssembly.instantiate(m.wasm, imports); eng = new WisentWasm(r.instance || r, fs); if (last) eng.setTarget(last); postMessage({ wasm: true }); }
    catch (err) { postMessage({ wasm: false, error: String(err) }); }
    if (!m.render) return; }
  if (m.render) {   // ganze Datei: m.render = Float32Array (mono), m.tail = Sekunden Ausklang; Ergebnis {L, R}
    const x = m.render, n = x.length, tail = Math.round(Math.min(60, Math.max(0, +m.tail || 0)) * fs), N = n + tail, L = new Float32Array(N), R = new Float32Array(N), B = 4096, tmp = new Float32Array(B), tl = new Float32Array(B), tr = new Float32Array(B); let last_ = 0;
    for (let p = 0; p < N; p += B) { const e2 = Math.min(N, p + B), len = e2 - p; for (let i = 0; i < len; i++) tmp[i] = p + i < n ? x[p + i] : 0;
      eng.process(tmp, len, tl, tr); L.set(tl.subarray(0, len), p); R.set(tr.subarray(0, len), p);
      const now = Date.now(); if (now - last_ > 200) { last_ = now; postMessage({ progress: Math.min(1, e2 / N) }); } }
    postMessage({ done: { L, R } }, [L.buffer, R.buffer]); return; }
  if (m.link) {
    m.link.onmessage = ev => { const d = ev.data, n = d.n, b = d.b, L = b.subarray(0, n), R = b.subarray(n, 2 * n), x = b.slice(0, n);
      if (eng && eng.ready) eng.process(x, n, L, R); else b.fill(0);
      m.link.postMessage(b, [b.buffer]);
      if (eng && eng.ready && (eyN += n) >= 0.03 * fs) { eyN = 0; postMessage({ eye: eng.eye() }); } };
  }
};
