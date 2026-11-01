// Kleiner Ersatz für die Cairo-Funktionen der Plugin-Oberflächen auf macOS, umgesetzt mit CoreGraphics und CoreText.
// Nur der Umfang, den gui.hpp, ygui.hpp und widgets.hpp benutzen; Verhalten wie Cairo (Pfade im Benutzerraum, Winkel im
// Uhrzeigersinn bei nach unten zeigender y-Achse, Verläufe mit PAD-Fortsetzung, Text an der Grundlinie des aktuellen Punkts).
// Der Zeichenkontext kommt von der NSView (gespiegelt: y nach unten), siehe macview.mm.
#pragma once
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef enum { CAIRO_FONT_SLANT_NORMAL } cairo_font_slant_t;
typedef enum { CAIRO_FONT_WEIGHT_NORMAL, CAIRO_FONT_WEIGHT_BOLD } cairo_font_weight_t;
typedef enum { CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24 } cairo_format_t;
typedef enum { CAIRO_LINE_CAP_BUTT, CAIRO_LINE_CAP_ROUND, CAIRO_LINE_CAP_SQUARE } cairo_line_cap_t;
typedef enum { CAIRO_LINE_JOIN_MITER, CAIRO_LINE_JOIN_ROUND, CAIRO_LINE_JOIN_BEVEL } cairo_line_join_t;
typedef enum { CAIRO_OPERATOR_OVER, CAIRO_OPERATOR_SOURCE, CAIRO_OPERATOR_SCREEN } cairo_operator_t;
struct cairo_text_extents_t { double x_bearing, y_bearing, width, height, x_advance, y_advance; };

struct cairo_surface_t { int w = 0, h = 0, stride = 0; unsigned char *data = nullptr; CGImageRef img = nullptr; bool dirty = true; };
struct cairo_pattern_t { int refs = 1; int kind = 0; double x0 = 0, y0 = 0, r0 = 0, x1 = 0, y1 = 0, r1 = 0; std::vector<double> stops; /* je Stopp: offset r g b a */ };
struct CairoCgSource { int kind = 0; double rgba[4] = {0, 0, 0, 1}; cairo_pattern_t *pat = nullptr; cairo_surface_t *surf = nullptr; double sx = 0, sy = 0; };   // kind: 0 Farbe, 1 Verlauf, 2 Bild, 3 Gruppe (nichts)
struct CairoCgState { CairoCgSource src; double fontSize = 10; bool bold = false; cairo_operator_t op = CAIRO_OPERATOR_OVER; };
struct cairo_t { CGContextRef cg; CGAffineTransform base; std::vector<CairoCgState> st; bool newSub = false; };

inline cairo_pattern_t *cairo_pattern_reference(cairo_pattern_t *p) { if (p) p->refs++; return p; }
inline void cairo_pattern_destroy(cairo_pattern_t *p) { if (p && --p->refs == 0) delete p; }
inline CairoCgState &cairo_cg_top(cairo_t *c) { return c->st.back(); }
inline void cairo_cg_setsrc(cairo_t *c, const CairoCgSource &s) { CairoCgSource &d = cairo_cg_top(c).src; if (d.pat) cairo_pattern_destroy(d.pat); d = s; if (d.pat) cairo_pattern_reference(d.pat); }

// Kontext aus einem CoreGraphics-Kontext (der Aufrufer besitzt cg)
inline cairo_t *cairo_cg_create(CGContextRef cg) { cairo_t *c = new cairo_t; c->cg = cg; c->base = CGContextGetCTM(cg); c->st.push_back(CairoCgState());
  CGContextSetTextMatrix(cg, CGAffineTransformMakeScale(1, -1)); CGContextSetInterpolationQuality(cg, kCGInterpolationHigh); CGContextSetShouldAntialias(cg, true); return c; }
inline void cairo_destroy(cairo_t *c) { if (!c) return; for (auto &s : c->st) if (s.src.pat) cairo_pattern_destroy(s.src.pat); delete c; }
inline void cairo_save(cairo_t *c) { CGContextSaveGState(c->cg); CairoCgState s = c->st.back(); if (s.src.pat) cairo_pattern_reference(s.src.pat); c->st.push_back(s); }
inline void cairo_restore(cairo_t *c) { if (c->st.size() <= 1) return; if (c->st.back().src.pat) cairo_pattern_destroy(c->st.back().src.pat); c->st.pop_back(); CGContextRestoreGState(c->cg); }
inline void cairo_push_group(cairo_t *) {}   // CoreGraphics puffert selbst; die Gruppe wird nur am Ende komplett gemalt
inline void cairo_pop_group_to_source(cairo_t *c) { CairoCgSource s; s.kind = 3; cairo_cg_setsrc(c, s); }
inline void cairo_set_operator(cairo_t *c, cairo_operator_t op) { cairo_cg_top(c).op = op;
  CGContextSetBlendMode(c->cg, op == CAIRO_OPERATOR_SCREEN ? kCGBlendModeScreen : op == CAIRO_OPERATOR_SOURCE ? kCGBlendModeCopy : kCGBlendModeNormal); }
// Transformationen
inline void cairo_translate(cairo_t *c, double x, double y) { CGContextTranslateCTM(c->cg, x, y); }
inline void cairo_scale(cairo_t *c, double x, double y) { CGContextScaleCTM(c->cg, x, y); }
inline void cairo_rotate(cairo_t *c, double a) { CGContextRotateCTM(c->cg, a); }
inline void cairo_identity_matrix(cairo_t *c) { CGAffineTransform cur = CGContextGetCTM(c->cg); CGContextConcatCTM(c->cg, CGAffineTransformConcat(c->base, CGAffineTransformInvert(cur))); }
// Pfade
inline void cairo_new_path(cairo_t *c) { CGContextBeginPath(c->cg); c->newSub = false; }
inline void cairo_new_sub_path(cairo_t *c) { c->newSub = true; }
inline void cairo_move_to(cairo_t *c, double x, double y) { CGContextMoveToPoint(c->cg, x, y); c->newSub = false; }
inline void cairo_line_to(cairo_t *c, double x, double y) { if (CGContextIsPathEmpty(c->cg) || c->newSub) CGContextMoveToPoint(c->cg, x, y); else CGContextAddLineToPoint(c->cg, x, y); c->newSub = false; }
inline void cairo_curve_to(cairo_t *c, double x1, double y1, double x2, double y2, double x3, double y3) { if (CGContextIsPathEmpty(c->cg) || c->newSub) CGContextMoveToPoint(c->cg, x1, y1); CGContextAddCurveToPoint(c->cg, x1, y1, x2, y2, x3, y3); c->newSub = false; }
inline void cairo_close_path(cairo_t *c) { CGContextClosePath(c->cg); }
inline void cairo_rectangle(cairo_t *c, double x, double y, double w, double h) { CGContextAddRect(c->cg, CGRectMake(x, y, w, h)); c->newSub = false; }
inline void cairo_arc(cairo_t *c, double xc, double yc, double r, double a1, double a2) {
  while (a2 < a1) a2 += 2 * M_PI;
  if (c->newSub || CGContextIsPathEmpty(c->cg)) CGContextMoveToPoint(c->cg, xc + r * std::cos(a1), yc + r * std::sin(a1));
  CGContextAddArc(c->cg, xc, yc, r, a1, a2, 0); c->newSub = false; }
// Linien
inline void cairo_set_line_width(cairo_t *c, double w) { CGContextSetLineWidth(c->cg, w); }
inline void cairo_set_line_cap(cairo_t *c, cairo_line_cap_t k) { CGContextSetLineCap(c->cg, k == CAIRO_LINE_CAP_ROUND ? kCGLineCapRound : k == CAIRO_LINE_CAP_SQUARE ? kCGLineCapSquare : kCGLineCapButt); }
inline void cairo_set_line_join(cairo_t *c, cairo_line_join_t k) { CGContextSetLineJoin(c->cg, k == CAIRO_LINE_JOIN_ROUND ? kCGLineJoinRound : k == CAIRO_LINE_JOIN_BEVEL ? kCGLineJoinBevel : kCGLineJoinMiter); }
inline void cairo_set_dash(cairo_t *c, const double *d, int n, double off) { if (n <= 0) { CGContextSetLineDash(c->cg, 0, nullptr, 0); return; }
  std::vector<CGFloat> v(d, d + n); CGContextSetLineDash(c->cg, off, v.data(), n); }
// Quellen
inline void cairo_set_source_rgba(cairo_t *c, double r, double g, double b, double a) { CairoCgSource s; s.kind = 0; s.rgba[0] = r; s.rgba[1] = g; s.rgba[2] = b; s.rgba[3] = a; cairo_cg_setsrc(c, s);
  CGContextSetRGBFillColor(c->cg, r, g, b, a); CGContextSetRGBStrokeColor(c->cg, r, g, b, a); }
inline void cairo_set_source_rgb(cairo_t *c, double r, double g, double b) { cairo_set_source_rgba(c, r, g, b, 1); }
inline cairo_pattern_t *cairo_pattern_create_linear(double x0, double y0, double x1, double y1) { cairo_pattern_t *p = new cairo_pattern_t; p->kind = 1; p->x0 = x0; p->y0 = y0; p->x1 = x1; p->y1 = y1; return p; }
inline cairo_pattern_t *cairo_pattern_create_radial(double x0, double y0, double r0, double x1, double y1, double r1) { cairo_pattern_t *p = new cairo_pattern_t; p->kind = 2;
  p->x0 = x0; p->y0 = y0; p->r0 = r0; p->x1 = x1; p->y1 = y1; p->r1 = r1; return p; }
inline void cairo_pattern_add_color_stop_rgba(cairo_pattern_t *p, double o, double r, double g, double b, double a) { double v[5] = {o, r, g, b, a}; p->stops.insert(p->stops.end(), v, v + 5); }
inline void cairo_pattern_add_color_stop_rgb(cairo_pattern_t *p, double o, double r, double g, double b) { cairo_pattern_add_color_stop_rgba(p, o, r, g, b, 1); }
inline void cairo_set_source(cairo_t *c, cairo_pattern_t *p) { CairoCgSource s; s.kind = 1; s.pat = p; cairo_cg_setsrc(c, s); }
// Bilder (RGB24/ARGB32 wie Cairo: 32 Bit je Bildpunkt, Byte-Reihenfolge B G R A)
inline cairo_surface_t *cairo_image_surface_create(cairo_format_t, int w, int h) { cairo_surface_t *s = new cairo_surface_t; s->w = w; s->h = h; s->stride = w * 4;
  s->data = (unsigned char*)std::calloc((size_t)s->stride * h, 1); return s; }
inline unsigned char *cairo_image_surface_get_data(cairo_surface_t *s) { s->dirty = true; return s->data; }
inline int cairo_image_surface_get_stride(cairo_surface_t *s) { return s->stride; }
inline void cairo_surface_mark_dirty(cairo_surface_t *s) { s->dirty = true; }
inline void cairo_surface_flush(cairo_surface_t *) {}
inline void cairo_surface_destroy(cairo_surface_t *s) { if (!s) return; if (s->img) CGImageRelease(s->img); std::free(s->data); delete s; }
inline CGImageRef cairo_cg_image(cairo_surface_t *s) {
  if (s->img && !s->dirty) return s->img; if (s->img) { CGImageRelease(s->img); s->img = nullptr; }
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB(); CGDataProviderRef dp = CGDataProviderCreateWithData(nullptr, s->data, (size_t)s->stride * s->h, nullptr);
  s->img = CGImageCreate(s->w, s->h, 8, 32, s->stride, cs, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little, dp, nullptr, true, kCGRenderingIntentDefault);
  CGDataProviderRelease(dp); CGColorSpaceRelease(cs); s->dirty = false; return s->img; }
inline void cairo_set_source_surface(cairo_t *c, cairo_surface_t *s, double x, double y) { CairoCgSource src; src.kind = 2; src.surf = s; src.sx = x; src.sy = y; cairo_cg_setsrc(c, src); }
// Füllen mit der aktuellen Quelle innerhalb des aktuellen Pfads (Pfad wird verbraucht)
inline void cairo_cg_paintsrc(cairo_t *c) {   // malt die Quelle in den aktuellen Beschnitt
  CairoCgSource &s = cairo_cg_top(c).src; CGContextRef g = c->cg;
  if (s.kind == 2 && s.surf) { CGImageRef im = cairo_cg_image(s.surf); if (!im) return; CGContextSaveGState(g); CGContextTranslateCTM(g, s.sx, s.sy + s.surf->h); CGContextScaleCTM(g, 1, -1);
    CGContextDrawImage(g, CGRectMake(0, 0, s.surf->w, s.surf->h), im); CGContextRestoreGState(g); return; }
  if (s.kind == 1 && s.pat && !s.pat->stops.empty()) { cairo_pattern_t *p = s.pat; size_t n = p->stops.size() / 5; std::vector<CGFloat> comp, loc;
    for (size_t i = 0; i < n; i++) { loc.push_back(p->stops[i * 5]); for (int k = 1; k < 5; k++) comp.push_back(p->stops[i * 5 + k]); }
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB(); CGGradientRef gr = CGGradientCreateWithColorComponents(cs, comp.data(), loc.data(), n); CGColorSpaceRelease(cs);
    const CGGradientDrawingOptions o = kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation;
    if (p->kind == 1) CGContextDrawLinearGradient(g, gr, CGPointMake(p->x0, p->y0), CGPointMake(p->x1, p->y1), o);
    else CGContextDrawRadialGradient(g, gr, CGPointMake(p->x0, p->y0), p->r0, CGPointMake(p->x1, p->y1), p->r1, o);
    CGGradientRelease(gr); return; }
  if (s.kind == 0) { CGRect r = CGContextGetClipBoundingBox(g); CGContextFillRect(g, r); } }
inline void cairo_cg_fill(cairo_t *c, bool keep) { CGContextRef g = c->cg; CairoCgSource &s = cairo_cg_top(c).src; if (CGContextIsPathEmpty(g)) return;
  CGPathRef path = keep ? CGContextCopyPath(g) : nullptr;
  if (s.kind == 3) CGContextBeginPath(g);
  else if (s.kind == 0) CGContextFillPath(g);
  else { CGContextSaveGState(g); CGContextClip(g); cairo_cg_paintsrc(c); CGContextRestoreGState(g); }
  if (path) { CGContextAddPath(g, path); CGPathRelease(path); } c->newSub = false; }
inline void cairo_fill(cairo_t *c) { cairo_cg_fill(c, false); }
inline void cairo_fill_preserve(cairo_t *c) { cairo_cg_fill(c, true); }
inline void cairo_stroke(cairo_t *c) { CGContextRef g = c->cg; CairoCgSource &s = cairo_cg_top(c).src; if (CGContextIsPathEmpty(g)) return;
  if (s.kind == 0) CGContextStrokePath(g);
  else if (s.kind == 3) CGContextBeginPath(g);
  else { CGContextSaveGState(g); CGContextReplacePathWithStrokedPath(g); CGContextClip(g); cairo_cg_paintsrc(c); CGContextRestoreGState(g); } c->newSub = false; }
inline void cairo_paint(cairo_t *c) { CairoCgSource &s = cairo_cg_top(c).src; if (s.kind == 3) return; CGContextSaveGState(c->cg); cairo_cg_paintsrc(c); CGContextRestoreGState(c->cg); }
// Text: Systemschrift (San Francisco), fett oder normal
inline void cairo_select_font_face(cairo_t *c, const char *, cairo_font_slant_t, cairo_font_weight_t w) { cairo_cg_top(c).bold = w == CAIRO_FONT_WEIGHT_BOLD; }
inline void cairo_set_font_size(cairo_t *c, double px) { cairo_cg_top(c).fontSize = px; }
inline CTLineRef cairo_cg_line(cairo_t *c, const char *t) {
  CTFontRef f = CTFontCreateUIFontForLanguage(cairo_cg_top(c).bold ? kCTFontUIFontEmphasizedSystem : kCTFontUIFontSystem, cairo_cg_top(c).fontSize, nullptr);
  CFStringRef str = CFStringCreateWithCString(nullptr, t, kCFStringEncodingUTF8); if (!str) str = CFStringCreateWithCString(nullptr, "", kCFStringEncodingUTF8);
  const void *keys[2] = {kCTFontAttributeName, kCTForegroundColorFromContextAttributeName}; const void *vals[2] = {f, kCFBooleanTrue};
  CFDictionaryRef at = CFDictionaryCreate(nullptr, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFAttributedStringRef as = CFAttributedStringCreate(nullptr, str, at); CTLineRef line = CTLineCreateWithAttributedString(as);
  CFRelease(as); CFRelease(at); CFRelease(str); CFRelease(f); return line; }
inline void cairo_text_extents(cairo_t *c, const char *t, cairo_text_extents_t *e) { CTLineRef line = cairo_cg_line(c, t);
  CGRect r = CTLineGetBoundsWithOptions(line, kCTLineBoundsUseGlyphPathBounds); double adv = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
  e->x_bearing = r.origin.x; e->width = r.size.width; e->height = r.size.height; e->y_bearing = -(r.origin.y + r.size.height); e->x_advance = adv; e->y_advance = 0; CFRelease(line); }
inline void cairo_show_text(cairo_t *c, const char *t) { CGContextRef g = c->cg; CGPoint p = CGContextIsPathEmpty(g) ? CGPointZero : CGContextGetPathCurrentPoint(g);
  CTLineRef line = cairo_cg_line(c, t); CGContextSaveGState(g); CGContextSetTextMatrix(g, CGAffineTransformMakeScale(1, -1)); CGContextSetTextPosition(g, p.x, p.y);
  CairoCgSource &s = cairo_cg_top(c).src; if (s.kind == 0) CGContextSetRGBFillColor(g, s.rgba[0], s.rgba[1], s.rgba[2], s.rgba[3]);
  CTLineDraw(line, g); CGContextRestoreGState(g); double adv = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr); CFRelease(line);
  CGContextBeginPath(g); CGContextMoveToPoint(g, p.x + adv, p.y); }
