# Röhrenbestückung in Laney-Seite (Web und App) und Worklets einbauen. Einmalig; idempotent (prüft vorhandene Marke).
import re, sys, os
R = sys.argv[1]
def patch_worklet(p):
    w = open(p, encoding='utf-8').read()
    if 'tubes' in w: return 'bereits'
    o = "m.nl ? m.vin : 1, m.nl ? 1 : 0, m.os || 2];"; assert w.count(o) == 1
    w = w.replace(o, "m.nl ? m.vin : 1, m.nl ? 1 : 0, m.os || 2, ...((s.tubes && s.tubes.length === 3) ? s.tubes.map(Number) : [0, 0, 0])];")
    o2 = "const tb = this.f64(this.x.aor_target_buffer(), 17);"; assert w.count(o2) == 1
    w = w.replace(o2, "const tb = this.f64(this.x.aor_target_buffer(), 20);")
    open(p, 'w', encoding='utf-8').write(w); return 'ok'
def patch_page(p, porder):
    h = open(p, encoding='utf-8').read()
    if 'id="tubesel"' in h: return 'bereits'
    def rep(o, n, cnt=1):
        nonlocal h
        assert h.count(o) == cnt, (p, o[:60], h.count(o)); h = h.replace(o, n)
    rep("showRef:true, pa:", "showRef:true, tubes:[0,0,0], pa:")
    rep("for (const p of P) if (!(p[0] in S.parts)) S.parts[p[0]] = p[4];", "for (const p of P) if (!(p[0] in S.parts)) S.parts[p[0]] = p[4];\nif (!Array.isArray(S.tubes) || S.tubes.length !== 3) S.tubes = [0,0,0];   // ältere Speicherstände\nconst TUBES = [['12AX7 / ECC83 (wie im Gerät)', false], ['12AT7 / ECC81', false], ['12AU7 / ECC82', false], ['5751', true], ['12AY7', true]];\nconst stockTubes = t => t.every(x => +x === 0);")
    rep("window.LANEY = { current: () => ({ v: vals(), s: setting(S.knobs), sw: {...S.sw}, pa: S.pa }),", "window.LANEY = { current: () => ({ v: vals(), s: {...setting(S.knobs), tubes: S.tubes.slice()}, sw: {...S.sw}, pa: S.pa }),")
    rep("snapshot: () => cmp ? { v: cmp.v, s: setting(cmp.kn), sw: {...cmp.sw}, pa: S.pa } : null,", "snapshot: () => cmp ? { v: cmp.v, s: {...setting(cmp.kn), tubes: (cmp.tubes || [0,0,0])}, sw: {...cmp.sw}, pa: S.pa } : null,")
    rep("$('snap').onclick = () => { cmp = {id:++cmpId, kn:{...S.knobs}, v:vals(), sw:{...S.sw}}; render(); };", "$('snap').onclick = () => { cmp = {id:++cmpId, kn:{...S.knobs}, v:vals(), sw:{...S.sw}, tubes:S.tubes.slice()}; render(); };")
    # Diagramm-Kern: Trioden mit Kleinsignalwerten der Bestückung
    o = "    const triode = (g,p,k,Ia) => { const rp = rpOf(Ia, v); R(p,k,rp); TR.push([N(g),N(p),N(k),v.mu/rp]); C(g,k,v.cgk); C(g,p,v.cga); };"
    i = h.index(o)
    h = h[:i] + "    let ti = 0; const triode = (g,p,k,Ia) => { const q = s.ss && s.ss[ti]; ti++; if (q) { R(p,k,q.rp); TR.push([N(g),N(p),N(k),q.gm]); C(g,k,q.cgk); C(g,p,q.cga); return; } const rp = rpOf(Ia, v); R(p,k,rp); TR.push([N(g),N(p),N(k),v.mu/rp]); C(g,k,v.cgk); C(g,p,v.cga); };   // ss: Kleinsignalwerte je System bei fremder Röhrenbestückung" + h[i+len(o):]
    rep("const evalFor = (kn, v, sw, pa) => { const ckt = ENGINE.build(v, setting(kn), sw),", "const evalFor = (kn, v, sw, pa, tubes) => { const st = setting(kn); if (tubes && !stockTubes(tubes)) st.ss = tubeSS(v, st, sw, tubes); const ckt = ENGINE.build(v, st, sw),")
    rep("evalFor(S.knobs, v, S.sw, S.pa)", "evalFor(S.knobs, v, S.sw, S.pa, S.tubes)")
    rep("evalFor(S.knobs, vals(), S.sw, S.pa)", "evalFor(S.knobs, vals(), S.sw, S.pa, S.tubes)")
    rep("evalFor(cmp.kn, cmp.v, cmp.sw, S.pa)", "evalFor(cmp.kn, cmp.v, cmp.sw, S.pa, cmp.tubes)", 2)
    rep("cached(`cmp${cmp.id}|${S.mode}|${S.taper}|${S.pa}`", "cached(`cmp${cmp.id}|${S.mode}|${S.taper}|${S.pa}|${(cmp.tubes||[]).join('')}`")
    # Kleinsignalwerte aus dem WebAssembly-Kern (Hauptthread, zwischengespeichert)
    rep("// Schnittstelle für den Audioteil (nur Apache-Fassung)", r"""// Kleinsignalwerte (gm, rp, cgk, cga je Triodensystem) aus dem WebAssembly-Kern für eine Bestückung; ohne WebAssembly bleibt es beim 12AX7-Modell
const TUBE_PORDER = """ + porder + r""";
const tubeSS = (() => { let inst = null, failed = false; const cache = new Map();
  const get = () => { if (inst || failed) return inst; try { const b64 = document.getElementById('wasmcore'); if (!b64 || typeof WebAssembly !== 'object') { failed = true; return null; }
      const bin = Uint8Array.from(atob(b64.textContent.trim()), c => c.charCodeAt(0)); const imports = { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => {} } };
      const x = new WebAssembly.Instance(new WebAssembly.Module(bin), imports).exports; if (x._initialize) x._initialize(); x.aor_init(48000); inst = x; } catch (e) { failed = true; } return inst; };
  return (v, st, sw, tubes) => { const key = tubes.join('') + '|' + TUBE_PORDER.map(k => v[k]).join(','); if (cache.has(key)) return cache.get(key); const x = get(); if (!x) return null;
    const pb = new Float64Array(x.memory.buffer, x.aor_parts_buffer(), TUBE_PORDER.length); TUBE_PORDER.forEach((k, i) => pb[i] = v[k]);
    const t = [st.v1, st.v2, st.t, st.m, st.b, st.mv, st.pl, st.pres || 0, 0, 0, 0, sw.ch === 1 ? 1 : 0, sw.inp === 'low' ? 1 : 0, 0, 1, 1, 1, ...tubes.map(Number)];
    const tb = new Float64Array(x.memory.buffer, x.aor_target_buffer(), 20); t.forEach((q, i) => tb[i] = +q); x.aor_set_target(); x.aor_small_signal();
    const r = new Float64Array(x.memory.buffer, x.aor_ss_buffer(), 24); const ss = []; for (let i = 0; i < 6; i++) ss.push({gm: r[4*i], rp: r[4*i+1], cgk: r[4*i+2], cga: r[4*i+3]});
    if (cache.size > 64) cache.clear(); cache.set(key, ss); return ss; }; })();

// Schnittstelle für den Audioteil (nur Apache-Fassung)""")
    # Bedienung: Auswahl je Fassung und Zurücksetzen
    o = '<label class="opt"><input type="checkbox" id="pa">'; i = h.index(o); j = h.index('</label>', i) + len('</label>')
    h = h[:j] + """
    <div class="opt" id="tubesel" style="display:flex;flex-wrap:wrap;align-items:center;gap:8px 14px">
      <span>Röhrenbestückung:</span>
      <label>V101 <select id="tube0"></select></label><label>V102 <select id="tube1"></select></label><label>V103 <select id="tube2"></select></label>
      <button class="btn" type="button" id="tubereset">Zurücksetzen auf Gerät (3 × ECC83)</button>
      <span id="tubenote" style="color:var(--ink2);font-size:.9em">Je Fassung eine Doppeltriode. Arbeitspunkte aus dem Röhrenmodell; 5751 und 12AY7 sind an den Datenblatt-Arbeitspunkt angepasste Modelle (abseits davon weniger gesichert).</span>
    </div>""" + h[j:]
    rep("$('pa').onchange = e => { S.pa = e.target.checked; render(); };", """$('pa').onchange = e => { S.pa = e.target.checked; render(); };
for (let i = 0; i < 3; i++) { const sel = $('tube' + i); TUBES.forEach((t, k) => { const o = document.createElement('option'); o.value = k; o.textContent = t[0] + (t[1] ? ' (angepasstes Modell)' : ''); sel.appendChild(o); });
  sel.onchange = e => { S.tubes[i] = +e.target.value; render(); }; }
$('tubereset').onclick = () => { S.tubes = [0,0,0]; render(); };
const syncTubes = () => { for (let i = 0; i < 3; i++) { const sel = $('tube' + i); if (+sel.value !== S.tubes[i]) sel.value = S.tubes[i]; } $('tubereset').disabled = stockTubes(S.tubes); $('tubesel').classList.toggle('mod', !stockTubes(S.tubes)); };""")
    # syncTubes bei jedem render (wo $('mod').checked gesetzt wird)
    rep("  $('mod').checked = P.filter(p => p[5] !== null).every(p => S.parts[p[0]] === p[5]);", "  $('mod').checked = P.filter(p => p[5] !== null).every(p => S.parts[p[0]] === p[5]); syncTubes();")
    open(p, 'w', encoding='utf-8').write(h); return 'ok'
wk = open(os.path.join(R, 'web/tonestack/laney-aor50/laney-worklet.js'), encoding='utf-8').read()
porder = re.search(r"const PORDER = (\[[^\]]*\]);", wk).group(1)
for f in ['web/tonestack/laney-aor50/laney-worklet.js', 'app/laney/assets/laney-worklet.js']: print(f, patch_worklet(os.path.join(R, f)))
for f in ['web/tonestack/laney-aor50/index.html', 'app/laney/assets/index.html']: print(f, patch_page(os.path.join(R, f), porder))
