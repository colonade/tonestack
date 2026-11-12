// Oberfläche des Lametta-B100-III-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die nachgezeichnete Frontplatte (engine/tools/b3_panel.py, kein Foto, kein Logo; ab 1.1 schwarz mit farbigen Knopfkappen wie am Gerät) mit Zeigern und den Anzeigen PARAMETRIC EQ 1 / 2 (Klick schaltet
// den Entzerrer wie der Fußschalter), darunter der Modellstreifen (Überabtastung, Endstufe, Poti-Kennlinie, Aussteuerung, Instrument/Ausgangspegel)
// und der Frequenzgang. Die Frequenzweiche ist gezeichnet, aber ohne Wirkung (abgeblendet).
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
#include "b3geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "b3panel_jpg.h"
#include "widgets.hpp"
#include "autoin.hpp"

namespace b3gui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_VOL, P_TREB, P_MID, P_BASS, P_LEV1, P_Q1, P_F1, P_PRE1, P_EQ1, P_LEV2, P_Q2, P_F2, P_PRE2, P_EQ2, P_PWR, P_VIN, P_OS, P_OUT, P_TAPER, P_INREF, P_COUNT };
constexpr int NK = 15;   // Knöpfe der Frontplatte (Reihenfolge B3KNOB); die drei der Frequenzweiche ohne Parameter
static const int KPARAM[NK] = {P_VOL, P_TREB, P_MID, P_BASS, P_LEV1, P_Q1, P_F1, P_PRE1, P_LEV2, P_Q2, P_F2, P_PRE2, -1, -1, -1};
static const char *KNAME[NK] = {"VOLUME", "TREBLE", "MIDDLE", "BASS", "EQ 1 LEVEL", "EQ 1 Q", "EQ 1 FREQUENCY", "EQ 1 PRESET LEVEL", "EQ 2 LEVEL", "EQ 2 Q", "EQ 2 FREQUENCY", "EQ 2 PRESET LEVEL", "CROSSOVER LPF LEVEL", "CROSSOVER FREQUENCY", "CROSSOVER HPF LEVEL"};
inline bool bipolar(int p) { return p == P_TREB || p == P_MID || p == P_BASS || p == P_LEV1 || p == P_PRE1 || p == P_LEV2 || p == P_PRE2; }   // −5 … +5
constexpr double FW = B3W, FH = B3H;
constexpr double STRIPU = 150;  // Modellstreifen in UI-Einheiten (tsw::UW = 1200 breit)
constexpr double PLOTU = 300;   // Frequenzgang
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;   // Gesamthöhe in Frontplatten-Einheiten
constexpr double SW_Y = 88;
constexpr int NSW = 3;
static const double SW_X[NSW] = {60, 172, 290};   // OVERSAMPLING, POWER AMP (Begrenzung / linear), POT TAPER
static const int SW_P[NSW] = {P_OS, P_PWR, P_TAPER};
static const double KN_X[2] = {1020, 1130}; // INPUT, OUTPUT, Beschriftung unter dem Knopf
constexpr double KN_Y = 72;
constexpr int NF = 120;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "volume 5, tone and EQ flat";
inline double potTaper(double x, int mode) { if (x <= 0) return 0; if (x >= 1) return 1; if (mode == 2) return x; const double m = mode == 1 ? 0.10 : 0.15; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // wie im Kern (ohne dessen Header, für mac/test/render.mm)
inline double freqOfKnob(double x) { const double xs[5] = {0, 0.29, 0.5, 0.71, 1}, fs[5] = {50, 100, 250, 500, 2500}; x = std::min(1.0, std::max(0.0, x));   // FREQUENCY-Skala wie im Kern
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
  static constexpr int NM = 5; double meter[NM] = {0, 0, 0, 0, 0}; bool meterOn = false;   // Aussteuerung: IC101 1/2, Vorstufe (IC101 2/2, Klangregelung), PEQ 1, PEQ 2 (1 = Grenze), Endstufe (W an 8 Ω)
  int osActive = 0;   // Überabtastung, mit der der Kern gerade rechnet (Umschalten dauert einen Augenblick)
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Knöpfe, 40/41 Anzeige PARAMETRIC EQ 1/2 (schaltet), 50/51 INPUT/OUTPUT (Modellstreifen)
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(B3PANEL_JPG, (int)B3PANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  // Knopfstellung 0 … 10 aus dem Parameter (−5 … +5 → 0 … 10)
  double knobPos(int i) const { int p = KPARAM[i]; if (p < 0) return 5; double v = getP(p); return bipolar(p) ? v + 5 : v; }
  void knobText(int i, char *buf, size_t n) const { int p = KPARAM[i]; if (p < 0) { std::snprintf(buf, n, "%s  (not simulated: only for external amps)", KNAME[i]); return; } double v = getP(p);
    int tp = std::max(0, std::min(2, (int)std::lround(getP(P_TAPER)))); const char *tn = tp == 2 ? "linear" : tp == 1 ? "log 10 %" : "log 15 %";
    if (p == P_LEV1 || p == P_LEV2) std::snprintf(buf, n, "%s  %+.1f  (center = flat)", KNAME[i], v);
    else if (p == P_PRE1 || p == P_PRE2) std::snprintf(buf, n, "%s  %+.1f  (gain when the EQ is on; center = unity)", KNAME[i], v);
    else if (bipolar(p)) std::snprintf(buf, n, "%s  %+.1f  (center = flat)", KNAME[i], v);
    else if (p == P_F1 || p == P_F2) std::snprintf(buf, n, "%s  %.1f  (about %.0f Hz)", KNAME[i], v, std::min(freqOfKnob(v / 10), 2340.0));
    else if (p == P_Q1 || p == P_Q2) std::snprintf(buf, n, "%s  %.1f  (%s; left = broad, right = narrow)", KNAME[i], v, tn);
    else std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, %s)", KNAME[i], v, potTaper(v / 10, tp) * 100, tn); }
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
    // Anzeigen PARAMETRIC EQ 1 / 2: rot leuchtend = eingeschaltet (wie mit dem Fußschalter; rote Leuchtdioden wie am Gerät)
    for (int j = 0; j < 2; j++) { const double *L = B3LED[j]; bool on = v[j ? P_EQ2 : P_EQ1] > 0.5;
      setrgb(c, on ? 0xff3a28 : 0x46161a); cairo_new_path(c); cairo_arc(c, L[0], L[1], 6, 0, 2 * PI); cairo_fill(c);
      if (on) { cairo_pattern_t *g = cairo_pattern_create_radial(L[0], L[1], 3, L[0], L[1], 18); cairo_pattern_add_color_stop_rgba(g, 0, 1.0, 0.25, 0.15, 0.55); cairo_pattern_add_color_stop_rgba(g, 1, 1.0, 0.25, 0.15, 0); cairo_new_path(c); cairo_arc(c, L[0], L[1], 18, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g); }
      if (hover == 40 + j) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, L[0], L[1], 13, 0, 2 * PI); cairo_stroke(c); } }
    // Knöpfe: Zeiger (dunkler Strich auf der farbigen Kappe); die Frequenzweiche steht abgeblendet mit festem Zeiger im Bild
    for (int i = 0; i < NK; i++) { const double *g = B3KNOB[i]; if (KPARAM[i] < 0) continue;
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, ang(knobPos(i))); setrgb(c, 0x1c1c1e); cairo_set_line_width(c, 3.5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.08); cairo_line_to(c, 0, -g[2] * 0.66); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xd09a10); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.1, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); int os = std::max(0, std::min(1, (int)std::lround(v[P_OS])));
    char vt[2][40]; tsin::valueText(vt[0], 40, v[P_VIN], v[P_INREF] > 0.5); std::snprintf(vt[1], 40, "%+.1f dB", v[P_OUT]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false); tsw::prefDraw(c, "VALUE POPUP", tsw::prefs().tips, 410, 20);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *ol[2] = {"2x", "4x"}, *op[2] = {"CLIP", "LINEAR"};
      tsw::toggle(c, SW_X[0], SW_Y, 2, os, "OVERSAMPLING", ol, os != osActive ? "switching ..." : "against aliasing");
      tsw::toggle(c, SW_X[1], SW_Y, 2, v[P_PWR] > 0.5 ? 0 : 1, "POWER AMP", op, v[P_PWR] > 0.5 ? "100 W into 8 ohm" : "no limit");
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; int tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER]))); tsw::toggle(c, SW_X[2], SW_Y, 3, tp, "POT TAPER", tl, "A pots (volume, Q)"); }
    // Aussteuerung: Stufen relativ zur Begrenzung (Strich = 100 %), Endstufe als Leistung (Strich = 100 W)
    { const char *tn[NM] = {"input stage (IC101)", "volume + tone stages", "parametric EQ 1", "parametric EQ 2", "power amp"};
      double lx = 440, bx = 560, bw = 160, bh = 13, y0 = 36, dy = 18; setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", lx, 30, 13, true);
      for (int i = 0; i < NM; i++) { double by = y0 + i * dy; bool dim = false; setrgb(c, dim ? 0x5e626a : 0x9a9ea6); tsw::text(c, tn[i], lx, by + 11, 11.5, false);
        setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, bx + 0.5, by + 0.5, bw - 1, bh - 1); cairo_stroke(c);
        double x = meter[i], f, mark; bool over; char t[24];
        if (i < 4) { f = std::min(1.0, x / 1.5); mark = 1 / 1.5; over = x >= 1; std::snprintf(t, sizeof t, "%.0f %%", x * 100); }
        else { double full = 100; f = std::min(1.0, x / full / 1.25); mark = 1 / 1.25; over = x > full; if (x < 0.05) std::snprintf(t, sizeof t, "-"); else std::snprintf(t, sizeof t, x < 10 ? "%.1f W" : "%.0f W", x); }
        if (meterOn && !dim) { setrgb(c, over ? 0xff3b2a : f > mark * 0.7 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, bx + 1, by + 1, (bw - 2) * f, bh - 2); cairo_fill(c); }
        setrgb(c, 0xffffff, 0.45); cairo_rectangle(c, bx + 1 + (bw - 2) * mark - 0.5, by, 1, bh); cairo_fill(c);
        setrgb(c, dim ? 0x5e626a : over && meterOn ? 0xffb3a8 : 0xcfd1d6); tsw::text(c, meterOn && !dim ? t : "-", bx + bw + 8, by + 11, 11.5, false); } }
    for (int i = 0; i < 2; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : (v[P_OUT] + 36) / 60;
      tsw::smallKnobV(c, KN_X[i], KN_Y, uu, i == 0 ? "INPUT" : "OUTPUT", vt[i], drag == 50 + i || hover == 50 + i, false); }
    { char xt[64]; tsin::extraText(xt, sizeof xt, inPk, v[P_INREF] > 0.5); setrgb(c, 0x9a9ea6); tsw::text(c, xt, KN_X[0], KN_Y + 76, 11, false, 1); }   // unter INPUT: gemerkte Spitze; Klick setzt zurück bzw. schaltet alte Projekte auf „auto“
    // Frequenzgang (Kleinsignal im Arbeitspunkt, bezogen auf 1 kHz), höchstens alle 100 ms neu
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_VIN && id != P_OS && id != P_OUT && id != P_PWR) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, "FREQUENCY RESPONSE  (small signal, speaker terminals, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, false); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[128]; double ax, ay;
      if (i < NK) { const double *g = B3KNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, buf, sizeof buf); }
      else if (i == 40 || i == 41) { const double *L = B3LED[i - 40]; ax = L[0] * s + 60 * s; ay = (L[1] + 4) * s; bool on = v[i == 40 ? P_EQ1 : P_EQ2] > 0.5; std::snprintf(buf, sizeof buf, "PARAMETRIC EQ %d: %s (click to switch, like the foot switch)", i - 39, on ? "ON" : "off"); }
      else { ax = KN_X[i - 50] * u; ay = FH * s + (KN_Y - 30) * u; std::snprintf(buf, sizeof buf, "%s", vt[i - 50]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < NK; i++) { if (KPARAM[i] < 0) continue; double dx = x - B3KNOB[i][0], dy = y - B3KNOB[i][1]; if (dx*dx + dy*dy <= B3KNOB[i][2] * B3KNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); for (int i = 0; i < 2; i++) { double dx = lx - KN_X[i], dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 50 + i; } return -1; }
  int hitPanel(double px, double py) const { double s = scale(), x = px / s, y = py / s;   // Anzeigen PARAMETRIC EQ 1 / 2
    for (int j = 0; j < 2; j++) { double dx = x - B3LED[j][0], dy = y - B3LED[j][1]; if (dx*dx + dy*dy <= 16 * 16) return 40 + j; } return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); for (int k = 0; k < NSW; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 62); if (half >= 0) return SW_P[k]; } return -1; }
  void moveSwitch(int p, int dir) {
    if (p == P_OS || p == P_TAPER) { int t = (int)std::lround(getP(p)), mx = p == P_OS ? 1 : 2; setP(p, std::max(0, std::min(mx, t + (dir == 0 ? 1 : dir)))); }
    else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 1 : 0); }   // oben = CLIP
  void clickPanel(int h) { if (h == 40) setP(P_EQ1, getP(P_EQ1) > 0.5 ? 0 : 1); else if (h == 41) setP(P_EQ2, getP(P_EQ2) > 0.5 ? 0 : 1); }
  int knobParam(int i) const { return i < NK ? KPARAM[i] : i == 50 ? P_VIN : P_OUT; }
  double knobVal(int i) const { if (i < NK) return knobPos(i); return i == 50 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : (getP(P_OUT) + 36) / 60 * 10; }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < NK) { int p = KPARAM[i]; if (p < 0) return; double r = std::round(u * 10) / 10; setP(p, bipolar(p) ? r - 5 : r); } else if (i == 50) setP(P_VIN, std::round(std::pow(1000, u / 10))); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
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
        { double plx, ply; toLower(x, y, plx, ply); if (tsw::prefHit(plx, ply, 410, 20, 125)) { tsw::prefs().tips = !tsw::prefs().tips; tsw::prefSave(); return true; } }   // Schalter der Kopfzeile (für alle Plugins gemeinsam gespeichert)
        if (hitInRef(x, y)) { if (getP(P_INREF) > 0.5) setP(P_INREF, 0); else if (resetPeak) resetPeak(); return true; }
        int k = hitKnob(x, y);
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 50 ? std::log(100.0) / std::log(1000.0) * 10 : k == 51 ? 6 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
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
} // namespace b3gui
