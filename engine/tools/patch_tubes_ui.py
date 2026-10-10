# Röhrenauswahl neben das Verzerrungs-Häkchen, Farben je Röhre, Erklärungstexte eingeklappt (Laney-Seite, Web und App). Idempotent.
import re, sys, os
R = sys.argv[1]
def patch(p):
    h = open(p, encoding='utf-8').read()
    if 'tubemenu' in h: return 'bereits'
    def rep(o, n, cnt=1):
        nonlocal h
        assert h.count(o) == cnt, (p, o[:60], h.count(o)); h = h.replace(o, n)
    # 1) alte Auswahlzeile unter den Bauteiloptionen entfernen
    i = h.index('<div class="opt" id="tubesel"'); j = h.index('</div>', i) + len('</div>'); h = h[:i] + h[j:]
    # 2) neue Zeile neben dem Verzerrungs-Häkchen
    rep('    <label class="opt" style="margin:0"><input type="checkbox" id="anl" checked> <span><b>Röhrenverzerrung einrechnen (Schätzung)</b></span></label>',
        '''    <div class="nlrow">
      <label class="opt" style="margin:0"><input type="checkbox" id="anl" checked> <span><b>Röhrenverzerrung einrechnen (Schätzung)</b></span></label>
      <div class="tubemenu" id="tubesel" aria-label="Röhrenbestückung">
        <label>V101 <select id="tube0"></select></label><label>V102 <select id="tube1"></select></label><label>V103 <select id="tube2"></select></label>
        <button class="btn" type="button" id="tubereset" title="Bestückung wie im Gerät: 3 × ECC83">Zurücksetzen</button>
      </div>
    </div>''')
    # 3) Farben je Röhre, Stile
    rep('.opt{display:flex;gap:8px;align-items:flex-start;margin:4px 0;font-size:.92rem}',
        '''.opt{display:flex;gap:8px;align-items:flex-start;margin:4px 0;font-size:.92rem}
.nlrow{display:flex;flex-wrap:wrap;align-items:center;gap:8px 18px}
.tubemenu{display:flex;flex-wrap:wrap;align-items:center;gap:6px 10px;font-size:.88rem}
.tubemenu label{display:inline-flex;align-items:center;gap:5px;color:var(--ink2)}
.tubemenu select{font:inherit;font-weight:600;padding:3px 6px;border-radius:5px;border:2px solid var(--t0);background:var(--field);color:var(--ink)}
.tubemenu select.t1{border-color:var(--t1)}.tubemenu select.t2{border-color:var(--t2)}.tubemenu select.t3{border-color:var(--t3)}.tubemenu select.t4{border-color:var(--t4)}
.tubemenu select.t0{border-color:var(--t0)}
.tubemenu option.t0{color:var(--t0)}.tubemenu option.t1{color:var(--t1)}.tubemenu option.t2{color:var(--t2)}.tubemenu option.t3{color:var(--t3)}.tubemenu option.t4{color:var(--t4)}
.tubemenu .btn{padding:3px 10px;font-size:.85rem}
:root{--t0:#8a8f99;--t1:#2f8f5b;--t2:#2a6fd6;--t3:#c2731a;--t4:#9a4fc9}
details.exp{margin:8px 0 0}details.exp>summary{cursor:pointer;color:var(--ink2);font-size:.86rem;list-style:none;display:inline-flex;align-items:center;gap:6px}
details.exp>summary::before{content:"▸";display:inline-block;transition:transform .15s}details.exp[open]>summary::before{transform:rotate(90deg)}
details.exp>summary::-webkit-details-marker{display:none}''')
    # 4) Auswahlmenüs füllen mit Farbklassen
    rep("for (let i = 0; i < 3; i++) { const sel = $('tube' + i); TUBES.forEach((t, k) => { const o = document.createElement('option'); o.value = k; o.textContent = t[0] + (t[1] ? ' (angepasstes Modell)' : ''); sel.appendChild(o); });",
        "for (let i = 0; i < 3; i++) { const sel = $('tube' + i); TUBES.forEach((t, k) => { const o = document.createElement('option'); o.value = k; o.className = 't' + k; o.textContent = t[0] + (t[1] ? ' *' : ''); sel.appendChild(o); });")
    rep("const syncTubes = () => { for (let i = 0; i < 3; i++) { const sel = $('tube' + i); if (+sel.value !== S.tubes[i]) sel.value = S.tubes[i]; }",
        "const syncTubes = () => { for (let i = 0; i < 3; i++) { const sel = $('tube' + i); if (+sel.value !== S.tubes[i]) sel.value = S.tubes[i]; sel.className = 't' + S.tubes[i]; }")
    rep("['12AX7 / ECC83 (wie im Gerät)', false]", "['12AX7 / ECC83 (Gerät)', false]")
    # 5) Erklärungen eingeklappt: Kopftext, Bedienhinweis, Verzerrungstexte, Hinweise im Anhörbereich, Fußnoten
    rep('<header>\n  <h1>Laney AOR 50: Vorverstärker mit allen Reglern</h1>\n  <p>', '<header>\n  <h1>Laney AOR 50: Vorverstärker mit allen Reglern</h1>\n  <details class="exp"><summary>Worum es geht</summary>\n  <p>')
    i = h.index('<details class="exp"><summary>Worum es geht</summary>'); j = h.index('</p>', i) + 4; h = h[:j] + '\n  </details>' + h[j:]
    i = h.index('<figcaption>'); j = h.index('</figcaption>', i) + len('</figcaption>'); inner = h[i + len('<figcaption>'):j - len('</figcaption>')]
    h = h[:i] + '<figcaption><details class="exp"><summary>Bedienung der Frontplatte</summary>' + inner + '</details></figcaption>' + h[j:]
    rep('    <p id="nloff" style="margin:6px 0 0">', '    <details class="exp" id="nlexp"><summary>Was die Verzerrung rechnet, Röhrenmodelle, Überabtastung</summary>\n    <p id="nloff" style="margin:6px 0 0">')
    i = h.index('<div id="nlon" hidden>'); j = h.index('\n    </div>', i) + len('\n    </div>'); h = h[:j] + '\n    </details>' + h[j:]
    # Überabtastung muss bedienbar bleiben: Auswahl aus dem Klapptext heraus vor die Erklärung ziehen
    i = h.index('      <div class="avol" style="margin-top:8px"><label for="aos">Überabtastung</label>'); j = h.index('</div>', i) + len('</div>'); aos = h[i:j]; h = h[:i] + h[j:]
    rep('    <details class="exp" id="nlexp">', aos.strip() + '\n    <p class="note" id="tubenote" style="margin:4px 0 0">Röhren je Fassung (Doppeltrioden); * = an den Datenblatt-Arbeitspunkt angepasstes Modell, abseits davon weniger gesichert.</p>\n    <details class="exp" id="nlexp">')
    rep('  <div id="linnotes">\n', '  <details class="exp"><summary>Hinweise zu Pegel und Verarbeitung</summary>\n  <div id="linnotes">\n')
    i = h.index('<div id="nlnotes" hidden>'); j = h.index('\n  </div>', i) + len('\n  </div>'); h = h[:j] + '\n  </details>' + h[j:]
    rep('<footer class="note">\n', '<footer class="note">\n  <details class="exp"><summary>Rechenweg, Kanäle, Eingänge, Annahmen</summary>\n')
    i = h.index('<details class="exp"><summary>Rechenweg, Kanäle'); j = h.index('</footer>', i); h = h[:j] + '  </details>\n' + h[j:]
    open(p, 'w', encoding='utf-8').write(h); return 'ok'
for f in ['web/tonestack/laney-aor50/index.html', 'app/laney/assets/index.html']: print(f, patch(os.path.join(R, f)))
