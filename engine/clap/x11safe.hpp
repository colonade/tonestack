// Eigene X11-Verbindungen der Plugin-Oberflächen, abgesichert gegen X-Fehler.
// Xlibs Standard-Fehlerbehandlung beendet den ganzen Prozess (also den Host). Ein X-Fehler auf der eigenen Verbindung ist aber
// meist harmlos, etwa wenn der Host sein Containerfenster schon zerstört hat und die Oberfläche noch einmal zeichnet.
// Ab Laney 3.32 / Yamaha 1.13 / AmpFett 1.13 nur noch während der eigenen X-Aufrufe (Guard): Fehlerbehandlung setzen, am Ende die
// eigenen Verbindungen abgleichen (XSync, Fehler kommen jetzt an) und die vorherige Fehlerbehandlung zurücksetzen. Bis dahin hing jedes
// Plugin dauerhaft in der prozessweiten Kette; mit mehreren Plugins konnte sie nach dem Entladen eines Plugins auf dessen Code zeigen.
#pragma once
#include <X11/Xlib.h>
#include <mutex>
#include <set>
#include <atomic>

namespace tsx {
struct Reg { std::mutex m; std::set<Display*> ours; std::atomic<int> errors{0}; };
inline Reg &reg() { static Reg r; return r; }
inline int &depth() { static thread_local int d = 0; return d; }
inline XErrorHandler &prevH() { static thread_local XErrorHandler p = nullptr; return p; }
inline int onError(Display *d, XErrorEvent *e) {
  Reg &r = reg(); { std::lock_guard<std::mutex> lk(r.m); if (r.ours.count(d)) { r.errors++; return 0; } }
  XErrorHandler p = prevH(); return (p && p != onError) ? p(d, e) : 0; }
// für die Dauer der eigenen X-Aufrufe (verschachtelbar)
struct Guard {
  Guard() { if (depth()++ == 0) { XErrorHandler p = XSetErrorHandler(onError); prevH() = p == onError ? nullptr : p; } }
  ~Guard() { if (--depth() == 0) { Reg &r = reg(); std::set<Display*> ds; { std::lock_guard<std::mutex> lk(r.m); ds = r.ours; }
      for (Display *d : ds) XSync(d, False);   // ausstehende Fehler der eigenen Verbindungen jetzt, solange onError zuständig ist
      XErrorHandler cur = XSetErrorHandler(prevH()); if (cur != onError) XSetErrorHandler(cur);   // nur zurücksetzen, wenn wir noch zuständig sind
      prevH() = nullptr; } }
  Guard(const Guard &) = delete; Guard &operator=(const Guard &) = delete; };
inline Display *open() { Guard g; Display *d = XOpenDisplay(nullptr); if (!d) return nullptr; Reg &r = reg(); std::lock_guard<std::mutex> lk(r.m); r.ours.insert(d); return d; }
inline void close(Display *d) {
  if (!d) return; Guard g; XSync(d, False); { Reg &r = reg(); std::lock_guard<std::mutex> lk(r.m); r.ours.erase(d); } XCloseDisplay(d); }
} // namespace tsx
