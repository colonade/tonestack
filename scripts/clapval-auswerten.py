#!/usr/bin/env python3
# Wertet die JSON-Ausgabe von clap-validator aus (ab 2026-11-27; aufgerufen von scripts/check-linux-plugins.sh):
#   python3 scripts/clapval-auswerten.py <ergebnis.json> <Name>
# Rückgabe 1 bei Fehlern oder Abstürzen; Abbruch am Zeitlimit des Prüfers („Timed out after 45 seconds“) nur als Warnung (GitHub-Hinweis),
# Warnungen des Prüfers werden aufgelistet.
import json, sys
try:
    d = json.load(open(sys.argv[1]))
except Exception as e:
    print('  clap-validator: keine Auswertung (%s)' % e); sys.exit(1)
name = sys.argv[2]; st = {}; bad = []; slow = []; warn = []
for r in d.get('results', []):
    t = list(r['test'].values())[0]['test']; c = r['status']['code']; det = ' '.join((r['status'].get('details') or '').split())
    st[c] = st.get(c, 0) + 1
    if c in ('failed', 'crashed'):
        (slow if det.startswith('Timed out after') else bad).append((t, det))
    elif c == 'warning':
        warn.append((t, det))
print('  clap-validator: ' + ', '.join('%d %s' % (v, k) for k, v in sorted(st.items())))
for t, det in warn: print('    Warnung %s: %s' % (t, det[:300]))
for t, det in slow: print('::warning title=clap-validator %s::%s – %s (Zeitlimit des Prüfers, kein Fehler im Plugin festgestellt)' % (name, t, det))
for t, det in bad: print('    FEHLER %s: %s' % (t, det[:600]))
sys.exit(1 if bad or not st else 0)
