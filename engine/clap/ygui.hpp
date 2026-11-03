// Oberfläche des Yamaha-Plugins, gezeichnet mit Cairo (drawTo): unter Linux im X11-Fenster (im Host eingebettet), auf dem Mac über cairo_cg.hpp/macview.mm. Foto der Frontplatte (Kappen retuschiert), Zeiger, Buchsen, Modellstreifen, Frequenzgang.
#pragma once
#ifndef TS_VERSION
#define TS_VERSION "?"
#endif
#ifdef __APPLE__
#include "cairo_cg.hpp"   // Cairo-Teilmenge auf CoreGraphics
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
#include "ygeo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "ypanel_jpg.h"
#include "widgets.hpp"

namespace ygui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_INP, P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4, P_TAPER, P_PWR, P_VIN, P_SPK, P_OUT, P_COUNT };
static const int KPARAM[9] = {P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4};   // Knopf (Reihenfolge YKG) → Parameter
static const char *KNAME[9] = {"VOLUME", "TREBLE", "MIDDLE", "BASS", "60 Hz", "150 Hz", "320 Hz", "640 Hz", "1250 Hz"};
// Unterer Bereich in UI-Einheiten (tsw::UW = 1200 breit, bei Standardgröße 1 Einheit = 1 Pixel), unabhängig von der Fotoauflösung
constexpr double STRIPU = 150;  // Modellstreifen: Kippschalter, Endstufenanzeige, Instrument/Ausgang
constexpr double PLOTU = 300;   // Frequenzgang-Diagramm
static const double TOTALH = YFH + (STRIPU + PLOTU) * YFW / tsw::UW;   // Gesamthöhe in Foto-Einheiten
constexpr double SW_Y = 88;     // Mittelpunkt der Kippschalter und kleinen Regler
static const double SW_X[2] = {64, 186};      // POWER AMP, SPEAKER (POT TAPER ab 1.11 entfallen: Potis wie im Original, A = logarithmisch)
static const int SW_P[2] = {P_PWR, P_SPK};
static const double KN_X[2] = {830, 1046};        // INPUT, OUTPUT
constexpr int NF = 120;         // Stützstellen des Diagramms (20 Hz … 20 kHz, logarithmisch)
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
constexpr double V_LIM_GUI = 42.0;
static const char *REFLABEL = "service setting (vol., treble, middle, bass 10, EQ flat)";   // Begrenzung der Endstufe (V Spitze), wie im Kern

#ifdef __APPLE__
struct Gui : tsmac::Client {
  void *win = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0; std::function<void()> onTick;   // win: NSView; onTick: Anzeigen auffrischen (Plugin)
#else
struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
#endif
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;   // gesture: Ziehen beginnt/endet (Parameter-ID)
  double meter = 0; bool meterOn = false;   // Spitze am Ausgang der Endstufe vor der Begrenzung (V), nur mit Endstufe
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;   // Kurven in dB (bezogen auf 1 kHz): aktuell, Service-Einstellung
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0; bool curveStale = false;
  ~Gui() { if (img) cairo_surface_destroy(img); if (base) cairo_surface_destroy(base); }   // Frontplattenbild gehört der Oberfläche (sonst bei jedem Öffnen/Schließen 1,5 MB verloren)
  // snprintf-Anhängen mit Begrenzung (n bleibt innerhalb des Puffers, auch wenn ein Wert unerwartet lang wird)
  template <class... A> static void sapp(char *b, size_t cap, int &n, const char *f, A... a) { if (n < 0 || (size_t)n >= cap - 1) return; int k = std::snprintf(b + n, cap - n, f, a...); if (k > 0) n = (int)std::min((size_t)(n + k), cap - 1); }
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / YFW; }
  double uscale() const { return (double)W / tsw::UW; }
  // Fensterpixel → UI-Einheiten des unteren Bereichs
  void toLower(double x, double y, double &lx, double &ly) const { double u = uscale(); lx = x / u; ly = (y - YFH * scale()) / u; }
  void loadImg() {
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(YPANEL_JPG, (int)YPANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  // ---- Hilfen ----
  static void setrgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
  static double ang(double v) { return (-150 + 30 * v) * PI / 180; }   // Knopfstellung 0 … 10
  void font(cairo_t *c, double px, bool bold = true) { cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL); cairo_set_font_size(c, px); }
  void textC(cairo_t *c, const char *t, double x, double y, double px, bool bold = true) { font(c, px, bold); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, x - e.width / 2 - e.x_bearing, y + e.height / 2); cairo_show_text(c, t); }
  static double knobPos(int i, double pv) { return i >= 4 ? pv + 5 : pv; }   // EQ-Parameter −5 … +5 → Knopf 0 … 10
  void knobText(int i, double pv, char *buf, size_t n) const {
    if (i >= 4) { if (std::fabs(pv) < 0.05) std::snprintf(buf, n, "%s  0 (flat)", KNAME[i]); else std::snprintf(buf, n, "%s  %+.1f (end stop about %s10 dB)", KNAME[i], pv, pv > 0 ? "+" : "-"); }
    else std::snprintf(buf, n, "%s  %.1f", KNAME[i], pv); }
  // ---- Zeichnen ----
#ifndef __APPLE__
  void draw() { if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); drawTo(c);
    cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy); }
#endif
  cairo_surface_t *base = nullptr; double baseS = 0;
  void drawTo(cairo_t *c) {
    double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    bool low = v[P_INP] > 0.5, pwr = v[P_PWR] > 0.5, spk = v[P_SPK] > 0.5;
    setrgb(c, 0x0a0a0a); cairo_paint(c);
#ifndef __APPLE__
    // Foto einmal je Fenstergröße vorskaliert (ab Laney 3.31 / Yamaha 1.12 / AmpFett 1.12; vorher je Bild neu skaliert)
    if (!base || baseS != s) { if (base) cairo_surface_destroy(base); base = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)std::ceil(YFW * s), (int)std::ceil(YFH * s)); baseS = s;
      cairo_t *b = cairo_create(base); cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, YFW, YFH); cairo_fill(b); cairo_destroy(b); }
    cairo_set_source_surface(c, base, 0, 0); cairo_rectangle(c, 0, 0, YFW * s, YFH * s); cairo_fill(c);
    cairo_save(c); cairo_scale(c, s, s);
#else
    cairo_cg_cached(c, base, YFW * s, YFH * s, [&](cairo_t *b) { cairo_scale(b, s, s); cairo_set_source_surface(b, img, 0, 0); cairo_rectangle(b, 0, 0, YFW, YFH); cairo_fill(b); });   // ab 1.14 vorgezeichnet (Gerätepixel)
    cairo_save(c); cairo_scale(c, s, s);
#endif
    // Buchsen: Ring an der belegten Buchse, Stecker als dunkler Punkt
    const double *J[2] = {YJACK_HIGH, YJACK_LOW}; for (int i = 0; i < 2; i++) { bool act = (i == 1) == low;
      if (act) { setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] + 4, 0, 2 * PI); cairo_stroke(c); setrgb(c, 0x202020); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] * 0.42, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x8a8a8a); cairo_set_line_width(c, 1.5); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] * 0.42, 0, 2 * PI); cairo_stroke(c); } }
    // Zeiger auf den retuschierten Kappen (schwarzer Strich wie im Original)
    for (int i = 0; i < 9; i++) { const YKnobGeo &g = YKG[i]; double pos = knobPos(i, v[KPARAM[i]]);
      cairo_save(c); cairo_translate(c, g.x, g.y); cairo_rotate(c, ang(pos)); setrgb(c, 0x101010, 0.93); cairo_set_line_width(c, YPW); cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT); cairo_move_to(c, 0, -g.r * YPIN); cairo_line_to(c, 0, -g.r * YPOUT); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g.x, g.y, g.r * 1.08, 0, 2 * PI); cairo_stroke(c); } }
    cairo_restore(c);   // Ende Foto-Maßstab
    // ---- unterer Bereich in UI-Einheiten ----
    const double u = uscale();
    char vt[2][40]; std::snprintf(vt[0], 40, "%.0f mV at 0 dBFS", v[P_VIN]); std::snprintf(vt[1], 40, "%+.1f dB", v[P_OUT]);
    cairo_save(c); cairo_translate(c, 0, YFH * s); cairo_scale(c, u, u);
    setrgb(c, 0x191b1f); cairo_rectangle(c, 0, 0, tsw::UW, STRIPU); cairo_fill(c); setrgb(c, 0x2a2d33); cairo_rectangle(c, 0, 0, tsw::UW, 1); cairo_fill(c);
    setrgb(c, 0x8d919a); tsw::text(c, "MODEL  (not on the amp)", 16, 20, 13, true); setrgb(c, 0x6c7079); tsw::text(c, "PLUGIN VERSION " TS_VERSION, 220, 20, 12, false);
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); tsw::text(c, d.c_str(), tsw::UW - 16, 20, 12, false, 2); }
    { const char *oo[2] = {"ON", "OFF"};
      tsw::toggle(c, SW_X[0], SW_Y, 2, pwr ? 0 : 1, "POWER AMP", oo, "MA board");
      tsw::toggle(c, SW_X[1], SW_Y, 2, spk ? 0 : 1, "SPEAKER", oo, "15\" CB158, 78 l"); }
    // Endstufe: Spitze vor der Begrenzung (dB bezogen auf die Grenze von 42 V; rot = übersteuert)
    { double mx = 440, mw = 320, my = 50, mh = 24; setrgb(c, pwr ? 0xdcdee2 : 0x6a6a68); tsw::text(c, "POWER AMP PEAK", mx, 38, 14, true); setrgb(c, 0x8d919a); tsw::text(c, "before clipping, 0 dB = 42 V", mx + mw, 38, 12, false, 2);
      setrgb(c, 0x0f1114); cairo_rectangle(c, mx, my, mw, mh); cairo_fill(c); setrgb(c, 0x2e3138); cairo_set_line_width(c, 1); cairo_rectangle(c, mx + 0.5, my + 0.5, mw - 1, mh - 1); cairo_stroke(c);
      if (meterOn && meter > 0.05) { double db = 20 * std::log10(meter / V_LIM_GUI), f = std::max(0.0, std::min(1.0, (db + 40) / 40)); bool over = meter >= V_LIM_GUI;
        setrgb(c, over ? 0xff3b2a : db > -6 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, mx + 1, my + 1, (mw - 2) * f, mh - 2); cairo_fill(c);
        char t[64]; if (over) std::snprintf(t, 64, "clipping: %.0f V asked, %.1fx over", meter, meter / V_LIM_GUI); else std::snprintf(t, 64, "%.1f V peak, %.1f dB below the limit", meter, -db);
        setrgb(c, over ? 0xffb3a8 : 0xcfd1d6); tsw::text(c, t, mx, my + mh + 20, 13, false); }
      else { setrgb(c, 0x7a7a78); tsw::text(c, pwr ? "silent" : "off: output is the PA board (linear, relative)", mx, my + mh + 20, 13, false); }
      for (int k = 1; k < 4; k++) { setrgb(c, 0xffffff, 0.22); cairo_rectangle(c, mx + mw * k / 4 - 0.5, my + 2, 1, mh - 4); cairo_fill(c); }
      setrgb(c, 0x6a6e76); const char *sc[5] = {"-40", "-30", "-20", "-10", "0 dB"}; for (int k = 0; k < 5; k++) tsw::text(c, sc[k], mx + mw * k / 4, my + mh + 40, 11, false, k == 0 ? 0 : k == 4 ? 2 : 1); }
    // kleine Regler: Instrument (log) und Ausgang
    for (int i = 0; i < 2; i++) { double uu = i == 0 ? std::log(v[P_VIN]) / std::log(1000) : (v[P_OUT] + 36) / 60;
      tsw::smallKnob(c, KN_X[i], SW_Y, uu, i == 0 ? "INPUT" : "OUTPUT", i == 0 && !pwr ? "(used with power amp)" : vt[i], drag == 9 + i || hover == 9 + i, i == 0 && !pwr); }
    // Frequenzgang (Kleinsignal, bezogen auf 1 kHz): Kurven bei Bedarf neu rechnen (höchstens alle 100 ms)
    { char kb[256]; int kn = 0; const int ks[] = {P_INP, P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4, P_SPK};
      for (int id : ks) sapp(kb, sizeof kb, kn, "%.4g|", v[id]); std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; }
      curveStale = computeCurves && key != curveKey; }   // noch nicht nachgerechnet: signature() sorgt für ein weiteres Neuzeichnen
    { cairo_save(c); cairo_translate(c, 0, STRIPU); setrgb(c, 0x14161a); cairo_rectangle(c, 0, 0, tsw::UW, PLOTU); cairo_fill(c);
      double px0 = 58, px1 = tsw::UW - 18, py0 = 36, py1 = PLOTU - 40;
      setrgb(c, 0xc8ccd4); tsw::text(c, spk ? "FREQUENCY RESPONSE  (small signal, re 1 kHz, with speaker)" : "FREQUENCY RESPONSE  (small signal, re 1 kHz)", px0, 22, 13, true);
      tsw::plot(c, curve, refCurve, NF, freqAt, px0, px1, py0, py1, REFLABEL, true); cairo_restore(c); }
    cairo_restore(c);   // Ende unterer Bereich
    // Wertanzeige in Bildschirmpixeln
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[96]; double ax, ay;
      if (i < 9) { const YKnobGeo &g = YKG[i]; ax = g.x * s; ay = (g.y - g.r * 1.12) * s; knobText(i, v[KPARAM[i]], buf, 96); }
      else { ax = KN_X[i - 9] * u; ay = YFH * s + (SW_Y - 30) * u; std::snprintf(buf, 96, "%s", vt[i - 9]); }
      tsw::tip(c, buf, ax, ay, u, W); }
  }
  // ---- Treffer ----
  int hitKnob(double px, double py) const { double s = scale(), x = px / s, y = py / s; for (int i = 0; i < 9; i++) { double dx = x - YKG[i].x, dy = y - YKG[i].y; if (dx*dx + dy*dy <= YKG[i].r * YKG[i].r * 1.69) return i; }
    double lx, ly; toLower(px, py, lx, ly); /* Fensterpixel, nicht Foto-Einheiten */ for (int i = 0; i < 2; i++) { double dx = lx - KN_X[i], dy = ly - SW_Y; if (dx*dx + dy*dy <= 32 * 32) return 9 + i; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; const double *J[2] = {YJACK_HIGH, YJACK_LOW}; for (int i = 0; i < 2; i++) { double dx = x - J[i][0], dy = y - J[i][1]; if (dx*dx + dy*dy <= (J[i][2] + 14) * (J[i][2] + 14)) return i; } return -1; }
  // Kippschalter: liefert Parameter und half (0 obere, 1 untere Hälfte)
  int hitSwitch(double x, double y, int &half) const { double lx, ly; toLower(x, y, lx, ly); for (int k = 0; k < 2; k++) { half = tsw::toggleHit(lx, ly, SW_X[k], SW_Y, 30); if (half >= 0) return SW_P[k]; } return -1; }
  // Kippschalter bewegen: dir −1 = nach oben, +1 = nach unten (zweistufig: oben = an); 0 = umschalten
  void moveSwitch(int p, int dir) { if (p == P_TAPER) { int t = (int)std::lround(getP(P_TAPER)); setP(P_TAPER, std::max(0, std::min(2, t + (dir == 0 ? 1 : dir)))); } else setP(p, dir == 0 ? (getP(p) > 0.5 ? 0 : 1) : dir < 0 ? 1 : 0); }
  int knobParam(int i) const { return i < 9 ? KPARAM[i] : i == 9 ? P_VIN : P_OUT; }
  double knobVal(int i) const { if (i < 9) return knobPos(i, getP(KPARAM[i])); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : (getP(P_OUT) + 36) / 60 * 10; }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 9) setP(KPARAM[i], std::round(u * 10) / 10 - (i >= 4 ? 5 : 0)); else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
  // ---- Ereignisse: liefert true, wenn neu gezeichnet werden soll ----
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
        int j = hitJack(x, y); if (j >= 0) { setP(P_INP, j == 0 ? 0 : 1); return true; }
        { int half, b = hitSwitch(x, y, half); if (b >= 0) { moveSwitch(b, b == P_TAPER ? (half == 0 ? -1 : 1) : 0); return true; } }
        return false; }
      case 1: {
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) * (drag >= 9 ? 10.0 / 200 / uscale() : 10.0 / 250 / scale())); return true; }   /* Foto-Knöpfe: 250 Foto-Einheiten, INPUT/OUTPUT: 200 Pixel bei Standardgröße für den ganzen Weg */
        int h = hitKnob(x, y); bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case 2: if (button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case 3: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) sapp(b, sizeof b, n, "%.4g|", getP(i));
    sapp(b, sizeof b, n, "%d|%d|%d|%d|%d|%d|%d", (int)std::lround(std::log10(1 + std::min(std::max(meter, 0.0), 99.0)) * 60), meterOn ? 1 : 0, drag, hover, showTip ? 1 : 0, W, curveStale ? (int)(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() * 10) : 0); std::string s(b, n); if (diag) s += diag(); return s; }
#ifndef __APPLE__
  // Fenster vom Host mit dem Containerfenster zerstört: nicht mehr zeichnen und nicht noch einmal zerstören
  void gone() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } win = 0; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (win && ev.type == DestroyNotify && ev.xdestroywindow.window == win) { gone(); continue; } if (win && ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
#endif
};
} // namespace ygui
