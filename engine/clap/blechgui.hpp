// Oberfläche des Blech-140-Plugins (Cairo): oben die gezeichnete Frontplatte (engine/tools/blech_panel.py) mit Zeiger des Instruments NACHHALLZEIT
// (zeigt die eingebaute Platte und wandert beim Überblenden), Lampe MOTOR (neue Platte wird gerechnet), Wahlschalter BASS CUT und Knöpfen;
// darunter Modellstreifen (Nachhallzeiten je Band) und die Nachhallzeit über der Frequenz.
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
#include "blech_core.hpp"
#include "blechgeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "blechpanel_jpg.h"
#include "widgets.hpp"

namespace blechgui {
constexpr double PI = 3.14159265358979323846;
enum { P_DECAY, P_BASS, P_MIX, P_PRE, P_OUT, P_COUNT };   // wie im Plugin
constexpr int NK = 4;
static const int KPARAM[NK] = {P_DECAY, P_MIX, P_PRE, P_OUT};
constexpr double FW = BLW, FH = BLH, STRIPU = 70, PLOTU = 300;
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
// Knopfstellung 0 … 1 aus den Parameterwerten (DECAY s, MIX %, PRE-DELAY ms quadratisch, OUTPUT dB)
inline double knob01(int p, double v) { switch (p) { case P_DECAY: return (v - blech::T500LO) / (blech::T500HI - blech::T500LO); case P_MIX: return v / 100; case P_PRE: return std::sqrt(std::max(0.0, v) / 200); default: return (v + 24) / 48; } }
inline double fromKnob(int p, double u) { u = std::max(0.0, std::min(1.0, u)); switch (p) { case P_DECAY: return std::round((blech::T500LO + u * (blech::T500HI - blech::T500LO)) * 100) / 100; case P_MIX: return std::round(u * 1000) / 10; case P_PRE: return std::round(200 * u * u); default: return std::round((u * 48 - 24) * 2) / 2; } }

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double needle = 2, shown = 2; bool motor = false; blech::Plate plate;   // shown: Nachhallzeit der eingebauten Platte (vom Audio-Faden), needle: Zeiger mit Trägheit
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(BLECHPANEL_JPG, (int)BLECHPANEL_JPG_LEN, &iw, &ih, &nch, 3);
      img = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iw, ih); unsigned char *dst = cairo_image_surface_get_data(img); int stride = cairo_image_surface_get_stride(img);
      for (int y = 0; y < ih; y++) for (int x = 0; x < iw; x++) { unsigned char *s = px + (y * iw + x) * 3; unsigned char *t = dst + y * stride + x * 4; t[0] = s[2]; t[1] = s[1]; t[2] = s[0]; t[3] = 255; }
      cairo_surface_mark_dirty(img); stbi_image_free(px); } }
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
    xs = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, scr), w, h); loadImg(); return true; }
  void show() { if (win && xs) { XMapWindow(dpy, win); XFlush(dpy); } }
  void hide() { if (win) { XUnmapWindow(dpy, win); XFlush(dpy); } }
  void resize(int w, int h) { W = w; H = h; if (win && xs) { XResizeWindow(dpy, win, w, h); cairo_xlib_surface_set_size(xs, w, h); } }
  void destroy() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } if (base) { cairo_surface_destroy(base); base = nullptr; } if (win) { XDestroyWindow(dpy, win); win = 0; } }
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c); cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  static void setrgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
  int bass() const { return std::max(0, std::min(3, (int)std::lround(getP(P_BASS)))); }
  void knobText(int i, char *b, size_t n) const { int p = KPARAM[i]; double v = getP(p);
    if (p == P_DECAY) std::snprintf(b, n, "DECAY %.2f s at 500 Hz (damper plate; low %.1f s at 125 Hz, high %.2f s at 4 kHz)", v, blech::t60At(plate, 125, v), blech::t60At(plate, 4000, v));
    else if (p == P_MIX) std::snprintf(b, n, "MIX %.0f %% reverb (dry %.0f %%)", v, 100 - v);
    else if (p == P_PRE) std::snprintf(b, n, "PRE-DELAY %.0f ms (model, not on the unit)", v);
    else std::snprintf(b, n, "OUTPUT %+.1f dB", v); }
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef __APPLE__
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); });
#endif
    cairo_save(c); cairo_scale(c, s, s);
    // Zeiger des Instruments (eingebaute Platte); Sollwert als kleine rote Marke
    { const double gx = BLGAUGE[0], gy = BLGAUGE[1], gr = BLGAUGE[2], t0 = BLGAUGE[3], t1 = BLGAUGE[4], ga = BLGAUGE[5];
      auto ang = [&](double t) { t = std::max(t0 - 0.1, std::min(t1 + 0.1, t)); return (-ga + 2 * ga * (t - t0) / (t1 - t0)) * PI / 180; };
      double at = ang(getP(P_DECAY)); setrgb(c, 0xa82820); cairo_set_line_width(c, 3); cairo_move_to(c, gx + (gr + 2) * std::sin(at), gy - (gr + 2) * std::cos(at)); cairo_line_to(c, gx + (gr + 10) * std::sin(at), gy - (gr + 10) * std::cos(at)); cairo_stroke(c);
      double a = ang(needle); setrgb(c, 0x141414); cairo_set_line_width(c, 2.2); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, gx + 20 * std::sin(a), gy - 20 * std::cos(a)); cairo_line_to(c, gx + (gr - 2) * std::sin(a), gy - (gr - 2) * std::cos(a)); cairo_stroke(c);
      setrgb(c, 0x222222); cairo_new_path(c); cairo_arc(c, gx, gy, 7, 0, 2 * PI); cairo_fill(c); }
    // Lampe MOTOR
    { double x = BLLAMP[0], y = BLLAMP[1]; setrgb(c, motor ? 0xff8a3a : 0x60321e); cairo_new_path(c); cairo_arc(c, x, y, 6, 0, 2 * PI); cairo_fill(c);
      if (motor) { cairo_pattern_t *g = cairo_pattern_create_radial(x, y, 2, x, y, 18); cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.55, 0.23, 0.6); cairo_pattern_add_color_stop_rgba(g, 1, 1, 0.55, 0.23, 0); cairo_new_path(c); cairo_arc(c, x, y, 18, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g); } }
    // Wahlschalter BASS CUT
    { double x = BLBASS[0], y = BLBASS[1], a = (-45 + 30 * bass()) * PI / 180; cairo_save(c); cairo_translate(c, x, y); cairo_rotate(c, a); setrgb(c, 0xf0f0ea); cairo_set_line_width(c, 5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -6); cairo_line_to(c, 0, -22); cairo_stroke(c); cairo_restore(c);
      if (hover == 50) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, x, y, 32, 0, 2 * PI); cairo_stroke(c); } }
    for (int i = 0; i < NK; i++) { const double *g = BLKNOB[i]; cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, (-150 + 300 * knob01(KPARAM[i], getP(KPARAM[i]))) * PI / 180);
      setrgb(c, 0xf0f0ea); cairo_set_line_width(c, 4); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.15); cairo_line_to(c, 0, -g[2] * 0.62); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.12, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU + PLOTU); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the unit)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 230, 20, 12, false); tsw::prefDraw(c, "VALUE POPUP", tsw::prefs().tips, 420, 20);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { double t = shown; char b[260]; std::snprintf(b, sizeof b, "PLATE NOW  RT60 %.2f s at 125 Hz · %.2f s at 500 Hz · %.2f s at 2 kHz · %.2f s at 8 kHz%s", blech::t60At(plate, 125, t), t, blech::t60At(plate, 2000, t), blech::t60At(plate, 8000, t),
        motor ? "   ·   damper moving" : ""); setrgb(c, 0xdcdee2); tsw::text(c, b, 16, 50, 13.5, true); }
    // Nachhallzeit über der Frequenz (log/log): eingebaute Platte, gestrichelt der Sollwert
    { cairo_translate(c, 0, STRIPU); double px0 = 58, px1 = tsw::UW - 18, py0 = 30, py1 = PLOTU - 40;
      auto X = [&](double f) { return px0 + (px1 - px0) * std::log(f / 20) / std::log(1000.0); }; auto Y = [&](double t) { return py1 - (py1 - py0) * std::log(t / 0.1) / std::log(200.0); };
      setrgb(c, 0xc8ccd4); tsw::text(c, "REVERBERATION TIME RT60 OVER FREQUENCY  (plate model: thermoelastic + damper plate + high-frequency loss)", px0, 18, 13, true);
      cairo_set_line_width(c, 1); for (double t : {0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0}) { setrgb(c, t == 1.0 ? 0x5a5e66 : 0x2a2d33); cairo_move_to(c, px0, Y(t)); cairo_line_to(c, px1, Y(t)); cairo_stroke(c); char l[16]; std::snprintf(l, 16, t < 1 ? "%.1f" : "%.0f", t); setrgb(c, 0x9a9ea6); tsw::text(c, l, px0 - 8, Y(t) + 4, 12, false, 2); }
      const double fl[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000}; const char *fn[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
      for (int i = 0; i < 10; i++) { setrgb(c, 0x3a3e46); cairo_move_to(c, X(fl[i]), py0); cairo_line_to(c, X(fl[i]), py1); cairo_stroke(c); setrgb(c, 0x9a9ea6); tsw::text(c, fn[i], X(fl[i]), py1 + 17, 12, false, 1); }
      setrgb(c, 0x9a9ea6); tsw::text(c, "Hz", (px0 + px1) / 2, py1 + 34, 12, false, 1); cairo_save(c); cairo_translate(c, 18, (py0 + py1) / 2); cairo_rotate(c, -PI / 2); tsw::text(c, "s", 0, 4, 12, false, 1); cairo_restore(c);
      auto curve = [&](double t5, int col, bool dash) { setrgb(c, col); cairo_set_line_width(c, dash ? 2 : 3); if (dash) { const double d2[] = {8, 6}; cairo_set_dash(c, d2, 2, 0); } cairo_new_path(c);
        for (int i = 0; i < 200; i++) { double f = 20 * std::pow(1000.0, i / 199.0), y = std::max(py0, std::min(py1, Y(blech::t60At(plate, f, t5)))); if (i == 0) cairo_move_to(c, X(f), y); else cairo_line_to(c, X(f), y); } cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); };
      if (std::fabs(getP(P_DECAY) - shown) > 0.005) curve(getP(P_DECAY), 0x8d919a, true); curve(shown, 0x4da3ff, false);
      setrgb(c, 0x4da3ff); cairo_move_to(c, px1 - 330, py0 - 12); cairo_line_to(c, px1 - 300, py0 - 12); cairo_stroke(c); setrgb(c, 0xc8ccd4); tsw::text(c, "plate now", px1 - 292, py0 - 8, 12, false);
      setrgb(c, 0x8d919a); const double d2[] = {8, 6}; cairo_set_dash(c, d2, 2, 0); cairo_move_to(c, px1 - 200, py0 - 12); cairo_line_to(c, px1 - 170, py0 - 12); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); setrgb(c, 0xc8ccd4); tsw::text(c, "DECAY setting", px1 - 162, py0 - 8, 12, false); }
    cairo_restore(c);
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[200]; double ax, ay;
      if (i < NK) { const double *g = BLKNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.2) * s; knobText(i, buf, sizeof buf); }
      else { ax = BLBASS[0] * s; ay = (BLBASS[1] - 40) * s; const char *bn[4] = {"0 (off: only DC blocked, 20 Hz)", "1 (high-pass 100 Hz)", "2 (high-pass 200 Hz)", "3 (high-pass 400 Hz)"}; std::snprintf(buf, sizeof buf, "BASS CUT %s", bn[bass()]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { double dx = x - BLKNOB[i][0], dy = y - BLKNOB[i][1]; if (dx * dx + dy * dy <= BLKNOB[i][2] * BLKNOB[i][2] * 1.69) return i; }
    double dx = x - BLBASS[0], dy = y - BLBASS[1]; if (dx * dx + dy * dy <= 55 * 55) return 50; return -1; }
  void setKnob(int i, double u01) { setP(KPARAM[i], fromKnob(KPARAM[i], u01)); }
#ifndef __APPLE__
  bool handle(XEvent &ev) { switch (ev.type) { case Expose: return true; case ButtonPress: return onMouse(0, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case MotionNotify: return onMouse(1, ev.xmotion.x, ev.xmotion.y, 0, 0); case ButtonRelease: return onMouse(2, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case LeaveNotify: return onMouse(3, 0, 0, 0, 0); default: return false; } }
#endif
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k == 50) setP(P_BASS, std::max(0, std::min(3, bass() + (button == 4 ? 1 : -1)))); else if (k >= 0) setKnob(k, knob01(KPARAM[k], getP(KPARAM[k])) + (button == 4 ? 0.02 : -0.02)); return true; }
        if (button != 1) return false;
        { double plx = x / uscale(), ply = (y - FH * scale()) / uscale(); if (tsw::prefHit(plx, ply, 420, 20, 125)) { tsw::prefs().tips = !tsw::prefs().tips; tsw::prefSave(); return true; } }
        int k = hitKnob(x, y);
        if (k == 50) { double s = scale(), a = std::atan2(x / s - BLBASS[0], -(y / s - BLBASS[1])) * 180 / PI; int pos = std::max(0, std::min(3, (int)std::lround((a + 45) / 30))); if (std::hypot(x / s - BLBASS[0], y / s - BLBASS[1]) < 22) pos = (bass() + 1) % 4; setP(P_BASS, pos); return true; }
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) setP(KPARAM[k], k == 0 ? 2.0 : k == 1 ? 30.0 : 0.0); else { drag = k; dragY0 = y; dragV0 = knob01(KPARAM[k], getP(KPARAM[k])); if (gesture) gesture(KPARAM[k], true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        return false; }
      case 1: { if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) / 250 / scale()); return true; } int h = hitKnob(x, y); bool ch = h != hover; hover = h; showTip = h >= 0; return ch; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(KPARAM[drag], false); drag = -1; return true; } return false;
      case 3: { bool ch = hover >= 0; hover = -1; if (drag < 0) showTip = false; return ch; }
      default: return false; } }
  std::string lastSig;
  std::string signature() const { char b[400]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i)); sapp(b, sizeof b, n, "%d|%d|%d|%d|%.3f|%.3f|%d", drag, hover, showTip ? 1 : 0, W, needle, shown, motor ? 1 : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace blechgui
