// Oberfläche des Kaiserin-Reverb-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die gezeichnete Frontplatte (engine/tools/kai_panel.py, eigene Gestaltung, kein Foto, kein Logo) mit Zeigern, Modustasten, Anzeigen und der Bedeutung
// von THING 1/2 im gewählten Modus, darunter der Modellstreifen (Nachhallzeiten, Werte) und die Tonkurve des Halls.
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
#include "kaiserin_core.hpp"
#include "kaigeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "kaipanel_jpg.h"
#include "widgets.hpp"

namespace kaigui {
constexpr double PI = 3.14159265358979323846;
enum { P_MODE, P_DECAY, P_MIX, P_OUT, P_LOW, P_HI, P_T1, P_T2, P_HOLD, P_COUNT };   // wie im Plugin
constexpr int NK = 7;
static const int KPARAM[NK] = {P_DECAY, P_MIX, P_OUT, P_LOW, P_HI, P_T1, P_T2};
constexpr double FW = KAIW, FH = KAIH;
constexpr double STRIPU = 96;
constexpr double PLOTU = 300;
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
constexpr int NF = 160;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "LOW and HI at 12 o'clock";
inline kai::Setting settingOf(const std::function<double(int)> &getP) { kai::Setting s; s.mode = std::max(0, std::min(kai::NMODE - 1, (int)std::lround(getP(P_MODE))));
  s.decay = getP(P_DECAY) / 10; s.mix = getP(P_MIX) / 10; s.out = getP(P_OUT) / 10; s.low = getP(P_LOW) / 10; s.hi = getP(P_HI) / 10; s.t1 = getP(P_T1) / 10; s.t2 = getP(P_T2) / 10; s.hold = getP(P_HOLD) > 0.5; return s; }

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey; double fsShow = 48000;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Knöpfe, 40 … 45 Modustasten, 46 HOLD
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(KAIPANEL_JPG, (int)KAIPANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  static double ang(double v) { return (-150 + 30 * v) * PI / 180; }   // Knopfstellung 0 … 10
  int mode() const { return std::max(0, std::min(kai::NMODE - 1, (int)std::lround(getP(P_MODE)))); }
  bool btnOn(int b) const { return b < 6 ? mode() == b : getP(P_HOLD) > 0.5; }
  void clickBtn(int b) { if (b < 6) setP(P_MODE, b); else setP(P_HOLD, getP(P_HOLD) > 0.5 ? 0 : 1); }
  void knobText(int i, char *buf, size_t n) const { kai::describe(settingOf(getP), i, buf, (int)n); }
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i); kai::Setting st = settingOf(getP); int m = st.mode;
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
    // Anzeigen: Modus (bernsteinfarben), HOLD (rot)
    for (int i = 0; i < 7; i++) { bool on = btnOn(i); double x = KAILED[i][0], y = KAILED[i][1]; int col = i < 6 ? 0xffb23c : 0xff4a3a; setrgb(c, on ? col : 0x46281e); cairo_new_path(c); cairo_arc(c, x, y, 5, 0, 2 * PI); cairo_fill(c);
      if (on) { double r = ((col >> 16) & 255) / 255.0, g = ((col >> 8) & 255) / 255.0, bb = (col & 255) / 255.0; cairo_pattern_t *gp = cairo_pattern_create_radial(x, y, 2, x, y, 16); cairo_pattern_add_color_stop_rgba(gp, 0, r, g, bb, 0.55); cairo_pattern_add_color_stop_rgba(gp, 1, r, g, bb, 0);
        cairo_new_path(c); cairo_arc(c, x, y, 16, 0, 2 * PI); cairo_set_source(c, gp); cairo_fill(c); cairo_pattern_destroy(gp); } }
    // gedrückte Tasten: dunkler, etwas tiefer, mit goldener Kante
    for (int b = 0; b < 7; b++) { const double *B = KAIBTN[b]; double x0 = B[0] - B[2] / 2, y0 = B[1] - B[3] / 2;
      if (btnOn(b)) { setrgb(c, 0x2a2026); tsw::rrect(c, x0, y0, B[2], B[3], 5); cairo_fill(c); setrgb(c, 0xb4aca2); tsw::rrect(c, x0 + 3, y0 + 5, B[2] - 6, B[3] - 7, 4); cairo_fill(c);
        const char *nm[7] = {"HALL", "PLATE", "SPRING", "ROOM", "MOD", "SPARKLE", "HOLD"}; setrgb(c, 0x2c1e28); tsw::text(c, nm[b], B[0], B[1] + 7, 12.5, true, 1);
        setrgb(c, 0xd6b266); cairo_set_line_width(c, 2.5); tsw::rrect(c, x0 + 1, y0 + 1, B[2] - 2, B[3] - 2, 5); cairo_stroke(c); }
      if (hover == 40 + b) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2); tsw::rrect(c, x0 - 4, y0 - 4, B[2] + 8, B[3] + 8, 7); cairo_stroke(c); } }
    // Knöpfe: Zeiger auf der Kappe
    for (int i = 0; i < NK; i++) { const double *g = KAIKNOB[i];
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, ang(v[KPARAM[i]])); setrgb(c, 0x241820); cairo_set_line_width(c, 3.5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.08); cairo_line_to(c, 0, -g[2] * 0.66); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.1, 0, 2 * PI); cairo_stroke(c); } }
    // Bedeutung von THING 1/2 im gewählten Modus
    setrgb(c, 0xd6b266); tsw::text(c, kai::THING1[m], KAIKNOB[5][0], 214, 12, true, 1); tsw::text(c, kai::THING2[m], KAIKNOB[6][0], 214, 12, true, 1);
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale();
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 16, 20, 12, false);   /* ohne „MODEL (not on the …)“ (Wunsch des Nutzers, 19.11.2026) */
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { char t[320]; setrgb(c, 0xdcdee2); std::snprintf(t, sizeof t, "%s%s", kai::MODE_NAME[m], st.hold ? "  ·  HOLD (input muted, tail frozen)" : ""); tsw::text(c, t, 16, 48, 14, true);
      double tm = kai::t60Mid(m, st.decay); std::snprintf(t, sizeof t, "RT60  low %.2f s   mid %.2f s   high %.2f s%s", tm * kai::lowRatio(st.low), tm, tm * kai::hiRatioMode(m, st), m == kai::M_SPRING ? "  (approx.)" : "");
      setrgb(c, 0xcfd1d6); tsw::text(c, t, 300, 48, 13, false);
      char a[120], b[120]; kai::describe(st, 5, a, sizeof a); kai::describe(st, 6, b, sizeof b); std::snprintf(t, sizeof t, "THING 1: %s     THING 2: %s", a, b); setrgb(c, 0x9a9ea6); tsw::text(c, t, 16, 74, 12.5, false);
      kai::describe(st, 1, a, sizeof a); setrgb(c, 0x9a9ea6); tsw::text(c, a, tsw::UW - 16, 74, 12.5, false, 2); }
    // Tonkurve des Halls
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_OUT && id != P_MIX && id != P_HOLD) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); if (computeCurves && key != curveKey) { computeCurves(curve, refCurve); curveKey = key; } }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, "REVERB TONE  (energy of the decay per frequency x LOW/HI, relative to 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, true); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[200]; double ax, ay;
      if (i < NK) { const double *g = KAIKNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.15) * s; knobText(i, buf, sizeof buf); }
      else { const double *B = KAIBTN[i - 40]; ax = B[0] * s; ay = (B[1] - B[3]) * s;
        if (i < 46) std::snprintf(buf, sizeof buf, "%s%s", kai::MODE_NAME[i - 40], btnOn(i - 40) ? " (selected)" : " (click to select)");
        else std::snprintf(buf, sizeof buf, "HOLD %s (infinite hold: input muted, tail kept)", btnOn(6) ? "on" : "off"); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { double dx = x - KAIKNOB[i][0], dy = y - KAIKNOB[i][1]; if (dx*dx + dy*dy <= KAIKNOB[i][2] * KAIKNOB[i][2] * 1.69) return i; } return -1; }
  int hitBtn(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int b = 0; b < 7; b++) { const double *B = KAIBTN[b]; if (std::fabs(x - B[0]) <= B[2] / 2 + 3 && std::fabs(y - B[1]) <= B[3] / 2 + 3) return 40 + b; }
    for (int b = 0; b < 7; b++) { double dx = x - KAILED[b][0], dy = y - KAILED[b][1]; if (dx*dx + dy*dy <= 12 * 12) return 40 + b; } return -1; }
  int knobParam(int i) const { return KPARAM[i]; }
  double knobVal(int i) const { return getP(KPARAM[i]); }
  void setKnob(int i, double uu) { uu = std::max(0.0, std::min(10.0, uu)); double r = std::round(uu * 100) / 100; if (std::fabs(r - 5) < 0.08) r = 5; setP(KPARAM[i], r); }   // leichte Rastung bei 12 Uhr
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
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else { int b = hitBtn(x, y); if (b >= 40 && b < 46) setP(P_MODE, std::max(0, std::min(5, mode() + (button == 4 ? -1 : 1)))); } return true; }
        if (button != 1) return false;
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) setKnob(k, 5); else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int b = hitBtn(x, y); if (b >= 0) { clickBtn(b - 40); return true; }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * 10.0 / 250 / scale()); return true; }
        int h = hitKnob(x, y); if (h < 0) h = hitBtn(x, y);
        bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    sapp(b, sizeof b, n, "%d|%d|%d|%d", drag, hover, showTip ? 1 : 0, W); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace kaigui
