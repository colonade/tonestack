// Zahlen in Zustand und Host-Texten unabhängig vom Gebietsschema (ab Laney 3.52 und Geschwistern, 2026-11-21).
// printf/atof richten sich nach LC_NUMERIC des Prozesses. Stellt ein anderes Modul im Host ein deutsches Gebietsschema ein (unter Windows teilen sich
// alle Module, die msvcrt.dll benutzen, ein Gebietsschema), würde „0,73“ gespeichert und von atof als 0 gelesen – Einstellungen gingen verloren.
//   fixDot: Dezimalzeichen des Gebietsschemas im gespeicherten Zustand durch „.“ ersetzen (der Zustand enthält nur „Nummer=Zahl“-Zeilen)
//   parse:  Zahl am Anfang eines Textes lesen; „.“ und „,“ gelten beide als Dezimalzeichen (Zustand aller Fassungen, Eingaben im Host)
#pragma once
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <cmath>
namespace tsnum {
inline char localePoint() { const std::lconv *l = std::localeconv(); return l && l->decimal_point && l->decimal_point[0] && !l->decimal_point[1] ? l->decimal_point[0] : '.'; }
inline void fixDot(char *s, int n) { char p = localePoint(); if (p == '.') return; for (int i = 0; i < n && s[i]; i++) if (s[i] == p) s[i] = '.'; }
inline double parse(const char *s) {   // ohne Ziffer: NaN (Aufrufer verwerfen nicht endliche Werte)
  if (!s) return std::nan(""); while (*s == ' ' || *s == '\t') s++;
  char t[64]; int k = 0; bool digit = false; char p = localePoint();
  for (; *s && k < 63; s++) { char c = *s;
    if (c >= '0' && c <= '9') { t[k++] = c; digit = true; }
    else if (c == '+' || c == '-' || c == 'e' || c == 'E') t[k++] = c;
    else if (c == '.' || c == ',') t[k++] = p;
    else break; }
  t[k] = 0; return digit ? std::strtod(t, nullptr) : std::nan(""); }
}
