// Oberfläche des CLAP-Plugins: X11-Fenster (im Host eingebettet), gezeichnet mit Cairo. Foto der Frontplatte, Vektor-Aufdruck, drehbare Zeiger.
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
#include <ctime>
#include "geo_gen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "panel_jpg.h"

namespace aorgui {
constexpr double PI = 3.14159265358979323846;
// Parameterindizes wie im Plugin
enum { P_INP, P_CH, P_V1, P_V2, P_PL, P_T, P_M, P_B, P_MV, P_MID, P_TOP, P_BASS, P_UMBAU, P_PA, P_TAPER, P_OUT, P_NL, P_VIN, P_OS, P_PRES, P_TUBE1, P_TUBE2, P_TUBE3, P_SPK, P_COUNT };
static const char *TUBE_SHORT[5] = {"12AX7", "12AT7", "12AU7", "5751", "12AY7"};
static const int KPARAM[9] = {P_V1, P_V2, P_PL, P_MV, P_T, P_M, P_B, P_PRES, -1};   // Knopf → Parameter (Reverb: keiner)
static const int PULLP[9] = {P_CH, -1, -1, -1, P_TOP, P_MID, P_BASS, -1, -1};       // Zugschalter je Knopf
static const char *KNAME[9] = {"PREAMP 1 VOLUME","PREAMP 2 VOLUME","PREAMP 1 LEVEL","MASTER VOLUME","TREBLE","MIDDLE","BASS","PRESENCE","REVERB"};
constexpr double STRIP = 70;   // Modellstreifen unter dem Foto (in Foto-Einheiten)
constexpr double TUBEROW = 44;  // Zeile Röhrenbestückung
constexpr double PLOT = 330 + TUBEROW;   // Röhrenzeile + Frequenzgang-Diagramm unter dem Modellstreifen
static const double TOTALH = FH + STRIP + PLOT;
constexpr int NF = 120;        // Stützstellen des Diagramms (20 Hz … 20 kHz, logarithmisch)
inline double freqAt(int i) { return 20 * std::pow(1000.0, (double)i / (NF - 1)); }

struct Gui {
  Display *dpy = nullptr; Window win = 0, parent = 0; cairo_surface_t *xs = nullptr; cairo_surface_t *img = nullptr; int W = 0, H = 0;
  std::function<double(int)> getP; std::function<void(int,double)> setP; std::function<std::string()> diag;
  double meter[5] = {0,0,0,0,0};
  std::function<void(std::vector<double>&, std::vector<double>&)> computeCurves;   // Kurven in dB (bezogen auf 1 kHz): aktuell, Original/alle 5
  std::vector<double> curve, refCurve; std::string curveKey; double curveTime = 0;
  int drag = -1; double dragY0 = 0, dragV0 = 0; int hover = -1; bool showTip = false; double lastClick = 0; int lastClickId = -2;
  double scale() const { return (double)W / FW; }
  bool init(Display *d, Window par, int w, int h) {
    dpy = d; parent = par; W = w; H = h;
    int scr = DefaultScreen(dpy); XSetWindowAttributes a{}; a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask | StructureNotifyMask; a.background_pixmap = None;
    win = XCreateWindow(dpy, par ? par : RootWindow(dpy, scr), 0, 0, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask | CWBackPixmap, &a);
    xs = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, scr), w, h);
    if (!img) { int iw, ih, nch; unsigned char *px = stbi_load_from_memory(PANEL_JPG, (int)PANEL_JPG_LEN, &iw, &ih, &nch, 3);
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
  static double ang(double v) { return (-150 + 30 * v) * PI / 180; }
  void font(cairo_t *c, double px, bool bold = true) { cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL); cairo_set_font_size(c, px); }
  // Text in ein Feld der Breite w einpassen (wie textLength im Web)
  void fitText(cairo_t *c, const char *t, double x, double y, double w, double h) {
    font(c, h / 0.72); cairo_text_extents_t e; cairo_text_extents(c, t, &e); if (e.width <= 0) return;
    cairo_new_path(c); cairo_save(c); cairo_translate(c, x, y + h); cairo_scale(c, w / e.width, 1); cairo_move_to(c, -e.x_bearing, 0); cairo_show_text(c, t); cairo_restore(c);
  }
  void textC(cairo_t *c, const char *t, double x, double y, double px, bool bold = true) { font(c, px, bold); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, x - e.width / 2 - e.x_bearing, y + e.height / 2); cairo_show_text(c, t); }
  // ---- Zeichnen ----
  void draw() {
    if (!xs) return; cairo_t *c = cairo_create(xs); cairo_push_group(c); double s = scale(); double v[P_COUNT]; for (int i = 0; i < P_COUNT; i++) v[i] = getP(i);
    bool ch1 = v[P_CH] < 0.5, low = v[P_INP] > 0.5, pa = v[P_PA] > 0.5;
    setrgb(c, 0x0a0a0a); cairo_paint(c);
    cairo_save(c); cairo_scale(c, s, s);
    cairo_set_source_surface(c, img, 0, 0); cairo_rectangle(c, 0, 0, FW, FH); cairo_fill(c);
    // Aufdruck: Skalen
    setrgb(c, 0xd3d2cc); cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);
    for (int i = 0; i < 9; i++) { const KnobGeo &g = KG[i]; double rm = (SEG0 + SEG1) / 2 * g.R, sw = (SEG1 - SEG0) * g.R, capA = (sw / 2) / rm; cairo_set_line_width(c, sw);
      for (int q = 0; q < 5; q++) { double cen = (-120 + 60 * q) * PI / 180; cairo_new_sub_path(c); cairo_arc(c, g.rx, g.ry, rm, cen - SEGHA * PI / 180 + capA - PI / 2, cen + SEGHA * PI / 180 - capA - PI / 2); cairo_stroke(c); }
      double rd = (DIG0 + DIG1) / 2 * g.R, dh = (DIG1 - DIG0) * g.R; const char *digs[6] = {"0","2","4","6","8","10"};
      for (int q = 0; q < 6; q++) { double a = ang(q * 2); cairo_save(c); cairo_translate(c, g.rx + rd * std::sin(a), g.ry - rd * std::cos(a)); cairo_rotate(c, a); font(c, dh / 0.72, true);
        cairo_text_extents_t e; cairo_text_extents(c, digs[q], &e); cairo_move_to(c, -e.width / 2 - e.x_bearing, dh / 2); cairo_show_text(c, digs[q]); cairo_restore(c); } }
    // Zugschalter-Hintergründe und Beschriftungen
    for (auto &l : LG) { int pp = -1; if (!std::strcmp(l.t, "PULL ON")) pp = P_CH; else if (!std::strcmp(l.t, "PULL BOOST")) pp = !std::strcmp(l.k, "t") ? P_TOP : !std::strcmp(l.k, "m") ? P_MID : P_BASS;
      bool on = pp >= 0 && (pp == P_CH ? ch1 : v[pp] > 0.5);
      if (on) { setrgb(c, 0x8c1a10); cairo_rectangle(c, l.x - 8, l.y - 7, l.w + 16, l.h + 14); cairo_fill(c); setrgb(c, 0xffb3a8); cairo_set_line_width(c, 1.5); cairo_rectangle(c, l.x - 8, l.y - 7, l.w + 16, l.h + 14); cairo_stroke(c); }
      bool dim = (!std::strcmp(l.k, "v1") && low && pp < 0) || (!std::strcmp(l.k, "pl") && !ch1) || (!std::strcmp(l.k, "pres") && !pa) || !std::strcmp(l.k, "rev");
      setrgb(c, dim ? 0x6a6a68 : 0xd3d2cc); fitText(c, l.t, l.x, l.y, l.w, l.h); }
    setrgb(c, 0xd3d2cc); cairo_rectangle(c, PULLLINE[0], PULLLINE[2] - PULLLINE[3] / 2, PULLLINE[1] - PULLLINE[0], PULLLINE[3]); cairo_fill(c);
    // Kontrollleuchte und Power
    if (ch1) { cairo_pattern_t *g = cairo_pattern_create_radial(LED[0], LED[1], 0, LED[0], LED[1], LED[2] * 3.2); cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.95, 0.9, 1); cairo_pattern_add_color_stop_rgba(g, 0.25, 1, 0.42, 0.29, 1); cairo_pattern_add_color_stop_rgba(g, 0.5, 0.95, 0.12, 0.05, 0.9); cairo_pattern_add_color_stop_rgba(g, 1, 0.75, 0.07, 0.02, 0);
      cairo_set_source(c, g); cairo_new_path(c); cairo_arc(c, LED[0], LED[1], LED[2] * 3.2, 0, 2 * PI); cairo_fill(c); cairo_pattern_destroy(g); }
    { cairo_pattern_t *g = cairo_pattern_create_radial(POWER[0] + POWER[2] / 2, POWER[1] + POWER[3] * 0.45, 0, POWER[0] + POWER[2] / 2, POWER[1] + POWER[3] * 0.45, POWER[2] * 0.9);
      cairo_pattern_add_color_stop_rgba(g, 0, 1, 0.54, 0.42, 0.75); cairo_pattern_add_color_stop_rgba(g, 0.55, 1, 0.16, 0.06, 0.45); cairo_pattern_add_color_stop_rgba(g, 1, 0.88, 0.06, 0, 0.1);
      cairo_set_operator(c, CAIRO_OPERATOR_SCREEN); cairo_set_source(c, g); cairo_rectangle(c, POWER[0] - 8, POWER[1] - 8, POWER[2] + 16, POWER[3] + 16); cairo_fill(c); cairo_set_operator(c, CAIRO_OPERATOR_OVER); cairo_pattern_destroy(g); }
    // Buchsen
    const double *J[2] = {JACK_LOW, JACK_HIGH}; for (int i = 0; i < 2; i++) { bool act = (i == 0) == low; if (act) { setrgb(c, 0xe0382a); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, J[i][0], J[i][1], J[i][2] + 4, 0, 2 * PI); cairo_stroke(c); } }
    // Zeiger
    for (int i = 0; i < 9; i++) { const KnobGeo &g = KG[i]; double val = i == 8 ? 9.3 : v[KPARAM[i]]; double r0 = g.kind == 0 ? 3 : g.cap + 3, r1 = g.kind == 0 ? 27 : g.skirt - 2, lw = g.kind == 0 ? 7.5 : 5.5;
      cairo_save(c); cairo_translate(c, g.px, g.py); cairo_rotate(c, ang(val)); setrgb(c, g.kind == 0 ? 0xcfc5ae : 0xf2f2ee); cairo_set_line_width(c, lw); cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT); cairo_move_to(c, 0, -r0); cairo_line_to(c, 0, -r1); cairo_stroke(c); cairo_restore(c);
      bool off = (i == 2 && !ch1) || (i == 0 && low) || (i == 7 && !pa) || i == 8;
      if (off) { cairo_new_path(c); setrgb(c, 0x000000, 0.52); cairo_arc(c, g.rx, g.ry, g.R * 1.22, 0, 2 * PI); cairo_fill(c); }
      if (i == drag || i == hover) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_new_path(c); cairo_arc(c, g.rx, g.ry, g.R * 0.8, 0, 2 * PI); cairo_stroke(c); } }
    // Modellstreifen
    double y0 = FH + 6; setrgb(c, 0x191b1f); cairo_rectangle(c, 0, FH, FW, STRIP); cairo_fill(c);
    setrgb(c, 0x8d919a); font(c, 13, false); cairo_move_to(c, 14, y0 + 14); cairo_show_text(c, "MODEL (not on the amp)");
    struct Btn { const char *t; int p; double x, w; }; const Btn B[5] = {{"BASS MOD", P_UMBAU, 14, 190}, {"FX LOOP + POWER AMP", P_PA, 214, 290}, {"DISTORTION", P_NL, 514, 200}, {"", P_OS, 724, 230}, {"", P_TAPER, 964, 270}};
    for (auto &b : B) { bool on = b.p == P_OS ? true : b.p == P_TAPER ? v[P_TAPER] > 0.5 : v[b.p] > 0.5; setrgb(c, 0x111317); cairo_rectangle(c, b.x, y0 + 22, b.w, 36); cairo_fill(c); setrgb(c, on ? 0x9a9aa0 : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_rectangle(c, b.x, y0 + 22, b.w, 36); cairo_stroke(c);
      setrgb(c, on ? 0xff2e1a : 0x47100e); cairo_new_path(c); cairo_arc(c, b.x + 16, y0 + 40, 6, 0, 2 * PI); cairo_fill(c);
      std::string txt = b.t; int osv = (int)std::lround(v[P_OS]); if (b.p == P_OS) txt = osv == 0 ? "1X OVERSAMPLING" : osv == 1 ? "2X OVERSAMPLING" : osv == 2 ? "4X OVERSAMPLING" : "8X OVERSAMPLING"; int tp = (int)std::lround(v[P_TAPER]); if (b.p == P_TAPER) txt = tp == 0 ? "MID/PRES TAPER LOG 15 %" : tp == 1 ? "MID/PRES TAPER LOG 10 %" : "MID/PRES TAPER LINEAR";
      setrgb(c, 0xececf0); textC(c, txt.c_str(), b.x + b.w / 2 + 8, y0 + 40, 14, true); }
    // kleine Regler: Instrument (log) und Ausgang
    const char *sm[2] = {"INPUT", "OUTPUT"}; char vt[2][32]; std::snprintf(vt[0], 32, "%.0f mV", v[P_VIN]); std::snprintf(vt[1], 32, "%+.1f dB", v[P_OUT]);
    for (int i = 0; i < 2; i++) { double cx = 1890 + i * 280, cy = y0 + 40, u = i == 0 ? std::log(v[P_VIN] / 1) / std::log(1000) : (v[P_OUT] + 36) / 60;
      setrgb(c, 0x0c0c0c); cairo_new_path(c); cairo_arc(c, cx, cy, 24, 0, 2 * PI); cairo_fill(c); setrgb(c, 0x3c3c3c); cairo_set_line_width(c, 1.5); cairo_arc(c, cx, cy, 24, 0, 2 * PI); cairo_stroke(c);
      cairo_save(c); cairo_translate(c, cx, cy); cairo_rotate(c, ang(u * 10)); setrgb(c, 0xf2f2ee); cairo_set_line_width(c, 4); cairo_move_to(c, 0, -6); cairo_line_to(c, 0, -21); cairo_stroke(c); cairo_restore(c);
      if (drag == 9 + i || hover == 9 + i) { setrgb(c, 0xffd34d); cairo_set_line_width(c, 3); cairo_arc(c, cx, cy, 30, 0, 2 * PI); cairo_stroke(c); }
      setrgb(c, 0xcfd1d6); font(c, 13, true); cairo_move_to(c, cx + 34, cy - 4); cairo_show_text(c, sm[i]); font(c, 13, false); cairo_move_to(c, cx + 34, cy + 14); cairo_show_text(c, vt[i]); }
    setrgb(c, ch1 ? 0xff2e1a : 0x47100e); cairo_new_path(c); cairo_arc(c, 1760, y0 + 40, 7, 0, 2 * PI); cairo_fill(c); setrgb(c, 0xb8bcc4); textC(c, "PREAMP 1", 1760, y0 + 14, 11, true);
    // Röhrenaussteuerung: fünf Balken (1,0 = Übersteuerungseinsatz)
    { const char *tn[5] = {"V101A","V101B","V102A","V102B","V103B"}; double mx0 = 1262, bw = 60, gap = 12, bh = 34, by = y0 + 24;
      setrgb(c, 0xb8bcc4); font(c, 10, true); cairo_move_to(c, mx0, y0 + 12); cairo_show_text(c, "TUBE DRIVE");
      for (int i = 0; i < 5; i++) { double bx = mx0 + i * (bw + gap); setrgb(c, 0x0f1114); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c);
        double v = meter[i], f = std::min(1.0, std::log10(1 + 9 * std::min(v, 2.0) / 2) );   // logarithmische Skala, 2,0 = voll
        double h = bh * f; setrgb(c, v < 0.7 ? 0x3ec25a : v < 1.0 ? 0xe8b23c : 0xff3b2a); cairo_rectangle(c, bx, by + bh - h, bw, h); cairo_fill(c);
        double hm = bh * std::log10(1 + 9 * 0.5); setrgb(c, 0xffffff, 0.35); cairo_rectangle(c, bx, by + bh - hm - 0.5, bw, 1); cairo_fill(c);   // Marke 1,0
        setrgb(c, 0x8d919a); textC(c, tn[i], bx + bw / 2, by + bh + 7, 9, false); } }
    if (diag) { std::string d = diag(); setrgb(c, 0x8d919a); font(c, 11, false); cairo_move_to(c, 260, y0 + 12); cairo_show_text(c, d.c_str()); }
    // Frequenzgang (Kleinsignal, bezogen auf 1 kHz): Kurven bei Bedarf neu rechnen (höchstens alle 100 ms)
    { char kb[256]; int kn = 0; const int ks[] = {P_INP, P_CH, P_V1, P_V2, P_PL, P_T, P_M, P_B, P_MV, P_MID, P_TOP, P_BASS, P_UMBAU, P_PA, P_TAPER, P_PRES, P_TUBE1, P_TUBE2, P_TUBE3, P_SPK};
      for (int id : ks) kn += std::snprintf(kb + kn, sizeof kb - kn, "%.4g|", v[id]); std::string key(kb, kn); double now = (double)std::clock() / CLOCKS_PER_SEC;
      if (computeCurves && key != curveKey && (now - curveTime > 0.1 || curve.empty())) { computeCurves(curve, refCurve); curveKey = key; curveTime = now; } }
    { double ty0 = FH + STRIP; setrgb(c, 0x14161a); cairo_rectangle(c, 0, ty0, FW, PLOT); cairo_fill(c);
      // Röhrenbestückung: V101, V102, V103 (Taste schaltet den Typ durch), RESET = Bestückung wie im Gerät
      setrgb(c, 0xb8bcc4); font(c, 12, true); cairo_move_to(c, 70, ty0 + 28); cairo_show_text(c, "TUBES");
      for (int i = 0; i < 3; i++) { double bx = 150 + i * 300, by = ty0 + 8, bw = 270, bh = 30; int tt = std::max(0, std::min(4, (int)std::lround(v[P_TUBE1 + i]))); bool stock = tt == 0;
        setrgb(c, 0x111317); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, stock ? 0x4d4d52 : 0xe8b23c); cairo_set_line_width(c, 1.5); cairo_rectangle(c, bx, by, bw, bh); cairo_stroke(c);
        char lbl[48]; std::snprintf(lbl, sizeof lbl, "V10%d  %s%s", i + 1, TUBE_SHORT[tt], aor::TUBE_MODELS[tt].fitted ? " *" : ""); setrgb(c, stock ? 0xb8bcc4 : 0xffd98a); textC(c, lbl, bx + bw / 2, by + bh / 2, 13, true); }
      { bool anyMod = v[P_TUBE1] > 0.5 || v[P_TUBE2] > 0.5 || v[P_TUBE3] > 0.5; double bx = 1060, by = ty0 + 8, bw = 150, bh = 30; setrgb(c, 0x111317); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, anyMod ? 0x9a9aa0 : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_rectangle(c, bx, by, bw, bh); cairo_stroke(c); setrgb(c, anyMod ? 0xececf0 : 0x6a6a68); textC(c, "RESET TO STOCK", bx + bw / 2, by + bh / 2, 12, true); }
      setrgb(c, 0x8d919a); font(c, 11, false); cairo_move_to(c, 1230, ty0 + 28); cairo_show_text(c, "twin triodes; * = model fitted to data sheet, less certain off that point");
      // Lautsprecher: 15" Eminence Legend CB158 im Bassreflexgehäuse der Portaflex-Größe (Thiele-Small aus dem Datenblatt)
      { bool on = v[P_SPK] > 0.5; double bx = 1760, by = ty0 + 8, bw = 420, bh = 30; setrgb(c, 0x111317); cairo_rectangle(c, bx, by, bw, bh); cairo_fill(c); setrgb(c, on ? 0xe8b23c : 0x4d4d52); cairo_set_line_width(c, 1.5); cairo_rectangle(c, bx, by, bw, bh); cairo_stroke(c); setrgb(c, on ? 0xffd98a : 0xb8bcc4); textC(c, on ? "SPEAKER  15\" CB158 · 78 L VENTED · ON" : "SPEAKER  15\" CB158 · 78 L VENTED · OFF", bx + bw / 2, by + bh / 2, 13, true); }
      double px0 = 70, px1 = FW - 24, py0 = ty0 + TUBEROW + 30, py1 = FH + STRIP + PLOT - 40;
      setrgb(c, 0xb8bcc4); font(c, 12, true); cairo_move_to(c, px0, ty0 + TUBEROW + 20); cairo_show_text(c, v[P_SPK] > 0.5 ? "FREQUENCY RESPONSE  (small signal, relative to 1 kHz, with speaker)" : "FREQUENCY RESPONSE  (small signal, relative to 1 kHz)");
      double lo = -25, hi = 15; if (!curve.empty()) { double mn = 1e9, mx = -1e9; for (double x : curve) { mn = std::min(mn, x); mx = std::max(mx, x); } for (double x : refCurve) { mn = std::min(mn, x); mx = std::max(mx, x); }
        lo = std::min(lo, std::floor((mn - 3) / 5) * 5); hi = std::max(hi, std::ceil((mx + 3) / 5) * 5); if (hi - lo < 40) { double add = 40 - (hi - lo); lo -= std::floor(add / 10) * 5; hi = lo + 40; } }
      double step = (hi - lo) > 60 ? 10 : 5; auto X = [&](double f) { return px0 + (px1 - px0) * std::log(f / 20) / std::log(1000.0); }; auto Y = [&](double d) { return py1 - (py1 - py0) * (d - lo) / (hi - lo); };
      cairo_set_line_width(c, 1); font(c, 11, false);
      for (double d = lo; d <= hi + 1e-9; d += step) { setrgb(c, d == 0 ? 0x5a5e66 : 0x2a2d33); cairo_move_to(c, px0, Y(d)); cairo_line_to(c, px1, Y(d)); cairo_stroke(c); setrgb(c, 0x8d919a); char t[16]; std::snprintf(t, 16, "%+.0f", d); cairo_text_extents_t e; cairo_text_extents(c, t, &e); cairo_move_to(c, px0 - 8 - e.width - e.x_bearing, Y(d) + 4); cairo_show_text(c, t); }
      const double fl[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000}; const char *fn[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
      for (double f = 20; f < 20000; f *= 10) for (int m = 1; m < 10; m++) { double ff = f * m; if (ff > 20000) break; setrgb(c, 0x24272c); cairo_move_to(c, X(ff), py0); cairo_line_to(c, X(ff), py1); cairo_stroke(c); }
      for (int i = 0; i < 10; i++) { setrgb(c, 0x3a3e46); cairo_move_to(c, X(fl[i]), py0); cairo_line_to(c, X(fl[i]), py1); cairo_stroke(c); setrgb(c, 0x8d919a); textC(c, fn[i], X(fl[i]), py1 + 14, 11, false); }
      setrgb(c, 0x8d919a); textC(c, "Hz", (px0 + px1) / 2, py1 + 30, 11, false); cairo_save(c); cairo_translate(c, 22, (py0 + py1) / 2); cairo_rotate(c, -PI / 2); textC(c, "dB", 0, 0, 11, false); cairo_restore(c);
      if (!refCurve.empty()) { setrgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double dash[] = {8, 6}; cairo_set_dash(c, dash, 2, 0); cairo_new_path(c);
        for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(refCurve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); }
      if (!curve.empty()) { setrgb(c, 0x4da3ff); cairo_set_line_width(c, 3.5); cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND); cairo_new_path(c);
        for (int i = 0; i < NF; i++) { double y = std::min(py1, std::max(py0, Y(curve[i]))); if (i == 0) cairo_move_to(c, X(freqAt(i)), y); else cairo_line_to(c, X(freqAt(i)), y); } cairo_stroke(c); }
      setrgb(c, 0x4da3ff); cairo_set_line_width(c, 3.5); cairo_move_to(c, px1 - 420, ty0 + TUBEROW + 16); cairo_line_to(c, px1 - 390, ty0 + TUBEROW + 16); cairo_stroke(c); setrgb(c, 0xb8bcc4); font(c, 11, false); cairo_move_to(c, px1 - 382, ty0 + TUBEROW + 20); cairo_show_text(c, "current setting");
      setrgb(c, 0x8d919a); cairo_set_line_width(c, 2); const double dash2[] = {8, 6}; cairo_set_dash(c, dash2, 2, 0); cairo_move_to(c, px1 - 250, ty0 + TUBEROW + 16); cairo_line_to(c, px1 - 220, ty0 + TUBEROW + 16); cairo_stroke(c); cairo_set_dash(c, nullptr, 0, 0); setrgb(c, 0xb8bcc4); cairo_move_to(c, px1 - 212, ty0 + TUBEROW + 20); cairo_show_text(c, "original parts, stock tubes, all controls 5, boosts off"); }
    // Wertanzeige
    if (showTip && (drag >= 0 || hover >= 0)) { int i = drag >= 0 ? drag : hover; char buf[96]; double tx, ty;
      if (i < 9) { const KnobGeo &g = KG[i]; tx = g.rx; ty = g.ry - g.R * 1.3; if (i == 8) std::snprintf(buf, 96, "REVERB: not modelled"); else if (i == 7 && !pa) std::snprintf(buf, 96, "PRESENCE %.1f (needs FX loop + power amp)", v[P_PRES]); else std::snprintf(buf, 96, "%s %.1f", KNAME[i], v[KPARAM[i]]); }
      else { tx = 1890 + (i - 9) * 280; ty = y0 + 4; std::snprintf(buf, 96, "%s", i == 9 ? vt[0] : vt[1]); }
      font(c, 15, true); cairo_text_extents_t e; cairo_text_extents(c, buf, &e); double bw = e.width + 16, bx = std::min(std::max(tx - bw / 2, 4.0), FW - bw - 4), by = ty - 24;
      setrgb(c, 0xf4efe3); cairo_rectangle(c, bx, by, bw, 24); cairo_fill(c); setrgb(c, 0x111111); cairo_move_to(c, bx + 8 - e.x_bearing, by + 17); cairo_show_text(c, buf); }
    cairo_restore(c); cairo_pop_group_to_source(c); cairo_set_operator(c, CAIRO_OPERATOR_SOURCE); cairo_paint(c); cairo_destroy(c); cairo_surface_flush(xs); XFlush(dpy);
  }
  // ---- Treffer ----
  int hitKnob(double x, double y) const { double s = scale(); x /= s; y /= s; for (int i = 0; i < 9; i++) { double dx = x - KG[i].rx, dy = y - KG[i].ry; if (dx*dx + dy*dy <= KG[i].R * KG[i].R * 1.1) return i; }
    for (int i = 0; i < 2; i++) { double cx = 1890 + i * 280, cy = FH + 46, dx = x - cx, dy = y - cy; if (dx*dx + dy*dy <= 30 * 30) return 9 + i; } return -1; }
  int hitPull(double x, double y) const { double s = scale(); x /= s; y /= s; for (auto &l : LG) { int pp = -1; if (!std::strcmp(l.t, "PULL ON")) pp = P_CH; else if (!std::strcmp(l.t, "PULL BOOST")) pp = !std::strcmp(l.k, "t") ? P_TOP : !std::strcmp(l.k, "m") ? P_MID : P_BASS;
      if (pp >= 0 && x >= l.x - 8 && x <= l.x + l.w + 8 && y >= l.y - 10 && y <= l.y + l.h + 10) return pp; } return -1; }
  int hitJack(double x, double y) const { double s = scale(); x /= s; y /= s; const double *J[2] = {JACK_HIGH, JACK_LOW}; for (int i = 0; i < 2; i++) { double dx = x - J[i][0], dy = y - J[i][1]; if (dx*dx + dy*dy <= (J[i][2] + 14) * (J[i][2] + 14)) return i; } return -1; }
  int hitTube(double x, double y) const { double s = scale(); x /= s; y /= s; double ty0 = FH + STRIP; if (y < ty0 + 8 || y > ty0 + 38) return -1; for (int i = 0; i < 3; i++) if (x >= 150 + i * 300 && x <= 420 + i * 300) return i; if (x >= 1060 && x <= 1210) return 3; if (x >= 1760 && x <= 2180) return 4; return -1; }
  int hitButton(double x, double y) const { double s = scale(); x /= s; y /= s; if (y < FH + 28 || y > FH + 64) return -1; const double bx[5] = {14, 214, 514, 724, 964}, bw[5] = {190, 290, 200, 230, 270}; const int bp[5] = {P_UMBAU, P_PA, P_NL, P_OS, P_TAPER};
    for (int i = 0; i < 5; i++) if (x >= bx[i] && x <= bx[i] + bw[i]) return bp[i]; return -1; }
  double knobVal(int i) const { if (i < 9) return i == 8 ? 9.3 : getP(KPARAM[i]); return i == 9 ? std::log(getP(P_VIN)) / std::log(1000) * 10 : (getP(P_OUT) + 36) / 60 * 10; }
  void setKnob(int i, double u) { u = std::max(0.0, std::min(10.0, u)); if (i < 9) { if (KPARAM[i] >= 0) setP(KPARAM[i], std::round(u * 10) / 10); } else if (i == 9) setP(P_VIN, std::round(std::pow(1000, u / 10))); else setP(P_OUT, std::round((u / 10 * 60 - 36) * 2) / 2); }
  // ---- Ereignisse: liefert true, wenn neu gezeichnet werden soll ----
  bool handle(XEvent &ev) {
    switch (ev.type) {
      case Expose: return true;
      case ButtonPress: { double x = ev.xbutton.x, y = ev.xbutton.y;
        if (ev.xbutton.button == 4 || ev.xbutton.button == 5) { int k = hitKnob(x, y); if (k >= 0 && k != 8) setKnob(k, knobVal(k) + (ev.xbutton.button == 4 ? 0.2 : -0.2)); return true; }
        if (ev.xbutton.button != 1) return false;
        int k = hitKnob(x, y); double now = ev.xbutton.time / 1000.0;
        if (k >= 0) { if (k == lastClickId && now - lastClick < 0.35 && k != 8) { setKnob(k, k == 9 ? std::log(100.0) / std::log(1000.0) * 10 : k == 10 ? 6 : 5); } else { drag = k; dragY0 = y; dragV0 = knobVal(k); } showTip = true; lastClick = now; lastClickId = k; return true; }
        int pp = hitPull(x, y); if (pp >= 0) { setP(pp, getP(pp) > 0.5 ? 0 : 1); return true; }
        int j = hitJack(x, y); if (j >= 0) { setP(P_INP, j == 0 ? 0 : 1); return true; }
        int tb2 = hitTube(x, y); if (tb2 == 3) { for (int i = 0; i < 3; i++) setP(P_TUBE1 + i, 0); return true; } if (tb2 == 4) { setP(P_SPK, getP(P_SPK) > 0.5 ? 0 : 1); return true; } if (tb2 >= 0) { setP(P_TUBE1 + tb2, std::fmod(std::lround(getP(P_TUBE1 + tb2)) + 1, 5)); return true; }
        int b = hitButton(x, y); if (b >= 0) { if (b == P_OS) setP(P_OS, std::fmod(std::lround(getP(P_OS)) + 1, 4)); else if (b == P_TAPER) setP(P_TAPER, std::fmod(std::lround(getP(P_TAPER)) + 1, 3)); else setP(b, getP(b) > 0.5 ? 0 : 1); return true; }
        return false; }
      case MotionNotify: { double x = ev.xmotion.x, y = ev.xmotion.y;
        if (drag >= 0 && drag != 8) { setKnob(drag, dragV0 + (dragY0 - y) / scale() * (10.0 / 250)); return true; }
        int h = hitKnob(x, y); bool changed = h != hover; hover = h; showTip = h >= 0; return changed; }
      case ButtonRelease: if (ev.xbutton.button == 1 && drag >= 0) { drag = -1; return true; } return false;
      case LeaveNotify: { bool changed = hover >= 0; hover = -1; if (drag < 0) showTip = false; return changed; }
      default: return false;
    }
  }
  std::string lastSig;
  std::string signature() const { char b[512]; int n = 0; for (int i = 0; i < P_COUNT; i++) n += std::snprintf(b + n, sizeof b - n, "%.4g|", getP(i));
    for (int i = 0; i < 5; i++) n += std::snprintf(b + n, sizeof b - n, "%.2f|", meter[i]); n += std::snprintf(b + n, sizeof b - n, "%d|%d|%d|%d", drag, hover, showTip ? 1 : 0, W); std::string s(b, n); if (diag) s += diag(); return s; }
  void pump() { if (!dpy) return; bool redraw = false; while (XPending(dpy)) { XEvent ev; XNextEvent(dpy, &ev); if (ev.xany.window == win && handle(ev)) redraw = true; }
    if (!win) return; std::string sig = signature(); if (redraw || sig != lastSig) { lastSig = sig; draw(); } }
};
} // namespace aorgui
