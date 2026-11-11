// Gemeinsame Bedienelemente der Plugin-Oberflächen (unterer Bereich): Kippschalter, Drehwahlschalter, Textausgabe.
// Koordinaten in "UI-Einheiten": bei Standardgröße (1200 Pixel breit) ist 1 Einheit = 1 Pixel, Schriftgrößen also in Pixeln.
#pragma once
#ifdef __APPLE__
#include "cairo_cg.hpp"
#else
#include <cairo/cairo.h>
#endif
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <chrono>
#include <sys/stat.h>

namespace tsw {
// Einstellungen der Oberfläche, die für alle tonestack-Plugins eines Benutzers gelten (nicht je Projekt): Wertanzeige am Knopf, Bandliste des Wien EQ.
// Datei: Linux $XDG_CONFIG_HOME/tonestack/ui.conf (sonst ~/.config/…), macOS ~/Library/Application Support/tonestack/ui.conf; Zeilen „name=0|1“.
// Lässt sich die Datei nicht schreiben (abgeschottete Hosts), gilt die Wahl bis zum Ende des Programms. Gelesen wird höchstens einmal je Sekunde,
// damit offene Fenster anderer Plugins der Wahl folgen.
struct Prefs { bool tips = true, bands = true; };
inline std::string prefDir() { const char *h = std::getenv("HOME");
#ifdef __APPLE__
  return h ? std::string(h) + "/Library/Application Support/tonestack" : std::string();
#else
  const char *x = std::getenv("XDG_CONFIG_HOME"); if (x && *x) return std::string(x) + "/tonestack"; return h ? std::string(h) + "/.config/tonestack" : std::string();
#endif
}
inline Prefs &prefs() { static Prefs p; static bool fileOk = true; static std::chrono::steady_clock::time_point last{}; auto now = std::chrono::steady_clock::now();
  if (fileOk && (last == std::chrono::steady_clock::time_point{} || now - last > std::chrono::seconds(1))) { last = now; std::string d = prefDir();
    if (d.empty()) fileOk = false; else if (FILE *f = std::fopen((d + "/ui.conf").c_str(), "r")) { char k[32]; int v; while (std::fscanf(f, " %31[a-z]=%d", k, &v) == 2) { if (std::string(k) == "tips") p.tips = v != 0; else if (std::string(k) == "bands") p.bands = v != 0; } std::fclose(f); } }
  return p; }
inline void prefSave() { std::string d = prefDir(); if (d.empty()) return; { size_t k = d.rfind('/'); if (k != std::string::npos && k > 0) ::mkdir(d.substr(0, k).c_str(), 0755); } ::mkdir(d.c_str(), 0755); std::string tmp = d + "/ui.conf.tmp"; if (FILE *f = std::fopen(tmp.c_str(), "w")) { const Prefs &p = prefs(); std::fprintf(f, "tips=%d\nbands=%d\n", p.tips ? 1 : 0, p.bands ? 1 : 0); std::fclose(f); std::rename(tmp.c_str(), (d + "/ui.conf").c_str()); } }
constexpr double UW = 1200;   // Breite des unteren Bereichs in UI-Einheiten
constexpr double TPI = 3.14159265358979323846;
inline void rgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
inline void font(cairo_t *c, double px, bool bold) { cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL); cairo_set_font_size(c, px); }
// Text an der Grundlinie y; align 0 links, 1 Mitte, 2 rechts
inline double text(cairo_t *c, const char *t, double x, double y, double px, bool bold, int align = 0) {
  font(c, px, bold); cairo_text_extents_t e; cairo_text_extents(c, t, &e);
  double x0 = align == 0 ? x - e.x_bearing : align == 1 ? x - e.width / 2 - e.x_bearing : x - e.width - e.x_bearing;
  cairo_move_to(c, x0, y); cairo_show_text(c, t); return e.x_advance; }
inline void rrect(cairo_t *c, double x, double y, double w, double h, double r) {
  cairo_new_sub_path(c); cairo_arc(c, x + w - r, y + r, r, -TPI / 2, 0); cairo_arc(c, x + w - r, y + h - r, r, 0, TPI / 2);
  cairo_arc(c, x + r, y + h - r, r, TPI / 2, TPI); cairo_arc(c, x + r, y + r, r, TPI, 3 * TPI / 2); cairo_close_path(c); }

// Kippschalter (Hebel aus Chrom auf Sechskantmutter). (cx, cy) = Mitte der Mutter.
// n = 2 (oben / unten) oder 3 (oben / Mitte / unten); idx = Stellung, 0 = oben. labels[i] steht rechts neben Stellung i.
// Beschriftung: Titel darüber, Zusatz darunter; Gesamthöhe etwa cy − 62 … cy + 60, Breite etwa cx − 30 … cx + 28 + Beschriftung.
inline void toggle(cairo_t *c, double cx, double cy, int n, int idx, const char *title, const char *const *labels, const char *sub, bool dim = false) {
  const int ink = dim ? 0x6a6a68 : 0xdcdee2;
  if (title) { rgb(c, ink); text(c, title, cx, cy - 48, 14, true, 1); }
  // Platte
  { cairo_pattern_t *g = cairo_pattern_create_linear(0, cy - 36, 0, cy + 36); cairo_pattern_add_color_stop_rgb(g, 0, 0.17, 0.18, 0.2); cairo_pattern_add_color_stop_rgb(g, 1, 0.08, 0.085, 0.095);
    rrect(c, cx - 19, cy - 36, 38, 72, 6); cairo_set_source(c, g); cairo_fill_preserve(c); cairo_pattern_destroy(g); rgb(c, 0x3e4148); cairo_set_line_width(c, 1.2); cairo_stroke(c); }
  // Stellungsmarken auf der Platte
  for (int i = 0; i < n; i++) { double y = n == 2 ? (i == 0 ? cy - 27 : cy + 27) : cy - 27 + 27 * i; rgb(c, i == idx ? 0xffd98a : 0x50535a); cairo_new_path(c); cairo_arc(c, cx + 13, y, 2, 0, 2 * TPI); cairo_fill(c); }
  // Mutter (Sechskant) und Buchse
  { cairo_new_path(c); for (int k = 0; k < 6; k++) { double a = k * TPI / 3 + TPI / 6; double x = cx + 13 * std::cos(a), y = cy + 13 * std::sin(a); if (k == 0) cairo_move_to(c, x, y); else cairo_line_to(c, x, y); } cairo_close_path(c);
    cairo_pattern_t *g = cairo_pattern_create_linear(cx - 13, cy - 13, cx + 13, cy + 13); cairo_pattern_add_color_stop_rgb(g, 0, 0.9, 0.9, 0.91); cairo_pattern_add_color_stop_rgb(g, 0.5, 0.62, 0.63, 0.65); cairo_pattern_add_color_stop_rgb(g, 1, 0.38, 0.39, 0.41);
    cairo_set_source(c, g); cairo_fill_preserve(c); cairo_pattern_destroy(g); rgb(c, 0x2b2c30); cairo_set_line_width(c, 1); cairo_stroke(c);
    rgb(c, 0x1b1c1f); cairo_new_path(c); cairo_arc(c, cx, cy, 7.5, 0, 2 * TPI); cairo_fill(c); }
  // Hebel: nach oben, nach unten oder (Mitte) auf den Betrachter zu
  double dir = n == 2 ? (idx == 0 ? -1 : 1) : (idx == 0 ? -1 : idx == 2 ? 1 : 0);
  if (dir == 0) { cairo_pattern_t *g = cairo_pattern_create_radial(cx - 3, cy - 3, 1, cx, cy, 10); cairo_pattern_add_color_stop_rgb(g, 0, 1, 1, 1); cairo_pattern_add_color_stop_rgb(g, 0.6, 0.75, 0.76, 0.78); cairo_pattern_add_color_stop_rgb(g, 1, 0.42, 0.43, 0.45);
    cairo_new_path(c); cairo_arc(c, cx, cy, 9, 0, 2 * TPI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g); }
  else { double tip = cy + dir * 31;
    rgb(c, 0x000000, 0.35); cairo_new_path(c); cairo_move_to(c, cx - 2, cy); cairo_line_to(c, cx + 6, tip + 3); cairo_line_to(c, cx + 12, tip + 3); cairo_line_to(c, cx + 5, cy); cairo_close_path(c); cairo_fill(c);   // Schatten
    cairo_pattern_t *g = cairo_pattern_create_linear(cx - 6, 0, cx + 6, 0); cairo_pattern_add_color_stop_rgb(g, 0, 0.5, 0.51, 0.53); cairo_pattern_add_color_stop_rgb(g, 0.45, 0.97, 0.97, 0.98); cairo_pattern_add_color_stop_rgb(g, 1, 0.45, 0.46, 0.48);
    cairo_new_path(c); cairo_move_to(c, cx - 3.5, cy); cairo_line_to(c, cx - 5.5, tip); cairo_line_to(c, cx + 5.5, tip); cairo_line_to(c, cx + 3.5, cy); cairo_close_path(c); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g);
    cairo_pattern_t *b = cairo_pattern_create_radial(cx - 2, tip - 2 * dir - 1, 1, cx, tip, 7.5); cairo_pattern_add_color_stop_rgb(b, 0, 1, 1, 1); cairo_pattern_add_color_stop_rgb(b, 0.6, 0.78, 0.79, 0.81); cairo_pattern_add_color_stop_rgb(b, 1, 0.45, 0.46, 0.48);
    cairo_new_path(c); cairo_arc(c, cx, tip, 7, 0, 2 * TPI); cairo_set_source(c, b); cairo_fill(c); cairo_pattern_destroy(b); }
  // Stellungsbeschriftung rechts
  for (int i = 0; i < n; i++) { double y = n == 2 ? (i == 0 ? cy - 22 : cy + 32) : cy - 22 + 27 * i; bool act = i == idx;
    rgb(c, dim ? 0x5a5a58 : act ? 0xffd98a : 0x8d919a); text(c, labels[i], cx + 24, y, 12.5, act); }
  if (sub) { rgb(c, dim ? 0x5a5a58 : 0x9a9ea6); text(c, sub, cx, cy + 56, 12, false, 1); }
}
// Trefferfläche eines Kippschalters (mit Beschriftung rechts); liefert −1 außerhalb, sonst 0 = obere Hälfte, 1 = untere Hälfte
inline int toggleHit(double x, double y, double cx, double cy, double labelW) {
  if (x < cx - 32 || x > cx + 26 + labelW || y < cy - 62 || y > cy + 60) return -1; return y < cy ? 0 : 1; }

// Drehwahlschalter mit n Stellungen (Zeiger auf Stellung idx), Beschriftungen im Bogen darüber
inline void rotarySel(cairo_t *c, double cx, double cy, int n, int idx, const char *title, const char *const *labels, const char *sub, bool dim = false) {
  if (title) { rgb(c, dim ? 0x6a6a68 : 0xdcdee2); text(c, title, cx, cy - 48, 14, true, 1); }
  const double span = n >= 5 ? 160 : 120; auto angOf = [&](int i) { return (-span / 2 + span * i / (n - 1)) * TPI / 180; };   // ab 5 Stellungen weiter gespreizt
  for (int i = 0; i < n; i++) { double a = angOf(i); bool act = i == idx; rgb(c, dim ? 0x5a5a58 : act ? 0xffd98a : 0x8d919a); text(c, labels[i], cx + 34 * std::sin(a), cy - 34 * std::cos(a) + 4, 12.5, act, 1);
    rgb(c, act && !dim ? 0xffd98a : 0x50535a); cairo_new_path(c); cairo_arc(c, cx + 23 * std::sin(a), cy - 23 * std::cos(a), 1.8, 0, 2 * TPI); cairo_fill(c); }
  cairo_pattern_t *g = cairo_pattern_create_radial(cx - 5, cy - 6, 2, cx, cy, 18); cairo_pattern_add_color_stop_rgb(g, 0, 0.32, 0.31, 0.3); cairo_pattern_add_color_stop_rgb(g, 1, 0.05, 0.05, 0.055);
  cairo_new_path(c); cairo_arc(c, cx, cy, 17, 0, 2 * TPI); cairo_set_source(c, g); cairo_fill_preserve(c); cairo_pattern_destroy(g); rgb(c, 0x45474c); cairo_set_line_width(c, 1.2); cairo_stroke(c);
  double a = angOf(std::max(0, std::min(n - 1, idx))); cairo_save(c); cairo_translate(c, cx, cy); cairo_rotate(c, a); rgb(c, dim ? 0x8a8a88 : 0xf2f2ee); cairo_set_line_width(c, 3.5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -4); cairo_line_to(c, 0, -14); cairo_stroke(c); cairo_restore(c);
  if (sub) { rgb(c, dim ? 0x5a5a58 : 0x9a9ea6); text(c, sub, cx, cy + 56, 12, false, 1); }
}
inline bool rotaryHit(double x, double y, double cx, double cy) { return x >= cx - 52 && x <= cx + 52 && y >= cy - 62 && y <= cy + 60; }

// Kleiner Drehregler (Instrument, Ausgang) mit Name und Wert rechts
inline void smallKnob(cairo_t *c, double cx, double cy, double u01, const char *name, const char *val, bool hot, bool dim) {
  rgb(c, 0x0c0c0c); cairo_new_path(c); cairo_arc(c, cx, cy, 24, 0, 2 * TPI); cairo_fill(c); rgb(c, 0x45474c); cairo_set_line_width(c, 1.5); cairo_arc(c, cx, cy, 24, 0, 2 * TPI); cairo_stroke(c);
  cairo_save(c); cairo_translate(c, cx, cy); cairo_rotate(c, (-150 + 300 * u01) * TPI / 180); rgb(c, 0xf2f2ee); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -6); cairo_line_to(c, 0, -21); cairo_stroke(c); cairo_restore(c);
  if (hot) { rgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, cx, cy, 30, 0, 2 * TPI); cairo_stroke(c); }
  rgb(c, dim ? 0x6a6a68 : 0xdcdee2); text(c, name, cx + 34, cy - 3, 14, true); rgb(c, dim ? 0x6a6a68 : 0xcfd1d6); text(c, val, cx + 34, cy + 16, 13, false);
}

// kleiner Drehknopf mit Beschriftung darunter (schmal, für drei Knöpfe nebeneinander); (cx, cy) = Knopfmitte
inline void smallKnobV(cairo_t *c, double cx, double cy, double u01, const char *name, const char *val, bool hot, bool dim) {
  rgb(c, 0x0c0c0c); cairo_new_path(c); cairo_arc(c, cx, cy, 22, 0, 2 * TPI); cairo_fill(c); rgb(c, 0x45474c); cairo_set_line_width(c, 1.5); cairo_arc(c, cx, cy, 22, 0, 2 * TPI); cairo_stroke(c);
  cairo_save(c); cairo_translate(c, cx, cy); cairo_rotate(c, (-150 + 300 * u01) * TPI / 180); rgb(c, 0xf2f2ee); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -6); cairo_line_to(c, 0, -19); cairo_stroke(c); cairo_restore(c);
  if (hot) { rgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, cx, cy, 28, 0, 2 * TPI); cairo_stroke(c); }
  rgb(c, dim ? 0x6a6a68 : 0xdcdee2); text(c, name, cx, cy + 42, 13, true, 1); rgb(c, dim ? 0x6a6a68 : 0xcfd1d6); text(c, val, cx, cy + 59, 12, false, 1);
}
// Schalter in der Kopfzeile des Modellstreifens: „NAME ON|OFF“ ab (x, y), anklickbar über die Breite w
inline void prefDraw(cairo_t *c, const char *name, bool on, double x, double y) { rgb(c, 0x6c7079); double w = text(c, name, x, y, 12, false); rgb(c, on ? 0xffd98a : 0x8d919a); text(c, on ? "ON" : "OFF", x + w + 7, y, 12, true); }
inline bool prefHit(double lx, double ly, double x, double y, double w) { return lx >= x - 6 && lx <= x + w + 6 && ly >= y - 15 && ly <= y + 7; }
// Werteanzeige (Sprechblase) in Bildschirm-Pixeln: Anker (px, py) = Mitte oberhalb des Bedienelements; u = Pixel je UI-Einheit
inline void tip(cairo_t *c, const char *t, double px, double py, double u, double winW) {
  if (!prefs().tips) return;   // „VALUE POPUP OFF“
  cairo_save(c); cairo_identity_matrix(c); cairo_translate(c, 0, 0); font(c, 14 * u, true); cairo_text_extents_t e; cairo_text_extents(c, t, &e);
  double bw = e.width + 18 * u, bh = 26 * u, bx = std::min(std::max(px - bw / 2, 4.0), winW - bw - 4), by = std::max(2.0, py - bh - 4 * u);
  rgb(c, 0xf4efe3); rrect(c, bx, by, bw, bh, 4 * u); cairo_fill(c); rgb(c, 0x111111); cairo_move_to(c, bx + 9 * u - e.x_bearing, by + bh / 2 + e.height / 2); cairo_show_text(c, t); cairo_restore(c);
}
// Frequenzgang-Diagramm in UI-Einheiten: Kurven in dB an den Stützstellen fAt(0 … nf−1), Legende rechts oben; marks: tiefes E und H des Basses
inline void plot(cairo_t *c, const std::vector<double> &curve, const std::vector<double> &refCurve, int NF, double (*freqAt)(int), double px0, double px1, double py0, double py1, const char *REFLABEL, bool marks) {
  double lo = -25, hi = 15; if (!curve.empty()) { double mn = 1e9, mx = -1e9; for (double x : curve) { mn = std::min(mn, x); mx = std::max(mx, x); } for (double x : refCurve) { mn = std::min(mn, x); mx = std::max(mx, x); }
    lo = std::min(lo, std::floor((mn - 3) / 5) * 5); hi = std::max(hi, std::ceil((mx + 3) / 5) * 5); if (hi - lo < 40) { double add = 40 - (hi - lo); lo -= std::floor(add / 10) * 5; hi = lo + 40; } }
  double step = (hi - lo) > 60 ? 10 : 5; auto X = [&](double f) { return px0 + (px1 - px0) * std::log(f / 20) / std::log(1000.0); }; auto Y = [&](double d) { return py1 - (py1 - py0) * (d - lo) / (hi - lo); };
  cairo_set_line_width(c, 1);
  // Teilstriche bei 1…9 · 10ⁿ
  for (double f = 10; f < 20000; f *= 10) for (int m = 1; m < 10; m++) { double ff = f * m; if (ff < 20) continue; if (ff > 20000) break; rgb(c, 0x23262b); cairo_move_to(c, X(ff), py0); cairo_line_to(c, X(ff), py1); cairo_stroke(c); }
  for (double d = lo; d <= hi + 1e-9; d += step) { rgb(c, d == 0 ? 0x5a5e66 : 0x2a2d33); cairo_move_to(c, px0, Y(d)); cairo_line_to(c, px1, Y(d)); cairo_stroke(c); rgb(c, 0x9a9ea6); char t[16]; std::snprintf(t, 16, d == 0 ? "0" : "%+.0f", d); text(c, t, px0 - 8, Y(d) + 4, 12, false, 2); }
  const double fl[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000}; const char *fn[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
  for (int i = 0; i < 10; i++) { rgb(c, 0x3a3e46); cairo_move_to(c, X(fl[i]), py0); cairo_line_to(c, X(fl[i]), py1); cairo_stroke(c); rgb(c, 0x9a9ea6); text(c, fn[i], X(fl[i]), py1 + 17, 12, false, 1); }
  if (marks) { const double bf[2] = {41.2, 30.9}; const char *bn[2] = {"E", "B"}; for (int i = 0; i < 2; i++) { rgb(c, 0x8d919a, 0.8); const double dash[] = {2, 4}; cairo_set_dash(c, dash, 2, 0); cairo_move_to(c, X(bf[i]), py0); cairo_line_to(c, X(bf[i]), py1); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); text(c, bn[i], X(bf[i]) + 4, py0 + 14, 12, true); } }
  rgb(c, 0x9a9ea6); text(c, "Hz", (px0 + px1) / 2, py1 + 34, 12, false, 1); cairo_save(c); cairo_translate(c, 18, (py0 + py1) / 2); cairo_rotate(c, -TPI / 2); text(c, "dB", 0, 4, 12, false, 1); cairo_restore(c);
  if (!refCurve.empty()) { rgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double dash[] = {8, 6}; cairo_set_dash(c, dash, 2, 0); cairo_new_path(c);
    for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(refCurve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); }
  if (!curve.empty()) { rgb(c, 0x4da3ff); cairo_set_line_width(c, 3); cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND); cairo_new_path(c);
    for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(curve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); }
  // Legende rechts oben
  { const char *l1 = "current setting", *l2 = REFLABEL; font(c, 12, false); cairo_text_extents_t e2, e1; cairo_text_extents(c, l2, &e2); cairo_text_extents(c, l1, &e1);
    double x2 = px1 - e2.x_advance, x1 = x2 - 44 - e1.x_advance - 40, y = py0 - 14;
    rgb(c, 0x4da3ff); cairo_set_line_width(c, 3); cairo_move_to(c, x1, y - 4); cairo_line_to(c, x1 + 30, y - 4); cairo_stroke(c); rgb(c, 0xc8ccd4); text(c, l1, x1 + 38, y, 12, false);
    rgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double d2[] = {8, 6}; cairo_set_dash(c, d2, 2, 0); cairo_move_to(c, x2 - 38, y - 4); cairo_line_to(c, x2 - 8, y - 4); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); rgb(c, 0xc8ccd4); text(c, l2, x2, y, 12, false); }
}
} // namespace tsw
