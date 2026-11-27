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
// rnd: auf dec Nachkommastellen runden, −0 wird 0 (ab 2026-11-27). Für Host-Texte, die aus dem Wert weitere Angaben ableiten (dBu zu mV, Nachhall bei
// 125 Hz zu dem bei 500 Hz …): diese aus dem gezeigten, gerundeten Wert rechnen – dann ergibt der zurückgelesene Text wieder denselben Text (clap-validator
// param-conversions), und „-0.0“ und „+0.0“ fallen zusammen
inline double rnd(double v, int dec) { double m = std::pow(10.0, dec); double r = std::round(v * m) / m; return r == 0 ? 0.0 : r; }
// Kehrwert einer steigenden Kennlinie f auf [a, b] durch Halbieren (Texteingabe in der Einheit der Anzeige, z. B. dB oder Hz statt Knopfstellung)
template <class F> inline double invert(F f, double y, double a, double b) { if (y <= f(a)) return a; if (y >= f(b)) return b;
  for (int i = 0; i < 80; i++) { double m = 0.5 * (a + b); if (f(m) < y) a = m; else b = m; } return 0.5 * (a + b); }
// Zahl hinter einem Wort am Anfang lesen („Q 0.70“): führende Buchstaben und Leerzeichen überspringen
inline double parseAfter(const char *s, const char *word) { if (!s) return std::nan(""); while (*s == ' ') s++; size_t n = std::strlen(word); if (std::strncmp(s, word, n) == 0) s += n; return parse(s); }
// Frequenz in Hz aus „350 Hz“, „1.20 kHz“ oder „1,2k“
inline double parseHz(const char *s) { double v = parse(s); if (!std::isfinite(v) || !s) return v; const char *k = s; while (*k && !(*k == 'k' || *k == 'K')) k++; return *k ? v * 1000 : v; }
}
