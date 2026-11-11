// Rechen-Thread des Wien-EQ: WebAssembly-Kern außerhalb des Audio-Threads.
// Betrieb „link": der Audio-Thread schickt jeden Eingangsblock über einen eigenen Kanal, der Block kommt gerechnet im selben Puffer zurück.
// Betrieb „render": eine ganze Datei durchrechnen (Download), mit Fortschrittsmeldungen.
importScripts('weq-worklet.js');
const { WeqWasm, imports } = self.WEQDSP;
let eng = null, last = null, fs = 48000;
onmessage = async e => {
  const m = e.data;
  if (m.params) { last = m.params; if (eng) { eng.setTarget(m.params); postMessage({ ready: true, latency: eng.x.weq_latency() }); } }
  if (m.wasm) { fs = m.fs || fs;
    try { const r = await WebAssembly.instantiate(m.wasm, imports); eng = new WeqWasm(r.instance || r, fs); if (last) eng.setTarget(last); postMessage({ wasm: true, latency: eng.x.weq_latency() }); }
    catch (err) { postMessage({ wasm: false, error: String(err) }); }
    if (!m.render) return; }
  if (m.render) {   // ganze Datei: m.render = Float32Array (mono), Ergebnis gleich lang, um die Latenz verschoben
    const x = m.render, n = x.length, y = new Float32Array(n), lat = eng.x.weq_latency(), B = 4096, tmp = new Float32Array(B); let last_ = 0;
    for (let p = 0; p < n + lat; p += B) { const e2 = Math.min(n + lat, p + B), len = e2 - p; for (let i = 0; i < len; i++) tmp[i] = p + i < n ? x[p + i] : 0;
      eng.process(tmp, len); for (let i = 0; i < len; i++) { const j = p + i - lat; if (j >= 0 && j < n) y[j] = tmp[i]; }
      const now = Date.now(); if (now - last_ > 200) { last_ = now; postMessage({ progress: Math.min(1, e2 / n) }); } }
    postMessage({ done: y }, [y.buffer]); return; }
  if (m.link) {
    m.link.onmessage = ev => { const b = ev.data, n = b.length;
      if (eng && eng.ready) eng.process(b, n); else b.fill(0);
      m.link.postMessage(b, [b.buffer]); };
  }
};
