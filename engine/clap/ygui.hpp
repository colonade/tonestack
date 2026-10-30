// Oberfläche des Yamaha-CLAP-Plugins: X11-Fenster (im Host eingebettet), gezeichnet mit Cairo. Foto der Frontplatte (Kappen retuschiert), Zeiger, Buchsen, Modellstreifen, Frequenzgang.
#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
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

namespace ygui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_INP, P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4, P_TAPER, P_PWR, P_VIN, P_SPK, P_OUT, P_COUNT };
static const int KPARAM[9] = {P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4};   // Knopf (Reihenfolge YKG) → Parameter
static const char *KNAME[9] = {"VOLUME", "TREBLE", "MIDDLE", "BASS", "60 Hz", "150 Hz", "320 Hz", "640 Hz", "1250 Hz"};
constexpr double STRIP = 80;    // Modellstreifen unter dem Foto (in Foto-Einheiten)
constexpr double PLOT = 330;    // Frequenzgang-Diagramm
static const double TOTALH = YFH + STRIP + PLOT;
constexpr int NF = 120;         // Stützstellen des Diagramms (20 Hz … 20 kHz, logarithmisch)
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }
constexpr double V_LIM_GUI = 42.0;   // Begrenzung der Endstufe (V Spitze), wie im Kern

struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag; std::function<void(int,bool)> gesture;   // gesture: Ziehen beginnt/endet (Parameter-ID)
  double meter = 0; bool meterOn = false;   // Spitze am Ausgang der Endstufe vor der Begrenzung (V), nur mit Endstufe
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;   // Kurven in dB (bezogen auf 1 kHz): aktuell, Service-Einstellung
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0;
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / YFW; }
  bool init(Display *d, Window par, int w, int h) {
    dpy = d; parent = par; W = w; H = h;
    int scr = DefaultScreen(dpy); XSetWindowAttributes a{}; a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask | StructureNotifyMask; a.background_pixmap = None;
    win = XCreateWindow(dpy, par ? par : RootWindow(dpy, scr), 0, 0, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask | CWBackPixmap, &a);
    xs = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, scr), w, h);
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(YPANEL_JPG, (int)YPANEL_JPG_LEN, &iw, &ih, &nch, 3);
      img = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iw, ih); unsigned char *dst = cairo_image_surface_get_data(img); int stride = cairo_image_surface_get_stride(img);
      for (int y = 0; y < ih; y++) for (int x = 0; x < iw; x++) { unsigned char *s = px + (y * iw + x) * 3; unsigned char *t = dst + y * stride + x * 4; t[0] = s[2]; t[1] = s[1]; t[2] = s[0]; t[3] = 255; }
      cairo_surface_mark_dirty(img); stbi_image_free(px); }
    return true;
  }
  void show() { if (win) { XMapWindow(dpy, win); XFlush(dpy); } }
  void hide() { if (win) { XUnmapWindow(dpy, win); XFlush(dpy); } }
  void resize(int w, int h) { W = w; H = h; if (win) { XResizeWindow(dpy, win, w, h); cairo_xlib_surface_set_size(xs, w, h); } }
  void destroy() { if (xs) { cairo_surface_destroy(xs); xs = nullptr; } if (win) { XDestroyWindow(dpy, win); win = 0; } }
  // ---- Hilfen ----
  static void setrgb(cairo_t *c, int hex, double a = 1) { cairo_set_source_rgba(c, ((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a); }
  static double ang(double v) { return (-150 + 30 * v) * PI / 180; }   // Knopfstellung 0 … 10
  void font(cairo_t *c, double px, bool bold = true) { cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL); cairo_set_font_size(c, px); }
  void textC(cairo_t *c, const char *t, double x, double y, double px, bool bold = true) { font(c, px, bold); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, x - e.width / 2 - e.x_bearing, y + e.height / 2); cairo_show_text(c, t); }
  static double knobPos(int i, double pv) { return i >= 4 ? pv + 5 : pv; }   // EQ-Parameter −5 … +5 → Knopf 0 … 10
  void knobText(int i, double pv, char *buf, size_t n) const {
    if (i >= 4) { if (std::fabs(pv) < 0.05) std::snprintf(buf, n, "%s  0 (flat)", KNAME[i]); else std::snprintf(buf, n, "%s  %+.1f (nominal %+.0f dB)", KNAME[i], pv, pv / 5 * 10); }
    else std::snprintf(buf, n, "%s  %.1f", KNAME[i], pv); }
  // Taste im Modellstreifen
  struct Btn { const char *t; int p; double x, w; };
  static const Btn *buttons() { static const Btn B[3] = {{"POWER AMP (MA BOARD)", P_PWR, 14, 270}, {"SPEAKER  15\" CB158 · 78 L VENTED", P_SPK, 294, 360}, {"", P_TAPER, 664, 240}}; return B; }
  // ---- Zeichnen ----
  void draw() {
    if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    bool low = v[P_INP] > 0.5, pwr = v[P_PWR] > 0.5, spk = v[P_SPK] > 0.5;
    setrgb(c, 0x0a0a0a); cairo_paint(c);
    cairo_save(c); cairo_scale(c, s, s);
    cairo_set_source_surface(c, img, 0, 0); cairo_rectangle(c, 0, 0, YFW, YFH); cairo_fill(c);
    // Buchsen: Ring an der belegten Buchse, Stecker als dunkler Punkt
    const double *J[2] = {YJACK_HIGH, YJACK_LOW}; for (int i = 0; i < 2; i++) { bool act = (i == 1) == low;
      if (act) { setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] + 4, 0, 2 * PI); cairo_stroke(c); setrgb(c, 0x202020); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] * 0.42, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x8a8a8a); cairo_set_line_width(c, 1.5); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] * 0.42, 0, 2 * PI); cairo_stroke(c); } }
    // Zeiger auf den retuschierten Kappen (schwarzer Strich wie im Original)
    for (int i = 0; i < 9; i++) { const YKnobGeo &g = YKG[i]; double pos = knobPos(i, v[KPARAM[i]]);
      cairo_save(c); cairo_translate(c, g.x, g.y); cairo_rotate(c, ang(pos)); setrgb(c, 0x101010, 0.93); cairo_set_line_width(c, YPW); cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT); cairo_move_to(c, 0, -g.r * YPIN); cairo_line_to(c, 0, -g.r * YPOUT); cairo_stroke(c); cairo_restore(c);
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g.x, g.y, g.r * 1.08, 0, 2 * PI); cairo_stroke(c); } }
    // Modellstreifen
    double y0 = YFH + 6; setrgb(c, 0x191b1f); cairo_rectangle(c, 0, YFH, YFW, STRIP); cairo_fill(c);
    setrgb(c, 0x8d919a); font(c, 13, false); cairo_move_to(c, 14, y0 + 14); cairo_show_text(c, "MODEL (not on the amp)");
    const Btn *B = buttons(); int tp = (int)std::lround(v[P_TAPER]);
    for (int k = 0; k < 3; k++) { const Btn &b = B[k]; bool on = b.p == P_TAPER ? tp > 0 : v[b.p] > 0.5; setrgb(c, 0x111317); cairo_rectangle(c, b.x, y0 + 22, b.w, 36); cairo_fill(c); setrgb(c, on ? 0x9a9aa0 : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_rectangle(c, b.x, y0 + 22, b.w, 36); cairo_stroke(c);
      setrgb(c, on ? 0xff2e1a : 0x47100e); cairo_new_path(c); cairo_arc(c, b.x + 16, y0 + 40, 6, 0, 2 * PI); cairo_fill(c);
      std::string txt = b.t; if (b.p == P_TAPER) txt = tp == 0 ? "POT TAPER LOG 15 %" : tp == 1 ? "POT TAPER LOG 10 %" : "POT TAPER LINEAR";
      setrgb(c, 0xececf0); textC(c, txt.c_str(), b.x + b.w / 2 + 8, y0 + 40, 14, true); }
    // Endstufe: Spitze vor der Begrenzung (dB bezogen auf die Grenze von 42 V; rot = übersteuert)
    { double mx = 924, mw = 300, my = y0 + 30, mh = 20; setrgb(c, 0xb8bcc4); font(c, 10, true); cairo_move_to(c, mx, y0 + 14); cairo_show_text(c, "POWER AMP  (peak before clipping, 0 dB = 42 V)");
      setrgb(c, 0x0f1114); cairo_rectangle(c, mx, my, mw, mh); cairo_fill(c);
      if (meterOn && meter > 0.05) { double db = 20 * std::log10(meter / V_LIM_GUI), f = std::max(0.0, std::min(1.0, (db + 40) / 40)); bool over = meter >= V_LIM_GUI;
        setrgb(c, over ? 0xff3b2a : db > -6 ? 0xe8b23c : 0x3ec25a); cairo_rectangle(c, mx, my, mw * f, mh); cairo_fill(c);
        char t[64]; if (over) std::snprintf(t, 64, "clipping: %.0f V asked, %.1fx over", meter, meter / V_LIM_GUI); else std::snprintf(t, 64, "%.1f V peak, %.1f dB below the limit", meter, -db);
        setrgb(c, over ? 0xffb3a8 : 0xcfd1d6); font(c, 11, false); cairo_move_to(c, mx, my + mh + 14); cairo_show_text(c, t); }
      else { setrgb(c, 0x6a6a68); font(c, 11, false); cairo_move_to(c, mx, my + mh + 14); cairo_show_text(c, pwr ? "silent" : "power amp off: output is the PA board (linear, relative level)"); }
      for (int k = 1; k < 4; k++) { setrgb(c, 0xffffff, 0.25); cairo_rectangle(c, mx + mw * k / 4 - 0.5, my, 1, mh); cairo_fill(c); } }
    // kleine Regler: Instrument (log) und Ausgang
    const char *sm[2] = {"INPUT", "OUTPUT"}; char vt[2][32]; std::snprintf(vt[0], 32, "%.0f mV at 0 dBFS", v[P_VIN]); std::snprintf(vt[1], 32, "%+.1f dB", v[P_OUT]);
    for (int i = 0; i < 2; i++) { double cx = 1300 + i * 240, cy = y0 + 40, u = i == 0 ? std::log(v[P_VIN] / 1) / std::log(1000) : (v[P_OUT] + 36) / 60;
      setrgb(c, 0x0c0c0c); cairo_new_path(c); cairo_arc(c, cx, cy, 24, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x3c3c3c); cairo_set_line_width(c, 1.5); cairo_arc(c, cx, cy, 24, 0, 2 * PI); cairo_stroke(c);
      cairo_save(c); cairo_translate(c, cx, cy); cairo_rotate(c, ang(u * 10)); setrgb(c, 0xf2f2ee); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -6); cairo_line_to(c, 0, -21); cairo_stroke(c); cairo_restore(c);
      if (drag == 9 + i || hover == 9 + i) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, cx, cy, 30, 0, 2 * PI); cairo_stroke(c); }
      bool dim = i == 0 && !pwr; setrgb(c, dim ? 0x6a6a68 : 0xcfd1d6); font(c, 13, true); cairo_move_to(c, cx + 34, cy - 4); cairo_show_text(c, sm[i]); font(c, 13, false); cairo_move_to(c, cx + 34, cy + 14); cairo_show_text(c, i == 0 && !pwr ? "(used with power amp)" : vt[i]); }
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); font(c, 11, false); cairo_move_to(c, 200, y0 + 12); cairo_show_text(c, d.c_str()); }
    // Frequenzgang (Kleinsignal, bezogen auf 1 kHz): Kurven bei Bedarf neu rechnen (höchstens alle 100 ms)
    { char kb[256]; int kn = 0; const int ks[] = {P_INP, P_VOL, P_T, P_M, P_B, P_E0, P_E1, P_E2, P_E3, P_E4, P_TAPER, P_SPK};
      for (int id : ks) kn += std::snprintf(kb + kn, sizeof kb - kn, "%.4g|", v[id]); std::string key(kb, kn); double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; } }
    { double ty0 = YFH + STRIP; setrgb(c, 0x14161a); cairo_rectangle(c, 0, ty0, YFW, PLOT); cairo_fill(c);
      double px0 = 70, px1 = YFW - 24, py0 = ty0 + 30, py1 = ty0 + PLOT - 40;
      setrgb(c, 0xb8bcc4); font(c, 12, true); cairo_move_to(c, px0, ty0 + 20); cairo_show_text(c, spk ? "FREQUENCY RESPONSE  (small signal, relative to 1 kHz, with speaker)" : "FREQUENCY RESPONSE  (small signal, relative to 1 kHz)");
      double lo = -25, hi = 15; if (!curve.empty()) { double mn = 1e9, mx = -1e9; for (double x : curve) { mn = std::min(mn, x); mx = std::max(mx, x); } for (double x : refCurve) { mn = std::min(mn, x); mx = std::max(mx, x); }
        lo = std::min(lo, std::floor((mn - 3) / 5) * 5); hi = std::max(hi, std::ceil((mx + 3) / 5) * 5); if (hi - lo < 40) { double add = 40 - (hi - lo); lo -= std::floor(add / 10) * 5; hi = lo + 40; } }
      double step = (hi - lo) > 60 ? 10 : 5; auto X = [&](double f) { return px0 + (px1 - px0) * std::log(f / 20) / std::log(1000.0); }; auto Y = [&](double d) { return py1 - (py1 - py0) * (d - lo) / (hi - lo); };
      cairo_set_line_width(c, 1); font(c, 11, false);
      for (double d = lo; d <= hi + 1e-9; d += step) { setrgb(c, d == 0 ? 0x5a5e66 : 0x2a2d33); cairo_move_to(c, px0, Y(d)); cairo_line_to(c, px1, Y(d)); cairo_stroke(c); setrgb(c, 0x8d919a); char t[16]; std::snprintf(t, 16, "%+.0f", d); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, px0 - 8 - e.width - e.x_bearing, Y(d) + 4); cairo_show_text(c, t); }
      const double fl[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000}; const char *fn[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
      for (double f = 20; f < 20000; f *= 10) for (int m = 1; m < 10; m++) { double ff = f * m; if (ff > 20000) break; setrgb(c, 0x24272c); cairo_move_to(c, X(ff), py0); cairo_line_to(c, X(ff), py1); cairo_stroke(c); }
      for (int i = 0; i < 10; i++) { setrgb(c, 0x3a3e46); cairo_move_to(c, X(fl[i]), py0); cairo_line_to(c, X(fl[i]), py1); cairo_stroke(c); setrgb(c, 0x8d919a); textC(c, fn[i], X(fl[i]), py1 + 14, 11, false); }
      // tiefes E und H des Basses
      { const double bf[2] = {41.2, 30.9}; const char *bn[2] = {"E", "B"}; for (int i = 0; i < 2; i++) { setrgb(c, 0x8d919a, 0.8); const double dash[] = {2, 4}; cairo_set_dash(c, dash, 2, 0); cairo_move_to(c, X(bf[i]), py0); cairo_line_to(c, X(bf[i]), py1); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); font(c, 10, false); cairo_move_to(c, X(bf[i]) + 4, py0 + 12); cairo_show_text(c, bn[i]); } }
      setrgb(c, 0x8d919a); textC(c, "Hz", (px0 + px1) / 2, py1 + 30, 11, false); cairo_save(c); cairo_translate(c, 22, (py0 + py1) / 2); cairo_rotate(c, -PI / 2); textC(c, "dB", 0, 0, 11, false); cairo_restore(c);
      if (!refCurve.empty()) { setrgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double dash[] = {8, 6}; cairo_set_dash(c, dash, 2, 0); cairo_new_path(c);
        for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(refCurve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); }
      if (!curve.empty()) { setrgb(c, 0x4da3ff); cairo_set_line_width(c, 3.5); cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND); cairo_new_path(c);
        for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(curve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); }
      setrgb(c, 0x4da3ff); cairo_set_line_width(c, 3.5); cairo_move_to(c, px1 - 640, ty0 + 16); cairo_line_to(c, px1 - 610, ty0 + 16); cairo_stroke(c); setrgb(c, 0xb8bcc4); font(c, 11, false); cairo_move_to(c, px1 - 602, ty0 + 20); cairo_show_text(c, "current setting");
      setrgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double dash2[] = {8, 6}; cairo_set_dash(c, dash2, 2, 0); cairo_move_to(c, px1 - 480, ty0 + 16); cairo_line_to(c, px1 - 450, ty0 + 16); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); setrgb(c, 0xb8bcc4); cairo_move_to(c, px1 - 442, ty0 + 20); cairo_show_text(c, "service setting: volume, treble, middle, bass 10, EQ flat (same jack, speaker)"); }
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[96]; double tx, ty;
      if (i < 9) { const YKnobGeo &g = YKG[i]; tx = g.x; ty = g.y - g.r * 1.3; knobText(i, v[KPARAM[i]], buf, 96); }
      else { tx = 1300 + (i - 9) * 240; ty = y0 + 4; std::snprintf(buf, 96, "%s", i == 9 ? vt[0] : vt[1]); }
      font(c, 15, true); cairo_text_extents_t e; cairo_text_extents(c, buf, &e); double bw = e.width + 16, bx = std::min(std::max(tx - bw / 2, 4.0), YFW - bw - 4), by = ty - 24;
      setrgb(c, 0xf4efe3); cairo_rectangle(c, bx, by, bw, 24); cairo_fill(c); setrgb(c, 0x111111); cairo_move_to(c, bx + 8 - e.x_bearing, by + 17); cairo_show_text(c, buf); }
    cairo_restore(c); cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy);
  }
  // ---- Treffer ----
  int hitKnob(double x, double y) const { double s = scale(); x /= s; y /= s; for (int i = 0; i < 9; i++) { double dx = x - YKG[i].x, dy = y - YKG[i].y; if (dx*dx + dy*dy <= YKG[i].r * YKG[i].r * 1.69) return i; }
    for (int i = 0; i < 2; i++) { double cx = 1300 + i * 240, cy = YFH + 46, dx = x - cx, dy = y - cy; if (dx*dx + dy*dy <= 30 * 30) return 9 + i; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; const double *J[2] = {YJACK_HIGH, YJACK_LOW}; for (int i = 0; i < 2; i++) { double dx = x - J[i][0], dy = y - J[i][1]; if (dx*dx + dy*dy <= (J[i][2] + 14) * (J[i][2] + 14)) return i; } return -1; }
  int hitButton(double x, double y) const { double s = scale(); x /= s; y /= s; if (y < YFH + 28 || y > YFH + 64) return -1; const Btn *B = buttons(); for (int i = 0; i < 3; i++) if (x >= B[i].x && x <= B[i].x + B[i].w) return B[i].p; return -1; }
  int knobParam(int i) const { return i < 9 ? KPARAM[i] : i == 9 ? P_VIN : P_OUT; }
  double knobVal(int i) const { if (i < 9) return knobPos(i, getP(KPARAM[i])); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : (getP(P_OUT) + 36) / 60 * 10; }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 9) setP(KPARAM[i], std::round(u * 10) / 10 - (i >= 4 ? 5 : 0)); else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
  // ---- Ereignisse: liefert true, wenn neu gezeichnet werden soll ----
  bool handle(XEvent &ev) {
    switch (ev.type) {
      case Expose: return true;
      case ButtonPress: { double x = ev.xbutton.x, y = ev.xbutton.y;
        if (ev.xbutton.button == 4 || ev.xbutton.button == 5) { int k = hitKnob(x, y); if (k >= 0) setKnob(k, knobVal(k) + (ev.xbutton.button == 4 ? 0.2 : -0.2)); return true; }
        if (ev.xbutton.button != 1) return false;
        int k = hitKnob(x, y); double now = ev.xbutton.time / 1000.0;
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35) { setKnob(k, k == 9 ? std::log(100.0) / std::log(1000.0) * 10 : k == 10 ? 6 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); if (gesture) gesture(knobParam(k), true); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int j = hitJack(x, y); if (j >= 0) { setP(P_INP, j == 0 ? 0 : 1); return true; }
        int b = hitButton(x, y); if (b >= 0) { if (b == P_TAPER) setP(P_TAPER, std::fmod(std::lround(getP(P_TAPER)) + 1, 3)); else setP(b, getP(b) > 0.5 ? 0 : 1); return true; }
        return false; }
      case MotionNotify: { double x = ev.xmotion.x, y = ev.xmotion.y;
        if (drag >= 0) { setKnob(drag, dragV0 + (dragY0 - y) / scale() * (10.0 / 250)); return true; }
        int h = hitKnob(x, y); bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case ButtonRelease: if (ev.xbutton.button == 1 && drag >= 0) { if (gesture) gesture(knobParam(drag), false); drag = -1; return true; } return false;
      case LeaveNotify: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) n += std::snprintf(b + n, sizeof b - n, "%.4g|", getP(i));
    n += std::snprintf(b + n, sizeof b - n, "%.2f|%d|%d|%d|%d|%d", meter, meterOn ? 1 : 0, drag, hover, showTip ? 1 : 0, W); std::string s(b, n); if (diag) s += diag(); return s; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
};
} // namespace ygui
