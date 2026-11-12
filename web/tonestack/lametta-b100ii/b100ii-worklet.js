// Lametta B100-II (PA-Board, Endstufe, Lautsprecher) als Zeitbereichs-Schaltungssimulation (Trapezregel wie SPICE), linear (Kleinsignal).
// Zustand: je Kondensator der "Verlaufsstrom" Ih. Pro Abtastwert: vc = K·Ih + k·u,  Ih' = 2G·vc − Ih,  y = c·Ih + d·u.
// K, k, c, d werden bei Regler-/Schalteränderung aus der Knotenmatrix neu berechnet (LU-Zerlegung).
(() => {
  const VT = 0.02585;
  const BANDCFG = [['cs0','cq0'],['cs1','cq1'],['cs2','cq2'],['cs3','cq3'],['cs4','cq4']];
  // Lametta B100-II, PA-Board: Klangregler, TR102, Volume, TR103/104, TR105 + fünf EQ-Bänder, Ausgang
  function build(v, s, fs) {
    const idx = {}; let n = 0;
    const N = k => k === '0' ? -1 : (k in idx ? idx[k] : (idx[k] = n++));
    const Rs = [], Cs = [], TR = []; let rc = 0;
    const R = (a, b, r) => Rs.push([N(a), N(b), 1 / Math.max(r, 1e-3)]);   // Untergrenze 1 mΩ: vermeidet schlecht konditionierte Gleichungssysteme
    const C = (a, b, c) => Cs.push([N(a), N(b), 2 * c * fs]);
    const RC = (a, b, r, c) => { const m = '_rc' + (rc++); R(a, m, r); C(m, b, c); };
    const bjt = (b, c, e, ic) => TR.push([N(b), N(c), N(e), ic / VT, v.hfe]);
    N('in');                      // Quelle: 1 V hinter dem Drainwiderstand (Thevenin)
    R('in', 'src', v.rs);
    RC('src', 'T', v.rt * (1 - s.t), v.ct);
    R('T', 'W', v.rt * s.t); R('src', 'A', v.rsl);
    C('A', 'W', v.cb); C('A', 'M', v.cm);
    R('W', 'M', v.rb * s.b); R('M', '0', v.rm * s.m);
    RC('T', 'b2', v.rs2, v.cl);
    R('b2', '0', v.rbias + 1 / (1 / 39e3 + 1 / 4.7e3));
    R('e2', '0', v.re); R('c2', '0', v.rc); C('b2', 'c2', v.ccb);
    bjt('b2', 'c2', 'e2', 1.40e-3);
    C('c2', 'vt', 2.2e-6);
    R('vt', 'vw', v.vol * (1 - s.vol)); R('vw', '0', v.vol * s.vol);
    RC('vw', 'b3', 560, 1e-6);
    R('b3', '0', 100e3 + 1 / (1 / 39e3 + 1 / 3.9e3));
    R('e3', '0', 3.3e3); R('c3', '0', 15e3); C('b3', 'c3', 100e-12);
    bjt('b3', 'c3', 'e3', 0.588e-3);
    R('c3', 'b4', 1e-6); R('e4', '0', 10e3); C('b4', 'e4', 1000e-12);
    bjt('b4', '0', 'e4', 2.06e-3);
    C('e4', 'x1', 10e-6); R('x1', '0', 100e3); C('x1', 'b5', 3.3e-6);
    R('b5', '0', 56e3 + 1 / (1 / 33e3 + 1 / 10e3));
    R('e5', '0', 4.7e3); R('c5', '0', 4.7e3); C('b5', 'c5', 100e-12);
    bjt('b5', 'c5', 'e5', 1.306e-3);
    C('c5', 'BA', 47e-6); C('e5', 'BB', 47e-6);
    BANDCFG.forEach(([cs, cq], k) => {
      const p = s['e' + k];
      // G50K mit geerdeter Mittenanzapfung (Schaltplan): Bahn BA–Mitte–BB, Schleifer auf der Hälfte, in der er steht; Mitte = Masse
      const Rq = v.vreq, w = 'w' + k;
      if (p >= 0.5) { R('BA', w, Math.max(Rq * (1 - p), 1)); R(w, '0', Math.max(Rq * (p - 0.5), 1)); R('0', 'BB', Rq / 2); }
      else { R('BA', '0', Rq / 2); R('0', w, Math.max(Rq * (0.5 - p), 1)); R(w, 'BB', Math.max(Rq * p, 1)); }
      C('w' + k, 'n' + k, v[cs]); R('n' + k, 'ge' + k, v.rg); C('n' + k, 'gb' + k, v[cq]);
      R('gb' + k, '0', 110e3); C('gb' + k, 'ge' + k, v.cg); R('ge' + k, '0', 10e3);
      bjt('gb' + k, '0', 'ge' + k, 1.44e-3);
    });
    RC('c5', 'po', 10e3, 4.7e-6); R('po', '0', 100e3);
    R('po', '0', 10e3); C('po', '0', 3.3e-9); C('po', 'o', 1e-6); R('o', '0', 100e3);   // Eingang MA-Board: 10 kΩ und 3300 pF gegen Masse, 1 µF aufs Gate (100 kΩ)
    const M = n + 1, A = new Float64Array(M * M);
    const st = (a, b, g) => { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
    for (const [a, b, g] of Rs) st(a, b, g);
    for (const [a, b, g] of Cs) st(a, b, g);
    for (const [bb, cc, ee, gm, beta] of TR) {
      const add = (row, col, g) => { if (row >= 0 && col >= 0) A[row*M+col] += g; };
      for (const [row, sg] of [[cc,1],[ee,-1]]) { if (row < 0) continue; add(row, bb, sg*gm); add(row, ee, -sg*gm); }
      for (const [row, sg] of [[bb,1],[ee,-1]]) { if (row < 0) continue; add(row, bb, sg*gm/beta); add(row, ee, -sg*gm/beta); }
    }
    const si = idx['in']; A[si*M+n] = 1; A[n*M+si] = 1;
    const piv = new Int32Array(M);
    for (let c = 0; c < M; c++) {
      let p = c, best = -1; for (let r = c; r < M; r++) { const m = Math.abs(A[r*M+c]); if (m > best) { best = m; p = r; } }
      piv[c] = p; if (p !== c) for (let q = 0; q < M; q++) { const t = A[c*M+q]; A[c*M+q] = A[p*M+q]; A[p*M+q] = t; }
      const d = A[c*M+c];
      for (let r = c + 1; r < M; r++) { const f = A[r*M+c] / d; if (f === 0) continue; A[r*M+c] = f; for (let q = c + 1; q < M; q++) A[r*M+q] -= f * A[c*M+q]; }
    }
    const solve = rhs => {
      for (let c = 0; c < M; c++) { const p = piv[c]; if (p !== c) { const t = rhs[c]; rhs[c] = rhs[p]; rhs[p] = t; } }
      for (let r = 1; r < M; r++) { let s2 = rhs[r]; for (let q = 0; q < r; q++) s2 -= A[r*M+q] * rhs[q]; rhs[r] = s2; }
      for (let r = M - 1; r >= 0; r--) { let s2 = rhs[r]; for (let q = r + 1; q < M; q++) s2 -= A[r*M+q] * rhs[q]; rhs[r] = s2 / A[r*M+r]; }
      return rhs;
    };
    const nc = Cs.length, diff = (x, a, b) => (a >= 0 ? x[a] : 0) - (b >= 0 ? x[b] : 0);
    const K = new Float64Array(nc * nc), k = new Float64Array(nc), G2 = new Float64Array(nc), c = new Float64Array(nc);
    const out = idx['o']; let d = 0;
    for (let j = 0; j <= nc; j++) {
      const rhs = new Float64Array(M);
      if (j < nc) { const [a, b] = Cs[j]; if (a >= 0) rhs[a] += 1; if (b >= 0) rhs[b] -= 1; } else rhs[n] = 1;
      const x = solve(rhs);
      for (let i = 0; i < nc; i++) { const vv = diff(x, Cs[i][0], Cs[i][1]); if (j < nc) K[i*nc+j] = vv; else k[i] = vv; }
      if (j < nc) c[j] = x[out]; else d = x[out];
    }
    for (let i = 0; i < nc; i++) G2[i] = 2 * Cs[i][2];
    return { nc, K, k, G2, c, d, mc: [], md: new Float64Array(0) };
  }
  const SMOOTH = ['t','b','m','vol','e0','e1','e2','e3','e4'];
  class Core {
    constructor(fs) { this.fs = fs; this.every = 256; this.sys = null; this.prevSys = null; this.blend = 1; this.cur = null; this.tgt = null; this.v = null; this.cnt = 0; this.Ih = null; this.vc = null; }
    setTarget(m) {
      const s = { ...m.s };
      if (Array.isArray(s.eq)) s.eq.forEach((x, i) => { s['e' + i] = x; });   // EQ-Stellungen als e0…e4, damit sie geglättet und übernommen werden
      const partsChanged = !this.v || Object.keys(m.v).some(k => m.v[k] !== this.v[k]);
      this.tgt = s;
      if (partsChanged || !this.cur) {
        const had = !!this.sys, oldG = had ? this.sys.G2 : null;
        this.v = { ...m.v }; if (!this.cur) this.cur = { ...s };
        this.rebuild();
        if (had) {
          // Bauteiltausch: sofort umschalten (keine Überblendung) und die Kondensatorspannungen erhalten.
          // Zustand Ih = G·v + i; bei neuem G ist Ih' = Ih + (G' − G)·v.
          this.prevSys = null; this.blend = 1;
          if (oldG.length === this.sys.nc) for (let j = 0; j < oldG.length; j++) this.Ih[j] += (this.sys.G2[j] - oldG[j]) / 2 * this.vc[j];
        }
      }
    }
    rebuild() {
      this.prevSys = this.sys; this.blend = this.prevSys ? 0 : 1;
      this.sys = build(this.v, this.cur, this.fs);
      if (!this.Ih || this.Ih.length !== this.sys.nc) { this.Ih = new Float64Array(this.sys.nc); this.vc = new Float64Array(this.sys.nc); } }
    tick() {
      if (!this.tgt || ++this.cnt < this.every) return; this.cnt = 0;
      const a = 1 - Math.exp(-this.every / (0.015 * this.fs)); let moved = false;
      for (const k of SMOOTH) {
        const d = this.tgt[k] - this.cur[k]; if (d === 0) continue; moved = true;
        if (Math.abs(d) > 1e-5) this.cur[k] += d * a; else this.cur[k] = this.tgt[k];
      }
      if (moved) this.rebuild();
    }
    step(u) {
      const S = this.sys, nc = S.nc, Ih = this.Ih, vc = this.vc;
      let y;
      if (this.blend < 1 && this.prevSys && this.prevSys.nc === nc) {
        const a = this.blend, b = 1 - a, P = this.prevSys, K1 = S.K, K0 = P.K;
        y = (a * S.d + b * P.d) * u;
        for (let i = 0; i < nc; i++) { let s1 = S.k[i] * u, s0 = P.k[i] * u; const o = i * nc;
          for (let j = 0; j < nc; j++) { s1 += K1[o + j] * Ih[j]; s0 += K0[o + j] * Ih[j]; }
          vc[i] = a * s1 + b * s0; y += (a * S.c[i] + b * P.c[i]) * Ih[i]; }
        this.blend = Math.min(1, this.blend + 1 / this.every);
      } else {
        const K = S.K; y = S.d * u;
        for (let i = 0; i < nc; i++) { let s1 = S.k[i] * u; const o = i * nc; for (let j = 0; j < nc; j++) s1 += K[o + j] * Ih[j]; vc[i] = s1; y += S.c[i] * Ih[i]; }
      }
      for (let i = 0; i < nc; i++) Ih[i] = S.G2[i] * vc[i] - Ih[i];
      return y;
    }
  }
  // ---------- Endstufe (MA-Board) und Lautsprecher, wie im C++-Kern (yamaha_core.hpp) ----------
  // Kette geeicht an Service Manual Tabelle 3: TR101 8,15-fach, PA-Board und MA-Eingang laut Netz, MA-Board 1,8 MΩ gegen 100 kΩ ∥ (10 kΩ + 1 µF) (199-fach ab 16 Hz);
  // Begrenzung ±42 V (Knie ab 38 V),
  // Ausgangselko 2200 µF an 8 Ω (Hochpass 9 Hz), Vollaussteuerung = 48 V. Lautsprecher: Eminence Legend CB158 (Thiele-Small) im 78-L-Bassreflexgehäuse, 50 Hz.
  const G_TR101 = 8.15, V_KNEE = 38.0, V_LIM = 42.0, V_FS = 48.0;
  const clipMA = x => { const a = Math.abs(x); if (a <= V_KNEE) return x; const y = V_KNEE + (V_LIM - V_KNEE) * Math.tanh((a - V_KNEE) / (V_LIM - V_KNEE)); return x < 0 ? -y : y; };
  const SPK = { Fs: 34.2, Qts: 0.34, Vas: 336, Vb: 78, Fb: 50, Ql: 7, Re: 6.15, Le: 0.33e-3, fc: 4000, Qc: 0.7 };
  function bilinear(bs, as, n, fs) {   // s-Polynome (aufsteigend) → z (absteigend, a[0] = 1)
    const K = 2 * fs, num = [0,0,0,0,0], den = [0,0,0,0,0];
    for (let k = 0; k <= n; k++) { const poly = [1,0,0,0,0]; let deg = 0;
      for (let i = 0; i < k; i++) { for (let j = deg + 1; j >= 1; j--) poly[j] -= poly[j-1]; deg++; }
      for (let i = 0; i < n - k; i++) { for (let j = deg + 1; j >= 1; j--) poly[j] += poly[j-1]; deg++; }
      const pk = Math.pow(K, k); for (let j = 0; j <= n; j++) { num[j] += bs[k] * pk * poly[j]; den[j] += as[k] * pk * poly[j]; } }
    const b = [], a = []; for (let j = 0; j <= n; j++) { b.push(num[j] / den[0]); a.push(den[j] / den[0]); } return { n, b, a, z: [0,0,0,0,0] };
  }
  const iirStep = (F, x) => { const { n, b, a, z } = F; const y = b[0] * x + z[0]; for (let i = 1; i < n; i++) z[i-1] = b[i] * x + z[i] - a[i] * y; z[n-1] = b[n] * x - a[n] * y; return y; };
  const iirReset = F => F.z.fill(0);
  function speakerDesign(fs, m = SPK) {
    const Ts = 1 / (2 * Math.PI * m.Fs), Tb = 1 / (2 * Math.PI * m.Fb), al = m.Vas / m.Vb;
    const lo = bilinear([0, 0, 0, 0, Ts*Ts*Tb*Tb], [1, Ts/m.Qts + Tb/m.Ql, Ts*Ts + Tb*Tb + Ts*Tb/(m.Qts*m.Ql) + al*Tb*Tb, Ts*Ts*Tb/m.Ql + Tb*Tb*Ts/m.Qts, Ts*Ts*Tb*Tb], 4, fs);
    const le = bilinear([1, 0], [1, m.Le / m.Re], 1, fs); const w0 = 2 * Math.PI * m.fc; const cone = bilinear([w0*w0, 0, 0], [w0*w0, w0/m.Qc, 1], 2, fs);
    return { lo, le, cone, step(x) { return iirStep(cone, iirStep(le, iirStep(lo, x))); }, reset() { iirReset(lo); iirReset(le); iirReset(cone); } };
  }
  class YEngine {                     // JS-Ersatz für den WebAssembly-Kern, verhält sich wie YCore::block (yamaha_core.hpp)
    constructor(fs) { this.fs = fs; this.a = new Core(fs); this.vin = 0.1; this.ing = 1; this.pwr = 0; this.spkT = 0; this.ingCur = 1; this.vinCur = -1; this.pwrMix = -1; this.spkMix = -1; this.pkPre = 0;
      this.speaker = speakerDesign(fs); this.hpOut = bilinear([0, 2200e-6 * 8], [1, 2200e-6 * 8], 1, fs); this.ma = maDesign(fs); this.st = 1 / (0.02 * fs); this.kv = 1 - Math.exp(-1 / (0.02 * fs)); }
    get sys() { return this.a.sys; }
    setTarget(m) { this.a.setTarget(m); const s = m.s || {}; this.vin = s.vin > 0 ? s.vin : 0.1; this.ing = s.ing > 0 ? s.ing : 1; this.pwr = s.pwr ? 1 : 0; this.spkT = s.spk ? 1 : 0;
      if (this.vinCur < 0) this.vinCur = this.vin; if (this.pwrMix < 0) this.pwrMix = this.pwr; if (this.spkMix < 0) this.spkMix = this.spkT; }
    tick() { this.a.tick(); }
    step(u) {   // Eingangsteiler, Endstufe und Lautsprecher gleiten in 20 ms (kein Knacken beim Umschalten)
      const st = this.st, ramp = (c, t) => c < t ? Math.min(t, c + st) : c > t ? Math.max(t, c - st) : c;
      this.ingCur = ramp(this.ingCur, this.ing); this.pwrMix = ramp(this.pwrMix, this.pwr); this.spkMix = ramp(this.spkMix, this.spkT); this.vinCur += (this.vin - this.vinCur) * this.kv;
      const lin = this.a.step(u) * this.ingCur; let y = lin;
      const vo = iirStep(this.ma, lin * this.vinCur * G_TR101);   // MA-Board-Filter läuft immer mit
      if (this.pwrMix > 0) { const ab = Math.abs(vo); if (ab > this.pkPre) this.pkPre = ab;
        const yp = iirStep(this.hpOut, clipMA(vo)) / V_FS; y = lin + this.pwrMix * (yp - lin); }
      else iirStep(this.hpOut, 0);
      const ys = this.speaker.step(y); y += this.spkMix * (ys - y);
      if (!Number.isFinite(y)) { this.a.Ih.fill(0); this.a.vc.fill(0); iirReset(this.hpOut); iirReset(this.ma); this.speaker.reset(); return 0; }
      return -y;   // Polarität wie am Eingang (wie im C++-Kern)
    }
    takePeak() { const p = this.pkPre; this.pkPre = 0; return p; }
  }
  // WebAssembly-Kern (C++-Übersetzung desselben Netzes): blockweise Verarbeitung
  const maDesign = fs => { const g0 = 1 + 1.8e6 / 100e3, tg = 10e3 * 1e-6; return bilinear([g0, g0 * tg + 1.8e6 * 1e-6], [1, tg], 1, fs); };
  const PORDER = ["rs", "rsl", "ct", "cb", "cm", "rt", "rb", "rm", "cl", "rs2", "rbias", "ccb", "hfe", "re", "rc", "vol", "vreq", "rg", "cg", "cs0", "cq0", "cs1", "cq1", "cs2", "cq2", "cs3", "cq3", "cs4", "cq4"];
  class WasmEngine {
    constructor(instance, fs) { this.x = instance.exports; if (this.x._initialize) this.x._initialize(); this.x.y_init(fs); this.ready = false;
      if (this.x.y_parts_count() !== PORDER.length) throw new Error('Bauteilliste passt nicht zum WebAssembly-Kern'); }
    f64(ptr, n) { return new Float64Array(this.x.memory.buffer, ptr, n); }
    setTarget(m) { const pb = this.f64(this.x.y_parts_buffer(), PORDER.length); PORDER.forEach((k, i) => pb[i] = m.v[k]);
      const s = m.s, eq = Array.isArray(s.eq) ? s.eq : [s.e0, s.e1, s.e2, s.e3, s.e4];
      const tb = this.f64(this.x.y_target_buffer(), 13); [s.t, s.b, s.m, s.vol, ...eq, s.vin > 0 ? s.vin : 0.1, s.ing > 0 ? s.ing : 1, s.pwr ? 1 : 0, s.spk ? 1 : 0].forEach((x, i) => tb[i] = +x); this.x.y_set_target(); this.ready = true; }
    takePeak() { return this.x.y_take_peak ? this.x.y_take_peak() : 0; }
    process(buf, n) { const b = new Float32Array(this.x.memory.buffer, this.x.y_buffer(), n); b.set(buf.subarray(0, n)); this.x.y_process(n); buf.set(new Float32Array(this.x.memory.buffer, this.x.y_buffer(), n)); }
  }
  const api = { build, YEngine, WasmEngine };
  if (typeof registerProcessor === 'function') {
    class B100Processor extends AudioWorkletProcessor {
      constructor(options) {
        super(); this.eng = new YEngine(sampleRate); this.bypass = false; this.mix = 1; this.rep = 0;
        // Bypass mit Pegelangleich (Effektivwerte, 1 s) und Eingangsteiler HIGH/LOW (ing, über 20 ms geglättet)
        this.match = true; this.msDry = 0; this.msAmp = 0; this.gMatch = 1; this.avg = 1 / (1.0 * sampleRate);
        const init = options && options.processorOptions;
        if (init && init.params) this.eng.setTarget(init.params);
        if (init && 'bypass' in init) { this.bypass = !!init.bypass; this.mix = this.bypass ? 0 : 1; }
        if (init && 'match' in init) this.match = !!init.match;
        this.weng = null; this.last = init && init.params ? init.params : null; this.mono = new Float32Array(128); this.dryb = new Float32Array(128); this.zero = [new Float32Array(128)]; this.idle = 0;
        this.port.onmessage = e => { const m = e.data;
          if (m.wasm) { const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
            WebAssembly.instantiate(m.wasm, imports).then(r => { const inst = r.instance || r; this.weng = new WasmEngine(inst, sampleRate); if (this.last) this.weng.setTarget(this.last); this.port.postMessage({ wasm: true }); })
              .catch(err => this.port.postMessage({ wasm: false, error: String(err) })); return; }
          if (m.params) this.idle = Math.round(0.5 * sampleRate);   // nach jeder Änderung ohne Wiedergabe noch kurz weiterrechnen: Umschalten wird fertig, Zustand kommt zur Ruhe
          if (m.params) { this.last = m.params; if (this.weng) this.weng.setTarget(m.params); else this.eng.setTarget(m.params); }
          if ('bypass' in m) this.bypass = !!m.bypass;
          if ('match' in m) this.match = !!m.match; };
      }
      finish(dry, amp, len) {   // Pegelverhältnis nachführen (nur solange der Klangregler hörbar ist), Mischung mit dem Original
        let md = this.msDry, ma = this.msAmp; const k = this.avg;
        for (let n = 0; n < len; n++) { md += k * (dry[n] * dry[n] - md); ma += k * (amp[n] * amp[n] - ma); }
        this.msDry = md; this.msAmp = ma;
        if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md)));
        return this.match ? this.gMatch : 1;
      }

      process(inputs, outputs) {
        let inp = inputs[0]; const out = outputs[0], eng = this.eng;
        // Ohne Eingang (keine Wiedergabe) noch 0,5 s mit Stille weiterrechnen: Ausklingen statt hartem Schnitt am Ende, laufende Umschaltungen
        // werden fertig und die Schaltung kommt zur Ruhe, sodass die nächste Wiedergabe sauber anfängt. Danach ruht die Rechnung.
        if (!inp || inp.length === 0) { if (this.idle <= 0 || (!this.weng && !eng.sys)) { for (const ch of out) ch.fill(0); return true; }
          if (this.zero[0].length !== out[0].length) this.zero = [new Float32Array(out[0].length)]; inp = this.zero; this.idle -= out[0].length; }
        else { this.idle = Math.round(0.5 * sampleRate); if (!this.weng && !eng.sys) { for (const ch of out) ch.fill(0); return true; } }
        const len = out[0].length, target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate); let mix = this.mix;
        if (this.weng && this.weng.ready) {
          if (this.mono.length < len) this.mono = new Float32Array(len); const mo = this.mono;
          for (let n = 0; n < len; n++) { let u = 0; for (let c = 0; c < inp.length; c++) u += inp[c][n]; mo[n] = u / inp.length; }
          if (this.dryb.length < len) this.dryb = new Float32Array(len); const dry = this.dryb; dry.set(mo.subarray(0, len)); this.weng.process(mo, len); const gd = this.finish(dry, mo, len);
          for (let n = 0; n < len; n++) { if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
            const o = mix * mo[n] + (1 - mix) * gd * dry[n]; for (let c = 0; c < out.length; c++) out[c][n] = o; }
          this.mix = mix; if ((this.rep += len) >= sampleRate / 10) { this.rep = 0; this.port.postMessage({ peak: this.weng.takePeak() }); } return true;
        }
        if (this.mono.length < 2 * len) this.mono = new Float32Array(2 * len); const dry = this.mono.subarray(0, len), wet = this.mono.subarray(len, 2 * len);
        for (let n = 0; n < len; n++) {
          let u = 0; for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length;     // Mono
          eng.tick(); dry[n] = u; wet[n] = eng.step(u);
        }
        const gd = this.finish(dry, wet, len);
        for (let n = 0; n < len; n++) {
          if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const o = mix * wet[n] + (1 - mix) * gd * dry[n]; for (let c = 0; c < out.length; c++) out[c][n] = o;
        }
        this.mix = mix; if ((this.rep += len) >= sampleRate / 10) { this.rep = 0; this.port.postMessage({ peak: eng.takePeak() }); }
        return true;
      }
    }
    registerProcessor('b100ii-tonestack', B100Processor);
  } else if (typeof module !== 'undefined') module.exports = api;
  else globalThis.B100DSP = api;
})();
