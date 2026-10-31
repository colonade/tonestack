// Rechen-Thread des Laney-Simulators: WebAssembly-Kern außerhalb des Audio-Threads.
// Der Audio-Thread (laney-worklet.js, Betrieb „link") schickt jeden Eingangsblock über einen eigenen Kanal; der Block kommt
// gerechnet im selben Puffer zurück. Einstellungen kommen vom Hauptthread; Neuaufbau des Kerns blockiert so nie die Ausgabe.
importScripts('laney-worklet.js');
const { WasmEngine } = self.LaneyDSP;
let eng = null, last = null, fs = 48000, rep = 0;
const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
onmessage = async e => {
  const m = e.data;
  if (m.params) { last = m.params; if (eng) eng.setTarget(m.params); }
  if (m.wasm) {
    fs = m.fs || fs;
    try { const r = await WebAssembly.instantiate(m.wasm, imports); eng = new WasmEngine(r.instance || r, fs); if (last) eng.setTarget(last); postMessage({ wasm: true }); }
    catch (err) { postMessage({ wasm: false, error: String(err) }); }
    return;
  }
  if (m.link) {
    m.link.onmessage = ev => { const b = ev.data, n = b.length;   // n vor dem Zurückgeben merken: danach ist der Puffer übertragen (Länge 0)
      if (eng && eng.ready) eng.process(b, n); else b.fill(0);
      m.link.postMessage(b, [b.buffer]);
      if (eng && eng.ready && (rep += n) >= fs / 10) { rep = 0; postMessage({ peaks: eng.takePeaks() }); } };
    return;
  }
};
