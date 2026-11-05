// Oberfläche des AmpFett-SVT-CL-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die nachgezeichnete Frontplatte (engine/tools/svt_panel.py, kein Foto, kein Logo) mit Zeigern, Buchsen, Schalterhebeln und Anzeige,
// darunter der Modellstreifen (Überabtastung, Ausgang 4/2 Ω, Poti-Kennlinie, Aussteuerung, Instrument/Ausgang) und der Frequenzgang.
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
#include "svtgeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "svtpanel_jpg.h"
#include "widgets.hpp"

namespace sgui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_GAIN, P_BASS, P_MID, P_FREQ, P_TREB, P_MASTER, P_UHI, P_ULO, P_JACK, P_TAP, P_STBY, P_PWR, P_VIN, P_OS, P_OUT, P_TAPER, P_COUNT };
static const int KPARAM[6] = {P_GAIN, P_BASS, P_MID, P_FREQ, P_TREB, P_MASTER};   // Knopf (Reihenfolge SVTKNOB) → Parameter
static const char *KNAME[6] = {"GAIN", "BASS", "MIDRANGE", "FREQUENCY", "TREBLE", "MASTER"};
static const char *FNAME[5] = {"220 Hz", "450 Hz", "800 Hz", "1.6 kHz", "3 kHz"};
static const int SWPARAM[4] = {P_UHI, P_ULO, P_STBY, P_PWR};   // Frontplattenschalter (Reihenfolge SVTSW), Hebel oben = ein
constexpr double FW = SVTW, FH = SVTH;
constexpr double STRIPU = 150;  // Modellstreifen in UI-Einheiten (tsw::UW = 1200 breit)
constexpr double PLOTU = 300;   // Frequenzgang
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;   // Gesamthöhe in Frontplatten-Einheiten
constexpr double SW_Y = 88;
static const double SW_X[3] = {64, 186, 310};   // OVERSAMPLING, OUTPUT TAP (4 / 2 Ω), POT TAPER
static const int SW_P[3] = {P_OS, P_TAP, P_TAPER};
static const double KN_X[2] = {1010, 1120}; // INPUT, OUTPUT
constexpr double KN_Y = 72;
constexpr int NF = 120;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "all knobs at 5, 800 Hz, ultra off";
inline double potTaper(double x, int mode) { if (x <= 0) return 0; if (x >= 1) return 1; if (mode == 2) return x; const double m = mode == 1 ? 0.10 : 0.15; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // wie im Kern (ohne dessen Header, für mac/test/render.mm)

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double meter[6] = {0, 0, 0, 0, 0, 0}; bool meterOn = false;   // Aussteuerung V1B, V1A, V2B, Endstufen-V1A, 6550 (1 = Gitter erreicht 0 V), Leistung (W an der Last)
  int osActive = 0;   // Überabtastung, mit der der Kern gerade rechnet (Umschalten dauert einen Augenblick)
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(SVTPANEL_JPG, (int)SVTPANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  static double knobAng(int i, double v) { if (KPARAM[i] == P_FREQ) { int k = std::max(0, std::min(4, (int)std::lround(v))); return SVTFPOS[k] * PI / 180; } return ang(v); }
  void knobText(int i, double v, char *buf, size_t n) const { int p = KPARAM[i];
    if (p == P_FREQ) { std::snprintf(buf, n, "MIDRANGE FREQUENCY  %s", FNAME[std::max(0, std::min(4, (int)std::lround(v)))]); return; }
    if (p == P_GAIN || p == P_BASS || p == P_TREB) { int tp = std::max(0, std::min(2, (int)std::lround(getP(P_TAPER)))); std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, %s)", KNAME[i], v, potTaper(v / 10, tp) * 100, tp == 2 ? "linear" : tp == 1 ? "log 10 %" : "log 15 %"); }
    else std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, linear)", KNAME[i], v, v * 10); }
  bool swUp(int k, const double *v) const { return v[SWPARAM[k]] > 0.5; }
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    int jack = std::max(0, std::min(1, (int)std::lround(v[P_JACK])));
    setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef __APPLE__
    // Foto einmal je Fenstergröße vorskaliert (ab Laney 3.31 / Yamaha 1.12 / AmpFett 1.12; vorher je Bild neu skaliert)
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
    cairo_save(c); cairo_scale(c, s, s);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); });   // ab 1.14 vorgezeichnet (Gerätepixel)
    cairo_save(c); cairo_scale(c, s, s);
#endif
    // Buchse mit Stecker: roter Ring und Stecker
    for (int i = 0; i < 2; i++) { if (i != jack) continue; const double *J = SVTJACK[i];
      setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] + 4, 0, 2 * PI); cairo_stroke(c);
      setrgb(c, 0x202020); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x8a8a8a); cairo_set_line_width(c, 1.5); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_stroke(c); }
    // Zeiger (weißer Strich auf der schwarzen Kappe)
    for (int i = 0; i < 6; i++) { const double *g = SVTKNOB[i];   // Zeiger: dunkler Strich auf der Metallkappe, weiße Spitze auf dem Riffelrand
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, knobAng(i, v[KPARAM[i]])); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
      setrgb(c, 0x202124); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -g[2] * 0.12); cairo_line_to(c, 0, -g[2] * 0.62); cairo_stroke(c);
      setrgb(c, 0xf2f2f2); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -g[2] * 0.72); cairo_line_to(c, 0, -g[2] * 0.95); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.08, 0, 2 * PI); cairo_stroke(c); } }
    // Anzeige STANDBY/POWER/FAULT: grün = Betrieb, rot = Standby, dunkel = aus
    { bool on = v[P_PWR] > 0.5, sb = v[P_STBY] > 0.5; if (on) { int col = sb ? 0xff3a28 : 0x3ee25a; setrgb(c, col); cairo_new_path(c); cairo_arc(c, SVTLED[0], SVTLED[1], 9, 0, 2 * PI); cairo_fill(c);
        cairo_pattern_t *gl = cairo_pattern_create_radial(SVTLED[0], SVTLED[1], 4, SVTLED[0], SVTLED[1], 24); double r = ((col >> 16) & 255) / 255.0, gg = ((col >> 8) & 255) / 255.0, b = (col & 255) / 255.0;
        cairo_pattern_add_color_stop_rgba(gl, 0, r, gg, b, 0.5); cairo_pattern_add_color_stop_rgba(gl, 1, r, gg, b, 0); cairo_new_path(c); cairo_arc(c, SVTLED[0], SVTLED[1], 24, 0, 2 * PI); cairo_set_source(c, gl); cairo_fill(c); cairo_pattern_destroy(gl); } }
    // Schalterhebel: von der Mutter schräg nach oben bzw. unten
    for (int k = 0; k < 4; k++) { double x = SVTSW[k][0], y = SVTSW[k][1], d = swUp(k, v) ? -1 : 1;
      cairo_new_path(c); cairo_move_to(c, x - 6, y); cairo_line_to(c, x - 8, y + d * 40); cairo_curve_to(c, x - 8 + 16.0 / 3, y + d * (40 + 16.0 / 3), x + 8 - 16.0 / 3, y + d * (40 + 16.0 / 3), x + 8, y + d * 40); cairo_line_to(c, x + 6, y); cairo_close_path(c);
      setrgb(c, 0xd8dadf); cairo_fill_preserve(c); setrgb(c, 0x4a4d52); cairo_set_line_width(c, 2); cairo_stroke(c);
      if (hover == 20 + k) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, x, y, 26, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); int os = std::max(0, std::min(2, (int)std::lround(v[P_OS])));
    char vt[2][40]; std::snprintf(vt[0], 40, "%.0f mV at 0 dBFS", v[P_VIN]); std::snprintf(vt[1], 40, "%+.1f dB", v[P_OUT]); const double rl = v[P_TAP] > 0.5 ? 2 : 4;
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *ol[3] = {"1x", "2x", "4x"}, *tt[2] = {"4 OHM", "2 OHM"};
      tsw::toggle(c, SW_X[0], SW_Y, 3, os, "OVERSAMPLING", ol, os != osActive ? "switching ..." : "against aliasing");
      tsw::toggle(c, SW_X[1], SW_Y, 2, v[P_TAP] > 0.5 ? 1 : 0, "OUTPUT", tt, "tap = load");
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; int tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER]))); tsw::toggle(c, SW_X[2], SW_Y, 3, tp, "POT TAPER", tl, "gain, bass, treble"); }
    // Aussteuerung: Gitterhub je Stufe (1 = Gitter erreicht 0 V, Strich), Leistung an 8 Ω (Strich = 30 W)
    { const char *tn[6] = {"V1B input", "V1A gain", "V2B mid", "amp in V1A", "6550 grid", "power"};
      double lx = 460, bx = 570, bw = 170, bh = 13, y0 = 36, dy = 18; setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", lx, 30, 13, true);
      for (int i = 0; i < 6; i++) { double by = y0 + i * dy; setrgb(c, 0x9a9ea6); tsw::text(c, tn[i], lx, by + 11, 11.5, false);
        setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, bx + 0.5, by + 0.5, bw - 1, bh - 1); cairo_stroke(c);
        double x = meter[i], f, mark; bool over; char t[24];
        if (i < 5) { f = std::min(1.0, x / 1.5); mark = 1 / 1.5; over = x >= 1; std::snprintf(t, sizeof t, "%.0f %%", x * 100); }
        else { f = std::min(1.0, x / 300 / 1.25); mark = 1 / 1.25; over = x > 300; if (x < 0.05) std::snprintf(t, sizeof t, "-"); else std::snprintf(t, sizeof t, x < 10 ? "%.1f W" : "%.0f W", x); }
        if (meterOn) { setrgb(c, over ? 0xff3b2a : f > mark * 0.7 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, bx + 1, by + 1, (bw - 2) * f, bh - 2); cairo_fill(c); }
        setrgb(c, 0xffffff, 0.45); cairo_rectangle(c, bx + 1 + (bw - 2) * mark - 0.5, by, 1, bh); cairo_fill(c);
        setrgb(c, over && meterOn ? 0xffb3a8 : 0xcfd1d6); tsw::text(c, meterOn ? t : "-", bx + bw + 10, by + 11, 11.5, false); } (void)rl; }
    for (int i = 0; i < 2; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : (v[P_OUT] + 36) / 60;
      tsw::smallKnobV(c, KN_X[i], KN_Y, uu, i == 0 ? "INPUT" : "OUTPUT", vt[i], drag == 9 + i || hover == 9 + i, false); }
    // Frequenzgang (Kleinsignal im Arbeitspunkt, bezogen auf 1 kHz), höchstens alle 100 ms neu
    { char kb[256]; int kn = 0; const int ks[] = {P_GAIN, P_BASS, P_MID, P_FREQ, P_TREB, P_MASTER, P_UHI, P_ULO, P_JACK, P_TAP, P_TAPER};
      for (int id : ks) sapp(kb, sizeof kb, kn, "%.4g|", v[id]); std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, "FREQUENCY RESPONSE  (small signal, output tap, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, true); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[112]; double ax, ay;
      if (i < 6) { const double *g = SVTKNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, v[KPARAM[i]], buf, sizeof buf); }
      else if (i >= 20) { int k = i - 20; ax = SVTSW[k][0] * s; ay = (SVTSW[k][1] - 60) * s; const char *nm[4] = {"ULTRA HI", "ULTRA LO", "STANDBY", "POWER"};
        std::snprintf(buf, sizeof buf, "%s: %s%s", nm[k], v[SWPARAM[k]] > 0.5 ? "ON" : "OFF", k == 2 && v[P_STBY] > 0.5 ? " (muted)" : k == 3 && v[P_PWR] < 0.5 ? " (muted)" : ""); }
      else if (i >= 30) { int k = i - 30; ax = SVTJACK[k][0] * s; ay = (SVTJACK[k][1] - 40) * s; std::snprintf(buf, sizeof buf, "%s INPUT%s", k ? "-15 dB (active basses)" : "0 dB (passive basses)", k == jack ? "" : " - click to plug in"); }
      else { ax = KN_X[i - 9] * u; ay = FH * s + (KN_Y - 30) * u; std::snprintf(buf, sizeof buf, "%s", vt[i - 9]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < 6; i++) { double dx = x - SVTKNOB[i][0], dy = y - SVTKNOB[i][1]; if (dx*dx + dy*dy <= SVTKNOB[i][2] * SVTKNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); for (int i = 0; i < 2; i++) { double dx = lx - KN_X[i], dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 9 + i; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; for (int i = 0; i < 2; i++) { const double *J = SVTJACK[i]; double dx = x - J[0], dy = y - J[1]; if (dx*dx + dy*dy <= (J[2] + 14) * (J[2] + 14)) return i; } return -1; }
  int hitPanelSw(double x, double y) const { double s = scale(); x /= s; y /= s; for (int k = 0; k < 4; k++) if (std::fabs(x - SVTSW[k][0]) <= 32 && std::fabs(y - SVTSW[k][1]) <= 52) return k; return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); for (int k = 0; k < 3; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 30); if (half >= 0) return SW_P[k]; } return -1; }
  void moveSwitch(int p, int dir) { if (p == P_OS || p == P_TAPER) { int t = (int)std::lround(getP(p)); setP(p, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); } else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 0 : 1); }   // OUTPUT: oben = 4 Ω
  void togglePanel(int k) { int p = SWPARAM[k]; setP(p, getP(p) > 0.5 ? 0 : 1); }
  int knobParam(int i) const { return i < 6 ? KPARAM[i] : i == 9 ? P_VIN : P_OUT; }
  double knobVal(int i) const { if (i < 6) return KPARAM[i] == P_FREQ ? getP(P_FREQ) * 2.5 : getP(KPARAM[i]); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : (getP(P_OUT) + 36) / 60 * 10; }   // FREQUENCY: 0 … 4 auf 0 … 10 Ziehweg
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 6) { if (KPARAM[i] == P_FREQ) setP(P_FREQ, std::lround(u / 2.5)); else setP(KPARAM[i], std::round(u * 10) / 10); } else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
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
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else { int half, p = hitSwitch(x, y, half); if (p >= 0) moveSwitch(p, button == 4 ? -1 : 1); } return true; }
        if (button != 1) return false;
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 9 ? std::log(100.0) / std::log(1000.0) * 10 : k == 10 ? 6 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int j = hitJack(x, y); if (j >= 0) { setP(P_JACK, j); return true; }
        int ps = hitPanelSw(x, y); if (ps >= 0) { togglePanel(ps); return true; }
        { int half, b = hitSwitch(x, y, half); if (b >= 0) { moveSwitch(b, b == P_OS || b == P_TAPER ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 9 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); if (h < 0) { int ps = hitPanelSw(x, y); if (ps >= 0) h = 20 + ps; } if (h < 0) { int jj = hitJack(x, y); if (jj >= 0) h = 30 + jj; }
        bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    for (int i = 0; i < 6; i++) sapp(b, sizeof b, n, "%d|", (int)std::lround(std::log10(1 + std::min(std::max(meter[i], 0.0), 999.0)) * 60));   /* nur sichtbare Änderungen neu zeichnen */
    sapp(b, sizeof b, n, "%d|%d|%d|%d|%d|%d|%d", meterOn ? 1 : 0, osActive, drag, hover, showTip ? 1 : 0, W, curveStale ? (int)(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() * 10) : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace sgui
