// Oberfläche des Viereck 34, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die gezeichnete Frontplatte (engine/tools/viereck_panel.py, eigene Gestaltung) mit Zeigern, Filtertasten mit Leuchten und dem Kippschalter
// STEPS/SMOOTH, darunter der Modellstreifen (Aussteuerung der beiden OPV, LEVEL, OUTPUT) und der Frequenzgang.
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
#include "viereckgeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "viereckpanel_jpg.h"
#include "widgets.hpp"

namespace vrgui {
constexpr double PI = 3.14159265358979323846;
enum { P_TILT, P_BASS, P_BAL, P_FILT, P_SLOPE, P_MODE, P_PIV, P_LEVEL, P_OUT, P_COUNT };
constexpr int NK = 4;   // BASS, TILT, BALANCE, TILT PIVOT
static const int KPARAM[NK] = {P_BASS, P_TILT, P_BAL, P_PIV};
constexpr double FW = VRW, FH = VRH;
constexpr double STRIPU = 150;
constexpr double PLOTU = 300;
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
constexpr double LV_X = 760, OUT_X = 1120, KN_Y = 62;   // LEVEL, OUTPUT im Modellstreifen
constexpr int NF = 160;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "flat, filters off";
static const char *BASSN[7] = {"step 300 Hz", "step 170 Hz", "step 100 Hz", "0 dB", "lift +3 dB", "lift +6 dB", "lift +9 dB"};

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves; std::function<void(double*)> peaks;
  std::vector<double> curve, refCurve; std::string curveKey; double pk[2] = {0, 0};
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Knöpfe, 40 … 42 Tasten F1/F2/SLOPE, 30 Kippschalter STEPS/SMOOTH, 50 LEVEL, 51 OUTPUT (Modellstreifen)
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(VRPANEL_JPG, (int)VRPANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  bool smooth() const { return getP(P_MODE) > 0.5; }
  int filt() const { return std::max(0, std::min(2, (int)std::lround(getP(P_FILT)))); }
  // Zeigerwinkel in Grad: BASS und TILT 7 Rasten über ±135° (TILT +3 = ganz links, wie am Gerät: rechts herum wärmer), BALANCE und PIVOT ±150°
  double angle(int i) const { double v = getP(KPARAM[i]);
    switch (i) { case 0: return -135 + 45 * std::lround(v);
      case 1: { double t = smooth() ? v : (double)std::lround(v); return -135 + 45 * (3 - t); }
      case 2: return 150 * v;
      default: return -150 + 300 * std::log(v / VRPIV[0]) / std::log(VRPIV[1] / VRPIV[0]); } }
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
    // Filtertasten: gedrückt dunkler mit Kante; Leuchten rot (SLOPE nur mit Filter, wie am Gerät)
    int f = filt(); bool on[3] = {f == 1, f == 2, f != 0 && v[P_SLOPE] > 0.5};
    for (int b = 0; b < 3; b++) { const double *B = VRBTN[b]; double x0 = B[0] - B[2] / 2, y0 = B[1] - B[3] / 2;
      if (on[b]) { setrgb(c, 0x2a2724); tsw::rrect(c, x0, y0, B[2], B[3], 4); cairo_fill(c); setrgb(c, 0x5c5850); tsw::rrect(c, x0 + 3, y0 + 5, B[2] - 6, B[3] - 7, 3); cairo_fill(c); }
      if (hover == 40 + b) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2); tsw::rrect(c, x0 - 4, y0 - 4, B[2] + 8, B[3] + 8, 6); cairo_stroke(c); }
      if (on[b]) { double x = B[0], y = VRLEDY; setrgb(c, 0xff4a2e); cairo_new_path(c); cairo_arc(c, x, y, 5, 0, 2 * PI); cairo_fill(c);
        cairo_pattern_t *g = cairo_pattern_create_radial(x, y, 2, x, y, 16); cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.3, 0.18, 0.55); cairo_pattern_add_color_stop_rgba(g, 1, 1, 0.3, 0.18, 0); cairo_new_path(c); cairo_arc(c, x, y, 16, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g); } }
    // Kippschalter STEPS (oben) / SMOOTH (unten)
    { const double mx = VRMODE[0], my = VRMODE[1]; double a = smooth() ? 1 : -1; setrgb(c, 0x3a3630); cairo_new_path(c); cairo_arc(c, mx, my, 13, 0, 2 * PI); cairo_fill(c);
      setrgb(c, 0xd8d6cf); cairo_set_line_width(c, 9); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, mx, my); cairo_line_to(c, mx, my + a * 30); cairo_stroke(c);
      setrgb(c, 0xf4f3ef); cairo_new_path(c); cairo_arc(c, mx, my + a * 30, 7, 0, 2 * PI); cairo_fill(c);
      if (hover == 30) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2.5); cairo_new_path(c); cairo_arc(c, mx, my, 40, 0, 2 * PI); cairo_stroke(c); } }
    // Knöpfe: heller Zeiger mit Saum
    for (int i = 0; i < NK; i++) { const double *g = VRKNOB[i];
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, angle(i) * PI / 180); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
      setrgb(c, 0xefe9da, 0.28); cairo_set_line_width(c, 6); cairo_move_to(c, 0, -g[2] * 0.2); cairo_line_to(c, 0, -g[2] * 0.82); cairo_stroke(c);
      setrgb(c, 0xefe9da); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -g[2] * 0.2); cairo_line_to(c, 0, -g[2] * 0.82); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.12, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); char vo[40], vl[60]; std::snprintf(vo, 40, "%+.1f dB", v[P_OUT]); std::snprintf(vl, 60, "%+.0f dB re 0.5 V", v[P_LEVEL]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 16, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    // Aussteuerung der beiden OPV (TL071, Grenze etwa ±7,5 V): Balken bis 100 %, darüber rot
    { setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", 30, 52, 13, true); const char *nm[2] = {"IC12 tilt / bass", "IC19 filter"};
      for (int k = 0; k < 2; k++) { double y = 66 + k * 30, x0 = 160, w = 300, a = std::min(1.2, pk[k]); setrgb(c, 0x9a9ea6); tsw::text(c, nm[k], 30, y + 13, 12, false);
        setrgb(c, 0x23262b); cairo_rectangle(c, x0, y, w, 16); cairo_fill(c); setrgb(c, a >= 1 ? 0xff5040 : a > 0.5 ? 0xf2c14e : 0x5cc46a); cairo_rectangle(c, x0, y, w * std::min(1.0, a), 16); cairo_fill(c);
        char t[32]; std::snprintf(t, 32, a < 0.001 ? "–" : "%.0f %%", a * 100); setrgb(c, 0xcfd1d6); tsw::text(c, t, x0 + w + 12, y + 13, 12, false); } }
    { setrgb(c, 0xdcdee2); char t[120]; std::snprintf(t, sizeof t, "BASS %s · TILT %s%+.*f · %s%s", BASSN[std::max(0, std::min(6, (int)std::lround(v[P_BASS])))], smooth() ? "" : "", smooth() ? 2 : 0, smooth() ? v[P_TILT] : (double)std::lround(v[P_TILT]),
        filt() == 0 ? "no filter" : filt() == 1 ? "filter 1" : "filter 2", filt() && v[P_SLOPE] > 0.5 ? " + slope" : "");
      tsw::text(c, t, 30, 136, 12, false); char p[60]; std::snprintf(p, 60, v[P_PIV] >= 1000 ? "pivot %.2f kHz" : "pivot %.0f Hz", v[P_PIV] >= 1000 ? v[P_PIV] / 1000 : v[P_PIV]); setrgb(c, 0x9a9ea6); tsw::text(c, p, 520, 136, 12, false); }
    tsw::smallKnobV(c, LV_X, KN_Y, (v[P_LEVEL] + 30) / 40, "LEVEL (0 dBFS)", vl, drag == 50 || hover == 50, false);
    tsw::smallKnobV(c, OUT_X, KN_Y, (v[P_OUT] + 24) / 48, "OUTPUT", vo, drag == 51 || hover == 51, false);
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_OUT && id != P_LEVEL) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); if (computeCurves && key != curveKey) { computeCurves(curve, refCurve); curveKey = key; } }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, "FREQUENCY RESPONSE  (small signal, left channel)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, false); cairo_restore(c); }
    cairo_restore(c);
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { double dx = x - VRKNOB[i][0], dy = y - VRKNOB[i][1]; if (dx*dx + dy*dy <= VRKNOB[i][2] * VRKNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); { double dx = lx - LV_X, dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 50; } { double dx = lx - OUT_X, dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 51; } return -1; }
  int hitBtn(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int b = 0; b < 3; b++) { const double *B = VRBTN[b]; if (std::fabs(x - B[0]) <= B[2] / 2 + 3 && std::fabs(y - B[1]) <= B[3] / 2 + 3) return 40 + b; }
    if (std::hypot(x - VRMODE[0], y - VRMODE[1]) < 40) return 30; return -1; }
  // Tasten wie am Gerät: F1 und F2 schließen sich aus, nochmals drücken schaltet ab; SLOPE nur mit Filter, ohne Filter wird SLOPE gelöscht
  void clickBtn(int b) { int f = filt();
    if (b == 0 || b == 1) { int nf = f == b + 1 ? 0 : b + 1; setP(P_FILT, nf); if (nf == 0) setP(P_SLOPE, 0); }
    else if (b == 2) { if (f != 0) setP(P_SLOPE, getP(P_SLOPE) > 0.5 ? 0 : 1); }
  }
  int knobParam(int i) const { return i < NK ? KPARAM[i] : i == 50 ? P_LEVEL : P_OUT; }
  // Knopfstellung 0 … 10 (für Ziehen und Mausrad)
  double knobVal(int i) const { double v = getP(knobParam(i));
    switch (i) { case 0: return v / 6 * 10; case 1: return (3 - v) / 6 * 10; case 2: return (v + 1) * 5; case 3: return 10 * std::log(v / VRPIV[0]) / std::log(VRPIV[1] / VRPIV[0]);
      case 50: return (v + 30) / 40 * 10; default: return (v + 24) / 48 * 10; } }
  void setKnob(int i, double uu) { uu = std::max(0.0, std::min(10.0, uu));
    switch (i) { case 0: setP(P_BASS, std::round(uu / 10 * 6)); break;
      case 1: { double t = 3 - uu / 10 * 6; setP(P_TILT, smooth() ? std::round(t * 100) / 100 : std::round(t)); break; }
      case 2: { double b = std::round((uu / 5 - 1) * 100) / 100; if (std::fabs(b) < 0.03) b = 0; setP(P_BAL, b); break; }
      case 3: { double f = VRPIV[0] * std::pow(VRPIV[1] / VRPIV[0], uu / 10); if (std::fabs(std::log(f / VRPIV[2])) < 0.02) f = VRPIV[2]; setP(P_PIV, f); break; }   // rastet am Original
      case 50: setP(P_LEVEL, std::round(uu / 10 * 40 - 30)); break;
      default: setP(P_OUT, std::round((uu / 10 * 48 - 24) * 2) / 2); } }
  double knobDefault(int i) const { switch (i) { case 0: return 5; case 1: return 5; case 2: return 5; case 3: return 10 * std::log(VRPIV[2] / VRPIV[0]) / std::log(VRPIV[1] / VRPIV[0]); case 50: return 5; default: return 5; } }
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
  // Rasten (BASS, TILT in STEPS) springen beim Mausrad um eine Stellung, stufenlose Knöpfe um 0,2
  double wheelStep(int k) const { return k == 0 || (k == 1 && !smooth()) ? 10.0 / 6 : 0.2; }
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 1 : -1) * wheelStep(k)); return k >= 0; }
        if (button != 1) return false;
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) setKnob(k, knobDefault(k)); else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } lastClick = now; lastClickId = k; return true; }
        int b = hitBtn(x, y); if (b == 30) { if (smooth()) setP(P_TILT, std::round(getP(P_TILT))); setP(P_MODE, smooth() ? 0 : 1); return true; }   // zurück auf STEPS: TILT auf die nächste Raste
        if (b >= 40) { clickBtn(b - 40); return true; }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 50 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); if (h < 0) h = hitBtn(x, y);
        bool changed = h != hover; hover = h; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() { if (peaks) peaks(pk); char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    sapp(b, sizeof b, n, "%d|%d|%d|%.0f|%.0f", drag, hover, W, pk[0] * 50, pk[1] * 50); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace vrgui
