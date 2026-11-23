// Cultec EQ-1A: Audio-Thread. Gerechnet wird im Rechen-Thread (cultec-render.js, WebAssembly-Kern der ganzen Schaltung);
// der Audio-Thread schickt jeden Eingangsblock dorthin und spielt das Ergebnis mit Vorlauf ab (Bypass mit Pegelangleich wie bei den anderen Seiten).
// Ohne Rechen-Thread (sehr alte Browser) rechnet der Audio-Thread selbst.
(() => {
  // WebAssembly-Kern (engine/wasm/cultec_wasm.cpp)
  class CtWasm {
    constructor(instance, fs) { this.x = instance.exports; if (this.x._initialize) this.x._initialize(); this.fs = fs; this.ready = false; this.os = 0; this.lat = this.x.ct_latency(); }
    // p = { lb, la, hb, hc, bw 0 … 10, lf 0 … 3, hf 0 … 6, hcs 0 … 2, inn, level dBu für 0 dBFS, os 1/2/4, md 0 EQP-1A / 1 EQP-1R, lfr 0 … 2, hfr 0 … 4 (Wahlschalter des EQP-1R) }
    setTarget(p) { if (!p) return; const t = new Float64Array(this.x.memory.buffer, this.x.ct_target_buffer(), 16), num = (v, d, lo, hi) => Number.isFinite(+v) ? Math.max(lo, Math.min(hi, +v)) : d;
      t[0] = num(p.lb, 0, 0, 10); t[1] = num(p.la, 0, 0, 10); t[2] = Math.round(num(p.lf, 2, 0, 3)); t[3] = num(p.hb, 0, 0, 10); t[4] = num(p.bw, 5, 0, 10); t[5] = Math.round(num(p.hf, 2, 0, 6));
      t[6] = num(p.hc, 0, 0, 10); t[7] = Math.round(num(p.hcs, 1, 0, 2)); t[8] = p.inn === false ? 0 : 1; t[9] = num(p.level, 18, 0, 30);
      const md = p.md === 1 ? 1 : 0; t[10] = md; t[11] = Math.round(num(p.lfr, 1, 0, 2)); t[12] = Math.round(num(p.hfr, 1, 0, 4));
      const os = [1, 2, 4].includes(p.os) ? p.os : 2;
      if (!this.ready || os !== this.os || md !== this.md) { this.x.ct_init(this.fs, os); this.os = os; this.md = md; this.ready = true; } else this.x.ct_set_target(); }   // Regler gleiten (15 ms), Schalter sofort; andere Überabtastung oder anderes Modell: neu eingeschwungen
    process(buf, n) { const b = new Float32Array(this.x.memory.buffer, this.x.ct_buffer(), n); b.set(buf.subarray(0, n)); this.x.ct_process(n); buf.set(new Float32Array(this.x.memory.buffer, this.x.ct_buffer(), n)); }
  }
  const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
  const api = { CtWasm, imports };
  if (typeof registerProcessor === 'function') {
    class CtProcessor extends AudioWorkletProcessor {
      constructor(options) {
        super(); this.bypass = false; this.mix = 1; this.match = true; this.msDry = 0; this.msAmp = 0; this.gMatch = 1; this.avg = 1 / sampleRate; this.idle = 0;
        const init = options && options.processorOptions; this.last = init && init.params || null;
        if (init && 'bypass' in init) { this.bypass = !!init.bypass; this.mix = this.bypass ? 0 : 1; } if (init && 'match' in init) this.match = !!init.match;
        this.weng = null; this.mono = new Float32Array(128); this.dryb = new Float32Array(128); this.lat = 0; this.dl = new Float32Array(4096); this.dli = 0;   // dl: Verzögerung des trockenen Signals um die Laufzeit des Kerns
        this.port.onmessage = e => { const m = e.data;
          if (m.link) { this.linkUp(m.link, m.prefill); this.lat = m.lat | 0; return; }
          if (m.params) { this.last = m.params; this.idle = Math.round(0.5 * sampleRate); if (this.weng) this.weng.setTarget(m.params); }
          if ('bypass' in m) this.bypass = !!m.bypass; if ('match' in m) this.match = !!m.match;
          if (m.wasm) WebAssembly.instantiate(m.wasm, imports).then(r => { this.weng = new CtWasm(r.instance || r, sampleRate); this.lat = this.weng.lat; if (this.last) this.weng.setTarget(this.last); this.port.postMessage({ wasm: true }); })
            .catch(err => this.port.postMessage({ wasm: false, error: String(err) })); };
      }
      linkUp(port, prefill) {
        const RING = 1 << 16; this.link = port; this.ringW = new Float32Array(RING); this.ringD = new Float32Array(RING); this.rmask = RING - 1;
        this.nS = 0; this.nR = 0; this.nP = 0; this.running = false; this.fade = 0; this.prefill = Math.max(256, Math.round(prefill || 0.06 * sampleRate)); this.pool = []; this.inc = Math.round(0.04 * sampleRate); this.xrun = false;
        port.onmessage = ev => { const b = ev.data; if (!(b instanceof Float32Array)) return; const w = this.ringW, m = this.rmask;
          for (let i = 0; i < b.length; i++) w[(this.nR + i) & m] = b[i]; this.nR += b.length; if (this.pool.length < 64) this.pool.push(b); };
      }
      processLinked(inp, out) {
        const len = out[0].length, m = this.rmask, D = this.ringD, W = this.ringW; let send = true;
        if (!inp || inp.length === 0) { if (this.idle > 0) { this.idle -= len; inp = null; } else send = false; } else this.idle = Math.round(0.5 * sampleRate);
        if (send && this.nS - this.nP + len <= m) { let b = this.pool.pop(); if (!b || b.length !== len) b = new Float32Array(len);
          for (let n = 0; n < len; n++) { let u = 0; if (inp) { for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length; } b[n] = u; D[(this.nS + n) & m] = u; }
          this.nS += len; this.link.postMessage(b, [b.buffer]); }
        const avail = this.nR - this.nP;
        if (!this.running && avail > 0 && (avail >= (this.xrun ? this.inc : this.prefill) || (!send && this.nS === this.nR))) { this.running = true; this.xrun = false; }
        if (!this.running) { for (const ch of out) ch.fill(0); return true; }
        const k = Math.min(len, avail), target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate), fstep = 1 / (0.005 * sampleRate);
        if (k < len && send && this.nS - this.nR > 0) { this.prefill = Math.min(Math.round(0.5 * sampleRate), this.prefill + this.inc); this.xrun = true; this.port.postMessage({ xrun: this.prefill / sampleRate }); }
        let mix = this.mix, md = this.msDry, ma = this.msAmp; const kk = this.avg, gd = this.match ? this.gMatch : 1; let f = this.fade; const fo = k < len ? Math.max(0, k - 32) : len;
        for (let n = 0; n < len; n++) { let y = 0, u = 0;
          if (n < k) { const i = (this.nP + n) & m; y = W[i]; u = D[(this.nP + n - this.lat) & m]; f = n >= fo ? Math.min(f, (k - n) / 32) : Math.min(1, f + fstep); } else f = 0;
          md += kk * (u * u - md); ma += kk * (y * y - ma);
          if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const o = f * (mix * y + (1 - mix) * gd * u); for (let c = 0; c < out.length; c++) out[c][n] = o; }
        this.nP += k; this.fade = f; this.mix = mix; this.msDry = md; this.msAmp = ma; if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md)));
        if (k < len) this.running = false; return true;
      }
      process(inputs, outputs) {
        if (this.link) return this.processLinked(inputs[0], outputs[0]);
        const out = outputs[0], inp = inputs[0], len = out[0].length;
        if (!this.weng || !this.weng.ready || ((!inp || !inp.length) && this.idle <= 0)) { for (const ch of out) ch.fill(0); return true; }
        if (!inp || !inp.length) this.idle -= len; else this.idle = Math.round(0.5 * sampleRate);
        if (this.mono.length < len) { this.mono = new Float32Array(len); this.dryb = new Float32Array(len); }
        const mo = this.mono, dry = this.dryb; for (let n = 0; n < len; n++) { let u = 0; if (inp && inp.length) { for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length; } mo[n] = u; const L = this.lat; if (L > 0) { this.dl[(this.dli + L) & 4095] = u; dry[n] = this.dl[this.dli]; this.dli = (this.dli + 1) & 4095; } else dry[n] = u; }
        this.weng.process(mo, len); let mix = this.mix, md = this.msDry, ma = this.msAmp; const k = this.avg, gd = this.match ? this.gMatch : 1, target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate);
        for (let n = 0; n < len; n++) { md += k * (dry[n] * dry[n] - md); ma += k * (mo[n] * mo[n] - ma); if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const o = mix * mo[n] + (1 - mix) * gd * dry[n]; for (let c = 0; c < out.length; c++) out[c][n] = o; }
        this.mix = mix; this.msDry = md; this.msAmp = ma; if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md))); return true;
      }
    }
    registerProcessor('cultec-eq1a', CtProcessor);
  } else if (typeof module !== 'undefined') module.exports = api;
  else globalThis.CTDSP = api;
})();
