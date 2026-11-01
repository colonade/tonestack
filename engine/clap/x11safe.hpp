// Eigene X11-Verbindungen der Plugin-Oberflächen, abgesichert gegen X-Fehler.
// Xlibs Standard-Fehlerbehandlung beendet den ganzen Prozess (also den Host). Ein X-Fehler auf der eigenen Verbindung ist aber
// meist harmlos, etwa wenn der Host sein Containerfenster schon zerstört hat und die Oberfläche noch einmal zeichnet (VST3 über
// clap-wrapper: dort ruft nach dem Schließen ein Hilfs-Thread den Timer). Darum: Fehler auf eigenen Verbindungen werden gezählt
// und verworfen, alle anderen gehen unverändert an die vorherige Fehlerbehandlung. Ist die letzte eigene Verbindung zu, wird die
// vorherige Fehlerbehandlung wiederhergestellt (sonst zeigte sie nach dem Entladen des Plugins ins Leere).
#pragma once
#include <X11/Xlib.h>
#include <mutex>
#include <set>
#include <atomic>

namespace tsx {
struct Reg { std::mutex m; std::set<Display*> ours; XErrorHandler prev = nullptr; bool installed = false; std::atomic<int> errors{0}; };
inline Reg &reg() { static Reg r; return r; }
inline int onError(Display *d, XErrorEvent *e) {
  Reg &r = reg(); XErrorHandler prev; { std::lock_guard<std::mutex> lk(r.m); if (r.ours.count(d)) { r.errors++; return 0; } prev = r.prev; }
  return prev ? prev(d, e) : 0; }
inline Display *open() {
  Display *d = XOpenDisplay(nullptr); if (!d) return nullptr;
  Reg &r = reg(); std::lock_guard<std::mutex> lk(r.m); r.ours.insert(d);
  if (!r.installed) { r.prev = XSetErrorHandler(onError); if (r.prev == onError) r.prev = nullptr; r.installed = true; }
  return d; }
inline void close(Display *d) {
  if (!d) return; XSync(d, False); XCloseDisplay(d);   // offene Anfragen noch mit unserer Fehlerbehandlung abarbeiten
  Reg &r = reg(); std::lock_guard<std::mutex> lk(r.m); r.ours.erase(d);
  if (r.ours.empty() && r.installed) { XErrorHandler cur = XSetErrorHandler(r.prev); if (cur != onError) XSetErrorHandler(cur); r.installed = false; r.prev = nullptr; } }   // nur zurücksetzen, wenn wir noch zuständig sind
} // namespace tsx
