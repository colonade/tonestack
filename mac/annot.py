# Letzte Zeilen eines Protokolls als GitHub-Fehlerhinweis ausgeben (Hinweise sind über die API lesbar, auch ohne Protokoll-Download).
#   python3 mac/annot.py <Titel> <Datei> [Zeilen] [Muster]
import sys, re
title, path = sys.argv[1], sys.argv[2]; n = int(sys.argv[3]) if len(sys.argv) > 3 else 40
lines = open(path, errors='replace').read().splitlines()
if len(sys.argv) > 4: lines = [l for l in lines if re.search(sys.argv[4], l)] or lines
txt = '\n'.join(lines[-n:]).replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
print(f'::error title={title}::{txt}')
