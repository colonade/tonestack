// Wisent Echoreck: Audio-Thread. Gerechnet wird im Rechen-Thread (wisent-render.js, WebAssembly-Kern des Echogeräts);
// der Audio-Thread schickt jeden Eingangsblock (mono) dorthin und spielt das Ergebnis (mono, auf beide Kanäle) mit Vorlauf ab.
// Bypass mit Pegelangleich wie bei den anderen Seiten. Nach dem Anhalten der Datei laufen die Echos aus (so lange, wie params.tail sagt; hold: unbegrenzt,
// die Schleife schwingt). Ohne Rechen-Thread (sehr alte Browser) rechnet der Audio-Thread selbst und meldet das magische Auge alle 30 ms.
(() => {
  // WebAssembly-Kern (engine/wasm/wisent_wasm.cpp); Ausgang mono, auf beide Kanäle
  class WisentWasm {
    constructor(instance, fs) { this.x = instance.exports; if (this.x._initialize) this.x._initialize(); this.fs = fs; this.ready = false; }
    // p = { t: Zielwerte wie ws_target_buffer (20 Zahlen: Regler 0 … 1, Betriebsart, Köpfe, Volt bei 0 dBFS, OUTPUT dB, Netzfrequenz, DRY 0 … 1, Varispeed, WOBBLE 0 … 1), tail (s), hold }
    setTarget(p) { if (!p || !Array.isArray(p.t)) return; const t = new Float64Array(this.x.memory.buffer, this.x.ws_target_buffer(), 24);
      for (let i = 0; i < 20; i++) { const v = +p.t[i]; t[i] = Number.isFinite(v) ? v : NaN; }   // NaN: Kern nimmt den Standard
      if (!this.ready) { this.x.ws_init(this.fs); this.ready = true; } }   // Ruhezustand beim ersten Mal (etwa 0,1 s); danach gleiten die Regler (70 ms), Tasten sofort
    reset() { if (this.ready) this.x.ws_reset(); }
    eye() { return this.ready ? this.x.ws_eye() : 0; }
    // buf: n Werte mono → L, R (je n, gleich)
    process(buf, n, L, R) { new Float32Array(this.x.memory.buffer, this.x.ws_buffer(), n).set(buf.subarray(0, n)); this.x.ws_process(n);
      const y = new Float32Array(this.x.memory.buffer, this.x.ws_buffer(), n); L.set(y); R.set(y); }
  }
  const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
  const api = { WisentWasm, imports };
  if (typeof registerProcessor === 'function') {
    class WisentProcessor extends AudioWorkletProcessor {
      constructor(options) {
        super(); this.bypass = false; this.mix = 1; this.match = true; this.msDry = 0; this.msAmp = 0; this.gMatch = 1; this.avg = 1 / sampleRate; this.idle = 0;
        const init = options && options.processorOptions; this.last = init && init.params || null;
        if (init && 'bypass' in init) { this.bypass = !!init.bypass; this.mix = this.bypass ? 0 : 1; } if (init && 'match' in init) this.match = !!init.match;
        this.weng = null; this.mono = new Float32Array(128); this.Lb = new Float32Array(128); this.Rb = new Float32Array(128);
        this.port.onmessage = e => { const m = e.data;
          if (m.link) { this.linkUp(m.link, m.prefill); return; }
          if (m.params) { this.last = m.params; this.idle = this.tailN(); if (this.weng) this.weng.setTarget(m.params); }
          if ('bypass' in m) this.bypass = !!m.bypass; if ('match' in m) this.match = !!m.match;
          if (m.wasm) WebAssembly.instantiate(m.wasm, imports).then(r => { this.weng = new WisentWasm(r.instance || r, sampleRate); if (this.last) this.weng.setTarget(this.last); this.port.postMessage({ wasm: true }); })
            .catch(err => this.port.postMessage({ wasm: false, error: String(err) })); };
      }
      // Ausklang nach dem Ende des Eingangs in Abtastwerten (hold: unbegrenzt)
      tailN() { const p = this.last || {}; if (p.hold) return Infinity; const t = Number.isFinite(+p.tail) ? Math.min(60, Math.max(0.5, +p.tail)) : 5; return Math.round(t * sampleRate); }
      linkUp(port, prefill) {
        const RING = 1 << 16; this.link = port; this.ringL = new Float32Array(RING); this.ringR = new Float32Array(RING); this.ringD = new Float32Array(RING); this.rmask = RING - 1;
        this.nS = 0; this.nR = 0; this.nP = 0; this.running = false; this.fade = 0; this.prefill = Math.max(256, Math.round(prefill || 0.06 * sampleRate)); this.pool = []; this.inc = Math.round(0.04 * sampleRate); this.xrun = false;
        port.onmessage = ev => { const b = ev.data; if (!(b instanceof Float32Array)) return; const n = b.length >> 1, L = this.ringL, R = this.ringR, m = this.rmask;
          for (let i = 0; i < n; i++) { L[(this.nR + i) & m] = b[i]; R[(this.nR + i) & m] = b[n + i]; } this.nR += n; if (this.pool.length < 64) this.pool.push(b); };
      }
      processLinked(inp, out) {
        const len = out[0].length, m = this.rmask, D = this.ringD, WL = this.ringL, WR = this.ringR; let send = true;
        if (!inp || inp.length === 0) { if (this.idle > 0) { this.idle -= len; inp = null; } else send = false; } else this.idle = this.tailN();
        if (send && this.nS - this.nP + len <= m) { let b = this.pool.pop(); if (!b || b.length !== 2 * len) b = new Float32Array(2 * len);   // Rückweg braucht 2·len
          for (let n = 0; n < len; n++) { let u = 0; if (inp) { for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length; } b[n] = u; D[(this.nS + n) & m] = u; }
          this.nS += len; this.link.postMessage({ n: len, b }, [b.buffer]); }
        const avail = this.nR - this.nP;
        if (!this.running && avail > 0 && (avail >= (this.xrun ? this.inc : this.prefill) || (!send && this.nS === this.nR))) { this.running = true; this.xrun = false; }
        if (!this.running) { for (const ch of out) ch.fill(0); return true; }
        const k = Math.min(len, avail), target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate), fstep = 1 / (0.005 * sampleRate);
        if (k < len && send && this.nS - this.nR > 0) { this.prefill = Math.min(Math.round(0.5 * sampleRate), this.prefill + this.inc); this.xrun = true; this.port.postMessage({ xrun: this.prefill / sampleRate }); }
        let mix = this.mix, md = this.msDry, ma = this.msAmp; const kk = this.avg, gd = this.match ? this.gMatch : 1; let f = this.fade; const fo = k < len ? Math.max(0, k - 32) : len;
        const o0 = out[0], o1 = out.length > 1 ? out[1] : null;
        for (let n = 0; n < len; n++) { let yl = 0, yr = 0, u = 0;
          if (n < k) { const i = (this.nP + n) & m; yl = WL[i]; yr = WR[i]; u = D[i]; f = n >= fo ? Math.min(f, (k - n) / 32) : Math.min(1, f + fstep); } else f = 0;
          md += kk * (u * u - md); ma += kk * (0.5 * (yl * yl + yr * yr) - ma);
          if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const dd = (1 - mix) * gd * u; if (o1) { o0[n] = f * (mix * yl + dd); o1[n] = f * (mix * yr + dd); } else o0[n] = f * (mix * 0.5 * (yl + yr) + dd); }
        this.nP += k; this.fade = f; this.mix = mix; this.msDry = md; this.msAmp = ma; if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md)));
        if (k < len) this.running = false; return true;
      }
      process(inputs, outputs) {
        if (this.link) return this.processLinked(inputs[0], outputs[0]);
        const out = outputs[0], inp = inputs[0], len = out[0].length;
        if (!this.weng || !this.weng.ready || ((!inp || !inp.length) && this.idle <= 0)) { for (const ch of out) ch.fill(0); return true; }
        if (!inp || !inp.length) this.idle -= len; else this.idle = this.tailN();
        if (this.mono.length < len) { this.mono = new Float32Array(len); this.Lb = new Float32Array(len); this.Rb = new Float32Array(len); }
        const mo = this.mono; for (let n = 0; n < len; n++) { let u = 0; if (inp && inp.length) { for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length; } mo[n] = u; }
        this.weng.process(mo, len, this.Lb, this.Rb); if ((this.eyN = (this.eyN || 0) + len) >= 0.03 * sampleRate) { this.eyN = 0; this.port.postMessage({ eye: this.weng.eye() }); }
        let mix = this.mix, md = this.msDry, ma = this.msAmp; const k = this.avg, gd = this.match ? this.gMatch : 1, target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate);
        const L = this.Lb, R = this.Rb, o0 = out[0], o1 = out.length > 1 ? out[1] : null;
        for (let n = 0; n < len; n++) { const u = mo[n]; md += k * (u * u - md); ma += k * (0.5 * (L[n] * L[n] + R[n] * R[n]) - ma); if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const dd = (1 - mix) * gd * u; if (o1) { o0[n] = mix * L[n] + dd; o1[n] = mix * R[n] + dd; } else o0[n] = mix * 0.5 * (L[n] + R[n]) + dd; }
        this.mix = mix; this.msDry = md; this.msAmp = ma; if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md))); return true;
      }
    }
    registerProcessor('wisent-echo', WisentProcessor);
  } else if (typeof module !== 'undefined') module.exports = api;
  else globalThis.WISENTDSP = api;
})();
