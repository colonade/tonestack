// Oberfläche des Wien-EQ-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die gezeichnete Frontplatte (engine/tools/weq_panel.py, eigene Gestaltung, kein Foto, kein Logo) mit Zeigern, gedrückten Tasten und der Anzeige EQ ON,
// darunter der Modellstreifen (Regelbereich der Ausführung ±6/±12/±20 dB, Ausgangspegel) und der Frequenzgang.
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
#include "weqgeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "weqpanel_jpg.h"
#include "widgets.hpp"

namespace weqgui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_LOWG, P_LOWF, P_M1G, P_M1Q, P_M1F, P_M2G, P_M2Q, P_M2F, P_M3G, P_M3Q, P_M3F, P_HIG, P_HIF, P_ON, P_RANGE, P_OUT, P_COUNT };
constexpr int NK = 11;
static const int KPARAM[NK] = {P_LOWG, P_M1G, P_M1Q, P_M1F, P_M2G, P_M2Q, P_M2F, P_M3G, P_M3Q, P_M3F, P_HIG};
static const char *KNAME[NK] = {"LOW GAIN", "MID 1 GAIN", "MID 1 Q", "MID 1 FREQ", "MID 2 GAIN", "MID 2 Q", "MID 2 FREQ", "MID 3 GAIN", "MID 3 Q", "MID 3 FREQ", "HIGH GAIN"};
inline bool isGain(int p) { return p == P_LOWG || p == P_M1G || p == P_M2G || p == P_M3G || p == P_HIG; }   // −5 … +5
constexpr double FW = WEQW, FH = WEQH;
constexpr double STRIPU = 150;
constexpr double PLOTU = 300;
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
constexpr double SW_Y = 88;
constexpr double SW_X = 70;      // RANGE
constexpr double KN_X = 1130, KN_Y = 72;   // OUTPUT
constexpr int NF = 160;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "flat (EQ off)";
// Werte wie im Kern (ohne dessen Header, für mac/test/render.mm)
inline double gainDb(double p, int range) { const double R[3] = {6, 12, 20}; p = std::max(-1.0, std::min(1.0, p)); double a = std::fabs(p); return (p < 0 ? -1 : 1) * R[std::max(0, std::min(2, range))] * (a + a * a) / 2; }
inline double midFreq(int k, double x) { const double lo[3] = {20, 60, 250}, hi[3] = {2000, 7000, 25000}; x = std::max(0.0, std::min(1.0, x)); return lo[k] * std::pow(hi[k] / lo[k], x); }
inline double midQ(double x) { x = std::max(0.0, std::min(1.0, x)); return 0.1824791693 * std::pow(4.318473046 / 0.1824791693, x); }   // 5 Oktaven … Terz (wie weq::midQ; bis 1.0 stand hier noch 1/3)
inline double qOct(double q) { return 2 * std::asinh(1 / (2 * q)) / std::log(2.0); }   // Bandbreite des Bandpasses in Oktaven (−3 dB)

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Knöpfe, 40 … 44 Tasten (LOW 60, LOW 100, HIGH 12k, HIGH 8k, EQ ON), 51 OUTPUT (Modellstreifen)
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(WEQPANEL_JPG, (int)WEQPANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  double knobPos(int i) const { int p = KPARAM[i]; double v = getP(p); return isGain(p) ? v + 5 : v; }
  // Taste gedrückt? (LOW: Bit 0 = 60, Bit 1 = 100; HIGH: Bit 0 = 12k, Bit 1 = 8k)
  bool btnOn(int b) const { if (b == 4) return getP(P_ON) > 0.5; int sel = (int)std::lround(getP(b < 2 ? P_LOWF : P_HIF)); return (sel >> (b & 1)) & 1; }
  void clickBtn(int b) { if (b == 4) { setP(P_ON, getP(P_ON) > 0.5 ? 0 : 1); return; } int p = b < 2 ? P_LOWF : P_HIF, sel = (int)std::lround(getP(p)); setP(p, sel ^ (1 << (b & 1))); }
  int range() const { return std::max(0, std::min(2, (int)std::lround(getP(P_RANGE)))); }
  void knobText(int i, char *buf, size_t n) const { int p = KPARAM[i]; double v = getP(p);
    if (isGain(p)) std::snprintf(buf, n, "%s  %+.1f dB  (knob %+.2f, range +/-%d dB)", KNAME[i], gainDb(v / 5, range()), v, range() == 0 ? 6 : range() == 1 ? 12 : 20);
    else if (p == P_M1F || p == P_M2F || p == P_M3F) { double f = midFreq((p - P_M1F) / 3, v / 10); std::snprintf(buf, n, f >= 1000 ? "%s  %.2f kHz" : "%s  %.0f Hz", KNAME[i], f >= 1000 ? f / 1000 : f); }
    else { double q = midQ(v / 10); std::snprintf(buf, n, "%s  %.2f  (bandwidth %.2f octaves; left = broad, right = narrow)", KNAME[i], q, qOct(q)); } }
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
    // Anzeige EQ ON (grün)
    { bool on = v[P_ON] > 0.5; double x = WEQLED[0], y = WEQLED[1]; setrgb(c, on ? 0x5cff6a : 0x284628); cairo_new_path(c); cairo_arc(c, x, y, 7, 0, 2 * PI); cairo_fill(c);
      if (on) { cairo_pattern_t *g = cairo_pattern_create_radial(x, y, 3, x, y, 20); cairo_pattern_add_color_stop_rgba(g, 0, 0.36, 1.0, 0.42, 0.5); cairo_pattern_add_color_stop_rgba(g, 1, 0.36, 1.0, 0.42, 0); cairo_new_path(c); cairo_arc(c, x, y, 20, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g); } }
    // gedrückte Tasten: dunkler, etwas tiefer, mit gelber Kante
    for (int b = 0; b < 5; b++) { const double *B = WEQBTN[b]; double x0 = B[0] - B[2] / 2, y0 = B[1] - B[3] / 2;
      if (btnOn(b)) { setrgb(c, 0x2c3038); tsw::rrect(c, x0, y0, B[2], B[3], 4); cairo_fill(c); setrgb(c, 0xa4aab4); tsw::rrect(c, x0 + 3, y0 + 5, B[2] - 6, B[3] - 7, 3); cairo_fill(c);
        setrgb(c, 0xf2c14e); cairo_set_line_width(c, 2.5); tsw::rrect(c, x0 + 1, y0 + 1, B[2] - 2, B[3] - 2, 4); cairo_stroke(c); }
      if (hover == 40 + b) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2); tsw::rrect(c, x0 - 4, y0 - 4, B[2] + 8, B[3] + 8, 6); cairo_stroke(c); } }
    // Knöpfe: Zeiger auf der farbigen Kappe
    for (int i = 0; i < NK; i++) { const double *g = WEQKNOB[i];
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, ang(knobPos(i))); setrgb(c, 0x16181c); cairo_set_line_width(c, 3.5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.08); cairo_line_to(c, 0, -g[2] * 0.66); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.1, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); char vo[40]; std::snprintf(vo, 40, "%+.1f dB", v[P_OUT]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the unit)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 230, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *rl[3] = {"+/-6 dB", "+/-12 dB", "+/-20 dB"}; tsw::toggle(c, SW_X, SW_Y, 3, range(), "RANGE", rl, "TM205 versions"); }
    // Bänder als Text: Frequenz, Güte, dB
    { setrgb(c, 0xdcdee2); tsw::text(c, "BANDS", 250, 30, 13, true); const char *bn[5] = {"LOW", "MID 1", "MID 2", "MID 3", "HIGH"}; const double lf[4] = {40, 60, 100, 160}; const char *hf[4] = {"shelf (20 kHz)", "12 kHz", "8 kHz", "4 kHz"};
      for (int b = 0; b < 5; b++) { double y = 50 + b * 18; char t[96]; double db = gainDb(v[b == 0 ? P_LOWG : b == 4 ? P_HIG : P_M1G + 3 * (b - 1)] / 5, range());
        if (b == 0) std::snprintf(t, sizeof t, "%.0f Hz, Q 1/3", lf[std::max(0, std::min(3, (int)std::lround(v[P_LOWF])))]);
        else if (b == 4) std::snprintf(t, sizeof t, "%s, Q 1/3", hf[std::max(0, std::min(3, (int)std::lround(v[P_HIF])))]);
        else { double f = midFreq(b - 1, v[P_M1F + 3 * (b - 1)] / 10), q = midQ(v[P_M1Q + 3 * (b - 1)] / 10); std::snprintf(t, sizeof t, f >= 1000 ? "%.2f kHz, Q %.2f (%.1f oct)" : "%.0f Hz, Q %.2f (%.1f oct)", f >= 1000 ? f / 1000 : f, q, qOct(q)); }
        setrgb(c, 0x9a9ea6); tsw::text(c, bn[b], 250, y + 11, 11.5, true); setrgb(c, v[P_ON] > 0.5 ? 0xcfd1d6 : 0x6c7079); tsw::text(c, t, 310, y + 11, 11.5, false);
        char g[24]; std::snprintf(g, sizeof g, "%+.1f dB", db); tsw::text(c, g, 560, y + 11, 11.5, true, 2); } }
    tsw::smallKnobV(c, KN_X, KN_Y, (v[P_OUT] + 24) / 48, "OUTPUT", vo, drag == 51 || hover == 51, false);
    // Frequenzgang (analog gerechnet, wie der Kern)
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_OUT) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); if (computeCurves && key != curveKey) { computeCurves(curve, refCurve); curveKey = key; } }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, v[P_ON] > 0.5 ? "FREQUENCY RESPONSE  (all bands)" : "FREQUENCY RESPONSE  (EQ off: flat)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, false); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[160]; double ax, ay;
      if (i < NK) { const double *g = WEQKNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, buf, sizeof buf); }
      else if (i >= 40 && i < 45) { const double *B = WEQBTN[i - 40]; ax = B[0] * s; ay = (B[1] - B[3]) * s; const char *nm[5] = {"LOW 60 Hz", "LOW 100 Hz", "HIGH 12 kHz", "HIGH 8 kHz", "EQ ON"};
        std::snprintf(buf, sizeof buf, i == 44 ? "%s: %s (click: EQ in / out, like the relay bypass)" : "%s: %s (both pressed: %s, none: %s)", nm[i - 40], btnOn(i - 40) ? "pressed" : "released", i < 42 ? "160 Hz" : "4 kHz", i < 42 ? "40 Hz" : "shelf"); }
      else { ax = KN_X * u; ay = FH * s + (KN_Y - 30) * u; std::snprintf(buf, sizeof buf, "OUTPUT %s (model, not on the unit)", vo); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { double dx = x - WEQKNOB[i][0], dy = y - WEQKNOB[i][1]; if (dx*dx + dy*dy <= WEQKNOB[i][2] * WEQKNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); { double dx = lx - KN_X, dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 51; } return -1; }
  int hitBtn(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int b = 0; b < 5; b++) { const double *B = WEQBTN[b]; if (std::fabs(x - B[0]) <= B[2] / 2 + 3 && std::fabs(y - B[1]) <= B[3] / 2 + 3) return 40 + b; }
    { double dx = x - WEQLED[0], dy = y - WEQLED[1]; if (dx*dx + dy*dy <= 14 * 14) return 44; } return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); half = tsw::toggleHit(lx, ly, SW_X, SW_Y, 70); return half >= 0 ? P_RANGE : -1; }
  void moveSwitch(int dir) { int t = range(); setP(P_RANGE, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); }
  int knobParam(int i) const { return i < NK ? KPARAM[i] : P_OUT; }
  double knobVal(int i) const { if (i < NK) return knobPos(i); return (getP(P_OUT) + 24) / 48 * 10; }
  // Gain: Rastung bei 0 (Stellungen innerhalb ±0,15 schnappen auf 0, wie die Rastung am Gerät)
  void setKnob(int i, double uu) { uu = std::max(0.0, std::min(10.0, uu)); if (i < NK) { int p = KPARAM[i]; double r = std::round(uu * 100) / 100; if (isGain(p)) { r -= 5; if (std::fabs(r) < 0.15) r = 0; } setP(p, r); }
    else setP(P_OUT, std::round((uu / 10 * 48 - 24) * 2) / 2); }
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
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else { int half, p = hitSwitch(x, y, half); if (p >= 0) moveSwitch(button == 4 ? -1 : 1); } return true; }
        if (button != 1) return false;
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) setKnob(k, k == 51 ? 5 : isGain(KPARAM[k]) ? 5 : 5); else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int b = hitBtn(x, y); if (b >= 0) { clickBtn(b - 40); return true; }
        { int half, p = hitSwitch(x, y, half); if (p >= 0) { moveSwitch(half == 0 ? -1 : 1); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 50 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
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
} // namespace weqgui
