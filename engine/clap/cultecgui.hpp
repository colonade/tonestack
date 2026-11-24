// Oberfläche des Cultec EQ-1A (Modelle EQP-1A und ab 1.1 EQP-1R, je eigene Frontplatte), gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm,
// unter Windows über winview.cpp. Oben die gezeichnete Frontplatte (engine/tools/cultec_panel.py, eigene Gestaltung) mit Zeigern, Wahlschaltern, IN-Schalter und
// Netzlampe, darunter der Modellstreifen (Aussteuerung, Ausgangspegel, LEVEL, OVERSAMPLING, OUTPUT) und der Frequenzgang.
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
#include "cultecgeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "cultecpanel_jpg.h"
#include "widgets.hpp"

namespace ctgui {
constexpr double PI = 3.14159265358979323846;
enum { P_LB, P_LA, P_LF, P_HB, P_BW, P_HF, P_HC, P_HCS, P_IN, P_LEVEL, P_OS, P_OUT, P_MODEL, P_LFR, P_HFR, P_COUNT };   // ab 1.1: Modell EQP-1A/EQP-1R mit eigenen Wahlschaltern
constexpr int NK = 5, NS = 3;   // Drehregler LOW BOOST, LOW ATTEN, HIGH BOOST, HIGH ATTEN, BANDWIDTH; Wahlschalter CPS, KCS, ATTEN SEL
static const int KPARAM[NK] = {P_LB, P_LA, P_HB, P_HC, P_BW};
static const int SPARAM[NS] = {P_LF, P_HF, P_HCS}, SPARAM_R[2] = {P_LFR, P_HFR};
constexpr double FW = CTW, FH = CTH;
constexpr double STRIPU = 150;
constexpr double PLOTU = 300;
static const double TOTALH = FH + (STRIPU + PLOTU) * FW / tsw::UW;
constexpr double MD_X = 545, LV_X = 700, OS_X = 880, OUT_X = 1060, KN_Y = 62;   // MODEL, LEVEL, OVERSAMPLING, OUTPUT im Modellstreifen
constexpr int NF = 160;
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
static const char *REFLABEL = "all controls 0 (flat)";
static const char *LFN[4] = {"20", "30", "60", "100"};
static const char *HFN[7] = {"3", "4", "5", "8", "10", "12", "16"};
static const char *HCN[3] = {"5", "10", "20"};
static const char *LFN_R[3] = {"30", "60", "100"};
static const char *HFN_R[5] = {"3", "5", "8", "10", "12"};
static const char *MDN[2] = {"EQP-1A", "EQP-1R"};
static const char *OSN[3] = {"1x", "2x", "4x"};

#ifdef TS_OWNVIEW
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves; std::function<void(double*)> peaks;
  std::vector<double> curve, refCurve; std::string curveKey; double pk[3] = {0, 0, 0};   // Ausgang (V Spitze an 600 Ω), Aussteuerung 12AU7, 12AX7 (Anteil der Vorspannung)
  cairo_surface_t *imgR = nullptr;   // Frontplatte EQ-1R
  ~Gui() { if (img) cairo_surface_destroy(img); if (imgR) cairo_surface_destroy(imgR); if (base) cairo_surface_destroy(base); }
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; double lastClick = 0; int lastClickId = -2;
  // Trefferkennungen: 0 … NK−1 Drehregler, 10 … 12 Wahlschalter CPS/KCS/ATTEN SEL (EQP-1R: 10, 11), 30 IN-Schalter, 50 LEVEL, 51 OUTPUT, 60 OVERSAMPLING, 61 MODEL (Modellstreifen)
  double scale() const { return (double)W / FW; }
  double uscale() const { return (double)W / tsw::UW; }
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - FH * scale()) / u; }
  static cairo_surface_t *decode(const unsigned char *jpg, unsigned len) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(jpg, (int)len, &iw, &ih, &nch, 3);
    cairo_surface_t *im = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iw, ih); unsigned char *dst = cairo_image_surface_get_data(im); int stride = cairo_image_surface_get_stride(im);
    for (int y = 0; y < ih; y++) for (int x = 0; x < iw; x++) { unsigned char *s = px + (y * iw + x) * 3; unsigned char *t = dst + y * stride + x * 4; t[0] = s[2]; t[1] = s[1]; t[2] = s[0]; t[3] = 255; }
    cairo_surface_mark_dirty(im); stbi_image_free(px); return im; }
  void loadImg() { if (!img) img = decode(CTPANEL_JPG, CTPANEL_JPG_LEN); if (!imgR) imgR = decode(CTPANEL_R_JPG, CTPANEL_R_JPG_LEN); }
  // Modell: 0 EQP-1A, 1 EQP-1R (eigene Frontplatte und Wahlschalter)
  // Modell einmal je Zeichnen bzw. Mausereignis lesen (snapM): der Host kann MODEL jederzeit umstellen, alle Indizes müssen zum selben Modell passen
  int curM = 0; void snapM() { curM = getP(P_MODEL) > 0.5 ? 1 : 0; }
  bool mR() const { return curM != 0; }
  int nsel() const { return mR() ? 2 : NS; }
  const double *selG(int k) const { return mR() ? CTSEL_R[k] : CTSEL[k]; }
  int sparam(int k) const { return mR() ? SPARAM_R[k] : SPARAM[k]; }
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
  int sel(int s) const { int n = (int)selG(s)[3]; return std::max(0, std::min(n - 1, (int)std::lround(getP(sparam(s))))); }
  bool inOn() const { return getP(P_IN) > 0.5; }
#ifndef TS_OWNVIEW
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0; int baseM = -1;   // zwischengespeicherte Frontplatte (Größe, Modell)
  static void pointer(cairo_t *c, double x, double y, double r, double deg, double w) {
    cairo_save(c); cairo_translate(c, x, y); cairo_rotate(c, deg * PI / 180); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    setrgb(c, 0xf3efe2, 0.28); cairo_set_line_width(c, w * 1.5); cairo_move_to(c, 0, -r * 0.18); cairo_line_to(c, 0, -r * 0.86); cairo_stroke(c);
    setrgb(c, 0xf3efe2); cairo_set_line_width(c, w); cairo_move_to(c, 0, -r * 0.18); cairo_line_to(c, 0, -r * 0.86); cairo_stroke(c); cairo_restore(c); }
  void drawTo(cairo_t *c) {
    snapM(); double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i); v[P_MODEL] = curM;
    setrgb(c, 0x0a0a0a); cairo_paint(c);
    const int md = mR() ? 1 : 0; cairo_surface_t *pimg = md ? imgR : img; if (base && md != baseM) { cairo_surface_destroy(base); base = nullptr; } baseM = md;
#ifndef TS_OWNVIEW
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(FW * s), (int)std::ceil(FH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, pimg, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, FW * s, FH * s); cairo_fill(c);
    cairo_save(c); cairo_scale(c, s, s);
#else
    cairo_cg_cached(c, base, FW * s, FH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, pimg, 0, 0); cairo_rectangle(b, 0, 0, FW, FH); cairo_fill(b); });
    cairo_save(c); cairo_scale(c, s, s);
#endif
    // Drehregler 0 … 10 über ±150°, Wahlschalter auf ihre Stellung
    for (int i = 0; i < NK; i++) { const double *g = CTKNOB[i]; pointer(c, g[0], g[1], g[2], -150 + 30 * std::max(0.0, std::min(10.0, v[KPARAM[i]])), g[2] > 38 ? 5 : 4.2);
      if (i == drag || i == hover) { setrgb(c, 0xf0b020); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.12, 0, 2 * PI); cairo_stroke(c); } }
    for (int k = 0; k < nsel(); k++) { const double *g = selG(k); int n = (int)g[3]; double a = -g[4] / 2 + g[4] * sel(k) / (n - 1); pointer(c, g[0], g[1], g[2], a, 4.2);
      if (10 + k == drag || 10 + k == hover) { setrgb(c, 0xf0b020); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g[0], g[1], g[2] * 1.15, 0, 2 * PI); cairo_stroke(c); } }
    // IN-Schalter: Hebel nach oben (IN) oder unten (OUT)
    { const double x = CTIN[0], y = CTIN[1], dir = inOn() ? -1 : 1, tip = y + dir * 34;
      setrgb(c, 0x000000, 0.3); cairo_new_path(c); cairo_move_to(c, x - 2, y); cairo_line_to(c, x + 6, tip + 4); cairo_line_to(c, x + 12, tip + 4); cairo_line_to(c, x + 5, y); cairo_close_path(c); cairo_fill(c);
      cairo_pattern_t *g = cairo_pattern_create_linear(x - 6, 0, x + 6, 0); cairo_pattern_add_color_stop_rgb(g, 0, 0.5, 0.51, 0.53); cairo_pattern_add_color_stop_rgb(g, 0.45, 0.97, 0.97, 0.98); cairo_pattern_add_color_stop_rgb(g, 1, 0.45, 0.46, 0.48);
      cairo_new_path(c); cairo_move_to(c, x - 4, y); cairo_line_to(c, x - 6, tip); cairo_line_to(c, x + 6, tip); cairo_line_to(c, x + 4, y); cairo_close_path(c); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g);
      cairo_pattern_t *b = cairo_pattern_create_radial(x - 2, tip - 3, 1, x, tip, 8); cairo_pattern_add_color_stop_rgb(b, 0, 1, 1, 1); cairo_pattern_add_color_stop_rgb(b, 0.6, 0.78, 0.79, 0.81); cairo_pattern_add_color_stop_rgb(b, 1, 0.45, 0.46, 0.48);
      cairo_new_path(c); cairo_arc(c, x, tip, 7.5, 0, 2 * PI); cairo_set_source(c, b); cairo_fill(c); cairo_pattern_destroy(b);
      if (hover == 30) { setrgb(c, 0xf0b020); cairo_set_line_width(c, 2.5); cairo_new_path(c); cairo_arc(c, x, y, 44, 0, 2 * PI); cairo_stroke(c); } }
    // Netzlampe (immer an: das Gerät läuft, solange das Plugin läuft)
    { const double x = CTLAMP[0], y = CTLAMP[1]; cairo_pattern_t *g = cairo_pattern_create_radial(x - 4, y - 4, 1, x, y, 14); cairo_pattern_add_color_stop_rgb(g, 0, 1, 0.86, 0.6); cairo_pattern_add_color_stop_rgb(g, 0.5, 0.95, 0.32, 0.12); cairo_pattern_add_color_stop_rgb(g, 1, 0.55, 0.1, 0.05);
      cairo_new_path(c); cairo_arc(c, x, y, 14, 0, 2 * PI); cairo_set_source(c, g); cairo_fill(c); cairo_pattern_destroy(g);
      cairo_pattern_t *h = cairo_pattern_create_radial(x, y, 6, x, y, 40); cairo_pattern_add_color_stop_rgba(h, 0, 1, 0.4, 0.15, 0.35); cairo_pattern_add_color_stop_rgba(h, 1, 1, 0.4, 0.15, 0); cairo_new_path(c); cairo_arc(c, x, y, 40, 0, 2 * PI); cairo_set_source(c, h); cairo_fill(c); cairo_pattern_destroy(h); }
    cairo_restore(c);
    // ---- Modellstreifen ----
    const double u = uscale(); char vo[40], vl[60]; std::snprintf(vo, 40, "%+.1f dB", v[P_OUT]); std::snprintf(vl, 60, "%+.0f dBu", v[P_LEVEL]);
    cairo_save(c); cairo_translate(c, 0, FH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 16, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { setrgb(c, 0xdcdee2); tsw::text(c, "DRIVE", 30, 50, 13, true);
      const char *nm[3] = {"output (dBm)", "12AU7 grids", md ? "12AY7 grids" : "12AX7 grids"}; double dbm = pk[0] > 1e-6 ? 20 * std::log10(pk[0] / std::sqrt(2.0) / 0.7746) : -99;
      double a3[3] = {std::max(0.0, (dbm + 20) / 44), pk[1], pk[2]};   // Ausgang: −20 … +24 dBm, Höchstpegel laut Werk +21 dBm (Strich)
      for (int k = 0; k < 3; k++) { double y = 60 + k * 24, x0 = 150, w = 290, a = std::min(1.2, a3[k]); setrgb(c, 0x9a9ea6); tsw::text(c, nm[k], 30, y + 12, 12, false);
        bool hot = k == 0 ? dbm > 21 : a >= 1; setrgb(c, 0x23262b); cairo_rectangle(c, x0, y, w, 14); cairo_fill(c); setrgb(c, hot ? 0xff5040 : (k == 0 ? dbm > 15 : a > 0.5) ? 0xf2c14e : 0x5cc46a); cairo_rectangle(c, x0, y, w * std::min(1.0, a), 14); cairo_fill(c);
        if (k == 0) { setrgb(c, 0xdcdee2); cairo_rectangle(c, x0 + w * 41 / 44, y - 2, 1.5, 18); cairo_fill(c); }
        char t[32]; if (k == 0) std::snprintf(t, 32, dbm < -60 ? "–" : "%+.1f dBm", dbm); else std::snprintf(t, 32, a < 0.001 ? "–" : "%.0f %%", a * 100); setrgb(c, 0xcfd1d6); tsw::text(c, t, x0 + w + 12, y + 12, 12, false); } }
    { setrgb(c, 0xdcdee2); char t[160]; if (md) std::snprintf(t, sizeof t, "EQP-1R · %s · LOW %s Hz · HIGH BOOST %s kHz · ATTEN 10 kHz", inOn() ? "IN" : "OUT (boosts and low atten as at 0)", LFN_R[sel(0)], HFN_R[sel(1)]);
      else std::snprintf(t, sizeof t, "EQP-1A · %s · LOW %s Hz · HIGH BOOST %s kHz · ATTEN %s kHz", inOn() ? "IN" : "OUT (all controls as at 0)", LFN[sel(0)], HFN[sel(1)], HCN[sel(2)]);
      tsw::text(c, t, 30, 140, 12, false); }
    tsw::smallKnobV(c, LV_X, KN_Y, (v[P_LEVEL] - 0) / 30, "LEVEL (0 dBFS)", vl, drag == 50 || hover == 50, false);
    tsw::toggle(c, MD_X, KN_Y + 12, 2, md, "MODEL", MDN, nullptr);   // Kippschalter: oben EQP-1A, unten EQP-1R
    if (hover == 61) { setrgb(c, 0xf0b020); cairo_set_line_width(c, 2); tsw::rrect(c, MD_X - 30, KN_Y - 34, 120, 92, 8); cairo_stroke(c); }
    tsw::rotarySel(c, OS_X, KN_Y + 12, 3, std::max(0, std::min(2, (int)std::lround(v[P_OS]))), "OVERSAMPLING", OSN, nullptr);
    if (hover == 60) { setrgb(c, 0xf0b020); cairo_set_line_width(c, 2); tsw::rrect(c, OS_X - 54, KN_Y - 52, 108, 112, 8); cairo_stroke(c); }
    tsw::smallKnobV(c, OUT_X, KN_Y, (v[P_OUT] + 24) / 48, "OUTPUT", vo, drag == 51 || hover == 51, false);
    { char kb[400]; int kn = 0; for (int id = 0; id < P_COUNT; id++) if (id != P_OUT && id != P_LEVEL && id != P_OS) sapp(kb, sizeof kb, kn, "%.4g|", v[id]);
      std::string key(kb, kn); if (computeCurves && key != curveKey) { computeCurves(curve, refCurve); curveKey = key; } }
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, "FREQUENCY RESPONSE  (small signal, transformers and amplifier included)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, false); cairo_restore(c); }
    cairo_restore(c);
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s;
    for (int i = 0; i < NK; i++) { double dx = x - CTKNOB[i][0], dy = y - CTKNOB[i][1]; if (dx*dx + dy*dy <= CTKNOB[i][2] * CTKNOB[i][2] * 1.69) return i; }
    for (int k = 0; k < nsel(); k++) { const double *g = selG(k); double dx = x - g[0], dy = y - g[1]; if (dx*dx + dy*dy <= g[2] * g[2] * 1.9) return 10 + k; }
    if (std::hypot(x - CTIN[0], y - CTIN[1]) < 44) return 30;
    double lx, ly; toLower(px, py, lx, ly); if (ly < 0) return -1;
    { double dx = lx - LV_X, dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 50; } { double dx = lx - OUT_X, dy = ly - KN_Y; if (dx*dx + dy*dy <= 32 * 32) return 51; }
    if (tsw::rotaryHit(lx, ly, OS_X, KN_Y + 12)) return 60; if (tsw::toggleHit(lx, ly, MD_X, KN_Y + 12, 64) >= 0) return 61; return -1; }
  int knobParam(int i) const { return i < NK ? KPARAM[i] : i >= 10 && i < 10 + nsel() ? sparam(i - 10) : i == 30 ? P_IN : i == 50 ? P_LEVEL : i == 51 ? P_OUT : i == 61 ? P_MODEL : P_OS; }
  // Stellung 0 … 10 (Ziehen, Mausrad); Wahlschalter in Stellungen
  double knobVal(int i) const { double v = getP(knobParam(i)); if (i < NK) return v; if (i == 50) return v / 3; if (i == 51) return (v + 24) / 48 * 10; return v; }
  void setKnob(int i, double uu) {
    if (i < NK) { uu = std::max(0.0, std::min(10.0, uu)); setP(KPARAM[i], std::round(uu * 20) / 20); return; }
    if (i >= 10 && i < 10 + nsel()) { int n = (int)selG(i - 10)[3]; setP(sparam(i - 10), std::max(0.0, std::min((double)n - 1, std::round(uu)))); return; }
    if (i == 60) { setP(P_OS, std::max(0.0, std::min(2.0, std::round(uu)))); return; }
    if (i == 61) { setP(P_MODEL, std::max(0.0, std::min(1.0, std::round(uu)))); return; }
    uu = std::max(0.0, std::min(10.0, uu));
    if (i == 50) setP(P_LEVEL, std::round(uu * 3)); else setP(P_OUT, std::round((uu / 10 * 48 - 24) * 2) / 2); }
  double knobDefault(int i) const { switch (i) { case 4: return 5; case 50: return 6; case 51: return 5; default: return 0; } }   // BANDWIDTH Mitte, LEVEL +18 dBu, OUTPUT 0 dB
#ifndef TS_OWNVIEW
  bool handle(XEvent &ev) {
    switch (ev.type) {
      case Expose: return true;
      case ButtonPress: return onMouse(0, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case MotionNotify: return onMouse(1, ev.xmotion.x, ev.xmotion.y, 0, 0);
      case ButtonRelease: return onMouse(2, ev.xbutton.x, ev.xbutton.y, (int)ev.xbutton.button, ev.xbutton.time / 1000.0);
      case LeaveNotify: return onMouse(3, 0, 0, 0, 0);
      default: return false; } }
#endif
  bool stepped(int k) const { return (k >= 10 && k < 10 + nsel()) || k == 60 || k == 61; }
  bool onMouse(int kind, double x, double y, int button, double now) { snapM();
    // Ziehen begonnen, dann Modell gewechselt (Host, Automation): Geste des gemerkten Parameters beenden statt auf einen anderen Regler zu fallen
    if (drag >= 0 && dragM != curM) { if (gesture && dragP >= 0) gesture(dragP, false); drag = -1; dragP = -1; if (kind == 1 || kind == 2) return true; }
    switch (kind) {
      case 0: {
        int k = hitKnob(x, y);
        if (button == 4 || button == 5) { if (k < 0 || k == 30) return false; setKnob(k, knobVal(k) + (button == 4 ? 1 : -1) * (stepped(k) ? 1 : 0.2)); return true; }
        if (button != 1 || k < 0) return false;
        if (k == 30) { setP(P_IN, inOn() ? 0 : 1); return true; }
        if (k == 60) { setKnob(60, std::fmod(knobVal(60) + 1, 3)); return true; }   // OVERSAMPLING: Klick schaltet weiter
        if (k == 61) { double lx, ly; toLower(x, y, lx, ly); setKnob(61, tsw::toggleHit(lx, ly, MD_X, KN_Y + 12, 64)); return true; }   // MODEL: obere Hälfte EQP-1A, untere EQP-1R
        if (k >= 10 && k < 10 + nsel()) {   // Wahlschalter: Klick rechts der Mitte eine Stellung weiter, links zurück; Ziehen wie ein Knopf
          double s = scale(), dx = x / s - selG(k - 10)[0]; drag = k; dragP = knobParam(k); dragM = curM; dragY0 = y; dragV0 = knobVal(k); dragMoved = false; clickRight = dx >= 0; if (gesture) gesture(dragP, true); return true; }
        if (k == lastClickId && now - lastClick < 0.35) setKnob(k, knobDefault(k)); else { drag = k; dragP = knobParam(k); dragM = curM; dragY0 = y; dragV0 = knobVal(k); dragMoved = true; if (gesture) gesture(dragP, true); }
        lastClick = now; lastClickId = k; return true; }
      case 1: {
        if (drag >= 0) { double d = dragY0 - y;
          if (drag >= 10 && drag < 10 + nsel()) { if (std::fabs(d) > 4 * scale()) dragMoved = true; if (dragMoved) setKnob(drag, dragV0 + d / (40 * scale())); return true; }
          setKnob(drag, dragV0 + d * (drag >= 50 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }
        int h = hitKnob(x, y); bool changed = h != hover; hover = h; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (drag >= 10 && drag < 10 + nsel() && !dragMoved) setKnob(drag, knobVal(drag) + (clickRight ? 1 : -1)); if (gesture) gesture(dragP, false); drag = -1; dragP = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; return changed; }
      default: return false;
    }
  }
  bool dragMoved = false, clickRight = true; int dragP = -1, dragM = 0;   // beim Druck: Parameter und Modell
  std::string lastSig;
  std::string signature() { if (peaks) peaks(pk); char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    sapp(b, sizeof b, n, "%d|%d|%d|%.0f|%.0f|%.0f", drag, hover, W, pk[0] > 1e-6 ? 20 * std::log10(pk[0]) * 4 : -999.0, pk[1] * 50, pk[2] * 50); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef TS_OWNVIEW
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace ctgui
