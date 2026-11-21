// Windows: die Oberflächen zeichnen mit echtem Cairo (statisch gelinkt, Bild-Oberfläche, Schrift über DirectWrite) in die eigene
// Ansicht aus winview.cpp. Diese Datei liefert die beiden Hilfen, die die Oberflächen für die „eigene Ansicht“ (macOS und Windows,
// TS_OWNVIEW) benutzen: cairo_cg_create und cairo_cg_cached. Auf dem Mac stammen sie aus cairo_cg.hpp (Cairo-Teilmenge auf
// CoreGraphics), hier sind sie dünne Hüllen um Cairo.
#pragma once
#ifndef CAIRO_WIN32_STATIC_BUILD
#define CAIRO_WIN32_STATIC_BUILD 1
#endif
#include <cairo.h>
#include <cmath>
typedef void *CGContextRef;   // winview.cpp reicht ein cairo_t* durch
// eigener Verweis auf den Zeichenkontext der Ansicht (die Oberfläche gibt ihn mit cairo_destroy wieder frei)
inline cairo_t *cairo_cg_create(CGContextRef ctx) { return cairo_reference((cairo_t *)ctx); }
// Hintergrund (Foto mit Aufdruck) einmal je Größe in Gerätepixeln vorzeichnen und danach nur noch 1:1 kopieren, wie unter Linux.
// w, h: Größe im aktuellen Benutzerraum; draw(cairo_t*) zeichnet in diesen Raum.
template <class F> inline void cairo_cg_cached(cairo_t *c, cairo_surface_t *&cache, double w, double h, F draw) {
  double dx = 1, dy = 0; cairo_user_to_device_distance(c, &dx, &dy); double ds = std::sqrt(dx * dx + dy * dy); if (!(ds > 0.01 && ds < 64)) ds = 1;
  int pw = (int)std::ceil(w * ds - 1e-6), ph = (int)std::ceil(h * ds - 1e-6); if (pw < 1 || ph < 1) return;
  if (!cache || cairo_image_surface_get_width(cache) != pw || cairo_image_surface_get_height(cache) != ph) {
    if (cache) cairo_surface_destroy(cache); cache = cairo_image_surface_create(CAIRO_FORMAT_RGB24, pw, ph);
    cairo_t *b = cairo_create(cache); cairo_scale(b, ds, ds); draw(b); cairo_destroy(b); }
  cairo_save(c); cairo_scale(c, 1 / ds, 1 / ds); cairo_set_source_surface(c, cache, 0, 0); cairo_pattern_set_filter(cairo_get_source(c), CAIRO_FILTER_NEAREST);
  cairo_rectangle(c, 0, 0, pw, ph); cairo_fill(c); cairo_restore(c); }
