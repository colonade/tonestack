// Messung (ab Web 2026-11-16-5): ein Prüfton läuft offline durch denselben Rechenkern wie beim Speichern als WAV,
// danach Grundton und Oberschwingungen k2 … k10 (dB und % bezogen auf den Grundton), Klirrfaktor und Spektrum.
// Pegel: der Prüfton hat seine Spitze bei INPUT (wie die lauteste Stelle einer Datei). Die Seite stellt bereit:
//   window.TSMESS = { render(buf, onProgress) → Promise<Float32Array> (Ausgang, gleiche Länge, Latenz ausgeglichen), busy() → bool }
// Anregung des Kritikers (Vergleich mit LTspice): Einstellungen, Pegel und Fenster sind dann eindeutig.
(() => {
  'use strict';
  const M = window.TSMESS, anchor = document.getElementById('sec-plot'); if (!M || !anchor) return;
  const FS = 48000, LEAD = 2.0, N = 65536, FREQS = [100, 220, 400, 1000, 2000];   // 2 s Einschwingen (Speisung, Koppelkondensatoren), dann 65536 Werte
  const $ = id => document.getElementById(id), de = (x, d = 1) => x.toFixed(d).replace('.', ',').replace('-', '−');
  const css = `#sec-mess .mrow{display:flex;flex-wrap:wrap;gap:10px 16px;align-items:center;margin:8px 0}
#sec-mess select{font:inherit}
#sec-mess table{border-collapse:collapse;font-variant-numeric:tabular-nums;margin:10px 0;font-size:.92rem}
#sec-mess th,#sec-mess td{padding:3px 10px;border-bottom:1px solid var(--line,#8884);text-align:right}
#sec-mess th:first-child,#sec-mess td:first-child{text-align:left}
#sec-mess canvas{width:100%;height:260px;display:block;margin-top:8px;border:1px solid var(--line,#8884);border-radius:8px}
#sec-mess .mnote{font-size:.85rem;opacity:.8;margin:6px 0 0}
#sec-mess .mwrap{overflow-x:auto}`;
  const st = document.createElement('style'); st.textContent = css; document.head.appendChild(st);
  const sec = document.createElement('section'); sec.className = 'panel'; sec.id = 'sec-mess'; sec.setAttribute('aria-labelledby', 'ph-mess');
  sec.innerHTML = `<h2 id="ph-mess">Messung · Klirr mit Prüfton</h2>
<p class="mnote">Ein Sinus läuft einmal durch die Schaltung, mit allen Einstellungen dieser Seite (auch den Modell-Schaltern) und so wie beim Speichern als WAV. Seine Spitze ist der Wert von INPUT. Ausgewertet werden 65536 Werte bei 48 kHz nach 2 s Einschwingen, mit Blackman-Harris-Fenster.</p>
<div class="mrow"><label>Frequenz <select id="mfreq">${FREQS.map(f => `<option value="${f}"${f === 400 ? ' selected' : ''}>${f} Hz</option>`).join('')}</select></label>
<span id="mlev"></span><button class="btn" type="button" id="mgo">Messen</button><button class="btn" type="button" id="mcopy" disabled>Werte kopieren</button><span id="mstat" aria-live="polite"></span></div>
<div class="mwrap"><table id="mtab" hidden></table></div><canvas id="mcan" hidden></canvas>`;
  anchor.after(sec);
  const lev = () => { const t = $('avinv'); $('mlev').textContent = 'Pegel: ' + (t ? t.textContent : '?') + ' (INPUT)'; };
  lev(); { const a = $('avin'); if (a) a.addEventListener('input', lev); }
  // FFT (Radix 2, an Ort und Stelle)
  function fft(re, im) { const n = re.length; for (let i = 1, j = 0; i < n; i++) { let b = n >> 1; for (; j & b; b >>= 1) j ^= b; j ^= b; if (i < j) { [re[i], re[j]] = [re[j], re[i]]; [im[i], im[j]] = [im[j], im[i]]; } }
    for (let len = 2; len <= n; len <<= 1) { const a = -2 * Math.PI / len, wr = Math.cos(a), wi = Math.sin(a);
      for (let i = 0; i < n; i += len) { let cr = 1, ci = 0; for (let k = 0; k < len / 2; k++) { const p = i + k, q = p + len / 2, tr = re[q] * cr - im[q] * ci, ti = re[q] * ci + im[q] * cr;
        re[q] = re[p] - tr; im[q] = im[p] - ti; re[p] += tr; im[p] += ti; const t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t; } } } }
  let last = null;
  async function run() {
    if (M.busy && M.busy()) { $('mstat').textContent = 'Es wird gerade gerechnet – bitte danach.'; return; }
    const f = +$('mfreq').value, n0 = Math.round(LEAD * FS), len = n0 + N, fade = Math.round(0.05 * FS);
    let buf; try { buf = new AudioBuffer({ length: len, numberOfChannels: 1, sampleRate: FS }); } catch (e) { $('mstat').textContent = 'Dieser Browser kann hier nicht messen.'; return; }
    const x = new Float32Array(len); for (let i = 0; i < len; i++) x[i] = Math.sin(2 * Math.PI * f * i / FS) * (i < fade ? 0.5 - 0.5 * Math.cos(Math.PI * i / fade) : 1);
    buf.copyToChannel(x, 0); $('mgo').disabled = true; $('mstat').textContent = 'Rechne … 0 %'; const levText = $('mlev').textContent;
    try {
      const y = await M.render(buf, p => { $('mstat').textContent = p >= 0 ? 'Rechne … ' + Math.round(p * 100) + ' %' : 'Rechne … ' + Math.round(-p) + ' s'; });
      const a0 = [0.35875, 0.48829, 0.14128, 0.01168], w = new Float64Array(N); let sw = 0, mean = 0;
      for (let i = 0; i < N; i++) { const t = 2 * Math.PI * i / (N - 1); w[i] = a0[0] - a0[1] * Math.cos(t) + a0[2] * Math.cos(2 * t) - a0[3] * Math.cos(3 * t); sw += w[i]; mean += y[n0 + i]; }
      mean /= N; let ok = true; for (let i = 0; i < N; i++) if (!Number.isFinite(y[n0 + i])) { ok = false; break; }
      if (!ok) throw new Error('Ausgang nicht endlich');
      // Oberschwingungen: Fourier-Summe genau bei m·f (mit Fenster), bis 10 oder bis 20 kHz
      const H = []; for (let m = 1; m <= 10 && m * f < 20000; m++) { const om = 2 * Math.PI * m * f / FS; let cr = 0, ci = 0;
        for (let i = 0; i < N; i++) { const v = (y[n0 + i] - mean) * w[i], ph = om * (n0 + i); cr += v * Math.cos(ph); ci -= v * Math.sin(ph); } H.push(2 * Math.hypot(cr, ci) / sw); }
      const re = new Float64Array(N), im = new Float64Array(N); for (let i = 0; i < N; i++) re[i] = (y[n0 + i] - mean) * w[i]; fft(re, im);
      const sp = new Float64Array(N / 2); for (let k = 0; k < N / 2; k++) sp[k] = 2 * Math.hypot(re[k], im[k]) / sw;
      let thd = 0; for (let m = 2; m <= H.length; m++) thd += H[m - 1] * H[m - 1]; thd = Math.sqrt(thd) / H[0];
      last = { f, H, sp, thd, lev: levText, when: new Date() }; show(last); $('mstat').textContent = '';
    } catch (err) { $('mstat').textContent = 'Messung nicht möglich: ' + (err && err.message ? err.message : err); }
    finally { $('mgo').disabled = false; }
  }
  function show(r) {
    const t = $('mtab'), db = v => 20 * Math.log10(Math.max(v, 1e-12));
    let h = `<tr><th>Anteil</th><th>Frequenz</th><th>dB re Grundton</th><th>%</th></tr><tr><td>Grundton</td><td>${r.f} Hz</td><td colspan="2">${de(db(r.H[0]))} dBFS am Ausgang</td></tr>`;
    for (let m = 2; m <= r.H.length; m++) { const d = db(r.H[m - 1] / r.H[0]); h += `<tr><td>k${m}</td><td>${m * r.f} Hz</td><td>${de(d)}</td><td>${(100 * r.H[m - 1] / r.H[0]).toFixed(m === 2 || m === 3 ? 2 : 3).replace('.', ',')}</td></tr>`; }
    h += `<tr><td>Klirrfaktor (k2…k${r.H.length})</td><td></td><td>${de(db(r.thd))}</td><td>${(100 * r.thd).toFixed(2).replace('.', ',')}</td></tr>`;
    t.innerHTML = h; t.hidden = false; $('mcopy').disabled = false; draw(r); }
  function draw(r) {
    const c = $('mcan'); c.hidden = false; const dpr = window.devicePixelRatio || 1, W = c.clientWidth || 600, Hh = c.clientHeight || 260; c.width = Math.round(W * dpr); c.height = Math.round(Hh * dpr);
    const g = c.getContext('2d'); g.setTransform(dpr, 0, 0, dpr, 0, 0); g.clearRect(0, 0, W, Hh);
    const cs = getComputedStyle(sec), ink = cs.color || '#888', acc = cs.getPropertyValue('--acc').trim() || '#3a7', L = 44, R = 10, T = 10, B = 24, x0 = Math.log10(20), x1 = Math.log10(20000);
    const X = f => L + (W - L - R) * (Math.log10(f) - x0) / (x1 - x0), Y = d => T + (Hh - T - B) * (-d / 140);
    g.font = '11px system-ui, sans-serif'; g.fillStyle = ink; g.strokeStyle = ink; g.globalAlpha = 0.25; g.lineWidth = 1;
    for (let d = 0; d >= -140; d -= 20) { g.beginPath(); g.moveTo(L, Y(d) + 0.5); g.lineTo(W - R, Y(d) + 0.5); g.stroke(); }
    for (const f of [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000]) { g.beginPath(); g.moveTo(X(f) + 0.5, T); g.lineTo(X(f) + 0.5, Hh - B); g.stroke(); }
    g.globalAlpha = 0.85; for (let d = 0; d >= -140; d -= 20) g.fillText(String(d).replace('-', '−'), 4, Y(d) + 4);
    for (const [f, s] of [[100, '100 Hz'], [1000, '1 kHz'], [10000, '10 kHz']]) g.fillText(s, X(f) - 14, Hh - 6);
    g.globalAlpha = 1; g.strokeStyle = acc; g.lineWidth = 1.2; g.beginPath(); const ref = r.H[0], df = FS / N; let px = -1, mx = -1e9, started = false;
    for (let k = Math.ceil(20 / df); k < r.sp.length && k * df <= 20000; k++) { const x = Math.round(X(k * df)), d = Math.max(-150, 20 * Math.log10(Math.max(r.sp[k] / ref, 1e-9)));
      if (x !== px) { if (px >= 0) { const yy = Y(Math.max(-140, mx)); started ? g.lineTo(px, yy) : g.moveTo(px, yy); started = true; } px = x; mx = d; } else if (d > mx) mx = d; }
    g.stroke(); g.fillStyle = ink; g.fillText('dB re Grundton', L + 6, T + 12); }
  function text(r) { const db = v => 20 * Math.log10(Math.max(v, 1e-12)); let s = `Messung ${document.title} – ${r.f} Hz, ${r.lev}, 48 kHz, Blackman-Harris, 65536 Werte\n`;
    s += `Grundton ${de(db(r.H[0]))} dBFS\n`; for (let m = 2; m <= r.H.length; m++) s += `k${m} ${de(db(r.H[m - 1] / r.H[0]))} dB (${(100 * r.H[m - 1] / r.H[0]).toFixed(3).replace('.', ',')} %)\n`;
    return s + `Klirrfaktor ${(100 * r.thd).toFixed(2).replace('.', ',')} %\n`; }
  $('mgo').onclick = run;
  $('mcopy').onclick = async () => { if (!last) return; const s = text(last); try { await navigator.clipboard.writeText(s); $('mstat').textContent = 'Kopiert.'; }
    catch (e) { const ta = document.createElement('textarea'); ta.value = s; sec.appendChild(ta); ta.select(); try { document.execCommand('copy'); $('mstat').textContent = 'Kopiert.'; } catch (e2) { $('mstat').textContent = 'Kopieren nicht möglich.'; } ta.remove(); } };
  window.addEventListener('resize', () => { if (last) draw(last); });
})();
