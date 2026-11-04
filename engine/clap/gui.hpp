// Oberfläche des Laney-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm. Foto der Frontplatte, Vektor-Aufdruck, drehbare Zeiger.
#pragma once
#ifndef TS_VERSION
#define TS_VERSION "?"
#endif
#ifdef __APPLE__
#include "cairo_cg.hpp"   // Cairo-Teilmenge auf CoreGraphics
#include "macview.h"
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#endif
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include "geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "panel_jpg.h"
#include "widgets.hpp"

namespace aorgui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_INP, P_CH, P_V1, P_V2, P_PL, P_T, P_M, P_B, P_MV, P_MID, P_TOP, P_BASS, P_UMBAU, P_PA, P_TAPER, P_OUT, P_NL, P_VIN, P_OS, P_PRES, P_TUBE1, P_TUBE2, P_TUBE3, P_SPK, P_SOFT, P_PAD, P_MIS, P_ECO, P_REV, P_TANK, P_COUNT };
static const char *TUBE_SHORT[5] = {"12AX7", "12AT7", "12AU7", "5751", "12AY7"};
static const int KPARAM[9] = {P_V1, P_V2, P_PL, P_MV, P_T, P_M, P_B, P_PRES, P_REV};   // Knopf → Parameter (REVERB ab 3.35)
static const char *KNAME[9] = {"PREAMP 1 VOLUME","PREAMP 2 VOLUME","PREAMP 1 LEVEL","MASTER VOLUME","TREBLE","MIDDLE","BASS","PRESENCE","REVERB"};
// Unterer Bereich in UI-Einheiten (tsw::UW = 1200 breit, bei Standardgröße 1 Einheit = 1 Pixel), unabhängig von der Fotoauflösung
constexpr double STRIPU = 150;  // Modellstreifen: Kippschalter, Überabtastung, Röhrenaussteuerung, Instrument/Ausgang
constexpr double TUBEU = 46;    // Zeile Röhrenbestückung
constexpr double PLOTU = 300;   // Frequenzgang-Diagramm
static const double TOTALH = FH + (STRIPU + TUBEU + PLOTU) * FW / tsw::UW;   // Gesamthöhe in Foto-Einheiten
constexpr double SW_Y = 88;     // Mittelpunkt der Schalter und kleinen Regler
static const double SW_X[6] = {56, 168, 280, 392, 726, 838};   // BASS MOD, FX LOOP, DISTORTION, SPEAKER, SOFT, LATE
static const int SW_P[6] = {P_UMBAU, P_PA, P_NL, P_SPK, P_SOFT, P_PAD};
constexpr double OS_X = 506, TAP_X = 614;   // Überabtastung (Drehwahlschalter), Kennlinie der log-Potis Middle/Presence (Kippschalter, 3 Stellungen; 3.27–3.33 entfallen, ab 3.34 wieder da)
// Stellung des Überabtastungs-Wahlschalters: 1x, eco (2x mit Endstufe in Grundrate), 2x, 4x (Parameter Oversampling und Eco; 8x ab 3.28 entfernt)
inline int osIdx(double os, double eco) { int o = std::max(0, std::min(2, (int)std::lround(os))); return o == 0 ? 0 : o == 1 ? (eco > 0.5 ? 1 : 2) : o + 1; }
static const double KN_X[3] = {935, 1035, 1135};    // INPUT, OUTPUT, MISMATCH (Streuung der Endröhren), Beschriftung unter dem Knopf
constexpr double KN_Y = 72;                         // Mittelpunkt der kleinen Regler
static const char *REFLABEL = "original parts, stock tubes, all controls 5, boosts off";
constexpr int NF = 120;        // Stützstellen des Diagramms (20 Hz … 20 kHz, logarithmisch)
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;   // win: NSView; onTick: Anzeigen auffrischen (Plugin)
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;   // gesture: Ziehen beginnt/endet (Parameter-ID)
  double meter[5] = {0,0,0,0,0};
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;   // Kurven in dB (bezogen auf 1 kHz): aktuell, Original/alle 5
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }   // Frontplattenbild gehört der Oberfläche (sonst bei jedem Öffnen/Schließen 1,5 MB verloren)
  // snprintf-Anhängen mit Begrenzung (n bleibt innerhalb des Puffers, auch wenn ein Wert unerwartet lang wird)
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  // Fensterpixel → UI-Einheiten des unteren Bereichs
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(PANEL_JPG, (int)PANEL_JPG_LEN, &iw, &ih, &nch, 3);
      img = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iw, ih); unsigned char *dst = cairo_image_surface_get_data(img); int stride = cairo_image_surface_get_stride(img);
      for (int y = 0; y < ih; y++) for (int x = 0; x < iw; x++) { unsigned char *s = px + (y * iw + x) * 3; unsigned char *t = dst + y * stride + x * 4; t[0] = s[2]; t[1] = s[1]; t[2] = s[0]; t[3] = 255; }
      cairo_surface_mark_dirty(img); stbi_image_free(px); }
  }
#ifdef __APPLE__
  bool attach(void *parentView, int w, int h) { W = w; H = h; loadImg(); win = tsmac::create(parentView, w, h, this); return win != nullptr; }
  void show() { tsmac::show(win); }
  void hide() { tsmac::hide(win); }
  void resize(int w, int h) { W = w; H = h; tsmac::setSize(win, w, h); }
  void destroy() { if (win) { tsmac::destroy(win); win = nullptr; } if (base) { cairo_surface_destroy(base); base = nullptr; } }
  void draw() { tsmac::redraw(win); }
  void drawCG(void *cg, double, double) override { cairo_t *c = cairo_cg_create((CGContextRef)cg); drawTo(c); cairo_destroy(c); }
  bool mouse(int kind, double x, double y, int button, double t) override { return onMouse(kind, x, y, button, t); }
  bool tick() override { if (onTick) onTick(); std::string sig = signature(); if (sig != lastSig) { lastSig = sig; return true; } return false; }
#else
  bool init(Display *d, Window par, int w, int h) {
    dpy = d; parent = par; W = w; H = h;
    int scr = DefaultScreen(dpy); XSetWindowAttributes a{}; a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask | StructureNotifyMask; a.background_pixmap = None;
    win = XCreateWindow(dpy, par ? par : RootWindow(dpy, scr), 0, 0, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask | CWBackPixmap, &a);
    xs = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, scr), w, h);
    loadImg();
    return true;
  }
  void show() { if (win && xs) { XMapWindow(dpy, win); XFlush(dpy); } }
  void hide() { if (win) { XUnmapWindow(dpy, win); XFlush(dpy); } }
  void resize(int w, int h) { W = w; H = h; if (win && xs) { XResizeWindow(dpy, win, w, h); cairo_xlib_surface_set_size(xs, w, h); } }
  void destroy() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } if (base) { cairo_surface_destroy(base); base = nullptr; } if (win) { XDestroyWindow(dpy, win); win = 0; } }
#endif
  // ---- Hilfen ----
  static void setrgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
  static double ang(double v) { return (-150 + 30 * v) * PI / 180; }
  void font(cairo_t *c, double px, bool bold = true) { cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL); cairo_set_font_size(c, px); }
  // Text in ein Feld der Breite w einpassen (wie textLength im Web)
  void fitText(cairo_t *c, const char *t, double x, double y, double w, double h) {
    font(c, h / 0.72); cairo_text_extents_t e; cairo_text_extents(c, t, &e); if (e.width <= 0) return;
    cairo_new_path(c); cairo_save(c); cairo_translate(c, x, y + h); cairo_scale(c, w / e.width, 1); cairo_move_to(c, -e.x_bearing, 0); cairo_show_text(c, t); cairo_restore(c);
  }
  void textC(cairo_t *c, const char *t, double x, double y, double px, bool bold = true) { font(c, px, bold); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, x - e.width / 2 - e.x_bearing, y + e.height / 2); cairo_show_text(c, t); }
  // ---- Zeichnen ----
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  // Foto mit Skalenaufdruck (Foto-Einheiten)
  void drawBase(cairo_t *c) {
    cairo_set_source_surface(c, img, 0, 0); cairo_rectangle(c, 0, 0, FW, FH); cairo_fill(c);
    setrgb(c, 0xd3d2cc); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);   // Aufdruck: Skalen
    for (int i = 0; i < 9; i++) { const KnobGeo &g = KG[i]; double rm = (SEG0 + SEG1) / 2 * g.R, sw = (SEG1 - SEG0) * g.R, capA = (sw / 2) / rm; cairo_set_line_width(c, sw);
      for (int q = 0; q < 5; q++) { double cen = (-120 + 60 * q) * PI / 180; cairo_new_sub_path(c); cairo_arc(c, g.rx, g.ry, rm, cen - SEGHA * PI / 180 + capA - PI / 2, cen + SEGHA * PI / 180 - capA - PI / 2); cairo_stroke(c); }
      double rd = (DIG0 + DIG1) / 2 * g.R, dh = (DIG1 - DIG0) * g.R; const char *digs[6] = {"0","2","4","6","8","10"};
      for (int q = 0; q < 6; q++) { double a = ang(q * 2); cairo_save(c); cairo_translate(c, g.rx + rd * std::sin(a), g.ry - rd * std::cos(a)); cairo_rotate(c, a); font(c, dh / 0.72, true);
        cairo_text_extents_t e; cairo_text_extents(c, digs[q], &e); cairo_move_to(c, -e.width / 2 - e.x_bearing, dh / 2); cairo_show_text(c, digs[q]); cairo_restore(c); } }
  }
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    bool ch1 = v[P_CH] < 0.5, low = v[P_INP] > 0.5, pa = v[P_PA] > 0.5;
    setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef __APPLE__
    // Foto und Skalenaufdruck ändern sich nur mit der Fenstergröße: einmal je Größe vorgezeichnet (ab 3.31; vorher je Bild neu skaliert,
    // die Oberfläche kostete bei offenen Fenster etwa einen halben Kern, der in der CPU-Anzeige nicht auftaucht)
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); drawBase(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
    cairo_save(c); cairo_scale(c, s, s);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); drawBase(b); });   // ab 3.33 wie unter Linux vorgezeichnet (Gerätepixel)
    cairo_save(c); cairo_scale(c, s, s);
#endif
    // Zugschalter-Hintergründe und Beschriftungen
    for (auto &l : LG) { int pp = -1; if (!std::strcmp(l.t, "PULL ON")) pp = P_CH; else if (!std::strcmp(l.t, "PULL BOOST")) pp = !std::strcmp(l.k, "t") ? P_TOP : !std::strcmp(l.k, "m") ? P_MID : P_BASS;
      bool on = pp >= 0 && (pp == P_CH ? ch1 : v[pp] > 0.5);
      if (on) { setrgb(c, 0x8c1a10); cairo_rectangle(c, l.x - 8, l.y - 7, l.w + 16, l.h + 14); cairo_fill(c); setrgb(c, 0xffb3a8); cairo_set_line_width(c, 1.5); cairo_rectangle(c, l.x - 8, l.y - 7, l.w + 16, l.h + 14); cairo_stroke(c); }
      bool dim = (!std::strcmp(l.k, "v1") && low && pp < 0) || (!std::strcmp(l.k, "pl") && !ch1) || ((!std::strcmp(l.k, "pres") || !std::strcmp(l.k, "rev")) && !pa);
      setrgb(c, dim ? 0x6a6a68 : 0xd3d2cc); fitText(c, l.t, l.x, l.y, l.w, l.h); }
    setrgb(c, 0xd3d2cc); cairo_rectangle(c, PULLLINE[0], PULLLINE[2] - PULLLINE[3] / 2, PULLLINE[1] - PULLLINE[0], PULLLINE[3]); cairo_fill(c);
    // Kontrollleuchte und Power
    if (ch1) { cairo_pattern_t *g = cairo_pattern_create_radial(LED[0], LED[1], 0, LED[0], LED[1], LED[2] * 3.2); cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.95, 0.9, 1); cairo_pattern_add_color_stop_rgba(g, 0.25, 1, 0.42, 0.29, 1); cairo_pattern_add_color_stop_rgba(g, 0.5, 0.95, 0.12, 0.05, 0.9); cairo_pattern_add_color_stop_rgba(g, 1, 0.75, 0.07, 0.02, 0);
      cairo_set_source(c, g); cairo_new_path(c); cairo_arc(c, LED[0], LED[1], LED[2] * 3.2, 0, 2 * PI); cairo_fill(c); cairo_pattern_destroy(g); }
    { cairo_pattern_t *g = cairo_pattern_create_radial(POWER[0] + POWER[2] / 2, POWER[1] + POWER[3] * 0.45, 0, POWER[0] + POWER[2] / 2, POWER[1] + POWER[3] * 0.45, POWER[2] * 0.9);
      cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.54, 0.42, 0.75); cairo_pattern_add_color_stop_rgba(g, 0.55, 1, 0.16, 0.06, 0.45); cairo_pattern_add_color_stop_rgba(g, 1, 0.88, 0.06, 0, 0.1);
      cairo_set_operator(c, CAIRO_OPERATOR_SCREEN); cairo_set_source(c, g); cairo_rectangle(c, POWER[0] - 8, POWER[1] - 8, POWER[2] + 16, POWER[3] + 16); cairo_fill(c); cairo_set_operator(c, CAIRO_OPERATOR_OVER); cairo_pattern_destroy(g); }
    // Buchsen
    const double *J[2] = {JACK_LOW, JACK_HIGH}; for (int i = 0; i < 2; i++) { bool act = (i == 0) == low; if (act) { setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] + 4, 0, 2 * PI); cairo_stroke(c); } }
    // Zeiger
    for (int i = 0; i < 9; i++) { const KnobGeo &g = KG[i]; double val = v[KPARAM[i]]; double r0 = g.kind == 0 ? 3 : g.cap + 3, r1 = g.kind == 0 ? 27 : g.skirt - 2, lw = g.kind == 0 ? 7.5 : 5.5;
      cairo_save(c); cairo_translate(c, g.px, g.py); cairo_rotate(c, ang(val)); setrgb(c, g.kind == 0 ? 0xcfc5ae : 0xf2f2ee); cairo_set_line_width(c, lw); cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT); cairo_move_to(c, 0, -r0); cairo_line_to(c, 0, -r1); cairo_stroke(c); cairo_restore(c);
      bool off = (i == 2 && !ch1) || (i == 0 && low) || ((i == 7 || i == 8) && !pa);
      if (off) { cairo_new_path(c); setrgb(c, 0x000000, 0.52); cairo_arc(c, g.rx, g.ry, g.R * 1.22, 0, 2 * PI); cairo_fill(c); }
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g.rx, g.ry, g.R * 0.8, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);   // Ende Foto-Maßstab
    // ---- unterer Bereich in UI-Einheiten ----
    const double u = uscale(); bool nl = v[P_NL] > 0.5; int osv = osIdx(v[P_OS], v[P_ECO]), tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER])));
    char vt[3][32]; std::snprintf(vt[0], 32, "%.0f mV", v[P_VIN]); std::snprintf(vt[1], 32, "%+.1f dB", v[P_OUT]); std::snprintf(vt[2], 32, "%.1f %%", v[P_MIS]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *oo[2] = {"ON", "OFF"}, *osl[4] = {"1x", "eco", "2x", "4x"};
      const char *tt[6] = {"BASS MOD", "FX LOOP", "DISTORTION", "SPEAKER", "SOFT", "LATE"}, *ts[6] = {"bass capacitors", "+ power amp", "tube model", "15\" CB158, 78 l", "mod: C3 off", "mod: 2.2 M pad"};
      for (int k = 0; k < 6; k++) tsw::toggle(c, SW_X[k], SW_Y, 2, v[SW_P[k]] > 0.5 ? 0 : 1, tt[k], oo, ts[k], k == 5 && low);   // LATE wirkt nur am HIGH-Eingang
      tsw::rotarySel(c, OS_X, SW_Y + 6, 4, osv, "OVERSAMPLING", osl, osv == 1 ? "eco: 2x, amp 1x" : "distortion only", !nl);
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; tsw::toggle(c, TAP_X, SW_Y, 3, tp, "POT TAPER", tl, "mid, pres., reverb"); }
    // kleine Regler: Instrument (log) und Ausgang
    for (int i = 0; i < 3; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : i == 1 ? (v[P_OUT] + 36) / 60 : v[P_MIS] / 10;
      tsw::smallKnobV(c, KN_X[i], KN_Y, uu, i == 0 ? "INPUT" : i == 1 ? "OUTPUT" : "TUBE MISMATCH", vt[i], drag == 9 + i || hover == 9 + i, (i == 0 && !nl) || (i == 2 && (!nl || !pa))); }   // MISMATCH: Endröhren-Streuung, nur Röhrenmodus mit Endstufe
    // Röhrenbestückung: V101, V102, V103 (Taste schaltet den Typ durch), RESET = Bestückung wie im Gerät
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x16181c); cairo_rectangle(c, 0, 0, tsw::UW, TUBEU); cairo_fill(c);
      setrgb(c, 0xdcdee2); tsw::text(c, "TUBES", 16, 28, 14, true);
      for (int i = 0; i < 3; i++) { double bx = 84 + i * 136, by = 7, bw = 128, bh = 32; int tt = std::max(0, std::min(4, (int)std::lround(v[P_TUBE1 + i]))); bool stock = tt == 0;
        setrgb(c, 0x111317); tsw::rrect(c, bx, by, bw, bh, 4); cairo_fill_preserve(c); setrgb(c, stock ? 0x4d4d52 : 0xe8b23c); cairo_set_line_width(c, 1.5); cairo_stroke(c);
        char lbl[48]; std::snprintf(lbl, sizeof lbl, "V10%d  %s%s", i + 1, TUBE_SHORT[tt], ""); setrgb(c, stock ? 0xc8ccd4 : 0xffd98a); tsw::text(c, lbl, bx + bw / 2, by + 21, 14, true, 1); }
      { bool anyMod = v[P_TUBE1] > 0.5 || v[P_TUBE2] > 0.5 || v[P_TUBE3] > 0.5; double bx = 494, by = 7, bw = 128, bh = 32; setrgb(c, 0x111317); tsw::rrect(c, bx, by, bw, bh, 4); cairo_fill_preserve(c); setrgb(c, anyMod ? 0x9a9aa0 : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_stroke(c); setrgb(c, anyMod ? 0xececf0 : 0x6a6a68); tsw::text(c, "RESET TO STOCK", bx + bw / 2, by + 21, 13, true, 1); }
      { bool ph = v[P_TANK] > 0.5; double bx = 630, by = 7, bw = 78, bh = 32; setrgb(c, 0x111317); tsw::rrect(c, bx, by, bw, bh, 4); cairo_fill_preserve(c); setrgb(c, pa ? (ph ? 0xe8b23c : 0x9a9aa0) : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_stroke(c);   // Federtank (ab 3.35): physikalisch / linear
        setrgb(c, pa ? 0x9a9ea6 : 0x5e626a); tsw::text(c, "REVERB TANK", bx + bw / 2, by + 12, 9, false, 1); setrgb(c, pa ? (ph ? 0xffd98a : 0xececf0) : 0x6a6a68); tsw::text(c, ph ? "PHYSICAL" : "LINEAR", bx + bw / 2, by + 26, 11.5, true, 1); }
      // Röhrenaussteuerung: fünf waagerechte Balken (1,0 = Übersteuerungseinsatz), logarithmisch, 2,0 = voll
      { const char *tn[5] = {"V101A","V101B","V102A","V102B","V103B"}; double mx0 = 772, bw = 76, gap = 6, bh = 14, by = 8;
        setrgb(c, nl ? 0xdcdee2 : 0x6a6a68); tsw::text(c, "DRIVE", 716, 28, 13, true);
        for (int i = 0; i < 5; i++) { double bx = mx0 + i * (bw + gap); setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, bx + 0.5, by + 0.5, bw - 1, bh - 1); cairo_stroke(c);
          double mv = meter[i], f = std::min(1.0, std::log10(1 + 9 * std::min(mv, 2.0) / 2)), wf = (bw - 2) * f;
          setrgb(c, mv < 0.7 ? 0x3ec25a : mv < 1.0 ? 0xe8b23c : 0xff3b2a); cairo_rectangle(c, bx + 1, by + 1, wf, bh - 2); cairo_fill(c);
          double wm = (bw - 2) * std::log10(1 + 9 * 0.5); setrgb(c, 0xffffff, 0.45); cairo_rectangle(c, bx + 1 + wm - 0.5, by, 1, bh); cairo_fill(c);   // Marke 1,0
          setrgb(c, 0x9a9ea6); tsw::text(c, tn[i], bx + bw / 2, by + bh + 15, 11, false, 1); } }
      cairo_restore(c); }
    // Frequenzgang (Kleinsignal, bezogen auf 1 kHz): Kurven bei Bedarf neu rechnen (höchstens alle 100 ms)
    { char kb[256]; int kn = 0; const int ks[] = {P_INP, P_CH, P_V1, P_V2, P_PL, P_T, P_M, P_B, P_MV, P_MID, P_TOP, P_BASS, P_UMBAU, P_PA, P_TAPER, P_PRES, P_TUBE1, P_TUBE2, P_TUBE3, P_SPK, P_SOFT, P_PAD};
      for (int id : ks) sapp(kb, sizeof kb, kn, "%.4g|", v[id]); std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }   // noch nicht nachgerechnet: signature() sorgt für ein weiteres Neuzeichnen
    { cairo_save(c); cairo_translate(c, 0, STRIPU + TUBEU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, v[P_SPK] > 0.5 ? "FREQUENCY RESPONSE  (small signal, re 1 kHz, with speaker)" : "FREQUENCY RESPONSE  (small signal, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, true); cairo_restore(c); }
    cairo_restore(c);   // Ende unterer Bereich
    // Wertanzeige in Bildschirmpixeln
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[96]; double ax, ay;
      if (i < 9) { const KnobGeo &g = KG[i]; ax = g.rx * s; ay = (g.ry - g.R * 1.12) * s; if ((i == 7 || i == 8) && !pa) std::snprintf(buf, 96, "%s %.1f (needs FX loop + power amp)", i == 7 ? "PRESENCE" : "REVERB", v[KPARAM[i]]); else std::snprintf(buf, 96, "%s %.1f", KNAME[i], v[KPARAM[i]]); }
      else { ax = KN_X[i - 9] * u; ay = FH * s + (KN_Y - 30) * u; if (i == 11) std::snprintf(buf, 96, "POWER TUBE MISMATCH %s%s", vt[2], nl && pa ? "" : " (needs tube distortion + power amp)"); else std::snprintf(buf, 96, "%s", vt[i - 9]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < 9; i++) { double dx = x - KG[i].rx, dy = y - KG[i].ry; if (dx*dx + dy*dy <= KG[i].R * KG[i].R * 1.1) return i; }
    double lx, ly; toLower(px, py, lx, ly); /* Fensterpixel, nicht Foto-Einheiten */ for (int i = 0; i < 3; i++) { double dx = lx - KN_X[i], dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 9 + i; } return -1; }
  int hitPull(double x, double y) const { double s = scale(); x /= s; y /= s; for (auto &l : LG) { int pp = -1; if (!std::strcmp(l.t, "PULL ON")) pp = P_CH; else if (!std::strcmp(l.t, "PULL BOOST")) pp = !std::strcmp(l.k, "t") ? P_TOP : !std::strcmp(l.k, "m") ? P_MID : P_BASS;
      if (pp >= 0 && x >= l.x - 8 && x <= l.x + l.w + 8 && y >= l.y - 10 && y <= l.y + l.h + 10) return pp; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; const double *J[2] = {JACK_HIGH, JACK_LOW}; for (int i = 0; i < 2; i++) { double dx = x - J[i][0], dy = y - J[i][1]; if (dx*dx + dy*dy <= (J[i][2] + 14) * (J[i][2] + 14)) return i; } return -1; }
  int hitTube(double x, double y) const { double lx, ly; toLower(x, y, lx, ly); ly -= STRIPU; if (ly < 5 || ly > 41) return -1; for (int i = 0; i < 3; i++) if (lx >= 84 + i * 136 && lx <= 212 + i * 136) return i; if (lx >= 494 && lx <= 622) return 3; if (lx >= 630 && lx <= 708) return 4; return -1; }
  // Schalter im Modellstreifen: liefert Parameter und half (0 obere, 1 untere Hälfte)
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly);
    for (int k = 0; k < 6; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 30); if (half >= 0) return SW_P[k]; }
    if (tsw::rotaryHit(lx, ly, OS_X, SW_Y + 6)) { double dx = lx - OS_X, dy = ly - (SW_Y + 6); half = -1;   // Klick auf eine Beschriftung wählt diese Stellung direkt, Klick auf den Knopf schaltet weiter
      if (dx * dx + dy * dy > 20 * 20) { double a = std::atan2(dx, -dy) * 180 / PI; half = std::max(0, std::min(3, (int)std::lround((a + 60) / 40))); } return P_OS; }
    half = tsw::toggleHit(lx, ly, TAP_X, SW_Y, 62); if (half >= 0) return P_TAPER;
    return -1; }
  // Schalter bewegen: dir −1 = nach oben / links, +1 = nach unten / rechts (zweistufig: oben = an); 0 = umschalten bzw. weiterdrehen
  void moveSwitch(int p, int dir) {
    if (p == P_OS) { int i = osIdx(getP(P_OS), getP(P_ECO)); i = dir >= 100 ? dir - 100 : dir == 0 ? (i + 1) % 4 : std::max(0, std::min(3, i + dir)); setP(P_OS, i == 0 ? 0 : i <= 2 ? 1 : i - 1); setP(P_ECO, i == 1 ? 1 : 0); }
    else if (p == P_TAPER) { int t = (int)std::lround(getP(P_TAPER)); setP(P_TAPER, dir == 0 ? (t + 1) % 3 : std::max(0, std::min(2, t + dir))); }
    else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 1 : 0); }
  int knobParam(int i) const { return i < 9 ? KPARAM[i] : i == 9 ? P_VIN : i == 10 ? P_OUT : P_MIS; }
  double knobVal(int i) const { if (i < 9) return getP(KPARAM[i]); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : i == 10 ? (getP(P_OUT) + 36) / 60 * 10 : getP(P_MIS); }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 9) { if (KPARAM[i] >= 0) setP(KPARAM[i], std::round(u * 10) / 10); } else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else if (i == 11) setP(P_MIS, std::round(u * 10) / 10); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
  // ---- Ereignisse: liefert true, wenn neu gezeichnet werden soll ----
#ifndef __APPLE__
  bool handle(XEvent &ev) {
    switch (ev.type) {
      case Expose: return true;
      case ButtonPress: return onMouse(0, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case MotionNotify: return onMouse(1, ev.xmotion.x, ev.xmotion.y, 0, 0);
      case ButtonRelease: return onMouse(2, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case LeaveNotify: return onMouse(3, 0, 0, 0, 0);
      default: return false; } }
#endif
  // kind 0 drücken (button 1 links, 4/5 Mausrad), 1 bewegen, 2 loslassen, 3 verlassen; now in Sekunden
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else if (k < 0) { int half, p = hitSwitch(x, y, half); if (p >= 0) moveSwitch(p, p == P_OS ? (button == 4 ? 1 : -1) : (button == 4 ? -1 : 1)); } return true; }
        if (button != 1) return false;
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 8 ? 0 : k == 9 ? std::log(100.0) / std::log(1000.0) * 10 : k == 10 ? 6 : k == 11 ? 8 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int pp = hitPull(x, y); if (pp >= 0) { setP(pp, getP(pp) > 0.5 ? 0 : 1); return true; }
        int j = hitJack(x, y); if (j >= 0) { setP(P_INP, j == 0 ? 0 : 1); return true; }
        int tb2 = hitTube(x, y); if (tb2 == 4) { setP(P_TANK, getP(P_TANK) > 0.5 ? 0 : 1); return true; } if (tb2 == 3) { for (int i = 0; i < 3; i++) setP(P_TUBE1 + i, 0); return true; } if (tb2 >= 0) { setP(P_TUBE1 + tb2, std::fmod(std::lround(getP(P_TUBE1 + tb2)) + 1, 5)); return true; }
        { int half, b = hitSwitch(x, y, half); if (b >= 0) { moveSwitch(b, b == P_OS && half >= 0 ? 100 + half : b == P_TAPER ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 9 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }   /* Foto-Knöpfe: 250 Foto-Einheiten, INPUT/OUTPUT: 200 Pixel bei Standardgröße für den ganzen Weg */
        int h = hitKnob(x, y); bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;   // Signatur: Anzeigebalken auf ganze Pixel gerundet, damit nur sichtbare Änderungen neu zeichnen
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    for (int i = 0; i < 5; i++) sapp(b, sizeof b, n, "%d|", (int)std::lround(std::log10(1 + 9 * std::min(std::max(meter[i], 0.0), 2.0) / 2) * 74));   sapp(b, sizeof b, n, "%d|%d|%d|%d|%d", drag, hover, showTip ? 1 : 0, W, curveStale ? (int)(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() * 10) : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  // Fenster vom Host mit dem Containerfenster zerstört: nicht mehr zeichnen und nicht noch einmal zerstören
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace aorgui
