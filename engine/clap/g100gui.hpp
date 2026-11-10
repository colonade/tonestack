// Oberfläche des Yamaha-G100-212II-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die nachgezeichnete Frontplatte (engine/tools/g100_panel.py, kein Foto, kein Logo) mit Zeigern, Zugschaltern (PULL FAT / PULL BRIGHT),
// A/B-Taste mit Anzeige und Eingangsbuchsen (HIGH / LOW), darunter der Modellstreifen (Überabtastung, Endstufe, Poti-Kennlinie, Aussteuerung,
// Instrument/Ausgangspegel) und der Frequenzgang.
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
#include "g100geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "g100panel_jpg.h"
#include "widgets.hpp"
#include "autoin.hpp"

namespace g100gui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_VOLA, P_TREBA, P_MIDA, P_BASSA, P_FATA, P_BRIGHTA, P_CHAN, P_GAINB, P_MASTERB, P_TREBB, P_MIDB, P_BASSB, P_FATB, P_BRIGHTB, P_LEVEL, P_Q, P_FREQ, P_LOW, P_PWR, P_VIN, P_OS, P_OUT, P_TAPER, P_REVERB, P_TANK, P_INREF, P_SPK, P_COUNT };
constexpr int NK = 13;   // Knöpfe der Frontplatte (Reihenfolge G100KNOB)
static const int KPARAM[NK] = {P_VOLA, P_TREBA, P_MIDA, P_BASSA, P_GAINB, P_MASTERB, P_TREBB, P_MIDB, P_BASSB, P_LEVEL, P_Q, P_FREQ, P_REVERB};
static const int KPULL[NK] = {P_FATA, P_BRIGHTA, -1, -1, P_FATB, -1, P_BRIGHTB, -1, -1, -1, -1, -1, -1};   // Zugschalter des Knopfs
static const char *KNAME[NK] = {"VOLUME (A)", "TREBLE (A)", "MIDDLE (A)", "BASS (A)", "GAIN (B)", "MASTER VOL (B)", "TREBLE (B)", "MIDDLE (B)", "BASS (B)", "LEVEL", "Q", "FREQUENCY", "REVERB"};
static const char *PULLNAME[NK] = {"PULL FAT (A)", "PULL BRIGHT (A)", "", "", "PULL FAT (B)", "", "PULL BRIGHT (B)", "", "", "", "", "", ""};
constexpr double FW = G100W, FH = G100H;
constexpr double STRIPU = 150;  // Modellstreifen in UI-Einheiten (tsw::UW = 1200 breit)
constexpr double PLOTU = 300;   // Frequenzgang
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;   // Gesamthöhe in Frontplatten-Einheiten
constexpr double SW_Y = 88;
constexpr int NSW = 4;
static const double SW_X[NSW] = {60, 178, 296, 414};   // SPEAKER (ab 1.10), OVERSAMPLING, POWER AMP (Begrenzung / linear), POT TAPER (REVERB TANK ab 1.4 als kleiner Regler, ab 1.10 rechts neben DRIVE)
static const int SW_P[NSW] = {P_SPK, P_OS, P_PWR, P_TAPER};
static const double KN_X[3] = {1020, 1130, 905}; // INPUT, OUTPUT, REVERB TANK (ab 1.4: Stärke der Nichtlinearität, 0 = linear), Beschriftung unter dem Knopf
constexpr double KN_Y = 72;
constexpr int NF = 120;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "knobs at 5, level 0, channel A";
inline double potTaper(double x, int mode) { if (x <= 0) return 0; if (x >= 1) return 1; if (mode == 2) return x; const double m = mode == 1 ? 0.10 : 0.15; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // wie im Kern (ohne dessen Header, für mac/test/render.mm)
inline double freqOfKnob(double x) { const double xs[5] = {0, 0.28, 0.5, 0.72, 1}, fs[5] = {100, 200, 500, 1000, 5000}; x = std::min(1.0, std::max(0.0, x));   // FREQUENCY-Skala wie im Kern
  for (int k = 0; k < 4; k++) if (x <= xs[k + 1]) { double t = (x - xs[k]) / (xs[k + 1] - xs[k]); return fs[k] * std::pow(fs[k + 1] / fs[k], t); } return fs[4]; }

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double inPk = -999; std::function<void()> resetPeak;   // Eingangsbezug: gemerkte Spitze (dBFS, −999 = nichts gehört), Klick setzt sie zurück (autoin.hpp)
  static constexpr int NM = 6; double meter[NM] = {0, 0, 0, 0, 0, 0}; bool meterOn = false;   // Aussteuerung: IC1, Kanal A, Dioden B, 2SK30A, EQ-Stufen (1 = Grenze), Endstufe (W)
  int osActive = 0;   // Überabtastung, mit der der Kern gerade rechnet (Umschalten dauert einen Augenblick)
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Knöpfe, 20 … 20+NK−1 Zugschalter, 40 A/B-Taste, 41/42 Buchse HIGH/LOW, 50/51 INPUT/OUTPUT (Modellstreifen)
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(G100PANEL_JPG, (int)G100PANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  // Knopfstellung 0 … 10 aus dem Parameter (LEVEL −5 … +5 → 0 … 10)
  double knobPos(int i) const { int p = KPARAM[i]; if (p < 0) return 0; double v = getP(p); return p == P_LEVEL ? v + 5 : v; }
  void knobText(int i, char *buf, size_t n) const { int p = KPARAM[i]; if (p < 0) { std::snprintf(buf, n, "%s  (not simulated)", KNAME[i]); return; } double v = getP(p);
    int tp = std::max(0, std::min(2, (int)std::lround(getP(P_TAPER)))); const char *tn = tp == 2 ? "linear" : tp == 1 ? "log 10 %" : "log 15 %";
    if (p == P_LEVEL) std::snprintf(buf, n, "%s  %+.1f  (center = flat)", KNAME[i], v);
    else if (p == P_FREQ) std::snprintf(buf, n, "%s  %.1f  (about %.0f Hz)", KNAME[i], v, freqOfKnob(v / 10));
    else if (p == P_Q) std::snprintf(buf, n, "%s  %.1f  (%s; left = broad, right = narrow)", KNAME[i], v, tn);
    else if (p == P_REVERB) std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, linear; spring tank)", KNAME[i], v, v * 10);
    else { int pp = KPULL[i]; bool pulled = pp >= 0 && getP(pp) > 0.5; std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, %s)%s", KNAME[i], v, potTaper(v / 10, tp) * 100, tn, pulled ? (pp == P_FATA || pp == P_FATB ? " - FAT" : " - BRIGHT") : ""); } }
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    const bool chanB = v[P_CHAN] > 0.5, low = v[P_LOW] > 0.5;
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
    // Buchsen: Stecker in der gewählten (HIGH oder LOW)
    for (int j = 0; j < 2; j++) { const double *J = G100JACK[j]; bool plug = (j == 1) == low;
      if (plug) { setrgb(c, 0x202020); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x8a8a8a); cairo_set_line_width(c, 1.5); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_stroke(c); }
      if (hover == 41 + j) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] + 6, 0, 2 * PI); cairo_stroke(c); } }
    // A/B-Taste: gedrückt bei Kanal B; Anzeige: grün A, rot B, mit Buchstaben
    { double x = G100BTN[0], y = G100BTN[1], h = G100BTN[2] - 3; setrgb(c, chanB ? 0x3a3c40 : 0x5a5d62); cairo_rectangle(c, x - h, y - h, 2 * h, 2 * h); cairo_fill(c);
      setrgb(c, chanB ? 0x1c1d20 : 0x8a8d93); cairo_set_line_width(c, 1.5); cairo_rectangle(c, x - h + 1, y - h + 1, 2 * h - 2, 2 * h - 2); cairo_stroke(c);
      double lx = G100LED[0], ly = G100LED[1]; setrgb(c, chanB ? 0xff3b2a : 0x3ec25a); cairo_rectangle(c, lx - 8, ly - 5, 16, 10); cairo_fill(c);
      cairo_pattern_t *g = cairo_pattern_create_radial(lx, ly, 3, lx, ly, 16); cairo_pattern_add_color_stop_rgba(g, 0, chanB ? 1 : 0.3, chanB ? 0.3 : 0.9, 0.2, 0.45); cairo_pattern_add_color_stop_rgba(g, 1, 1, 0.3, 0.2, 0); cairo_new_path(c); cairo_arc(c, lx, ly, 16, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g);
      setrgb(c, 0x101010); tsw::text(c, chanB ? "B" : "A", lx, ly + 4, 10, true, 1);
      if (hover == 40) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_rectangle(c, x - h - 5, y - h - 5, 2 * h + 10, 2 * h + 10); cairo_stroke(c); } }
    // Knöpfe: Zeiger (weißer Strich), gezogene Knöpfe mit hellem Kragen
    for (int i = 0; i < NK; i++) { const double *g = G100KNOB[i]; int pp = KPULL[i]; bool pulled = pp >= 0 && v[pp] > 0.5;
      if (KPARAM[i] < 0) { setrgb(c, 0x0a0a0a, 0.55); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] + 2, 0, 2 * PI); cairo_fill(c); }
      if (pulled) { setrgb(c, 0xe8e2cf); cairo_set_line_width(c, 5); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] - 2, 0, 2 * PI); cairo_stroke(c);
        setrgb(c, 0xffd98a); tsw::rrect(c, g[0] - 36, G100PULLY - 2, 72, 14, 3); cairo_fill(c); setrgb(c, 0x111111); tsw::text(c, pp == P_FATA || pp == P_FATB ? "PULL FAT" : "PULL BRIGHT", g[0], G100PULLY + 9, 9.5, true, 1); }
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, ang(knobPos(i))); setrgb(c, KPARAM[i] < 0 ? 0x6a6a6a : 0xf2f2f2); cairo_set_line_width(c, 4); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.25); cairo_line_to(c, 0, -g[2] * 0.92); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.1, 0, 2 * PI); cairo_stroke(c); }
      if (hover == 20 + i && pp >= 0) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 2); tsw::rrect(c, g[0] - 38, G100PULLY - 4, 76, 18, 4); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); int os = std::max(0, std::min(2, (int)std::lround(v[P_OS])));
    char vt[3][40]; tsin::valueText(vt[0], 40, v[P_VIN], v[P_INREF] > 0.5); std::snprintf(vt[1], 40, "%+.1f dB", v[P_OUT]); if (v[P_TANK] <= 0) std::snprintf(vt[2], 40, "linear (as 1.1)"); else std::snprintf(vt[2], 40, "%.1f nonlinear", v[P_TANK]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *ol[3] = {"1x", "2x", "4x"}, *op[2] = {"CLIP", "LINEAR"}, *oo[2] = {"ON", "OFF"};
      tsw::toggle(c, SW_X[0], SW_Y, 2, v[P_SPK] > 0.5 ? 0 : 1, "SPEAKER", oo, "G12T-75, open");
      tsw::toggle(c, SW_X[1], SW_Y, 3, os, "OVERSAMPLING", ol, os != osActive ? "switching ..." : "against aliasing");
      tsw::toggle(c, SW_X[2], SW_Y, 2, v[P_PWR] > 0.5 ? 0 : 1, "POWER AMP", op, v[P_PWR] > 0.5 ? "100 W into 4 ohm" : "no limit");
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; int tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER]))); tsw::toggle(c, SW_X[3], SW_Y, 3, tp, "POT TAPER", tl, "A pots (vol, tone, Q)");
    }
    // Aussteuerung: Stufen relativ zur Begrenzung (Strich = 100 %), Endstufe als Leistung (Strich = 100 W)
    { const char *tn[NM] = {"input stage (IC1)", "channel A stages", "ch. B diodes", "ch. B 2SK30A", "EQ stages", "power amp"};
      double lx = 548, bx = 655, bw = 120, bh = 13, y0 = 36, dy = 18; setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", lx, 30, 13, true);
      for (int i = 0; i < NM; i++) { double by = y0 + i * dy; bool dim = (i == 1 && chanB) || ((i == 2 || i == 3) && !chanB); setrgb(c, dim ? 0x5e626a : 0x9a9ea6); tsw::text(c, tn[i], lx, by + 11, 11.5, false);
        setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, bx + 0.5, by + 0.5, bw - 1, bh - 1); cairo_stroke(c);
        double x = meter[i], f, mark; bool over; char t[24];
        if (i < 5) { f = std::min(1.0, x / 1.5); mark = 1 / 1.5; over = x >= 1; std::snprintf(t, sizeof t, "%.0f %%", x * 100); }
        else { double full = 100; f = std::min(1.0, x / full / 1.25); mark = 1 / 1.25; over = x > full; if (x < 0.05) std::snprintf(t, sizeof t, "-"); else std::snprintf(t, sizeof t, x < 10 ? "%.1f W" : "%.0f W", x); }
        if (meterOn && !dim) { setrgb(c, over ? 0xff3b2a : f > mark * 0.7 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, bx + 1, by + 1, (bw - 2) * f, bh - 2); cairo_fill(c); }
        setrgb(c, 0xffffff, 0.45); cairo_rectangle(c, bx + 1 + (bw - 2) * mark - 0.5, by, 1, bh); cairo_fill(c);
        setrgb(c, dim ? 0x5e626a : over && meterOn ? 0xffb3a8 : 0xcfd1d6); tsw::text(c, meterOn && !dim ? t : "-", bx + bw + 8, by + 11, 11.5, false); } }
    for (int i = 0; i < 3; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : i == 1 ? (v[P_OUT] + 36) / 60 : v[P_TANK] / 10;
      tsw::smallKnobV(c, KN_X[i], KN_Y, uu, i == 0 ? "INPUT" : i == 1 ? "OUTPUT" : "REVERB TANK", vt[i], drag == 50 + i || hover == 50 + i, false); }
    { char xt[64]; tsin::extraText(xt, sizeof xt, inPk, v[P_INREF] > 0.5); setrgb(c, 0x9a9ea6); tsw::text(c, xt, KN_X[0], KN_Y + 76, 11, false, 1); }   // unter INPUT: gemerkte Spitze; Klick setzt zurück bzw. schaltet alte Projekte auf „auto“
    // Frequenzgang (Kleinsignal im Arbeitspunkt, bezogen auf 1 kHz), höchstens alle 100 ms neu
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_VIN && id != P_OS && id != P_OUT && id != P_PWR) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, v[P_SPK] > 0.5 ? (chanB ? "FREQUENCY RESPONSE  (small signal, channel B, sound pressure 2x12 G12T-75 open back, re 1 kHz)" : "FREQUENCY RESPONSE  (small signal, channel A, sound pressure 2x12 G12T-75 open back, re 1 kHz)") : chanB ? "FREQUENCY RESPONSE  (small signal, channel B, speaker terminals, re 1 kHz)" : "FREQUENCY RESPONSE  (small signal, channel A, speaker terminals, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, false); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[128]; double ax, ay;
      if (i < NK) { const double *g = G100KNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, buf, sizeof buf); }
      else if (i >= 20 && i < 20 + NK) { int k = i - 20; ax = G100KNOB[k][0] * s; ay = (G100PULLY - 10) * s; std::snprintf(buf, sizeof buf, "%s: %s (click to %s)", PULLNAME[k], v[KPULL[k]] > 0.5 ? "PULLED" : "pushed in", v[KPULL[k]] > 0.5 ? "push in" : "pull"); }
      else if (i == 40) { ax = G100BTN[0] * s; ay = (G100BTN[1] - 54) * s; std::snprintf(buf, sizeof buf, "A/B: channel %s (click to switch)", chanB ? "B" : "A"); }
      else if (i == 41 || i == 42) { const double *J = G100JACK[i - 41]; ax = J[0] * s; ay = (J[1] - J[2] - 46) * s; std::snprintf(buf, sizeof buf, i == 41 ? "INPUT HIGH (1 Mohm)%s" : "INPUT LOW (60 kohm, -12 dB)%s", ((i == 42) == low) ? " - plugged in" : ""); }
      else { ax = KN_X[i - 50] * u; ay = FH * s + (KN_Y - 30) * u; std::snprintf(buf, sizeof buf, "%s", vt[i - 50]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { if (KPARAM[i] < 0) continue; double dx = x - G100KNOB[i][0], dy = y - G100KNOB[i][1]; if (dx*dx + dy*dy <= G100KNOB[i][2] * G100KNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); for (int i = 0; i < 3; i++) { double dx = lx - KN_X[i], dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 50 + i; } return -1; }
  int hitPanel(double px, double py) const { double s = scale(), x = px / s, y = py / s;   // Zugschalter, A/B-Taste, Buchsen
    for (int i = 0; i < NK; i++) if (KPULL[i] >= 0 && std::fabs(x - G100KNOB[i][0]) <= 40 && y >= G100PULLY - 8 && y <= G100PULLY + 16) return 20 + i;
    if (std::fabs(x - G100BTN[0]) <= G100BTN[2] + 8 && y >= G100LED[1] - 12 && y <= G100BTN[1] + G100BTN[2] + 8) return 40;
    for (int j = 0; j < 2; j++) { double dx = x - G100JACK[j][0], dy = y - G100JACK[j][1]; if (dx*dx + dy*dy <= (G100JACK[j][2] + 8) * (G100JACK[j][2] + 8)) return 41 + j; } return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); for (int k = 0; k < NSW; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 62); if (half >= 0) return SW_P[k]; } return -1; }
  void moveSwitch(int p, int dir) {
    if (p == P_OS || p == P_TAPER) { int t = (int)std::lround(getP(p)); setP(p, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); }
    else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 1 : 0); }   // oben = CLIP
  void clickPanel(int h) { if (h >= 20 && h < 20 + NK) { int p = KPULL[h - 20]; if (p >= 0) setP(p, getP(p) > 0.5 ? 0 : 1); } else if (h == 40) setP(P_CHAN, getP(P_CHAN) > 0.5 ? 0 : 1); else if (h == 41) setP(P_LOW, 0); else if (h == 42) setP(P_LOW, 1); }
  int knobParam(int i) const { return i < NK ? KPARAM[i] : i == 50 ? P_VIN : i == 52 ? P_TANK : P_OUT; }
  double knobVal(int i) const { if (i < NK) return knobPos(i); return i == 50 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : i == 52 ? getP(P_TANK) : (getP(P_OUT) + 36) / 60 * 10; }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < NK) { int p = KPARAM[i]; if (p < 0) return; double r = std::round(u * 10) / 10; setP(p, p == P_LEVEL ? r - 5 : r); } else if (i == 50) setP(P_VIN, std::round(std::pow(1000, u / 10))); else if (i == 52) setP(P_TANK, std::round(u * 10) / 10); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
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
  bool hitInRef(double px, double py) const { double lx, ly; toLower(px, py, lx, ly); return std::fabs(lx - KN_X[0]) < 85 && ly > KN_Y + 64 && ly < KN_Y + 82; }   // Zeile unter INPUT
  bool onMouse(int kind, double x, double y, int button, double now) {
    switch (kind) {
      case 0: {
        if (button == 4 || button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (button == 4 ? 0.2 : -0.2)); else { int half, p = hitSwitch(x, y, half); if (p >= 0) moveSwitch(p, button == 4 ? -1 : 1); } return true; }
        if (button != 1) return false;
        if (hitInRef(x, y)) { if (getP(P_INREF) > 0.5) setP(P_INREF, 0); else if (resetPeak) resetPeak(); return true; }
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 50 ? std::log(100.0) / std::log(1000.0) * 10 : k == 51 ? 6 : 5); /* REVERB TANK (52): Doppelklick = 5 */ } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int ps = hitPanel(x, y); if (ps >= 0) { clickPanel(ps); return true; }
        { int half, b = hitSwitch(x, y, half); if (b >= 0) { moveSwitch(b, b == P_OS || b == P_TAPER ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 50 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); if (h < 0) h = hitPanel(x, y);
        bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i)); sapp(b, sizeof b, n, "%.1f|", inPk);
    for (int i = 0; i < NM; i++) sapp(b, sizeof b, n, "%d|", (int)std::lround(std::log10(1 + std::min(std::max(meter[i], 0.0), 999.0)) * 60));
    sapp(b, sizeof b, n, "%d|%d|%d|%d|%d|%d|%d", meterOn ? 1 : 0, osActive, drag, hover, showTip ? 1 : 0, W, curveStale ? (int)(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() * 10) : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace g100gui
