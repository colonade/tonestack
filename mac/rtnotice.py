# Ergebnisse der Mac-Prüfprogramme (Zeilen "RESULT ...") als GitHub-Hinweis ausgeben (über die API lesbar) und auf harte Fehler prüfen.
#   python3 mac/rtnotice.py <Titel> <Protokoll>
# Fehler (Rückgabe 1): nicht endliche Ausgaben (nan=…>0), Fehler der Prüfprogramme (kind=*_error), Sanitizer-Meldungen, fehlende Ergebnisse.
# Zeitwerte sind nur Hinweise (die Rechner der CI schwanken), keine Fehlergrenze.
import re, sys
title, path = sys.argv[1], sys.argv[2]
txt = open(path, errors='replace').read()
res = [l for l in txt.splitlines() if l.startswith('RESULT') or l.startswith('WORST') or l.startswith('== ')]
bad = []
for l in res:
    m = re.search(r'\bnan=(\d+)', l)
    if m and int(m.group(1)) > 0: bad.append(l)
    if re.search(r'kind=\w*_error', l): bad.append(l)
for pat in ('ERROR: AddressSanitizer', 'runtime error:', 'Segmentation fault', 'Abort trap', 'libc++abi: terminating'):
    if pat in txt: bad.append(pat)
def esc(s): return s.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
# höchstens 60000 Zeichen je Hinweis; lange Protokolle auf mehrere Hinweise verteilen (je Schritt nur wenige Hinweise erlaubt)
body = '\n'.join(res) if res else '(keine Ergebnisse)'
parts = [body[i:i + 60000] for i in range(0, len(body), 60000)][:8]
for k, p in enumerate(parts): print(f'::notice title={title} {k + 1}/{len(parts)}::{esc(p)}')
if bad or not res:
    print(f'::error title={title}::{esc(chr(10).join(bad[:40]) or "keine Ergebnisse")}'); sys.exit(1)
