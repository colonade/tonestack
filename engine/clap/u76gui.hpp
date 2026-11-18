// Oberfläche des AmpFett-76-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die gezeichnete Frontplatte (engine/tools/u76_panel.py, eigene Gestaltung, kein Foto, kein Logo) mit Zeigern, gedrückten Tasten und VU-Nadel,
// darunter der Modellstreifen (Cali76-Erweiterungen: Ratio-Art und stufenlose Ratio, DRY, Hochpass der Seitenkette, zweite Stufe; Pegel, Poti-Kennlinie,
// 10-LED-Anzeige der Gain Reduction) und der Verlauf der Gain Reduction.
#pragma once
#ifndef TS_VERSION
#define TS_VERSION "?"
#endif
#ifdef __APPLE__
#include "cairo_cg.hpp"
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
#include "u76geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "u76panel_jpg.h"
#include "widgets.hpp"
#include "autoin.hpp"

namespace u76gui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_IN, P_OUTK, P_ATT, P_REL, P_RATIO, P_RVAR, P_METER, P_DRY, P_HPF, P_HPFF, P_STACK, P_THRU, P_ATT2, P_REL2, P_OUT2, P_VIN, P_INREF, P_OUT, P_TAPER, P_COUNT };
static const int KPARAM[4] = {P_IN, P_OUTK, P_ATT, P_REL};
static const char *KNAME[4] = {"INPUT", "OUTPUT", "ATTACK", "RELEASE"};
constexpr double FW = U76W, FH = U76H;
constexpr double STRIPU = 312;   // Modellstreifen (zwei Reihen)
constexpr double PLOTU = 200;    // Verlauf der Gain Reduction
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
constexpr int NF = 240;           // Stützstellen des Verlaufs (4 s, 60 je Sekunde)
// Modellstreifen: Kippschalter (Reihe 1) und Knöpfe
constexpr double R1 = 98, R2 = 238;   // Mitte der Reihen (Kippschalter-Mutter bzw. Knopf)
static const double SW_X[4] = {60, 470, 690, 60};   // RATIO-Art, SC HPF, STACK, POT TAPER (Reihe 2)
// Knöpfe im Streifen: 0 VAR RATIO, 1 DRY, 2 HPF FREQ, 3 THRU, 4 ATTACK 2, 5 RELEASE 2, 6 OUTPUT 2, 7 INPUT LEVEL (mV), 8 OUTPUT (dB)
static const double SK_X[9] = {215, 330, 580, 805, 895, 985, 1075, 960, 1110};
static const double SK_Y[9] = {R1 - 20, R1 - 20, R1 - 20, R1 - 20, R1 - 20, R1 - 20, R1 - 20, R2 - 22, R2 - 22};
static const int SK_P[9] = {P_RVAR, P_DRY, P_HPFF, P_THRU, P_ATT2, P_REL2, P_OUT2, P_VIN, P_OUT};
inline double potTaper(double x, int mode) { if (x <= 0) return 0; if (x >= 1) return 1; if (mode == 2) return x; const double m = mode == 1 ? 0.10 : 0.15; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // wie im Kern
// Winkel der Frontplattenknöpfe (Grad): INPUT/OUTPUT 0 … 10 über 300°; ATTACK: OFF bei −150°, 1 … 7 von −120° bis +150°; RELEASE 1 … 7 über 300°
inline double knobDeg(int i, double v) { if (i < 2) return -150 + 30 * v; if (i == 2) return v < 0.5 ? -150 : -120 + 45 * (std::min(7.0, std::max(1.0, v)) - 1); return -150 + 50 * (std::min(7.0, std::max(1.0, v)) - 1); }
inline double vuDeg(double db) { double x = std::pow(10, db / 20); return -42 + 84 * (x - 0.1) / (std::pow(10, 0.15) - 0.1); }   // wie u76_panel.py

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double inPk = -999; std::function<void()> resetPeak;   // Eingangsbezug (autoin.hpp)
  double gr = 0, gr2 = 0, outDbu = -99; bool meterOn = false;   // GR je Stufe (dB), Ausgang (dBu eff., für +4/+8)
  double needle = 0;   // Nadelstellung in dB (VU-Ballistik, im Plugin geglättet)
  std::vector<double> hist, hist2;   // Verlauf der GR (dB), ältester zuerst
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  // Trefferkennungen: 0 … 3 Frontplattenknöpfe, 40 … 47 Tasten (RATIO 20, 12, 8, 4; METER GR, +8, +4, OFF), 60 … 68 Knöpfe im Streifen
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(U76PANEL_JPG, (int)U76PANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  static void setrgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
  int ratio() const { return std::max(0, std::min(5, (int)std::lround(getP(P_RATIO)))); }
  int meter() const { return std::max(0, std::min(3, (int)std::lround(getP(P_METER)))); }
  // Taste gedrückt? 40 … 43 = RATIO 20, 12, 8, 4 (Parameter 3, 2, 1, 0; „alle“ = alle vier); 44 … 47 = METER GR, +8, +4, OFF
  bool btnOn(int b) const { if (b < 4) { int r = ratio(); return r == 4 || r == 3 - b; } return meter() == b - 4; }
  void clickBtn(int b) { if (b < 4) setP(P_RATIO, 3 - b); else setP(P_METER, b - 4); }
  void knobText(int i, char *buf, size_t n) const { double v = getP(KPARAM[i]); int tp = std::max(0, std::min(2, (int)std::lround(getP(P_TAPER))));
    if (i < 2) std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, %s)", KNAME[i], v, potTaper(v / 10, tp) * 100, tp == 2 ? "linear" : tp == 1 ? "log 10 %" : "log 15 %");
    else if (i == 2 && v < 0.5) std::snprintf(buf, n, "ATTACK  OFF  (gain reduction disabled, S9)");
    else std::snprintf(buf, n, "%s  %.1f  (%s)", KNAME[i], std::max(1.0, v), i == 2 ? "1 slow ... 7 fast, about 0.8 ms ... 20 us" : "1 slow ... 7 fast, about 1.1 s ... 50 ms"); }
  // Streifenknöpfe: Stellung 0 … 1 und Text
  double skU(int k) const { double v = getP(SK_P[k]);
    switch (k) { case 0: return (v - 4) / 16; case 2: return std::log(v / 20) / std::log(30.0); case 4: return v < 0.5 ? 0 : 0.1 + 0.9 * (v - 1) / 6; case 5: return (v - 1) / 6;
      case 7: return std::log(v / 10) / std::log(1000.0); case 8: return (v + 36) / 60; default: return v / 10; } }
  void skText(int k, char *b, size_t n) const { double v = getP(SK_P[k]);
    switch (k) { case 0: std::snprintf(b, n, "%.1f : 1", v); break; case 2: std::snprintf(b, n, "%.0f Hz", v); break; case 4: if (v < 0.5) std::snprintf(b, n, "OFF"); else std::snprintf(b, n, "%.1f", v); break;
      case 5: std::snprintf(b, n, "%.1f", v); break; case 7: tsin::valueText(b, (int)n, v, getP(P_INREF) > 0.5); break; case 8: std::snprintf(b, n, "%+.1f dB", v); break; default: std::snprintf(b, n, "%.1f", v); } }
  void setSk(int k, double u) { u = std::max(0.0, std::min(1.0, u));
    switch (k) { case 0: setP(SK_P[k], std::round((4 + 16 * u) * 10) / 10); break; case 2: setP(SK_P[k], std::round(20 * std::pow(30.0, u))); break;
      case 4: setP(SK_P[k], u < 0.05 ? 0 : std::round((1 + 6 * std::max(0.0, u - 0.1) / 0.9) * 10) / 10); break; case 5: setP(SK_P[k], std::round((1 + 6 * u) * 10) / 10); break;
      case 7: setP(SK_P[k], std::round(10 * std::pow(1000.0, u))); break; case 8: setP(SK_P[k], std::round((u * 60 - 36) * 2) / 2); break; default: setP(SK_P[k], std::round(u * 100) / 10); } }
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef __APPLE__
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
    cairo_save(c); cairo_scale(c, s, s);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); });
    cairo_save(c); cairo_scale(c, s, s);
#endif
    // gedrückte Tasten: dunkler, tiefer, gelbe Kante; METER OFF: Instrument dunkel
    for (int b = 0; b < 8; b++) { const double *B = U76BTN[b]; double x0 = B[0] - B[2] / 2, y0 = B[1] - B[3] / 2;
      if (btnOn(b)) { setrgb(c, 0x23262c); tsw::rrect(c, x0, y0, B[2], B[3], 5); cairo_fill(c); setrgb(c, 0x9ca2ac); tsw::rrect(c, x0 + 3, y0 + 6, B[2] - 6, B[3] - 8, 4); cairo_fill(c);
        const char *lt[8] = {"20", "12", "8", "4", "GR", "+8", "+4", "OFF"}; setrgb(c, 0x16181c); tsw::text(c, lt[b], B[0], B[1] + 8, 15, true, 1);
        setrgb(c, 0xf2c14e); cairo_set_line_width(c, 2.5); tsw::rrect(c, x0 + 1, y0 + 1, B[2] - 2, B[3] - 2, 5); cairo_stroke(c); }
      if (hover == 40 + b) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2); tsw::rrect(c, x0 - 4, y0 - 4, B[2] + 8, B[3] + 8, 7); cairo_stroke(c); } }
    // VU-Nadel (GR: 0 = Ruhe, Ausschlag nach links; +8/+4: Ausgangspegel)
    // ohne Clip (den gibt es in cairo_cg.hpp nicht): Winkel auf ±49° begrenzt (Spitze bleibt im Fenster), Anfang der Nadel an der Unterkante des Fensters
    { const double *V = U76VU;
      if (meter() == 3 || !meterOn) { setrgb(c, 0x000000, 0.45); cairo_rectangle(c, V[0], V[1], V[2] - V[0], V[3] - V[1]); cairo_fill(c); }
      double db = meter() == 3 ? -40 : std::max(-25.0, std::min(4.5, needle)); double deg = std::max(-49.0, std::min(49.0, db < -20 ? vuDeg(-20) - (-20 - db) * 0.6 : vuDeg(db))), a = deg * PI / 180;
      double r0 = (V[5] - V[3]) / std::cos(a) + 2;
      cairo_save(c); cairo_translate(c, V[4], V[5]); cairo_rotate(c, a); setrgb(c, 0x000000, 0.25); cairo_set_line_width(c, 3); cairo_move_to(c, 3, -r0); cairo_line_to(c, 3, -V[6]); cairo_stroke(c);
      setrgb(c, 0x16161a); cairo_set_line_width(c, 2.2); cairo_move_to(c, 0, -r0); cairo_line_to(c, 0, -V[6]); cairo_stroke(c); cairo_restore(c); }
    // Knöpfe: Zeiger auf der Metallkappe
    for (int i = 0; i < 4; i++) { const double *g = U76KNOB[i];
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, knobDeg(i, v[KPARAM[i]]) * PI / 180); setrgb(c, 0x16181c); cairo_set_line_width(c, i < 2 ? 5 : 4); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
      cairo_move_to(c, 0, -g[2] * 0.12); cairo_line_to(c, 0, -g[2] * 0.58); cairo_stroke(c); setrgb(c, 0xf2f2f2); cairo_move_to(c, 0, -g[2] * 0.7); cairo_line_to(c, 0, -g[2] * 0.95); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.1, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); bool stk = v[P_STACK] > 0.5, hpf = v[P_HPF] > 0.5; int r = ratio();
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the unit; Cali76 extensions)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 380, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *rl[3] = {"BUTTONS", "ALL", "VARIABLE"}; tsw::toggle(c, SW_X[0], R1, 3, r < 4 ? 0 : r == 4 ? 1 : 2, "RATIO", rl, nullptr);
      const char *ol[2] = {"OFF", "ON"}; tsw::toggle(c, SW_X[1], R1, 2, hpf ? 1 : 0, "SC HPF", ol, nullptr); tsw::toggle(c, SW_X[2], R1, 2, stk ? 1 : 0, "STACK", ol, nullptr);
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; int tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER]))); tsw::toggle(c, SW_X[3], R2, 3, tp, "POT TAPER", tl, nullptr); }
    { const char *nm[9] = {"VAR RATIO", "DRY", "HPF FREQ", "THRU", "ATTACK 2", "RELEASE 2", "OUTPUT 2", "INPUT", "OUTPUT"};
      for (int k = 0; k < 9; k++) { char t[40]; skText(k, t, sizeof t); bool dim = (k == 0 && r != 5) || (k == 2 && !hpf) || (k >= 3 && k <= 6 && !stk); tsw::smallKnobV(c, SK_X[k], SK_Y[k], skU(k), nm[k], t, drag == 60 + k || hover == 60 + k, dim); } }
    { char xt[64]; tsin::extraText(xt, sizeof xt, inPk, v[P_INREF] > 0.5); setrgb(c, 0x9a9ea6); tsw::text(c, xt, SK_X[7], SK_Y[7] + 76, 11, false, 1); }
    // 10-LED-Anzeige der Gain Reduction (wie beim Cali76 V2: je LED 2 dB), Stufe 2 darunter
    { setrgb(c, 0xdcdee2); tsw::text(c, "GAIN REDUCTION", 250, R2 - 46, 13, true);
      for (int row = 0; row < (stk ? 2 : 1); row++) { double g = row ? gr2 : gr, y = R2 - 34 + row * 30; setrgb(c, 0x9a9ea6); tsw::text(c, row ? "stage 2" : stk ? "stage 1" : "", 250, y + 15, 11, false);
        for (int k = 0; k < 10; k++) { double x = 310 + k * 30; bool on = meterOn && g >= 2 * k + 1.0; setrgb(c, on ? (k < 6 ? 0xf2f2f2 : k < 8 ? 0xffd98a : 0xff8a6a) : 0x2a2c31); tsw::rrect(c, x, y, 22, 18, 3); cairo_fill(c); }
        char t[24]; std::snprintf(t, sizeof t, meterOn ? "%.1f dB" : "-", g); setrgb(c, 0xcfd1d6); tsw::text(c, t, 616, y + 14, 12, false); }
      setrgb(c, 0x6c7079); for (int k = 0; k < 10; k += 2) { char t[8]; std::snprintf(t, sizeof t, "%d", 2 * k + 2); tsw::text(c, t, 321 + k * 30 + 30, R2 + 36 + (stk ? 26 : 0), 10, false, 1); } }
    // Verlauf der Gain Reduction (4 s)
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 34, py1 = PLOTU - 30; setrgb(c, 0xc8ccd4); tsw::text(c, "GAIN REDUCTION  (last 4 s)", px0, 22, 13, true);
      const double top = 30; auto yOf = [&](double g) { return py0 + (py1 - py0) * std::min(top, std::max(0.0, g)) / top; };
      for (int g = 0; g <= 30; g += 6) { double y = yOf(g); setrgb(c, 0x2a2d33); cairo_rectangle(c, px0, y, px1 - px0, 1); cairo_fill(c); char t[8]; std::snprintf(t, sizeof t, g ? "-%d" : "0", g); setrgb(c, 0x8d919a); tsw::text(c, t, px0 - 8, y + 4, 11, false, 2); }
      for (int sec = 0; sec <= 4; sec++) { double x = px1 - (px1 - px0) * sec / 4.0; char t[8]; std::snprintf(t, sizeof t, sec ? "-%d s" : "now", sec); setrgb(c, 0x8d919a); tsw::text(c, t, x, py1 + 18, 11, false, 1); }
      auto line = [&](const std::vector<double> &h, int col) { if (h.size() < 2) return; setrgb(c, col); cairo_set_line_width(c, 2); cairo_new_path(c);
        for (size_t i = 0; i < h.size(); i++) { double x = px1 - (px1 - px0) * (double)(h.size() - 1 - i) / (NF - 1), y = yOf(h[i]); if (i == 0) cairo_move_to(c, x, y); else cairo_line_to(c, x, y); } cairo_stroke(c); };
      if (stk) line(hist2, 0xe8b23c); line(hist, 0x62a0e6);
      if (stk) { setrgb(c, 0x62a0e6); tsw::text(c, "stage 1", px1 - 140, 22, 12, true); setrgb(c, 0xe8b23c); tsw::text(c, "stage 2", px1 - 70, 22, 12, true); }
      cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[160]; double ax, ay;
      if (i < 4) { const double *g = U76KNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, buf, sizeof buf); }
      else if (i >= 40 && i < 48) { const double *B = U76BTN[i - 40]; ax = B[0] * s; ay = (B[1] - B[3]) * s; const char *nm[8] = {"RATIO 20:1", "RATIO 12:1", "RATIO 8:1", "RATIO 4:1", "METER: gain reduction", "METER: output, 0 VU = +8 dBu", "METER: output, 0 VU = +4 dBu", "METER: off"};
        std::snprintf(buf, sizeof buf, "%s%s", nm[i - 40], i < 44 && ratio() == 4 ? "  (all buttons in)" : i < 44 && ratio() == 5 ? "  (variable ratio active)" : ""); }
      else { int k = i - 60; ax = SK_X[k] * u; ay = FH * s + (SK_Y[k] - 30) * u; char t[40]; skText(k, t, sizeof t);
        const char *ex[9] = {"VAR RATIO (Cali76): continuous 4:1 ... 20:1, tap slides along both ratio chains", "DRY (Cali76): uncompressed input mixed in", "SIDECHAIN HPF (Cali76 Bass)", "THRU: input of stage 2 (Cali76 Stacked)",
          "ATTACK stage 2", "RELEASE stage 2", "OUTPUT stage 2", "INPUT LEVEL", "OUTPUT LEVEL (digital)"}; std::snprintf(buf, sizeof buf, "%s  %s", ex[k], t); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < 4; i++) { double dx = x - U76KNOB[i][0], dy = y - U76KNOB[i][1]; if (dx*dx + dy*dy <= U76KNOB[i][2] * U76KNOB[i][2] * 1.44) return i; }
    double lx, ly; toLower(px, py, lx, ly); for (int k = 0; k < 9; k++) { double dx = lx - SK_X[k], dy = ly - SK_Y[k]; if (dx*dx + dy*dy <= 30 * 30) return 60 + k; } return -1; }
  int hitBtn(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int b = 0; b < 8; b++) { const double *B = U76BTN[b]; if (std::fabs(x - B[0]) <= B[2] / 2 + 3 && std::fabs(y - B[1]) <= B[3] / 2 + 3) return 40 + b; } return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); const double cy[4] = {R1, R1, R1, R2}, lw[4] = {80, 40, 40, 80}; for (int k = 0; k < 4; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], cy[k], lw[k]); if (half >= 0) return k; } return -1; }
  void moveSwitch(int k, int dir) {   // dir −1 hoch, +1 runter, 0 umschalten
    if (k == 0) { int r = ratio(), pos = r < 4 ? 0 : r == 4 ? 1 : 2; pos = std::max(0, std::min(2, pos + (dir == 0 ? 1 : dir))); if (dir == 0 && r == 5) pos = 0; setP(P_RATIO, pos == 0 ? (r < 4 ? r : 0) : pos == 1 ? 4 : 5); }
    else if (k == 3) { int t = (int)std::lround(getP(P_TAPER)); setP(P_TAPER, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); }
    else { int p = k == 1 ? P_HPF : P_STACK; setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 0 : 1); } }
  int knobParam(int i) const { return i < 4 ? KPARAM[i] : SK_P[i - 60]; }
  // ATTACK: Ziehweg 0 … 1 = OFF, 1 … 10 = Stellung 1 … 7 (wie der Winkel in knobDeg)
  double knobVal(int i) const { if (i < 4) { double v = getP(KPARAM[i]); if (i < 2) return v; if (i == 2) return v < 0.5 ? 0 : 1 + 9 * (v - 1) / 6; return (v - 1) / 6 * 10; } return skU(i - 60) * 10; }
  void setKnob(int i, double uu) { uu = std::max(0.0, std::min(10.0, uu));
    if (i < 2) setP(KPARAM[i], std::round(uu * 10) / 10);
    else if (i == 2) { if (uu < 0.5) setP(P_ATT, 0); else setP(P_ATT, std::round((1 + 6 * std::max(0.0, std::min(1.0, (uu - 1.0) / 9))) * 10) / 10); }
    else if (i == 3) setP(P_REL, std::round((1 + 6 * uu / 10) * 10) / 10);
    else setSk(i - 60, uu / 10); }
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
  bool hitInRef(double px, double py) const { double lx, ly; toLower(px, py, lx, ly); return std::fabs(lx - SK_X[7]) < 85 && ly > SK_Y[7] + 64 && ly < SK_Y[7] + 82; }
  double defaultVal(int k) const { switch (k) { case 0: case 1: return 5; case 2: return 1 + 9 * 3.0 / 6; case 3: return 5; case 60: return (8.0 - 4) / 16 * 10; case 61: return 0; case 62: return std::log(5.0) / std::log(30.0) * 10;
      case 63: case 66: return 5; case 64: return 0.1 * 10 + 0.9 * 10 * 3 / 6; case 65: return 5; case 67: return std::log(30.0) / std::log(1000.0) * 10; case 68: return 6; default: return 5; } }
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else { int half, sw = hitSwitch(x, y, half); if (sw >= 0) moveSwitch(sw, button == 4 ? -1 : 1); } return true; }
        if (button != 1) return false;
        if (hitInRef(x, y)) { if (getP(P_INREF) > 0.5) setP(P_INREF, 0); else if (resetPeak) resetPeak(); return true; }
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) setKnob(k, defaultVal(k)); else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int b = hitBtn(x, y); if (b >= 0) { clickBtn(b - 40); return true; }
        { int half, sw = hitSwitch(x, y, half); if (sw >= 0) { moveSwitch(sw, sw == 0 || sw == 3 ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 60 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); if (h < 0) h = hitBtn(x, y);
        bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[1024]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i)); sapp(b, sizeof b, n, "%.1f|%.1f|%.1f|%.2f|", inPk, gr, gr2, needle);
    double hs = 0; for (double h : hist) hs += h; for (double h : hist2) hs += 3 * h; sapp(b, sizeof b, n, "%.1f|%zu|%d|%d|%d|%d|%d", hs, hist.size(), meterOn ? 1 : 0, drag, hover, showTip ? 1 : 0, W); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace u76gui
