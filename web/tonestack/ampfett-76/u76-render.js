// Rechen-Thread des AmpFett-76-Simulators: WebAssembly-Kern außerhalb des Audio-Threads.
// Betrieb „link": der Audio-Thread schickt jeden Eingangsblock über einen eigenen Kanal, der Block kommt gerechnet im selben Puffer zurück.
// Betrieb „render": eine ganze Datei durchrechnen (Download), mit Fortschrittsmeldungen.
// Betrieb „plot": nur die Kennlinie rechnen (eigener Thread der Seite, damit Einschwingen und Rechnen weder Bedienung noch Ton aufhalten).
importScripts('u76-worklet.js');
const { U76Wasm, imports } = self.U76DSP;
let eng = null, last = null, fs = 48000, rep = 0;
onmessage = async e => {
  const m = e.data;
  if (m.plot) { if (!eng) { postMessage({ curve: null, id: m.plot.id }); return; } const r = eng.plot(m.plot.params, m.plot.levels); postMessage({ curve: r.out, gr: r.gr, id: m.plot.id }); return; }
  if (m.params) { last = m.params; if (eng) { eng.setTarget(m.params); postMessage({ ready: true, latency: eng.x.u7_latency() }); } }
  if (m.wasm) { fs = m.fs || fs;
    try { const r = await WebAssembly.instantiate(m.wasm, imports); eng = new U76Wasm(r.instance || r, fs); if (last && !m.plotOnly) eng.setTarget(last); postMessage({ wasm: true, latency: eng.x.u7_latency() }); }
    catch (err) { postMessage({ wasm: false, error: String(err) }); }
    if (!m.render) return; }
  if (m.render) {   // ganze Datei: m.render = Float32Array (mono), Ergebnis gleich lang, um die Latenz verschoben
    const x = m.render, n = x.length, y = new Float32Array(n), lat = eng.x.u7_latency(), B = 4096, tmp = new Float32Array(B); let last_ = 0;
    for (let p = 0; p < n + lat; p += B) { const e2 = Math.min(n + lat, p + B), len = e2 - p; for (let i = 0; i < len; i++) tmp[i] = p + i < n ? x[p + i] : 0;
      eng.process(tmp, len); for (let i = 0; i < len; i++) { const j = p + i - lat; if (j >= 0 && j < n) y[j] = tmp[i]; }
      const now = Date.now(); if (now - last_ > 200) { last_ = now; postMessage({ progress: Math.min(1, e2 / n) }); } }
    postMessage({ done: y }, [y.buffer]); return; }
  if (m.link) {
    m.link.onmessage = ev => { const b = ev.data, n = b.length;
      if (eng && eng.ready) eng.process(b, n); else b.fill(0);
      m.link.postMessage(b, [b.buffer]);
      if (eng && eng.ready && (rep += n) >= fs / 10) { rep = 0; postMessage({ peaks: eng.takePeaks() }); } };
  }
};
