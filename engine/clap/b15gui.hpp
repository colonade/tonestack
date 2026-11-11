// Oberfläche des AmpFett-B-15NF-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm.
// Oben die nachgezeichnete Frontplatte (engine/tools/b15_panel.py, kein Foto, kein Logo) mit Zeigern, Buchsen und Schalterhebeln,
// darunter der Modellstreifen (Lautsprecher, Überabtastung, Aussteuerung, Instrument/Ausgang) und der Frequenzgang.
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
#include "b15geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "b15panel_jpg.h"
#include "widgets.hpp"
#include "autoin.hpp"

namespace bgui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_JACK, P_V1, P_T1, P_B1, P_V2, P_T2, P_B2, P_STBY, P_PWR, P_SPK, P_VIN, P_OS, P_OUT, P_MIS, P_MAINS, P_PSU, P_TAPER, P_INREF, P_COUNT };
static const int KPARAM[6] = {P_V1, P_T1, P_B1, P_V2, P_T2, P_B2};   // Knopf (Reihenfolge B15KNOB) → Parameter
static const char *KNAME[6] = {"VOLUME 1", "TREBLE 1", "BASS 1", "VOLUME 2", "TREBLE 2", "BASS 2"};
static const int JACKVAL[3] = {1, 0, 2};   // Buchse (Reihenfolge B15JACK: Guitar, Bass, Inst.) → Parameterwert (0 Bass, 1 Guitar, 2 Inst.)
constexpr double FW = B15W, FH = B15H;
constexpr double STRIPU = 150;  // Modellstreifen in UI-Einheiten (tsw::UW = 1200 breit)
constexpr double PLOTU = 300;   // Frequenzgang
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;   // Gesamthöhe in Frontplatten-Einheiten
constexpr double SW_Y = 88;
static const double SW_X[4] = {64, 186, 310, 434};   // SPEAKER, OVERSAMPLING, POWER SUPPLY (DC / voll 60 Hz / voll 50 Hz; ab 1.15 statt MAINS), POT TAPER (ab 1.16: Kennlinie der Volume-Potis)
static const int SW_P[4] = {P_SPK, P_OS, P_PSU, P_TAPER};
inline int psuPos(const double *v) { return v[P_PSU] > 0.5 ? (v[P_MAINS] > 0.5 ? 2 : 1) : 0; }
static const double KN_X[3] = {905, 1010, 1120}; // INPUT, OUTPUT, MISMATCH (Streuung der Endröhren), Beschriftung unter dem Knopf (ab 1.16 weiter rechts: Platz für POT TAPER)
constexpr double KN_Y = 72;
constexpr int NF = 120;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "all knobs at 5, same input";
inline double potTaper(double x, int mode) { if (x <= 0) return 0; if (x >= 1) return 1; if (mode == 2) return x; const double m = mode == 1 ? 0.10 : 0.15; double b = (1 - m) / m; b = b * b; return (std::pow(b, x) - 1) / (b - 1); }   // wie im Kern (ohne dessen Header, für mac/test/render.mm)

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  double inPk = -999; std::function<void()> resetPeak;   // Eingangsbezug: gemerkte Spitze (dBFS, −999 = nichts gehört), Klick setzt sie zurück (autoin.hpp)
  double meter[5] = {0, 0, 0, 0, 0}; bool meterOn = false;   // Aussteuerung V1A, V1B, V3, 6L6GC (1 = Gitter erreicht 0 V), Leistung (W an 8 Ω)
  int osActive = 0;   // Überabtastung, mit der der Kern gerade rechnet (Umschalten dauert einen Augenblick)
  int psuActive = 0;   // Netzteil, mit dem der Kern gerade rechnet (Umschalten dauert einen Augenblick)
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(B15PANEL_JPG, (int)B15PANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  void knobText(int i, double v, char *buf, size_t n) const {
    if (i == 0 || i == 3) { int tp = std::max(0, std::min(2, (int)std::lround(getP(P_TAPER)))); std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, %s)", KNAME[i], v, potTaper(v / 10, tp) * 100, tp == 2 ? "linear" : tp == 1 ? "log 10 %" : "log 15 %"); }
    else std::snprintf(buf, n, "%s  %.1f  (pot %.0f %%, linear)", KNAME[i], v, v * 10); }
  // Schalter auf der Frontplatte (ab 1.15 nur POWER; STANDBY und GROUND entfernt): Hebel oben = ON
  bool swUp(int, const double *v) const { return v[P_PWR] > 0.5; }
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    int jack = std::max(0, std::min(2, (int)std::lround(v[P_JACK]))); bool spk = v[P_SPK] > 0.5;
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
    for (int i = 0; i < 3; i++) { if (JACKVAL[i] != jack) continue; const double *J = B15JACK[i];
      setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] + 4, 0, 2 * PI); cairo_stroke(c);
      setrgb(c, 0x202020); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x8a8a8a); cairo_set_line_width(c, 1.5); cairo_new_path(c); cairo_arc(c, J[0], J[1], J[2] * 0.42, 0, 2 * PI); cairo_stroke(c); }
    // Zeiger (weißer Strich auf der schwarzen Kappe)
    for (int i = 0; i < 6; i++) { const double *g = B15KNOB[i];
      cairo_save(c); cairo_translate(c, g[0], g[1]); cairo_rotate(c, ang(v[KPARAM[i]])); setrgb(c, 0xf2f2f2); cairo_set_line_width(c, 4.5); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND); cairo_move_to(c, 0, -g[2] * 0.25); cairo_line_to(c, 0, -g[2] * 0.92); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.08, 0, 2 * PI); cairo_stroke(c); } }
    // Schalterhebel: von der Mutter schräg nach oben bzw. unten
    for (int k = 0; k < B15NSW; k++) { double x = B15SW[k][0], y = B15SW[k][1], d = swUp(k, v) ? -1 : 1;
      cairo_new_path(c); cairo_move_to(c, x - 6, y); cairo_line_to(c, x - 8, y + d * 40); cairo_curve_to(c, x - 8 + 16.0 / 3, y + d * (40 + 16.0 / 3), x + 8 - 16.0 / 3, y + d * (40 + 16.0 / 3), x + 8, y + d * 40); cairo_line_to(c, x + 6, y); cairo_close_path(c);
      setrgb(c, 0xd8dadf); cairo_fill_preserve(c); setrgb(c, 0x4a4d52); cairo_set_line_width(c, 2); cairo_stroke(c);
      if (hover == 20 + k) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, x, y, 26, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); int os = std::max(0, std::min(2, (int)std::lround(v[P_OS])));
    char vt[3][40]; tsin::valueText(vt[0], 40, v[P_VIN], v[P_INREF] > 0.5); std::snprintf(vt[1], 40, "%+.1f dB", v[P_OUT]); std::snprintf(vt[2], 40, "%.1f %%", v[P_MIS]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false); tsw::prefDraw(c, "VALUE POPUP", tsw::prefs().tips, 410, 20);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *oo[2] = {"ON", "OFF"}, *ol[3] = {"1x", "2x", "4x"};
      tsw::toggle(c, SW_X[0], SW_Y, 2, spk ? 0 : 1, "SPEAKER", oo, "CTS 15\", sealed");
      tsw::toggle(c, SW_X[1], SW_Y, 3, os, "OVERSAMPLING", ol, os != osActive ? "switching ..." : "against aliasing");
      const char *mh[3] = {"DC", "60 Hz", "50 Hz"}; int pp = psuPos(v);
      tsw::toggle(c, SW_X[2], SW_Y, 3, pp, "SUPPLY", mh, (pp > 0) != (psuActive > 0) ? "switching ..." : pp == 0 ? "DC, no hum" : "mains, 5AR4");
      const char *tl[3] = {"LOG 15 %", "LOG 10 %", "LINEAR"}; int tp = std::max(0, std::min(2, (int)std::lround(v[P_TAPER]))); tsw::toggle(c, SW_X[3], SW_Y, 3, tp, "POT TAPER", tl, "volume 1 and 2"); }
    // Aussteuerung: Gitterhub je Stufe (1 = Gitter erreicht 0 V, Strich), Leistung an 8 Ω (Strich = 30 W)
    { const char *tn[5] = {jack == 2 ? "V2A" : "V1A", jack == 2 ? "V2B" : "V1B", "V3 phase inv.", "6L6GC grid", "power"};
      double lx = 540, bx = 630, bw = 170, bh = 15, y0 = 40, dy = 21; setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", lx, 32, 13, true);
      for (int i = 0; i < 5; i++) { double by = y0 + i * dy; setrgb(c, 0x9a9ea6); tsw::text(c, tn[i], lx, by + 12, 12, false);
        setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, bx + 0.5, by + 0.5, bw - 1, bh - 1); cairo_stroke(c);
        double x = meter[i], f, mark; bool over; char t[24];
        if (i < 4) { f = std::min(1.0, x / 1.5); mark = 1 / 1.5; over = x >= 1; std::snprintf(t, sizeof t, "%.0f %%", x * 100); }
        else { f = std::min(1.0, x / 30 / 1.25); mark = 1 / 1.25; over = x > 30; if (x < 0.05) std::snprintf(t, sizeof t, "-"); else std::snprintf(t, sizeof t, x < 10 ? "%.1f W" : "%.0f W", x); }
        if (meterOn) { setrgb(c, over ? 0xff3b2a : f > mark * 0.7 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, bx + 1, by + 1, (bw - 2) * f, bh - 2); cairo_fill(c); }
        setrgb(c, 0xffffff, 0.45); cairo_rectangle(c, bx + 1 + (bw - 2) * mark - 0.5, by, 1, bh); cairo_fill(c);
        setrgb(c, over && meterOn ? 0xffb3a8 : 0xcfd1d6); tsw::text(c, meterOn ? t : "-", bx + bw + 10, by + 12, 12, false); } }
    for (int i = 0; i < 3; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : i == 1 ? (v[P_OUT] + 36) / 60 : v[P_MIS] / 10;
      tsw::smallKnobV(c, KN_X[i], KN_Y, uu, i == 0 ? "INPUT" : i == 1 ? "OUTPUT" : "TUBE MISMATCH", vt[i], drag == 9 + i || hover == 9 + i, false); }   // MISMATCH: Ruhestrom-Unterschied der 6L6GC
    { char xt[64]; tsin::extraText(xt, sizeof xt, inPk, v[P_INREF] > 0.5); setrgb(c, 0x9a9ea6); tsw::text(c, xt, KN_X[0], KN_Y + 76, 11, false, 1); }   // unter INPUT: gemerkte Spitze; Klick setzt zurück bzw. schaltet alte Projekte auf „auto“
    // Frequenzgang (Kleinsignal im Arbeitspunkt, bezogen auf 1 kHz), höchstens alle 100 ms neu
    { char kb[256]; int kn = 0; const int ks[] = {P_JACK, P_V1, P_T1, P_B1, P_V2, P_T2, P_B2, P_SPK, P_PSU, P_TAPER};
      for (int id : ks) sapp(kb, sizeof kb, kn, "%.4g|", v[id]); std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, spk ? "FREQUENCY RESPONSE  (small signal, re 1 kHz, sound pressure)" : "FREQUENCY RESPONSE  (small signal, speaker terminal, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, true); cairo_restore(c); }
    cairo_restore(c);
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[112]; double ax, ay;
      if (i < 6) { const double *g = B15KNOB[i]; ax = g[0] * s; ay = (g[1] - g[2] * 1.12) * s; knobText(i, v[KPARAM[i]], buf, sizeof buf); }
      else if (i >= 20) { int k = i - 20; ax = B15SW[k][0] * s; ay = (B15SW[k][1] - 60) * s;
        std::snprintf(buf, sizeof buf, "%s", v[P_PWR] > 0.5 ? "POWER: ON" : "POWER: OFF (muted)"); }
      else { ax = KN_X[i - 9] * u; ay = FH * s + (KN_Y - 30) * u; if (i == 11) std::snprintf(buf, sizeof buf, "6L6GC MISMATCH %s (idle current difference)", vt[2]); else std::snprintf(buf, sizeof buf, "%s", vt[i - 9]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < 6; i++) { double dx = x - B15KNOB[i][0], dy = y - B15KNOB[i][1]; if (dx*dx + dy*dy <= B15KNOB[i][2] * B15KNOB[i][2] * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); for (int i = 0; i < 3; i++) { double dx = lx - KN_X[i], dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 9 + i; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; for (int i = 0; i < 3; i++) { const double *J = B15JACK[i]; double dx = x - J[0], dy = y - J[1]; if (dx*dx + dy*dy <= (J[2] + 14) * (J[2] + 14)) return i; } return -1; }
  int hitPanelSw(double x, double y) const { double s = scale(); x /= s; y /= s; for (int k = 0; k < B15NSW; k++) if (std::fabs(x - B15SW[k][0]) <= 32 && std::fabs(y - B15SW[k][1]) <= 52) return k; return -1; }
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); for (int k = 0; k < 4; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 30); if (half >= 0) return SW_P[k]; } return -1; }
  void setPsuPos(int pos) { pos = std::max(0, std::min(2, pos)); if (pos > 0) setP(P_MAINS, pos == 2 ? 1 : 0); setP(P_PSU, pos > 0 ? 1 : 0); }   // DC / voll 60 Hz / voll 50 Hz
  void moveSwitch(int p, int dir) { if (p == P_PSU) { double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i); int t = psuPos(v); setPsuPos(dir == 0 ? (t + 1) % 3 : t + dir); return; }
    if (p == P_OS || p == P_TAPER) { int t = (int)std::lround(getP(p)); setP(p, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); } else if (p == P_MAINS) setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 0 : 1); else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 1 : 0); }   // MAINS: oben = 60 Hz
  void togglePanel(int) { setP(P_PWR, getP(P_PWR) > 0.5 ? 0 : 1); }   // einziger Frontplattenschalter: POWER
  int knobParam(int i) const { return i < 6 ? KPARAM[i] : i == 9 ? P_VIN : i == 10 ? P_OUT : P_MIS; }
  double knobVal(int i) const { if (i < 6) return getP(KPARAM[i]); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : i == 10 ? (getP(P_OUT) + 36) / 60 * 10 : getP(P_MIS); }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 6) setP(KPARAM[i], std::round(u * 10) / 10); else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else if (i == 11) setP(P_MIS, std::round(u * 10) / 10); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
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
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 9 ? std::log(100.0) / std::log(1000.0) * 10 : k == 10 ? 6 : k == 11 ? 8 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int j = hitJack(x, y); if (j >= 0) { setP(P_JACK, JACKVAL[j]); return true; }
        int ps = hitPanelSw(x, y); if (ps >= 0) { togglePanel(ps); return true; }
        { int half, b = hitSwitch(x, y, half); if (b >= 0) { moveSwitch(b, b == P_OS || b == P_PSU || b == P_TAPER ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 9 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); if (h < 0) { int ps = hitPanelSw(x, y); if (ps >= 0) h = 20 + ps; }
        bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i)); sapp(b, sizeof b, n, "%.1f|", inPk);
    for (int i = 0; i < 5; i++) sapp(b, sizeof b, n, "%d|", (int)std::lround(std::log10(1 + std::min(std::max(meter[i], 0.0), 999.0)) * 60));   /* nur sichtbare Änderungen neu zeichnen */
    sapp(b, sizeof b, n, "%d|%d|%d|%d|%d|%d|%d|%d", meterOn ? 1 : 0, osActive, psuActive, drag, hover, showTip ? 1 : 0, W, curveStale ? (int)(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() * 10) : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace bgui
