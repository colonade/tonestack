// Rechen-Thread des Blech 140: WebAssembly-Kern der Hallplatte außerhalb des Audio-Threads.
// Betrieb „link“: der Audio-Thread schickt jeden Eingangsblock {n, b} (b mono, Platz für 2·n), zurück kommt b mit links (0 … n−1) und rechts (n … 2n−1);
// alle 30 ms geht der Zustand der eingebauten Platte {plate: {shown, changing}} an die Seite. Neue Platten kommen von der Seite als {offer: {L, R, t}}.
// Betrieb „render“: eine ganze Datei durchrechnen (Download), mit Ausklang (render.tail Sekunden Stille dahinter) und Fortschrittsmeldungen.
importScripts('blech-worklet.js');
const { BlechWasm, imports } = self.BLECHDSP;
let eng = null, last = null, fs = 48000, internal = false, stN = 0;
onmessage = async e => {
  const m = e.data;
  if ('internal' in m) { internal = !!m.internal; if (eng) eng.setInternal(internal); }
  if (m.params) { last = m.params; if (eng) eng.setTarget(m.params); }
  if (m.offer && eng) eng.offer(m.offer.L, m.offer.R, m.offer.t);
  if (m.wasm) { fs = m.fs || fs;
    try { const r = await WebAssembly.instantiate(m.wasm, imports); eng = new BlechWasm(r.instance || r, fs, internal); if (last) eng.setTarget(last); postMessage({ wasm: true }); }
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
      if (eng && eng.ready && (stN += n) >= 0.03 * fs) { stN = 0; postMessage({ plate: eng.state() }); } };
  }
};
