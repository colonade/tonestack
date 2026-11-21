// Brücke zwischen der Plugin-Oberfläche (C++) und einer eigenen Ansicht: NSView auf macOS (macview.mm), HWND unter Windows (winview.cpp).
// drawCG bekommt auf dem Mac einen CGContextRef, unter Windows ein cairo_t* (Bild in Fenstergröße).
#pragma once
namespace tsmac {
struct Client {   // die Oberfläche: zeichnen, Maus, Takt
  virtual ~Client() {}
  virtual void drawCG(void *cgContext, double w, double h) = 0;
  // kind: 0 drücken, 1 bewegen/ziehen, 2 loslassen, 3 verlassen; button 1 links, 4/5 Mausrad hoch/runter; t in Sekunden
  virtual bool mouse(int kind, double x, double y, int button, double t) = 0;
  virtual bool tick() = 0;   // etwa 30-mal je Sekunde; true = neu zeichnen
};
void *create(void *parentView, int w, int h, Client *c);   // Eltern: NSView bzw. HWND; liefert einen Griff auf die Ansicht
void setSize(void *view, int w, int h);
void show(void *view);
void hide(void *view);
void redraw(void *view);
void destroy(void *view);
}
