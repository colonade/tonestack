// Laney AOR 50 Vorverstärker als Zeitbereichs-Schaltungssimulation (Trapezregel wie SPICE), linear (Kleinsignal).
// Zustand: je Kondensator der "Verlaufsstrom" Ih. Pro Abtastwert: vc = K·Ih + k·u,  Ih' = 2G·vc − Ih,  y = c·Ih + d·u.
// K, k, c, d werden bei Regler-/Schalteränderung aus der Knotenmatrix neu berechnet (LU-Zerlegung).
(() => {
// Nichtlineares Modell des Laney AOR 50 Vorverstärkers (Schätzung).
// Röhren: Koren-12AX7-Modell (veröffentlicht), Gitterstrom als weiche Diode mit 2 kΩ.
// Schaltung in Abschnitte zwischen den Röhren zerlegt; jeder Abschnitt linear (Trapezregel), Röhrenströme per Newton.
// Die Kapazitäten Gitter–Anode von V101A..V102B werden als Miller-Ersatzkapazität am Gitter angenähert.
const NL = (() => {
  const KP = 600, MU = 100, EX = 1.4, KG1 = 1060, KVB = 300, RGI = 2000, SG = 0.02;
  function tube(vgk, vpk, out) {                   // Anodenstrom und Ableitungen nach vgk, vpk
    if (vpk <= 1e-6) { out[0] = 0; out[1] = 0; out[2] = 0; return; }
    const s = Math.sqrt(KVB + vpk * vpk), q = KP * (1 / MU + vgk / s);
    let sp, sig;
    if (q > 35) { sp = q; sig = 1; } else { const eq = Math.exp(q); sp = Math.log1p(eq); sig = eq / (1 + eq); }
    const e1 = vpk / KP * sp;
    if (e1 <= 1e-300) { out[0] = 0; out[1] = 0; out[2] = 0; return; }
    const p = Math.exp((EX - 1) * Math.log(e1)), I = 2 * p * e1 / KG1, dI = 2 * EX * p / KG1;
    out[0] = I; out[1] = dI * vpk * sig / s; out[2] = dI * (sp / KP - sig * vgk * vpk * vpk / (s * s * s));
  }
  // Kennlinientabelle: Gitterspannung −12…+3 V (10 mV), Anodenspannung 0…320 V (1 V); bilinear interpoliert
  const G0 = -12, GS = 0.01, GN = 1501, P0 = 0, PS = 1, PN = 321; let TAB = null;
  function makeTab() { TAB = new Float32Array(GN * PN); const o = [0, 0, 0]; for (let i = 0; i < GN; i++) for (let j = 0; j < PN; j++) { tube(G0 + i * GS, P0 + j * PS, o); TAB[i * PN + j] = o[0]; } }
  function tubeT(vgk, vpk, out) {
    if (!TAB) makeTab();
    let x = (vgk - G0) / GS, y = (vpk - P0) / PS;
    if (x < 0) x = 0; else if (x > GN - 1.000001) { tube(vgk, vpk, out); return; }
    if (y < 0) y = 0; else if (y > PN - 1.000001) { tube(vgk, vpk, out); return; }
    const i = x | 0, j = y | 0, fx = x - i, fy = y - j, o = i * PN + j;
    const a = TAB[o], b = TAB[o + 1], c = TAB[o + PN], d = TAB[o + PN + 1];
    const ab = a + (b - a) * fy, cd = c + (d - c) * fy;
    out[0] = ab + (cd - ab) * fx;
    out[1] = (vgk - G0 < 0) ? 0 : (cd - ab) / GS;
    out[2] = (vpk - P0 < 0) ? 0 : ((b - a) + ((d - c) - (b - a)) * fx) / PS;
  }
  function grid(v, out) {                          // Gitterstrom und Ableitung
    const x = v / SG; if (x > 35) { out[0] = v / RGI; out[1] = 1 / RGI; return; }
    const e = Math.exp(x); out[0] = SG * Math.log1p(e) / RGI; out[1] = e / (1 + e) / RGI;
  }
  // --- linearer Abschnitt ---------------------------------------------------------------
  function net() {
    const idx = {}; let n = 0;
    const N = k => k === '0' ? -1 : (k in idx ? idx[k] : (idx[k] = n++));
    const o = { idx, R: [], C: [], S: [], P: [], probes: [], N };
    o.r = (a, b, r) => o.R.push([N(a), N(b), 1 / Math.max(r, 1e-3)]);
    o.c = (a, b, c) => o.C.push([N(a), N(b), c]);
    let rc = 0; o.rcs = (a, b, r, c) => { const m = '_rc' + (rc++); o.r(a, m, r); o.c(m, b, c); };
    o.src = a => { o.S.push(N(a)); return o.S.length - 1; };            // ideale Spannungsquelle gegen Masse
    o.port = (from, to) => { o.P.push([N(from), N(to)]); return o.P.length - 1; };  // Strom von "from" nach "to"
    o.probe = a => { o.probes.push(N(a)); return o.probes.length - 1; };
    o.nodes = () => n;
    return o;
  }
  function compile(o, fs, dc) {
    const n = o.nodes(), ns = o.S.length, M = n + ns, A = new Float64Array(M * M);
    const st = (a, b, g) => { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
    for (const [a, b, g] of o.R) st(a, b, g);
    const caps = dc ? [] : o.C.map(([a, b, c]) => [a, b, 2 * c * fs]);
    for (const [a, b, g] of caps) st(a, b, g);
    o.S.forEach((a, k) => { A[a*M + n + k] = 1; A[(n + k)*M + a] = 1; });
    const piv = new Int32Array(M);
    for (let c = 0; c < M; c++) { let p = c, b = -1; for (let r = c; r < M; r++) { const m = Math.abs(A[r*M+c]); if (m > b) { b = m; p = r; } }
      piv[c] = p; if (p !== c) for (let q = 0; q < M; q++) { const t = A[c*M+q]; A[c*M+q] = A[p*M+q]; A[p*M+q] = t; }
      const d = A[c*M+c]; for (let r = c + 1; r < M; r++) { const f = A[r*M+c] / d; if (f === 0) continue; A[r*M+c] = f; for (let q = c + 1; q < M; q++) A[r*M+q] -= f * A[c*M+q]; } }
    const solve = rhs => { for (let c = 0; c < M; c++) { const p = piv[c]; if (p !== c) { const t = rhs[c]; rhs[c] = rhs[p]; rhs[p] = t; } }
      for (let r = 1; r < M; r++) { let s = rhs[r]; for (let q = 0; q < r; q++) s -= A[r*M+q] * rhs[q]; rhs[r] = s; }
      for (let r = M - 1; r >= 0; r--) { let s = rhs[r]; for (let q = r + 1; q < M; q++) s -= A[r*M+q] * rhs[q]; rhs[r] = s / A[r*M+r]; } return rhs; };
    const nc = caps.length, np = o.P.length, npr = o.probes.length, ncol = nc + ns + np;
    const V = (x, a) => a >= 0 ? x[a] : 0;
    const Mc = new Float64Array(nc * ncol), Mp = new Float64Array(npr * ncol), full = dc ? [] : null;
    for (let j = 0; j < ncol; j++) {
      const rhs = new Float64Array(M);
      if (j < nc) { const [a, b] = caps[j]; if (a >= 0) rhs[a] += 1; if (b >= 0) rhs[b] -= 1; }
      else if (j < nc + ns) rhs[n + (j - nc)] = 1;
      else { const [f, t] = o.P[j - nc - ns]; if (f >= 0) rhs[f] -= 1; if (t >= 0) rhs[t] += 1; }
      const x = solve(rhs);
      for (let i = 0; i < nc; i++) Mc[i*ncol + j] = V(x, caps[i][0]) - V(x, caps[i][1]);
      for (let i = 0; i < npr; i++) Mp[i*ncol + j] = V(x, o.probes[i]);
      if (dc) full.push(Float64Array.from(x.subarray(0, n)));
    }
    return { nc, ns, np, npr, ncol, Mc, Mp, G2: Float64Array.from(caps, c => 2 * c[2]), capsAll: o.C, full };
  }
  return { tube, tubeT, grid, net, compile };
})();


// Abschnitte des Laney-Vorverstärkers und nichtlinearer Zeitschritt (Schätzung)
const NLAMP = ((NL) => {
  const UB = [240, 252, 252, 260];                 // Betriebsspannungen laut Plan
  const CM = { g1: 81e-12, g2: 97e-12, g3: 16e-12, g4: 72e-12 };   // Miller-Ersatzkapazitäten (Cgk + (1+|A|)·Cga)
  const blendR = (ron, roff, x, rref) => { if (x >= 1) return ron; if (x <= 0) return roff;
    const yon = rref / (ron + rref), yoff = rref / (roff + rref), y = yoff + x * (yon - yoff); return rref * (1 / y - 1); };
  // Abschnittsdefinitionen: Netz + Ports + Proben
  function defs(v, s) {
    const S = [];
    { const o = NL.net(); o.src('in'); o.r('in','g1',v.r1); o.c('g1','0',CM.g1);   // bei LOW liegt an 'in' 0 V (Gitterzweig über Ruhekontakt an Masse)
      o.port('g1','0'); o.probe('g1'); S.push(o); }                                   // S0: Eingang → Gitter V101A
    { const o = NL.net(); o.src('B'); o.r('B','p1',100e3); o.r('k1','0',2.7e3); o.c('k1','0',47e-6);
      o.c('p1','n5',v.c2); o.r('n5','t1',v.r5); o.r('t1','0', s.ch1 <= 0 ? v.ronvt + v.r6 : s.ch1 >= 1 ? 1e12 : blendR(v.ronvt + v.r6, 1e12, 1 - s.ch1, 5e4));   // VT1 leuchtet in Preamp 2
      o.r('t1','g2',v.vr101*(1-s.v1)); o.r('g2','0',v.vr101*s.v1); o.c('g2','0',CM.g2);
      o.port('p1','k1'); o.port('g2','0'); o.port('0','k1'); o.probe('p1'); o.probe('k1'); o.probe('g2'); S.push(o); }   // S1: V101A
    { const o = NL.net(); o.src('B'); o.r('B','p2',100e3); o.r('k2','0',2.7e3); o.c('k2','0',v.c3);
      o.src('lin'); o.r('lin','t2', s.low ? 1e-3 : 1e12);                       // LOW-Eingang direkt auf Preamp 2 Volume
      if (!s.low && v.rpad > 0) { o.c('p2','t2p',v.ck); o.r('t2p','t2',v.rpad); } else o.c('p2', s.low ? '0' : 't2', v.ck);   // Umbauidee „Später"
      o.r('t2','w2',v.vr102*(1-s.v2)); o.r('w2','0',v.vr102*s.v2); o.r('w2','g3',68e3); o.c('g3','0',CM.g3);
      o.port('p2','k2'); o.port('g3','0'); o.port('0','k2'); o.probe('p2'); o.probe('k2'); o.probe('g3'); S.push(o); }   // S2: V101B
    { const o = NL.net(); o.src('B'); o.r('B','p3',100e3); o.r('k3','0',10e3);
      o.c('p3','n12',v.c4); o.r('n12','g4',470e3); o.c('n12','g4',v.c5); o.r('g4','0',470e3); o.c('g4','0',CM.g4);
      o.port('p3','k3'); o.port('g4','0'); o.port('0','k3'); o.probe('p3'); o.probe('k3'); o.probe('g4'); S.push(o); }   // S3: V102A
    { const o = NL.net(); o.src('B');
      o.r('B','p4',100e3); o.r('k4','0',820);                                            // V102B
      o.r('k5','0',100e3); o.c('p4','0',v.cga); o.c('p4','k5',v.cgk);                    // V103A Kathodenfolger
      o.c('k5','X',v.c6); o.c('X','Z',v.c7); o.rcs('X','Z', s.mid >= 1 ? 1e-3 : s.mid <= 0 ? 10e6 : blendR(1e-3, 10e6, s.mid, 1 / (2 * Math.PI * 1000 * v.c8)), v.c8);
      o.r('X','Y',v.r18); o.c('Y','W',v.c11);
      o.r('Z','T',v.vr105*(1-s.t)); o.r('T','W',v.vr105*s.t); o.r('W','P',v.vr106*s.b);
      o.r('P','Mw',v.vr107*(1-s.m)); o.r('Mw','0',v.vr107*s.m); o.c('Y','Mw',v.c10);
      o.c('T','n19',v.c9); o.r('n19','g6',1e6); o.r('g6','0',1e6); o.r('g6','n20',3.3e6); o.c('n20','p6',v.c12);
      o.r('B','p6',100e3); o.r('k6','0',820); o.c('k6','0',v.c13); o.c('g6','k6',v.cgk); o.c('g6','p6',v.cga);   // V103B
      o.c('p6','n24',v.c14); o.r('n24','J',68e3); o.r('J','Q',33e3); o.c('Q','0',v.c15);
      o.r('Q','0', s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 33e3));
      o.r('J','W4',v.vr104*(1-s.mv)); o.r('W4','N2',v.vr104*s.mv); o.r('N2','0',22e3); o.c('N2','0',v.c16);
      o.r('N2','0', s.bass <= 0 ? v.rts : s.bass >= 1 ? 1e12 : blendR(1e12, v.rts, s.bass, 22e3));
      o.rcs('J','W4', s.top >= 1 ? 22e3 : s.top <= 0 ? 1e9 : blendR(22e3, 1e9, s.top, 22e3), v.c106);
      o.r('W4','W3',v.vr103*(1-s.pl)); o.r('W3','B3',Math.max(v.vr103*s.pl, 1e-3));
      o.r('B3','0', s.ch1 >= 1 ? v.ronvt : s.ch1 <= 0 ? 1e12 : blendR(v.ronvt, 1e12, s.ch1, v.vr103)); o.r('W3','0',1e6);
      o.port('p4','k4'); o.port('0','k5'); o.port('p4','k5'); o.port('p6','k6'); o.port('g6','k6'); o.port('0','k4');
      ['p4','k4','k5','g6','p6','k6','W3'].forEach(o.probe); S.push(o); }                // S4: V102B, V103A, Klangregler, V103B, Ausgang
    return S;
  }
  // unbekannte Ports je Abschnitt: [Portindex, Art]; bekannte Ports (Gitterstrom der vorigen Stufe) separat
  const UNK = [[0], [0, 1], [0, 1], [0, 1], [0, 1, 2, 3, 4]], KNOWN = [null, 2, 2, 2, 5];

  class Amp {
    constructor(fsBase, os = 2) {
      this.os = os; this.fs = os * fsBase; this.sec = null; this.st = null;
      this.vkPrev = new Float64Array(4); this.tb = new Float64Array(3); this.gb = new Float64Array(2);
      this.vgkDC = new Float64Array(5); this.peak = new Float64Array(5); this.w3DC = 0;
      this.stats = { steps: 0, iters: 0, fails: 0 }; this.rowArgs = new Float64Array(40); this.Ib = new Float64Array(8); this.Dn = new Float64Array(8); this.tol = 1e-9;
      const L = 63, h = new Float64Array(L), fc = 0.229; let sum = 0;
      for (let k = 0; k < L; k++) { const m = k - (L - 1) / 2, w = 0.42 - 0.5 * Math.cos(2 * Math.PI * k / (L - 1)) + 0.08 * Math.cos(4 * Math.PI * k / (L - 1));
        h[k] = (m === 0 ? 2 * fc : Math.sin(2 * Math.PI * fc * m) / (Math.PI * m)) * w; sum += h[k]; }
      for (let k = 0; k < L; k++) h[k] /= sum;
      // Polyphasen-Aufteilung: gerade/ungerade Koeffizienten
      this.h0 = h.filter((_, k) => k % 2 === 0); this.h1 = h.filter((_, k) => k % 2 === 1); this.h = h;
      this.xb = new Float64Array(2 * this.h0.length); this.xi = 0; this.yb = new Float64Array(2 * L); this.yi = 0;
    }
    prep(C, k) {                     // Hilfsdaten je Abschnitt (einmal pro Neuaufbau)
      const unk = UNK[k], nu = unk.length, nc = C.nc, ns = C.ns, ncol = C.ncol, npr = C.npr;
      C.nu = nu; C.dV = new Float64Array(npr * nu);
      for (let r = 0; r < npr; r++) for (let u = 0; u < nu; u++) C.dV[r * nu + u] = C.Mp[r * ncol + nc + ns + unk[u]];
      C.kn = KNOWN[k]; C.w = new Float64Array(npr); C.pv = new Float64Array(npr); C.J = new Float64Array(nu * nu); C.F = new Float64Array(nu);
      C.I = new Float64Array(nu); C.pI = new Float64Array(C.np); C.k = k;
      return C;
    }
    compileAll(v, s) { const D = defs(v, s); this.sec = D.map((o, k) => this.prep(NL.compile(o, this.fs, false), k)); this.dcSec = D.map((o, k) => this.prep(NL.compile(o, this.fs, true), k)); }
    // Röhrengleichungen: füllt F = I − f(V), J = dF/dI
    residual(C, vg) {
      const pv = C.pv, J = C.J, F = C.F, I = C.I, dV = C.dV, nu = C.nu, tb = this.tb, gb = this.gb, T = NL.tubeT, G = NL.grid, k = C.k;
      for (let q = 0; q < nu * nu; q++) J[q] = 0;
      for (let u = 0; u < nu; u++) J[u * nu + u] = 1;
      let f, a1, a2, a3, u;
      // Zeile u: F[u] = I[u] − f, J[u][q] −= a1·dV[r1][q] + a2·dV[r2][q] + a3·dV[r3][q]
      if (k === 0) {
        G(pv[0] - this.vkPrev[0], gb); F[0] = I[0] - gb[0]; J[0] -= gb[1] * dV[0];
        return;
      }
      if (k <= 3) {
        T(vg - pv[1], pv[0] - pv[1], tb); F[0] = I[0] - tb[0]; a1 = tb[2]; a2 = -tb[1] - tb[2];
        for (let q = 0; q < 2; q++) J[q] -= a1 * dV[q] + a2 * dV[2 + q];
        G(pv[2] - this.vkPrev[k], gb); F[1] = I[1] - gb[0];
        for (let q = 0; q < 2; q++) J[2 + q] -= gb[1] * dV[4 + q];
        return;
      }
      // Abschnitt 4: Proben 0 p4, 1 k4, 2 k5, 3 g6, 4 p6, 5 k6; Unbekannte Ip4, Ip5, Ig5, Ip6, Ig6
      T(vg - pv[1], pv[0] - pv[1], tb); F[0] = I[0] - tb[0]; a1 = tb[2]; a2 = -tb[1] - tb[2];
      for (let q = 0; q < 5; q++) J[q] -= a1 * dV[q] + a2 * dV[5 + q];
      T(pv[0] - pv[2], UB[3] - pv[2], tb); F[1] = I[1] - tb[0]; a1 = tb[1]; a2 = -tb[1] - tb[2];
      for (let q = 0; q < 5; q++) J[5 + q] -= a1 * dV[q] + a2 * dV[10 + q];
      G(pv[0] - pv[2], gb); F[2] = I[2] - gb[0];
      for (let q = 0; q < 5; q++) J[10 + q] -= gb[1] * (dV[q] - dV[10 + q]);
      T(pv[3] - pv[5], pv[4] - pv[5], tb); F[3] = I[3] - tb[0]; a1 = tb[1]; a2 = tb[2]; a3 = -tb[1] - tb[2];
      for (let q = 0; q < 5; q++) J[15 + q] -= a1 * dV[15 + q] + a2 * dV[20 + q] + a3 * dV[25 + q];
      G(pv[3] - pv[5], gb); F[4] = I[4] - gb[0];
      for (let q = 0; q < 5; q++) J[20 + q] -= gb[1] * (dV[15 + q] - dV[25 + q]);
    }
    evalF(C, vg) {
      const w = C.w, pv = C.pv, dV = C.dV, I = C.I, nu = C.nu, npr = C.npr, F = C.F;
      for (let r = 0; r < npr; r++) { let s = w[r]; for (let u = 0; u < nu; u++) s += dV[r * nu + u] * I[u]; pv[r] = s; }
      this.residual(C, vg); let e = 0; for (let u = 0; u < nu; u++) { const x = Math.abs(F[u]); if (x > e) e = x; } return e;
    }
    solve(C, Ih, src, known, vg, maxIt) {
      const nc = C.nc, ns = C.ns, ncol = C.ncol, Mp = C.Mp, npr = C.npr, nu = C.nu, w = C.w, pv = C.pv, dV = C.dV, I = C.I, J = C.J, F = C.F;
      for (let r = 0; r < npr; r++) { let s = 0; const o = r * ncol;
        if (Ih) for (let j = 0; j < nc; j++) s += Mp[o + j] * Ih[j];
        for (let j = 0; j < ns; j++) s += Mp[o + nc + j] * src[j];
        if (C.kn !== null) s += Mp[o + nc + ns + C.kn] * known;
        w[r] = s; }
      let it = 0, ok = false;
      const Ib = this.Ib, D = this.Dn;
      let err = this.evalF(C, vg);
      for (; it < maxIt; it++) {
        if (err < this.tol && it > 0) { ok = true; break; }     // mindestens ein Newton-Schritt, sonst gehen sehr kleine Signale verloren
        for (let u = 0; u < nu; u++) D[u] = -F[u];
        for (let c = 0; c < nu; c++) { let p = c, bb = -1; for (let r = c; r < nu; r++) { const m = Math.abs(J[r*nu+c]); if (m > bb) { bb = m; p = r; } }
          if (p !== c) { for (let q = 0; q < nu; q++) { const t = J[c*nu+q]; J[c*nu+q] = J[p*nu+q]; J[p*nu+q] = t; } const t = D[c]; D[c] = D[p]; D[p] = t; }
          const d = J[c*nu+c]; for (let r = c + 1; r < nu; r++) { const f = J[r*nu+c] / d; if (f === 0) continue; for (let q = c; q < nu; q++) J[r*nu+q] -= f * J[c*nu+q]; D[r] -= f * D[c]; } }
        for (let r = nu - 1; r >= 0; r--) { let s = D[r]; for (let q = r + 1; q < nu; q++) s -= J[r*nu+q] * D[q]; D[r] = s / J[r*nu+r]; }
        // Schrittweitensteuerung: nur Schritte annehmen, die den Fehler verkleinern
        for (let u = 0; u < nu; u++) Ib[u] = I[u];
        let lam = 1; for (let u = 0; u < nu; u++) { const m = Math.abs(D[u]); if (m * lam > 5e-3) lam = 5e-3 / m; }
        let e2 = Infinity;
        for (let ls = 0; ls < 12; ls++) { for (let u = 0; u < nu; u++) I[u] = Ib[u] + lam * D[u]; e2 = this.evalF(C, vg); if (e2 < err) break; lam *= 0.5; }
        err = e2;
      }
      if (!ok && err < this.tol) ok = true;
      return ok ? it : -1;
    }
    advance(C, St, src, known) {     // Kondensatorzustände fortschreiben
      const nc = C.nc, ns = C.ns, ncol = C.ncol, Mc = C.Mc, np = C.np, pI = C.pI, I = C.I, Ih = St.Ih, vc = St.vc;
      const k = C.k; if (k === 0) pI[0] = I[0]; else if (k <= 3) { pI[0] = I[0]; pI[1] = I[1]; pI[2] = known; } else { for (let u = 0; u < 5; u++) pI[u] = I[u]; pI[5] = known; }
      for (let c = 0; c < nc; c++) { let s = 0; const o = c * ncol;
        for (let j = 0; j < nc; j++) s += Mc[o + j] * Ih[j];
        for (let j = 0; j < ns; j++) s += Mc[o + nc + j] * src[j];
        for (let j = 0; j < np; j++) s += Mc[o + nc + ns + j] * pI[j];
        vc[c] = s; }
      for (let c = 0; c < nc; c++) Ih[c] = C.G2[c] * vc[c] - Ih[c];
    }
    run(u, dc) {                      // ein Durchlauf aller Abschnitte
      if (!dc && this.bTo && ((++this.bpos) & 31) === 0) this.blendNow();
      let known = 0, vg = 0; const secs = dc ? this.dcSec : this.sec, maxIt = dc ? 300 : 60;
      for (let k = 0; k < 5; k++) {
        const C = secs[k], src = k === 0 ? [this.low ? 0 : u] : k === 2 ? [UB[1], this.low ? u : 0] : [UB[k - 1]];
        if (!dc) { const I = C.I, P1 = C.Ip1 || (C.Ip1 = Float64Array.from(I)), P2 = C.Ip2 || (C.Ip2 = Float64Array.from(I));
          for (let q = 0; q < C.nu; q++) { const cur = I[q]; I[q] = 2 * cur - P2[q]; P2[q] = cur; } }
        const it = this.solve(C, dc ? null : this.st[k].Ih, src, known, vg, maxIt);
        if (!dc) { this.stats.iters += it < 0 ? maxIt : it; if (it < 0) this.stats.fails++; this.advance(C, this.st[k], src, known); }
        const pv = C.pv, I = C.I;
        if (k === 0) { vg = pv[0]; known = I[0]; } else if (k <= 3) { vg = pv[2]; known = I[1]; this.vkPrev[k - 1] = pv[1]; } else this.vkPrev[3] = pv[1];
      }
      if (!dc) this.stats.steps++;
      const P0 = secs[0].pv, P1 = secs[1].pv, P2 = secs[2].pv, P3 = secs[3].pv, P4 = secs[4].pv;
      this.g0 = P0[0] - P1[1]; this.g1 = P1[2] - P2[1]; this.g2 = P2[2] - P3[1]; this.g3 = P3[2] - P4[1]; this.g4 = P4[3] - P4[5];
      return P4[6];
    }
    // Arbeitspunkt einmal rechnen und für frische Schaltungen wiederverwenden (hängt nicht von Reglern/Schaltern ab)
    snapshot() { return { Ih: this.st.map(x => Float64Array.from(x.Ih)), vc: this.st.map(x => Float64Array.from(x.vc)), I: this.sec.map(C => Float64Array.from(C.I)),
      vkPrev: Float64Array.from(this.vkPrev), vgkDC: Float64Array.from(this.vgkDC), w3DC: this.w3DC }; }
    initFrom(v, s, snap) {
      this.compileAll(v, s);
      this.st = snap.Ih.map((Ih, k) => ({ Ih: Float64Array.from(Ih), vc: Float64Array.from(snap.vc[k]) }));
      this.sec.forEach((C, k) => C.I.set(snap.I[k])); this.vkPrev.set(snap.vkPrev); this.vgkDC.set(snap.vgkDC); this.w3DC = snap.w3DC; this.peak.fill(0);
    }
    // Neuaufbau nach Reglerbewegung: Matrizen über 512 schnelle Abtastwerte überblenden
    retarget(v, s) {
      const D = defs(v, s), fresh = D.map(o => NL.compile(o, this.fs, false));
      this.bFrom = this.sec.map(C => ({ Mp: Float64Array.from(C.Mp), Mc: Float64Array.from(C.Mc), dV: Float64Array.from(C.dV) }));
      this.bTo = fresh.map((F, k) => { const P = this.prep(F, k); return { Mp: P.Mp, Mc: P.Mc, dV: P.dV }; });
      this.bpos = 0; this.blendNow();
    }
    blendNow() {
      if (!this.bTo) return; const a = Math.min(1, this.bpos / 512), b = 1 - a;
      for (let k = 0; k < 5; k++) { const C = this.sec[k], f = this.bFrom[k], t = this.bTo[k];
        for (let i = 0; i < C.Mp.length; i++) C.Mp[i] = b * f.Mp[i] + a * t.Mp[i];
        for (let i = 0; i < C.Mc.length; i++) C.Mc[i] = b * f.Mc[i] + a * t.Mc[i];
        for (let i = 0; i < C.dV.length; i++) C.dV[i] = b * f.dV[i] + a * t.dV[i]; }
      if (a >= 1) { this.bTo = null; this.bFrom = null; }
    }
    init(v, s) {
      this.compileAll(v, s);
      this.st = this.sec.map(C => ({ Ih: new Float64Array(C.nc), vc: new Float64Array(C.nc) }));
      const guess = [[0], [0.6e-3, 0], [0.6e-3, 0], [0.25e-3, 0], [1.1e-3, 1.5e-3, 0, 1.1e-3, 0]];
      [this.sec, this.dcSec].forEach(S => S.forEach((C, k) => C.I.set(guess[k])));
      this.vkPrev.set([1.6, 1.7, 2.5, 0.9]);
      for (let pass = 0; pass < 4; pass++) this.run(0, true);
      // Kondensatorzustände aus dem Arbeitspunkt; Newton-Startwerte übernehmen
      let known = 0;
      for (let k = 0; k < 5; k++) {
        const D = this.dcSec[k], C = this.sec[k], src = k === 0 ? 0 : UB[k - 1];
        const pI = k === 0 ? [D.I[0]] : k <= 3 ? [D.I[0], D.I[1], known] : [...D.I, known];
        const x = new Float64Array(D.full[0].length);
        for (let j = 0; j < D.ns; j++) for (let q = 0; q < x.length; q++) x[q] += D.full[j][q] * (j === 0 ? src : 0);
        for (let j = 0; j < D.np; j++) for (let q = 0; q < x.length; q++) x[q] += D.full[D.ns + j][q] * pI[j];
        C.capsAll.forEach(([a, b], ci) => { const vc = (a >= 0 ? x[a] : 0) - (b >= 0 ? x[b] : 0); this.st[k].Ih[ci] = C.G2[ci] / 2 * vc; this.st[k].vc[ci] = vc; });
        C.I.set(D.I); known = k === 0 ? D.I[0] : D.I[1];
      }
      this.vgkDC.set([this.g0, this.g1, this.g2, this.g3, this.g4]);
      this.w3DC = this.dcSec[4].pv[6]; this.peak.fill(0);
    }
    step(x) {                        // Grundrate: 2-fach polyphasig überabtasten (oder direkt bei os = 1)
      if (this.os === 1) { const y = this.run(x, false) - this.w3DC, pk = this.peak, dc = this.vgkDC; let d;
        d = Math.abs(this.g0 - dc[0]); if (d > pk[0]) pk[0] = d; d = Math.abs(this.g1 - dc[1]); if (d > pk[1]) pk[1] = d;
        d = Math.abs(this.g2 - dc[2]); if (d > pk[2]) pk[2] = d; d = Math.abs(this.g3 - dc[3]); if (d > pk[3]) pk[3] = d;
        d = Math.abs(this.g4 - dc[4]); if (d > pk[4]) pk[4] = d; return y; }
      const h0 = this.h0, h1 = this.h1, L0 = h0.length, xb = this.xb, L = this.h.length, yb = this.yb, h = this.h;
      this.xi = this.xi === 0 ? L0 - 1 : this.xi - 1; xb[this.xi] = xb[this.xi + L0] = x;
      const base = this.xi, pk = this.peak, dc = this.vgkDC; let out = 0;
      for (let ph = 0; ph < 2; ph++) {
        const hh = ph === 0 ? h0 : h1; let u = 0;
        for (let k = 0; k < hh.length; k++) u += hh[k] * xb[base + k];
        const y = this.run(2 * u, false) - this.w3DC;
        let d;
        d = Math.abs(this.g0 - dc[0]); if (d > pk[0]) pk[0] = d; d = Math.abs(this.g1 - dc[1]); if (d > pk[1]) pk[1] = d;
        d = Math.abs(this.g2 - dc[2]); if (d > pk[2]) pk[2] = d; d = Math.abs(this.g3 - dc[3]); if (d > pk[3]) pk[3] = d;
        d = Math.abs(this.g4 - dc[4]); if (d > pk[4]) pk[4] = d;
        this.yi = this.yi === 0 ? L - 1 : this.yi - 1; yb[this.yi] = yb[this.yi + L] = y;
        if (ph === 0) { const yo = this.yi; for (let k = 0; k < L; k++) out += h[k] * yb[yo + k]; }   // nach der ersten Teilphase abgreifen: Latenz genau 31 wie im C++-Kern
      }
      return out;
    }
  }
  return { Amp, defs };
})(NL);


  const ln = Math.log, ex = Math.exp;
  // Schaltelement weich überblenden: nicht der Widerstand, sondern seine Wirkung y = Rref/(R+Rref) ändert sich gleichmäßig.
  // x=1: ein (ron), x=0: aus (roff); Endpunkte exakt.
  const blendR = (ron, roff, x, rref) => {
    if (x >= 1) return ron; if (x <= 0) return roff;
    const yon = rref / (ron + rref), yoff = rref / (roff + rref), y = yoff + x * (yon - yoff);
    return rref * (1 / y - 1);
  };
  const rpOf = (Ia, v) => v.rp0 * Math.pow(v.ia0 / Ia, 1 / 3);
  const MON = [['g1','k1',1.6,'V101A'],['g2','k2',1.7,'V101B'],['g3','k3',3.1,'V102A'],['g4','k4',1.0,'V102B'],['g6','k6',1.0,'V103B']];

  function build(v, s, fs) {
    const idx = {}; let n = 0;
    const N = k => k === '0' ? -1 : (k in idx ? idx[k] : (idx[k] = n++));
    const Rs = [], Cs = [], TR = []; let rcCount = 0;
    const R = (a, b, r) => Rs.push([N(a), N(b), 1 / Math.max(r, 1e-3)]);
    const C = (a, b, c) => Cs.push([N(a), N(b), 2 * c * fs]);
    const RC = (a, b, r, c) => { const m = 'rc' + (rcCount++); R(a, m, r); C(m, b, c); };
    const triode = (g, p, k, Ia) => { const rp = rpOf(Ia, v); R(p, k, rp); TR.push([N(g), N(p), N(k), v.mu / rp]); C(g, k, v.cgk); C(g, p, v.cga); };
    N('in');
    if (s.low) { R('g1','0',v.r1); R('in','t2',1e-3); } else R('in','g1',v.r1);   // LOW direkt auf Preamp 2 Volume
    triode('g1','p1','k1',0.62e-3); R('k1','0',2.7e3); C('k1','0',47e-6); R('p1','0',100e3);
    C('p1','n5',v.c2); R('n5','t1',v.r5);
    R('t1','0', vt1R(v, 1 - s.ch1));                          // VT1 leuchtet in Preamp 2
    R('t1','g2',v.vr101*(1-s.v1)); R('g2','0',v.vr101*s.v1);
    triode('g2','p2','k2',0.69e-3); R('k2','0',2.7e3); C('k2','0',v.c3); R('p2','0',100e3);
    if (!s.low && v.rpad > 0) { C('p2','t2p',v.ck); R('t2p','t2',v.rpad); } else C('p2', s.low ? '0' : 't2', v.ck);
    R('t2','w2',v.vr102*(1-s.v2)); R('w2','0',v.vr102*s.v2); R('w2','g3',68e3);
    triode('g3','p3','k3',0.31e-3); R('k3','0',10e3); R('p3','0',100e3);
    C('p3','n12',v.c4); R('n12','g4',470e3); C('n12','g4',v.c5); R('g4','0',470e3);
    triode('g4','p4','k4',1.10e-3); R('k4','0',820); R('p4','0',100e3);   // Anodenstrom aus Plan: (260 V − 150 V) / 100 kΩ
    triode('p4','0','k5',1.5e-3); R('k5','0',100e3);
    C('k5','X',v.c6); C('X','Z',v.c7); RC('X','Z', blendR(1e-3, 10e6, s.mid, 1 / (2 * Math.PI * 1000 * v.c8)), v.c8);
    R('X','Y',v.r18); C('Y','W',v.c11);
    R('Z','T',v.vr105*(1-s.t)); R('T','W',v.vr105*s.t); R('W','P',v.vr106*s.b);
    R('P','Mw',v.vr107*(1-s.m)); R('Mw','0',v.vr107*s.m); C('Y','Mw',v.c10);
    C('T','n19',v.c9); R('n19','g6',1e6); R('g6','0',1e6); R('g6','n20',3.3e6); C('n20','p6',v.c12);
    triode('g6','p6','k6',1.18e-3); R('k6','0',820); C('k6','0',v.c13); R('p6','0',100e3);
    C('p6','n24',v.c14); R('n24','J',68e3);
    R('J','Q',33e3); C('Q','0',v.c15); R('Q','0', blendR(1e12, v.rts, s.bass, 33e3));
    R('J','W4',v.vr104*(1-s.mv)); R('W4','N2',v.vr104*s.mv);
    R('N2','0',22e3); C('N2','0',v.c16); R('N2','0', blendR(1e12, v.rts, s.bass, 22e3));
    RC('J','W4', blendR(22e3, 1e9, s.top, 22e3), v.c106);
    R('W4','W3',v.vr103*(1-s.pl)); R('W3','B3',Math.max(v.vr103*s.pl, 1e-3));
    R('B3','0', blendR(v.ronvt, 1e12, s.ch1, v.vr103));
    R('W3','0',1e6);
    return compile(idx, n, Rs, Cs, TR, 'W3', MON);
  }
  function compile(idx, n, Rs, Cs, TR, outName, MON) {
    const M = n + 1, A = new Float64Array(M * M);
    const st = (a, b, g) => { if (a >= 0) A[a*M+a] += g; if (b >= 0) A[b*M+b] += g; if (a >= 0 && b >= 0) { A[a*M+b] -= g; A[b*M+a] -= g; } };
    for (const [a, b, g] of Rs) st(a, b, g);
    for (const [a, b, g] of Cs) st(a, b, g);
    for (const [g, p, k, gm] of TR) { if (p >= 0) { A[p*M+g] += gm; if (k >= 0) A[p*M+k] -= gm; } if (k >= 0) { A[k*M+g] -= gm; A[k*M+k] += gm; } }
    const si = idx['in']; A[si*M+n] = 1; A[n*M+si] = 1;       // ideale Spannungsquelle am Eingang
    // LU-Zerlegung mit Spaltenpivot
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
    const K = new Float64Array(nc * nc), k = new Float64Array(nc), G2 = new Float64Array(nc);
    const out = idx[outName], c = new Float64Array(nc), mon = MON.map(([g, kk]) => [idx[g], idx[kk]]);
    const mc = MON.map(() => new Float64Array(nc)), md = new Float64Array(MON.length);
    let d = 0;
    const col = (x, j) => {         // Spalte j (0..nc-1: Kondensator j, nc: Eingang)
      for (let i = 0; i < nc; i++) { const v2 = diff(x, Cs[i][0], Cs[i][1]); if (j < nc) K[i*nc+j] = v2; else k[i] = v2; }
      if (j < nc) { c[j] = x[out]; mon.forEach(([g, kk], m) => mc[m][j] = x[g] - x[kk]); }
      else { d = x[out]; mon.forEach(([g, kk], m) => md[m] = x[g] - x[kk]); }
    };
    for (let j = 0; j <= nc; j++) {
      const rhs = new Float64Array(M);
      if (j < nc) { const [a, b] = Cs[j]; if (a >= 0) rhs[a] += 1; if (b >= 0) rhs[b] -= 1; } else rhs[n] = 1;
      col(solve(rhs), j);
    }
    for (let i = 0; i < nc; i++) G2[i] = 2 * Cs[i][2];
    return { nc, K, k, G2, c, d, mc, md };
  }

  // Endstufe mit Gegenkopplung und Presence: Phasenumkehr V104, R38/R105/VR108/C110 vom braunen Abgriff; Endröhren + Übertrager linear (Gp).
  // Gleiches Netz wie engine.paBuild; Eingang = normiertes Vorstufensignal, Ausgang 'br' = brauner Abgriff.
  function buildPA(v, pres, fs) {
    const idx = {}; let n = 0;
    const N = k => k === '0' ? -1 : (k in idx ? idx[k] : (idx[k] = n++));
    const Rs = [], Cs = [], TR = [];
    const R = (a, b, r) => Rs.push([N(a), N(b), 1 / Math.max(r, 1e-3)]);
    const C = (a, b, c) => Cs.push([N(a), N(b), 2 * c * fs]);
    const triode = (g, p, k, Ia) => { const rp = rpOf(Ia, v); R(p, k, rp); TR.push([N(g), N(p), N(k), v.mu / rp]); C(g, k, v.cgk); C(g, p, v.cga); };
    const buf = (a, b) => { TR.push([N(a), -1, N(b), 1]); R(b, '0', 1); };
    N('in');
    R('in','fa',1e3); C('fa','fb',v.c201); C('fb','fc',v.c202); R('fc','0',1/(1/39e3+1/55e3));
    buf('fc','bb'); R('bb','ga0',1e3); C('ga0','ga',v.c18); R('ga','n',1e6); R('gb','n',1e6); R('n','k',470); R('n','x',10e3);
    triode('ga','pa','k',1.23e-3); triode('gb','pb','k',1.17e-3); R('pa','0',82e3); R('pb','0',100e3); C('pa','pb',v.c20);
    C('pa','g1',v.c21); R('g1','0',220e3); C('pb','g2',v.c21); R('g2','0',220e3);
    C('gb','x',v.c19); R('x','0',v.r105); R('x','px',v.vr108*(1-pres)); C('px','0',v.c110);
    TR.push([N('g2'), -1, N('br'), 1]); TR.push([N('g1'), N('br'), -1, 1]); R('br','0',1);
    R('br','x', v.r38 * 0.5 / Math.max(v.gp, 1e-6));
    return compile(idx, n, Rs, Cs, TR, 'br', []);
  }
  class PaCore {
    constructor(fs) { this.fs = fs; this.every = 256; this.sys = null; this.prevSys = null; this.blend = 1; this.cur = 0; this.tgt = 0; this.v = null; this.cnt = 0; this.Ih = null; this.vc = null; this.norm = 1; }
    setTarget(v, pres, norm) {
      this.tgt = pres; this.norm = norm || 1;
      const partsChanged = !this.v || Object.keys(v).some(k => v[k] !== this.v[k]);
      if (partsChanged || !this.sys) { this.v = { ...v }; if (!this.sys) this.cur = pres; this.rebuild(); this.prevSys = null; this.blend = 1; }
    }
    rebuild() {
      this.prevSys = this.sys; this.blend = this.prevSys ? 0 : 1;
      this.sys = buildPA(this.v, this.cur, this.fs);
      if (!this.Ih || this.Ih.length !== this.sys.nc) { this.Ih = new Float64Array(this.sys.nc); this.vc = new Float64Array(this.sys.nc); }
    }
    tick() {
      if (++this.cnt < this.every) return; this.cnt = 0;
      const d = this.tgt - this.cur; if (d === 0) return;
      const a = 1 - Math.exp(-this.every / (0.015 * this.fs));
      if (Math.abs(d) > 1e-5) this.cur += d * a; else this.cur = this.tgt;
      this.rebuild();
    }
    step(u) {
      const S = this.sys, nc = S.nc, Ih = this.Ih, vc = this.vc; let y;
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
      return y / this.norm;
    }
    clone() { const c = new PaCore(this.fs); Object.assign(c, this, { v: { ...this.v }, Ih: Float64Array.from(this.Ih), vc: Float64Array.from(this.vc) }); return c; }
  }

  // Hochpässe im Effektweg und am Endstufeneingang (erster Ordnung, bilinear)
  function hpCoef(fc, fs) { const K = Math.tan(Math.PI * fc / fs), a0 = 1 + K; return { b0: 1 / a0, b1: -1 / a0, a1: (K - 1) / a0 }; }
  function paStages(v, fs) {
    const cser = 1 / (1 / v.c201 + 1 / v.c202), two = Math.PI * 2;
    return [1 / (two * (1e3 + 1 / (1 / 39e3 + 1 / 55e3)) * cser), 1 / (two * (1e3 + 1e6) * v.c18), 1 / (two * 280e3 * v.c21)].map(f => hpCoef(f, fs));
  }

  // Kanalwechsel: VT1 so überblenden, dass sich die Dämpfung am Teiler (1 kHz) gleichmäßig in dB ändert.
  const vt1Div = (v, rvt) => {
    const w = 2 * Math.PI * 1000, zs = 1 / (1 / 100e3 + 1 / 77.9e3), xc = -1 / (w * v.c2), z = 1 / (1 / v.vr101 + 1 / rvt);
    return z / Math.hypot(zs + v.r5 + z, xc);
  };
  function vt1R(v, x) {
    const ron = v.ronvt + v.r6, roff = 1e12;
    if (x >= 1) return ron; if (x <= 0) return roff;
    const g1 = vt1Div(v, ron), g0 = vt1Div(v, roff), gt = Math.exp(Math.log(g0) + x * (Math.log(g1) - Math.log(g0)));
    let lo = Math.log(ron), hi = Math.log(roff);            // größeres R -> weniger Dämpfung (monoton)
    for (let i = 0; i < 60; i++) { const mid = (lo + hi) / 2; if (vt1Div(v, Math.exp(mid)) > gt) hi = mid; else lo = mid; }
    return Math.exp((lo + hi) / 2);
  }
  const chanShape = (v, x) => vt1Div(v, vt1R(v, 1 - x));
  const SMOOTH = ['v1','v2','t','b','m','mv','pl','mid','top','bass'];
  class Core {
    constructor(fs) { this.fs = fs; this.every = 256; this.sys = null; this.prevSys = null; this.blend = 1; this.prevNorm = 1; this.cur = null; this.tgt = null; this.v = null; this.cnt = 0; this.Ih = null; this.vc = null;
      this.pa = false; this.paCore = new PaCore(fs); this.norm = 1; this.normP1 = 1; this.normP2 = 1; this.vin = 0.1; this.peak = new Float64Array(MON.length); }
    setTarget(m) {
      const s = { ...m.s, mid: m.sw.mid ? 1 : 0, top: m.sw.top ? 1 : 0, bass: m.sw.bass ? 1 : 0, ch1: m.sw.ch === 1 ? 1 : 0, low: m.sw.inp === 'low' ? 1 : 0 };
      const partsChanged = !this.v || Object.keys(m.v).some(k => m.v[k] !== this.v[k]);
      this.tgt = s; if ('normP1' in m) { this.normP1 = m.normP1; this.normP2 = m.normP2; } if ('vin' in m) this.vin = m.vin;
      const chChanged = this.cur && this.cur.ch1 !== s.ch1;
      if (chChanged) this.cur.ch1 = s.ch1;
      if (partsChanged || !this.cur || chChanged) {
        const had = !!this.sys && partsChanged, oldG = had ? this.sys.G2 : null;
        this.v = { ...m.v }; if (!this.cur) this.cur = { ...s };
        this.rebuild();
        if (had) {
          // Bauteiltausch: sofort umschalten (keine Überblendung) und die Kondensatorspannungen erhalten.
          // Zustand Ih = G·v + i; bei neuem G ist Ih' = Ih + (G' − G)·v.
          this.prevSys = null; this.blend = 1; this.prevNorm = this.norm;
          if (oldG.length === this.sys.nc) for (let j = 0; j < oldG.length; j++) this.Ih[j] += (this.sys.G2[j] - oldG[j]) / 2 * this.vc[j];
        }
      }
      this.pa = !!m.pa; this.paCore.setTarget(m.v, m.s.pres || 0, m.normPa);
    }
    rebuild() {
      this.prevSys = this.sys; this.prevNorm = this.norm; this.blend = this.prevSys ? 0 : 1;
      this.sys = build(this.v, this.cur, this.fs);
      const x = this.cur.ch1, r0 = chanShape(this.v, 0), r1 = chanShape(this.v, 1);
      this.norm = this.normP2 * (chanShape(this.v, x) / r0) * Math.pow(this.normP1 / (this.normP2 * r1 / r0), x);
      if (!this.prevSys) this.prevNorm = this.norm; if (!this.Ih || this.Ih.length !== this.sys.nc) { this.Ih = new Float64Array(this.sys.nc); this.vc = new Float64Array(this.sys.nc); } }
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
      let y, nrm;
      if (this.blend < 1 && this.prevSys && this.prevSys.nc === nc) {
        const a = this.blend, b = 1 - a, P = this.prevSys, K1 = S.K, K0 = P.K;
        y = (a * S.d + b * P.d) * u;
        for (let i = 0; i < nc; i++) { let s1 = S.k[i] * u, s0 = P.k[i] * u; const o = i * nc;
          for (let j = 0; j < nc; j++) { s1 += K1[o + j] * Ih[j]; s0 += K0[o + j] * Ih[j]; }
          vc[i] = a * s1 + b * s0; y += (a * S.c[i] + b * P.c[i]) * Ih[i]; }
        for (let m = 0; m < S.mc.length; m++) { let s1 = (a * S.md[m] + b * P.md[m]) * u; const r1 = S.mc[m], r0 = P.mc[m];
          for (let j = 0; j < nc; j++) s1 += (a * r1[j] + b * r0[j]) * Ih[j]; const ab = Math.abs(s1); if (ab > this.peak[m]) this.peak[m] = ab; }
        nrm = a * this.norm + b * this.prevNorm; this.blend = Math.min(1, this.blend + 1 / this.every);
      } else {
        const K = S.K; y = S.d * u;
        for (let i = 0; i < nc; i++) { let s1 = S.k[i] * u; const o = i * nc; for (let j = 0; j < nc; j++) s1 += K[o + j] * Ih[j]; vc[i] = s1; y += S.c[i] * Ih[i]; }
        for (let m = 0; m < S.mc.length; m++) { let s1 = S.md[m] * u; const r = S.mc[m]; for (let j = 0; j < nc; j++) s1 += r[j] * Ih[j]; const ab = Math.abs(s1); if (ab > this.peak[m]) this.peak[m] = ab; }
        nrm = this.norm;
      }
      for (let i = 0; i < nc; i++) Ih[i] = S.G2[i] * vc[i] - Ih[i];
      y /= nrm;
      if (this.pa) { this.paCore.tick(); y = this.paCore.step(y); }
      return y;
    }
    clone() {
      const c = new Core(this.fs);
      Object.assign(c, this, { cur: { ...this.cur }, tgt: { ...this.tgt }, v: { ...this.v }, Ih: Float64Array.from(this.Ih), vc: Float64Array.from(this.vc),
        paCore: this.paCore.clone(), peak: Float64Array.from(this.peak) });
      return c;
    }
    takePeaks() { const r = Array.from(this.peak, (p, i) => p * this.vin / MON[i][2]); this.peak.fill(0); return r; }
  }
  // Nichtlinearer Kern (Schätzung): Eingang in Volt, Ausgang 20 V = Vollaussteuerung
  const NL_DC = {};
  class NlCore {
    constructor(fs, os) { this.fs = fs; this.os = os; this.nl = true; this.amp = new NLAMP.Amp(fs, os); this.cur = null; this.tgt = null; this.v = null;
      this.cnt = 0; this.vin = 0.1; this.pa = false; this.paCore = new PaCore(fs); this.peak = new Float64Array(MON.length); }
    setTarget(m) {
      const s = { ...m.s, mid: m.sw.mid ? 1 : 0, top: m.sw.top ? 1 : 0, bass: m.sw.bass ? 1 : 0, ch1: m.sw.ch === 1 ? 1 : 0, low: m.sw.inp === 'low' ? 1 : 0 };
      if ('vin' in m) this.vin = m.vin; this.pa = !!m.pa; this.amp.low = !!s.low;
      const partsChanged = !this.v || Object.keys(m.v).some(k => m.v[k] !== this.v[k]);
      if (partsChanged) {
        this.v = { ...m.v }; this.cur = { ...s }; this.tgt = { ...s };
        const key = JSON.stringify(this.v) + '|' + this.fs + '|' + this.os;
        if (!NL_DC[key]) { this.amp.init(this.v, this.cur); NL_DC[key] = this.amp.snapshot(); } else this.amp.initFrom(this.v, this.cur, NL_DC[key]);
      } else this.tgt = s;
      this.paCore.setTarget(m.v, m.s.pres || 0, m.normPa);
    }
    tick() {
      if (!this.tgt || ++this.cnt < 256) return; this.cnt = 0;
      const a = 1 - Math.exp(-256 / (0.015 * this.fs)); let moved = false;
      for (const k of SMOOTH) { const d = this.tgt[k] - this.cur[k]; if (d === 0) continue; moved = true; if (Math.abs(d) > 1e-5) this.cur[k] += d * a; else this.cur[k] = this.tgt[k]; }
      if (this.tgt.ch1 !== this.cur.ch1) { this.cur.ch1 = this.tgt.ch1; moved = true; }
      if (moved) this.amp.retarget(this.v, this.cur);
    }
    step(u) {
      let y = this.amp.step(u * this.vin) / 20;
      if (this.pa) { this.paCore.tick(); y = this.paCore.step(y); }
      return y;
    }
    get sys() { return true; }
    takePeaks() { const p = this.amp.peak, dc = this.amp.vgkDC, r = Array.from(p, (x, i) => x / Math.abs(dc[i])); p.fill(0); return r; }
  }

  // Verwaltung des Kanalwechsels: Die neue Schaltung startet leer, rechnet die letzten 150 ms Eingangssignal nach
  // (verteilt über mehrere Audioblöcke, damit der Audio-Thread nicht überlastet wird) und wird dann über 30 ms
  // eingeblendet. So sind alle Kondensatoren beim Einblenden eingeschwungen wie im echten Betrieb.
  // ---------- Lautsprecher (wie im C++-Kern): Eminence Legend CB158 im 78-l-Bassreflex 50 Hz ----------
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
  class LaneyEngine {
    constructor(fs) {
      this.fs = fs; this.a = new Core(fs); this.b = null; this.pend = null; this.x = 0; this.bch = null; this.spk = false; this.speaker = speakerDesign(fs);
      this.hist = new Float32Array(Math.round(0.15 * fs)); this.hpos = 0; this.hfill = 0;
      this.q = null; this.qlen = 0; this.qpos = 0;
    }
    get sys() { return this.a.sys; }
    get cur() { return this.a.cur; }
    chOf(core) { return core.cur.ch1 === 1 ? 1 : 2; }
    kindOf(core) { return (core.nl ? 'nl' + core.os : 'lin') + '|' + (core.cur && core.cur.low ? 'low' : 'high'); }
    setTarget(m) {
      if (!!m.spk !== this.spk) { this.spk = !!m.spk; this.speaker.reset(); }
      const want = m.sw.ch, kind = (m.nl ? 'nl' + (m.os || 2) : 'lin') + '|' + (m.sw.inp === 'low' ? 'low' : 'high');
      if (this.pend && kind !== this.kindOf(this.pend)) { this.pend = null; this.bch = null; }   // Art während des Nachrechnens erneut geändert: Nachrechnen verwerfen
      if (!this.a.sys || (!this.a.cur)) { if (!m.nl !== !this.a.nl || (m.nl && this.a.os !== (m.os || 2))) this.a = m.nl ? new NlCore(this.fs, m.os || 2) : new Core(this.fs); this.a.setTarget(m); return; }
      const newestCore = this.b || this.a;
      if (!this.pend && kind !== this.kindOf(newestCore)) {           // Moduswechsel: frische Schaltung einblenden
        if (this.b) { this.a = this.b; this.b = null; }
        if (kind.startsWith('lin')) { this.pend = new Core(this.fs); this.pend.setTarget(m); this.bch = want;
          const n = this.hfill, L = this.hist.length; this.q = new Float32Array(n + Math.round(0.2 * this.fs)); this.qlen = 0; this.qpos = 0;
          for (let i = 0; i < n; i++) this.q[this.qlen++] = this.hist[(this.hpos - n + i + L) % L]; }
        else { this.b = new NlCore(this.fs, m.os || 2); this.b.setTarget(m); this.x = 0; this.bch = want; }
        return;
      }
      if (this.kindOf(this.a).startsWith('nl') && kind === this.kindOf(this.a) && !this.b && want !== this.chOf(this.a)) {   // Kanalwechsel im Verzerrungsmodus
        const old = { ...m, sw: { ...m.sw, ch: this.chOf(this.a) } };
        this.b = new NlCore(this.fs, m.os || 2); this.b.setTarget(m); this.x = 0; this.bch = want; this.a.setTarget(old); return;
      }

      const newest = this.pend ? this.bch : this.b ? this.bch : this.chOf(this.a);
      if (want !== newest) {
        if (this.b) { this.a = this.b; this.b = null; }       // laufende Überblendung abschließen
        const oldCh = this.chOf(this.a);
        if (want === oldCh) { this.pend = null; this.a.setTarget(m); this.bch = null; return; }   // zurück, bevor es fertig war
        if (m.nl) {                                           // Verzerrungsmodus: immer ein Verzerrungskern (frisch im Arbeitspunkt, überblendet)
          this.pend = null; this.b = new NlCore(this.fs, m.os || 2); this.b.setTarget(m); this.x = 0; this.bch = want;
          this.a.setTarget({ ...m, sw: { ...m.sw, ch: oldCh } }); return;
        }
        this.pend = new Core(this.fs); this.pend.setTarget(m); this.bch = want;
        const n = this.hfill, L = this.hist.length;
        this.q = new Float32Array(n + Math.round(0.2 * this.fs)); this.qlen = 0; this.qpos = 0;
        for (let i = 0; i < n; i++) this.q[this.qlen++] = this.hist[(this.hpos - n + i + L) % L];
        this.a.setTarget({ ...m, sw: { ...m.sw, ch: oldCh } });
      } else {
        const oldCh = this.chOf(this.a);
        this.a.setTarget(this.pend || this.b ? { ...m, sw: { ...m.sw, ch: oldCh } } : m);
        if (this.pend) this.pend.setTarget(m); if (this.b) this.b.setTarget(m);
      }
    }
    // pro Audioblock aufrufen: höchstens n Abtastwerte nachrechnen
    work(n) {
      if (!this.pend) return;
      const end = Math.min(this.qlen, this.qpos + n);
      for (; this.qpos < end; this.qpos++) { this.pend.tick(); this.pend.step(this.q[this.qpos]); }
      if (this.qpos >= this.qlen) { this.pend.peak.fill(0); this.b = this.pend; this.pend = null; this.x = 0; this.q = null; }
    }
    tick() { this.a.tick(); if (this.b) this.b.tick(); }
    step(u) {
      this.hist[this.hpos] = u; this.hpos = (this.hpos + 1) % this.hist.length; if (this.hfill < this.hist.length) this.hfill++;
      if (this.pend) { if (this.qlen >= this.q.length) { const q2 = new Float32Array(this.q.length * 2); q2.set(this.q); this.q = q2; } this.q[this.qlen++] = u; }
      const ya = this.a.step(u); let y;
      if (!this.b) y = ya;
      else { const yb = this.b.step(u); this.x += 1 / (0.03 * this.fs);
        if (this.x >= 1) { this.a = this.b; this.b = null; this.bch = null; y = yb; } else y = (1 - this.x) * ya + this.x * yb; }
      if (this.spk) y = this.speaker.step(y);
      return -y;   // Polarität wie am Eingang (wie im C++-Kern)
    }
    takePeaks() { return (this.b || this.a).takePeaks(); }
    rebuild() { if (this.a.rebuild) this.a.rebuild(); }
  }
  // WebAssembly-Kern (C++-Übersetzung desselben Modells): blockweise Verarbeitung, Überabtastung bis 8-fach
  const PORDER = ["r1", "r5", "r6", "vr101", "vr102", "ronvt", "rts", "mu", "rp0", "ia0", "cgk", "cga", "vr108", "c110", "c19", "c20", "r38", "r105", "c2", "c3", "ck", "c4", "c5", "c6", "r18", "c7", "c8", "c11", "c10", "vr105", "vr106", "vr107", "c9", "c12", "c13", "c14", "c15", "c16", "c106", "vr104", "vr103", "c201", "c202", "c18", "c21", "gp", "rpad"];
  class WasmEngine {
    constructor(instance, fs) { this.x = instance.exports; if (this.x._initialize) this.x._initialize(); this.x.aor_init(fs); this.fs = fs; this.ready = false; }
    f64(ptr, n) { return new Float64Array(this.x.memory.buffer, ptr, n); }
    setTarget(m) {
      if (this.x.aor_parts_count() !== PORDER.length) throw new Error('Bauteilliste passt nicht zum WebAssembly-Kern');
      const pb = this.f64(this.x.aor_parts_buffer(), PORDER.length); PORDER.forEach((k, i) => pb[i] = m.v[k]);
      const s = m.s, sw = m.sw, t = [s.v1, s.v2, s.t, s.m, s.b, s.mv, s.pl, s.pres || 0, sw.mid ? 1 : 0, sw.top ? 1 : 0, sw.bass ? 1 : 0, sw.ch === 1 ? 1 : 0, sw.inp === 'low' ? 1 : 0, m.pa ? 1 : 0, m.nl ? m.vin : 1, m.nl ? 1 : 0, m.os || 2, ...((s.tubes && s.tubes.length === 3) ? s.tubes.map(Number) : [0, 0, 0]), m.spk ? 1 : 0];
      const tb = this.f64(this.x.aor_target_buffer(), 21); t.forEach((v, i) => tb[i] = +v);
      this.x.aor_set_target(); this.ready = true;
    }
    process(buf, n) { const b = new Float32Array(this.x.memory.buffer, this.x.aor_buffer(), n); b.set(buf.subarray(0, n)); this.x.aor_process(n); buf.set(new Float32Array(this.x.memory.buffer, this.x.aor_buffer(), n)); }
    takePeaks() { this.x.aor_take_peaks(); return Array.from(this.f64(this.x.aor_peaks_buffer(), 5)); }
  }
  const api = { build, LaneyEngine, MON, NlCore, NLAMP, NL, WasmEngine };
  if (typeof registerProcessor === 'function') {
    class LaneyProcessor extends AudioWorkletProcessor {
      constructor(options) {
        super(); this.eng = new LaneyEngine(sampleRate); this.bypass = false; this.mix = 1; this.rep = 0;
        // Bypass mit Pegelangleich: Effektivwerte von Verstärkerausgang und Originalsignal werden mit 1 s Zeitkonstante gemittelt;
        // im Bypass wird das Original mit dem zuletzt (vor dem Umschalten) gemessenen Verhältnis skaliert, damit der Vergleich die Klangfarbe zeigt, nicht die Lautstärke
        this.match = true; this.msDry = 0; this.msAmp = 0; this.gMatch = 1; this.avg = 1 / (1.0 * sampleRate);
        const init = options && options.processorOptions;
        if (init && init.params) this.eng.setTarget(init.params);
        if (init && 'bypass' in init) { this.bypass = !!init.bypass; this.mix = this.bypass ? 0 : 1; }
        if (init && 'match' in init) this.match = !!init.match;
        this.weng = null; this.last = init && init.params ? init.params : null; this.mono = new Float32Array(128);   // last auch aus den Startoptionen, damit ein später geladener WebAssembly-Kern die Einstellung bekommt
        this.port.onmessage = e => { const m = e.data;
          if (m.wasm) { const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
            WebAssembly.instantiate(m.wasm, imports).then(r => { const inst = r.instance || r; this.weng = new WasmEngine(inst, sampleRate); if (this.last) this.weng.setTarget(this.last); this.port.postMessage({ wasm: true }); })
              .catch(err => this.port.postMessage({ wasm: false, error: String(err) })); return; }
          if (m.params) { this.last = m.params; if (this.weng) this.weng.setTarget(m.params); else this.eng.setTarget(m.params); }
          if ('bypass' in m) this.bypass = !!m.bypass;
          if ('match' in m) this.match = !!m.match; };
      }
      // Verhältnis der Spitzenwerte nachführen; nur solange der Verstärker hörbar ist (sonst bliebe das Verhältnis im Bypass stehen)
      track(dry, amp, len) {
        let md = this.msDry, ma = this.msAmp; const k = this.avg;
        for (let n = 0; n < len; n++) { md += k * (dry[n] * dry[n] - md); ma += k * (amp[n] * amp[n] - ma); }
        this.msDry = md; this.msAmp = ma;
        if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md)));
      }
      dryGain() { return this.match ? this.gMatch : 1; }
      process(inputs, outputs) {
        const inp = inputs[0], out = outputs[0], eng = this.eng;
        if (!inp || inp.length === 0 || (!this.weng && !eng.sys)) { for (const ch of out) ch.fill(0); return true; }
        const len = out[0].length, target = this.bypass ? 0 : 1, step = 1 / (0.02 * sampleRate); let mix = this.mix;
        if (this.weng && this.weng.ready) {
          if (this.mono.length < len) this.mono = new Float32Array(len); const mo = this.mono;
          for (let n = 0; n < len; n++) { let u = 0; for (let c = 0; c < inp.length; c++) u += inp[c][n]; mo[n] = u / inp.length; }
          const dry = Float32Array.from(mo.subarray(0, len)); this.weng.process(mo, len); this.track(dry, mo, len); const gd = this.dryGain();
          for (let n = 0; n < len; n++) { if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
            const o = mix * mo[n] + (1 - mix) * gd * dry[n]; for (let c = 0; c < out.length; c++) out[c][n] = o; }
          this.mix = mix; if ((this.rep += len) >= sampleRate / 10) { this.rep = 0; this.port.postMessage({ peaks: this.weng.takePeaks() }); } return true;
        }
        eng.work(384);
        const gd = this.dryGain(); let md = this.msDry, ma = this.msAmp; const k = this.avg;
        for (let n = 0; n < len; n++) {
          let u = 0; for (let c = 0; c < inp.length; c++) u += inp[c][n]; u /= inp.length;     // Mono
          eng.tick(); const y = eng.step(u);
          md += k * (u * u - md); ma += k * (y * y - ma);
          if (mix < target) mix = Math.min(target, mix + step); else if (mix > target) mix = Math.max(target, mix - step);
          const o = mix * y + (1 - mix) * gd * u; for (let c = 0; c < out.length; c++) out[c][n] = o;
        }
        this.mix = mix; this.msDry = md; this.msAmp = ma; if (!this.bypass && md > 1e-8 && ma > 1e-8) this.gMatch = Math.min(100, Math.max(0.01, Math.sqrt(ma / md)));
        if ((this.rep += len) >= sampleRate / 10) { this.rep = 0; this.port.postMessage({ peaks: eng.takePeaks() }); }
        return true;
      }
    }
    registerProcessor('laney-preamp', LaneyProcessor);
  } else if (typeof module !== 'undefined') module.exports = api;
  else globalThis.LaneyDSP = api;
})();
