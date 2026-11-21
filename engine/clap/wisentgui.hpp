// Oberfläche des Wisent Echoreck (Cairo): oben die gezeichnete Frontplatte (engine/tools/wisent_panel.py) mit magischem Auge (Aufnahmepegel),
// Knöpfen, Tasten ECHO/REPEAT/SWELL, zwei Reihen Kopftasten (oben Rückkopplung, unten Wiedergabe) und dem Schalter 50/60 Hz; darunter ein Streifen
// mit Betriebsart, Echozeiten und Aufnahmepegel. Ab 1.4 im Modellteil der Frontplatte vier Knöpfe HEAD 1–4 (Lage der Wiedergabeköpfe), ab 1.5 Kippschalter LINK:
// dann drehen alle vier im Verhältnis mit (Rhythmus bleibt), begrenzt, sobald ein Kopf an 20 oder 770 ms stößt; Doppelklick stellt alle auf das Original.
#pragma once
#ifndef TS_VERSION
#define TS_VERSION "?"
#endif
#include "platform.hpp"   // TS_OWNVIEW: eigene Ansicht (macOS, Windows)
#if defined(__APPLE__)
#include "cairo_cg.hpp"
#include "macview.h"
#elif defined(_WIN32)
#include "cairo_win.hpp"   // echtes Cairo, Ansicht aus winview.cpp
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
#include "wisentgeo_gen.h"
#include "wisent_core.hpp"   // loopThreshold
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "wisentpanel_jpg.h"
#include "widgets.hpp"

namespace wisentgui {
constexpr double PI = 3.14159265358979323846;
enum { P_INPUT, P_SWELL, P_VOLUME, P_BASS, P_TREBLE, P_MODE, P_OUT1, P_FB1 = P_OUT1 + 4, P_LEVEL = P_FB1 + 4, P_OUTDB, P_MAINS, P_DRY, P_SPEED, P_WOB, P_H1, P_H2, P_H3, P_H4, P_LINK, P_COUNT };   // wie im Plugin
constexpr int NK = 14;
static const int KPARAM[NK] = {P_INPUT, P_SWELL, P_VOLUME, P_BASS, P_TREBLE, P_LEVEL, P_OUTDB, P_DRY, P_SPEED, P_WOB, P_H1, P_H2, P_H3, P_H4};   // ab 1.4: HEAD 1–4
constexpr double FW = WSW, FH = WSH, STRIPU = 92;
static const double TOTALH = FH + STRIPU * FW / tsw::UW;
inline double knob01(int p, double v) { if (p >= P_H1 && p <= P_H4) return (v - wisent::Core::HEAD_MIN) / (wisent::Core::HEAD_MAX - wisent::Core::HEAD_MIN); switch (p) { case P_LEVEL: return std::log(std::max(20.0, v) / 20) / std::log(100.0); case P_OUTDB: return (v + 24) / 48; case P_SPEED: return std::log(std::max(0.25, v) / 0.25) / std::log(8.0); default: return v / 10; } }
inline double fromKnob(int p, double u) { u = std::max(0.0, std::min(1.0, u)); if (p >= P_H1 && p <= P_H4) return std::round((wisent::Core::HEAD_MIN + u * (wisent::Core::HEAD_MAX - wisent::Core::HEAD_MIN)) * 2) / 2; switch (p) { case P_LEVEL: return std::round(20 * std::pow(100.0, u)); case P_OUTDB: return std::round((u * 48 - 24) * 2) / 2; case P_SPEED: return std::round(25 * std::pow(8.0, u)) / 100; default: return std::round(u * 100) / 10; } }
inline double knobDefault(int p) { if (p >= P_H1 && p <= P_H4) return wisent::Core::HEAD_MS0 * (p - P_H1 + 1); switch (p) { case P_INPUT: return 2.5; case P_SWELL: return 5; case P_VOLUME: return 10; case P_LEVEL: return 200; case P_OUTDB: return 0; case P_DRY: return 6.7; case P_SPEED: return 1; case P_WOB: return 0; default: return 5; } }
static const char *MODEN[3] = {"ECHO", "REPEAT", "SWELL"};

#ifdef TS_OWNVIEW
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double eye = 0;   // Aufnahmepegel je Sättigung (vom Audio-Faden, geglättet)
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0, dragB[4] = {}; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(WISENTPANEL_JPG, (int)WISENTPANEL_JPG_LEN, &iw, &ih, &nch, 3);
      img = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iw, ih); unsigned char *dst = cairo_image_surface_get_data(img); int stride = cairo_image_surface_get_stride(img);
      for (int y = 0; y < ih; y++) for (int x = 0; x < iw; x++) { unsigned char *s = px + (y * iw + x) * 3; unsigned char *t = dst + y * stride + x * 4; t[0] = s[2]; t[1] = s[1]; t[2] = s[0]; t[3] = 255; }
      cairo_surface_mark_dirty(img); stbi_image_free(px); } }
#ifdef TS_OWNVIEW
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
  int mode() const { return std::max(0, std::min(2, (int)std::lround(getP(P_MODE)))); }
  bool on(int p) const { return getP(p) > 0.5; }
  bool hz60() const { return getP(P_MAINS) > 0.5; }
  bool linked() const { return getP(P_LINK) > 0.5; }
  double headMs(int i) const { return getP(P_H1 + i) / rel(); }   // Echozeit je Kopf bei der aktuellen Drehzahl (ab 1.4: Kopfstellung)
  double rel() const { return (hz60() ? 1.2 : 1.0) * std::max(0.25, getP(P_SPEED)); }   // Bandgeschwindigkeit gegen 50 Hz ohne Varispeed
  void knobText(int i, char *b, size_t n) const { int p = KPARAM[i]; double v = getP(p);
    switch (p) { case P_INPUT: std::snprintf(b, n, "INPUT %.1f (record level, log)", v); break; case P_SWELL: { double t = wisent::loopThreshold(rel()); if (t > 1) std::snprintf(b, n, "LENGTH OF SWELL %.1f (feedback; at this speed the loop does not run away)", v); else std::snprintf(b, n, "LENGTH OF SWELL %.1f (feedback; one feedback head runs away from about %.1f)", v, 10 * t); break; }
      case P_VOLUME: std::snprintf(b, n, "VOLUME %.1f (echo level, log)", v); break; case P_BASS: std::snprintf(b, n, "BASS %.1f (5 = middle)", v); break; case P_TREBLE: std::snprintf(b, n, "TREBLE %.1f (5 = middle)", v); break;
      case P_LEVEL: std::snprintf(b, n, "LEVEL: 0 dBFS = %.0f mV peak at the input", v); break; case P_DRY: std::snprintf(b, n, "DRY %.1f: trimmer TD inside the unit, dry level (plan setting 6.7)", v); break;
      case P_SPEED: std::snprintf(b, n, "VARISPEED %.2fx: drum speed, a later modification (PE 603 T: fixed); heads at %.0f / %.0f / %.0f / %.0f ms", v, headMs(0), headMs(1), headMs(2), headMs(3)); break;
      case P_WOB: std::snprintf(b, n, "WOBBLE %.1f: worn drum, spacing +%.0f um and speed +/-%.1f %% once per turn", v, v * 3, v / 10); break;
      default: std::snprintf(b, n, "OUTPUT %+.1f dB", v); } }
  // Tasten: 0–2 Betriebsart, 10–13 Rückkopplung, 20–23 Wiedergabe, 30 Netzfrequenz
  int hitButton(double x, double y) const {
    for (int i = 0; i < 3; i++) if (std::fabs(x - WSMODE[i][0]) <= WSBTN[0] / 2 + 3 && std::fabs(y - WSMODE[i][1]) <= WSBTN[1] / 2 + 3) return i;
    for (int i = 0; i < 4; i++) { if (std::fabs(x - WSHEADX[i]) <= WSHBTN[0] / 2 + 3) { if (std::fabs(y - WSHFB) <= WSHBTN[1] / 2 + 3) return 10 + i; if (std::fabs(y - WSHOUT) <= WSHBTN[1] / 2 + 3) return 20 + i; } }
    if (std::hypot(x - WSMAINS[0], y - WSMAINS[1]) < 40) return 30; if (std::hypot(x - WSLINK[0], y - WSLINK[1]) < 40) return 31; return -1; }
  void key(cairo_t *c, double x, double y, double w, double h, bool down, const char *t, double px) {
    cairo_save(c); double dy = down ? 3 : 0; setrgb(c, down ? 0xb8ad8e : 0xe8dfc4); tsw::rrect(c, x - w / 2, y - h / 2 + dy, w, h - 3, 4); cairo_fill(c);
    setrgb(c, down ? 0x8c8268 : 0xb4aa8c); cairo_rectangle(c, x - w / 2 + 3, y + h / 2 - 6 + dy, w - 6, 3); cairo_fill(c);
    setrgb(c, 0x262826); tsw::text(c, t, x, y + 5 + dy, px, true, 1);
    if (down) { setrgb(c, 0xffb030); cairo_new_path(c); cairo_arc(c, x + w / 2 - 8, y - h / 2 + 8 + dy, 3, 0, 2 * PI); cairo_fill(c); }
    cairo_restore(c); }
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef TS_OWNVIEW
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); });
#endif
    cairo_save(c); cairo_scale(c, s, s);
    // magisches Auge (EM84): zwei Leuchtbänder wachsen mit dem Aufnahmepegel aufeinander zu; bei Sättigung (1) treffen sie sich
    { const double ex = WSEYE[0], ey = WSEYE[1], ew = WSEYE[2], eh = WSEYE[3], f = std::max(0.0, std::min(1.0, std::sqrt(std::max(0.0, eye)))), bw = 0.12 + 0.38 * f;
      cairo_pattern_t *g = cairo_pattern_create_linear(0, ey - eh / 2, 0, ey + eh / 2); cairo_pattern_add_color_stop_rgba(g, 0, 0.25, 0.85, 0.45, 0.55); cairo_pattern_add_color_stop_rgba(g, 0.5, 0.45, 1.0, 0.6, 0.95); cairo_pattern_add_color_stop_rgba(g, 1, 0.25, 0.85, 0.45, 0.55);
      cairo_set_source(c, g); cairo_rectangle(c, ex - ew / 2, ey - eh / 2 + 3, ew * bw, eh - 6); cairo_fill(c); cairo_rectangle(c, ex + ew / 2 - ew * bw, ey - eh / 2 + 3, ew * bw, eh - 6); cairo_fill(c); cairo_pattern_destroy(g);
      if (eye > 1.05) { setrgb(c, 0x80ffb0, std::min(0.6, (eye - 1) * 0.6)); cairo_rectangle(c, ex - 6, ey - eh / 2 + 3, 12, eh - 6); cairo_fill(c); } }
    // Tasten
    for (int i = 0; i < 3; i++) key(c, WSMODE[i][0], WSMODE[i][1], WSBTN[0], WSBTN[1], mode() == i, MODEN[i], 12.5);
    { setrgb(c, 0xaaa898); for (int i = 0; i < 4; i++) { char t[16]; std::snprintf(t, sizeof t, "%.0f ms", headMs(i)); tsw::text(c, t, WSHEADX[i], 66, 10, false, 1); } }   // Echozeiten je Netzfrequenz
    for (int i = 0; i < 4; i++) { char t[4]; std::snprintf(t, 4, "%d", i + 1); key(c, WSHEADX[i], WSHFB, WSHBTN[0], WSHBTN[1], on(P_FB1 + i), t, 14); key(c, WSHEADX[i], WSHOUT, WSHBTN[0], WSHBTN[1], on(P_OUT1 + i), t, 14); }
    { int hb = hover; if (hb >= 0 && hb < 30 && (hb < 3 || hb >= 10)) { double x, y, w, h;
        if (hb < 3) { x = WSMODE[hb][0]; y = WSMODE[hb][1]; w = WSBTN[0]; h = WSBTN[1]; } else { x = WSHEADX[hb % 10]; y = hb < 20 ? WSHFB : WSHOUT; w = WSHBTN[0]; h = WSHBTN[1]; }
        setrgb(c, 0xd09a10); cairo_set_line_width(c, 2.5); tsw::rrect(c, x - w / 2 - 4, y - h / 2 - 4, w + 8, h + 8, 6); cairo_stroke(c); } }
    // Kippschalter 50/60 Hz
    { const double mx = WSMAINS[0], my = WSMAINS[1]; double a = hz60() ? -1 : 1; setrgb(c, 0xd8d8cc); cairo_set_line_width(c, 9); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, mx, my); cairo_line_to(c, mx, my + a * 30); cairo_stroke(c);
      setrgb(c, 0xf2f2ea); cairo_new_path(c); cairo_arc(c, mx, my + a * 30, 7, 0, 2 * PI); cairo_fill(c);
      if (hover == 30) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2.5); cairo_new_path(c); cairo_arc(c, mx, my, 38, 0, 2 * PI); cairo_stroke(c); } }
    // Kippschalter LINK (ab 1.5): oben = Kopfknöpfe gekoppelt
    { const double mx = WSLINK[0], my = WSLINK[1]; double a = linked() ? -1 : 1; setrgb(c, 0xd8d8cc); cairo_set_line_width(c, 9); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, mx, my); cairo_line_to(c, mx, my + a * 30); cairo_stroke(c);
      setrgb(c, 0xf2f2ea); cairo_new_path(c); cairo_arc(c, mx, my + a * 30, 7, 0, 2 * PI); cairo_fill(c);
      if (hover == 31) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 2.5); cairo_new_path(c); cairo_arc(c, mx, my, 38, 0, 2 * PI); cairo_stroke(c); } }
    for (int i = 0; i < NK; i++) { const double *g = WSKNOB[i]; cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, (-150 + 300 * knob01(KPARAM[i], getP(KPARAM[i]))) * PI / 180);
      setrgb(c, 0x2a2622); cairo_set_line_width(c, 4); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.15); cairo_line_to(c, 0, -g[2] * 0.68); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover - 100) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.15, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c);
    setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 16, 20, 12, false);   /* ohne „MODEL (not on the …)“ (Wunsch des Nutzers, 19.11.2026) */
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { char hb[64] = "", fb[64] = ""; int nh = 0, nf = 0;
      for (int i = 0; i < 4; i++) { if (on(P_OUT1 + i)) sapp(hb, sizeof hb, nh, "%s%.0f", nh ? " + " : "", headMs(i)); if (on(P_FB1 + i)) sapp(fb, sizeof fb, nf, "%s%d", nf ? "+" : "", i + 1); }
      char b[300]; int m = mode();
      if (m == 2) std::snprintf(b, sizeof b, "SWELL  all four heads to the output (%.0f + %.0f + %.0f + %.0f ms), feedback heads %s", headMs(0), headMs(1), headMs(2), headMs(3), nf ? fb : "none (no repeats)");
      else std::snprintf(b, sizeof b, "%s  playback %s%s, feedback %s", MODEN[m], nh ? hb : "none (no echo)", nh ? " ms" : "", m == 0 ? "off (single echoes)" : nf ? fb : "none (single echoes)");
      setrgb(c, 0xdcdee2); tsw::text(c, b, 16, 50, 13.5, true);
      double db = eye > 1e-5 ? 20 * std::log10(eye) : -100; char e[260]; std::snprintf(e, sizeof e, "RECORD LEVEL %s dB re wire saturation (magic eye)   ·   drum %.0f rpm at %s Hz%s, %.0f mm/s", db > -99 ? (std::to_string((int)std::lround(db))).c_str() : "below −99", 71.0 * rel(), hz60() ? "60" : "50", std::fabs(getP(P_SPEED) - 1) > 1e-6 ? " with varispeed" : "", 457.0 * rel());
      if (getP(P_WOB) > 1e-6) { const size_t l = std::strlen(e); std::snprintf(e + l, sizeof e - l, ", wobble +%.0f um, +/-%.2f %% speed", getP(P_WOB) * 3, getP(P_WOB) / 10); }
      setrgb(c, 0x9a9ea6); tsw::text(c, e, 16, 76, 12.5, false); }
    cairo_restore(c);
    // Wert-Popup: ab 1.4 entfernt (Wunsch des Nutzers, alle Plugins)
  }
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { double dx = x - WSKNOB[i][0], dy = y - WSKNOB[i][1]; if (dx * dx + dy * dy <= WSKNOB[i][2] * WSKNOB[i][2] * 1.69) return i; } return -1; }
  // Ziel unter der Maus: Knopf 100 + i, Taste 0 … 30, sonst −1
  int hit(double px, double py) const { int k = hitKnob(px, py); if (k >= 0) return 100 + k; double s = scale(); return hitButton(px / s, py / s); }
  void press(int b) { if (b < 3) setP(P_MODE, b); else if (b < 20) setP(P_FB1 + b - 10, on(P_FB1 + b - 10) ? 0 : 1); else if (b < 30) setP(P_OUT1 + b - 20, on(P_OUT1 + b - 20) ? 0 : 1); else if (b == 31) setP(P_LINK, linked() ? 0 : 1); else setP(P_MAINS, hz60() ? 0 : 1); }
  static int kp(int i) { return KPARAM[i]; }
  static bool isHead(int i) { return kp(i) >= P_H1 && kp(i) <= P_H4; }
  // LINK (ab 1.5): neuer Wert des bedienten Kopfs v gegen Ausgang b[k] → Faktor f für alle vier, so begrenzt, dass jeder Kopf in 20 … 770 ms bleibt;
  // ohne Rundung (Verhältnisse bleiben genau); liegen danach alle vier am Original (±0,3 ms), genau das Original
  void setLinked(int k, double v, const double *b) { double f = v / b[k], lo = 0, hi = 1e9;
    for (int j = 0; j < 4; j++) { lo = std::max(lo, wisent::Core::HEAD_MIN / b[j]); hi = std::min(hi, wisent::Core::HEAD_MAX / b[j]); } f = std::max(lo, std::min(hi, f));
    double nv[4]; bool orig = true; for (int j = 0; j < 4; j++) { nv[j] = b[j] * f; if (std::fabs(nv[j] - knobDefault(P_H1 + j)) > 0.3) orig = false; }
    for (int j = 0; j < 4; j++) setP(P_H1 + j, orig ? knobDefault(P_H1 + j) : nv[j]); }
  void headBase(double *b) const { for (int j = 0; j < 4; j++) b[j] = getP(P_H1 + j); }
  void setKnob(int i, double u01) { if (isHead(i) && linked()) { const int k = kp(i) - P_H1; double b[4]; if (drag == i) for (int j = 0; j < 4; j++) b[j] = dragB[j]; else headBase(b); setLinked(k, fromKnob(kp(i), u01), b); return; }
    setP(kp(i), fromKnob(kp(i), u01)); }
  void gestureHeads(int i, bool on) { if (!gesture) return; if (isHead(i) && linked()) for (int j = 0; j < 4; j++) gesture(P_H1 + j, on); else gesture(kp(i), on); }
#ifndef TS_OWNVIEW
  bool handle(XEvent &ev) { switch (ev.type) { case Expose: return true; case ButtonPress: return onMouse(0, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case MotionNotify: return onMouse(1, ev.xmotion.x, ev.xmotion.y, 0, 0); case ButtonRelease: return onMouse(2, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case LeaveNotify: return onMouse(3, 0, 0, 0, 0); default: return false; } }
#endif
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        int h = hit(x, y);
        if (button == 4 || button == 5) { if (h >= 100) { int k = h - 100; setKnob(k, knob01(kp(k), getP(kp(k))) + (button == 4 ? 0.02 : -0.02)); return true; }
          if (h >= 0 && h < 3) { setP(P_MODE, std::max(0, std::min(2, mode() + (button == 4 ? -1 : 1)))); return true; } return false; }
        if (button != 1) return false;
        if (h >= 100) { int k = h - 100; if (k == lastClickId && now - lastClick < 0.35) { if (isHead(k) && linked()) for (int j = 0; j < 4; j++) setP(P_H1 + j, knobDefault(P_H1 + j)); else setP(kp(k), knobDefault(kp(k))); } else { drag = k; dragY0 = y; dragV0 = knob01(kp(k), getP(kp(k))); headBase(dragB); gestureHeads(k, true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        if (h >= 0) { press(h); showTip = true; return true; }
        return false; }
      case 1: { if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) / 250 / scale()); return true; } int h = hit(x, y); bool ch = h != hover; hover = h; showTip = h >= 0; return ch; }
      case 2: if (button == 1 && drag >= 0) { gestureHeads(drag, false); drag = -1; return true; } return false;
      case 3: { bool ch = hover >= 0; hover = -1; if (drag < 0) showTip = false; return ch; }
      default: return false; } }
  std::string lastSig;
  std::string signature() const { char b[400]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i)); sapp(b, sizeof b, n, "%d|%d|%d|%d|%.3f", drag, hover, showTip ? 1 : 0, W, eye); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef TS_OWNVIEW
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace wisentgui
