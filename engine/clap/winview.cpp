// Windows: Kindfenster (HWND) für die Plugin-Oberfläche, Gegenstück zu macview.mm mit derselben Schnittstelle (macview.h).
// Zeichnen: die Oberfläche zeichnet mit Cairo in ein Bild in Fenstergröße (Gerätepixel), das Bild geht mit SetDIBitsToDevice ins Fenster
// (kein Flackern, kein Löschen des Hintergrunds). Maus wie unter X11/macOS: y nach unten, Taste 1 links, Mausrad als Tasten 4/5 in Rasten,
// Ziehen hält die Maus fest (SetCapture). Eigener Takt 30/s (SetTimer) für Anzeigen und Neuzeichnen, nur bei sichtbarem Fenster.
// Die Fensterklasse wird je geladener DLL mit eigenem Namen angelegt (Adresse eines eigenen Symbols) und mit der letzten Ansicht wieder
// abgemeldet: eine DLL gibt beim Entladen ihre Klassen nicht von selbst frei, eine verwaiste Klasse zeigte auf entladenen Code.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include "cairo_win.hpp"
#include "macview.h"

namespace {
struct View { HWND hw = nullptr; tsmac::Client *client = nullptr; cairo_surface_t *buf = nullptr; int bw = 0, bh = 0; bool tracking = false, down = false; LPARAM lastPos = 0; double wheelAcc = 0; };
const UINT_PTR kTimer = 1;
wchar_t clsName[64]; int clsRefs = 0;   // nur im Oberflächen-Faden benutzt
HINSTANCE moduleOf() { HMODULE m = nullptr; GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)(void *)&moduleOf, &m); return m; }
View *viewOf(HWND h) { return (View *)GetWindowLongPtrW(h, GWLP_USERDATA); }
double secs() { return GetMessageTime() / 1000.0; }   // Zeit der Nachricht (Doppelklick-Erkennung der Oberfläche)
void send(View *v, int kind, LPARAM lp, int button) { if (!v || !v->client) return; v->lastPos = lp;
  if (v->client->mouse(kind, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), button, secs())) InvalidateRect(v->hw, nullptr, FALSE); }
void paint(HWND h, HDC given = nullptr) { PAINTSTRUCT ps; HDC dc = given ? given : BeginPaint(h, &ps); View *v = viewOf(h); RECT r; GetClientRect(h, &r); int w = r.right - r.left, hh = r.bottom - r.top;
  if (v && v->client && w > 0 && hh > 0) {
    if (!v->buf || v->bw != w || v->bh != hh) { if (v->buf) cairo_surface_destroy(v->buf); v->buf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, hh); v->bw = w; v->bh = hh; }
    if (cairo_surface_status(v->buf) == CAIRO_STATUS_SUCCESS && cairo_image_surface_get_stride(v->buf) == w * 4) {
      cairo_t *c = cairo_create(v->buf); v->client->drawCG(c, w, hh); cairo_destroy(c); cairo_surface_flush(v->buf);
      BITMAPINFO bi{}; bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -hh; bi.bmiHeader.biPlanes = 1;
      bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;   // Cairo RGB24 = BGRx im Speicher, oben zuerst (negative Höhe)
      SetDIBitsToDevice(dc, 0, 0, (DWORD)w, (DWORD)hh, 0, 0, 0, (UINT)hh, cairo_image_surface_get_data(v->buf), &bi, DIB_RGB_COLORS); } }
  if (!given) EndPaint(h, &ps); }
LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  View *v = viewOf(h);
  switch (msg) {
    case WM_PAINT: paint(h); return 0;
    case WM_PRINTCLIENT: paint(h, (HDC)wp); return 0;   // Bildschirmfoto (PrintWindow), Prüfprogramm wingui
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN: SetCapture(h); if (v) v->down = true; send(v, 0, lp, 1); return 0;
    case WM_MOUSEMOVE:
      if (v && !v->tracking) { TRACKMOUSEEVENT t{}; t.cbSize = sizeof t; t.dwFlags = TME_LEAVE; t.hwndTrack = h; if (TrackMouseEvent(&t)) v->tracking = true; }
      send(v, 1, lp, (wp & MK_LBUTTON) && v && v->down ? 1 : 0); return 0;
    case WM_LBUTTONUP: { bool was = v && v->down; if (v) v->down = false; if (GetCapture() == h) ReleaseCapture(); if (was) send(v, 2, lp, 1); return 0; }
    case WM_CAPTURECHANGED: if (v && v->down && (HWND)lp != h) { v->down = false; send(v, 2, v->lastPos, 1); } return 0;   // Ziehen abgebrochen (anderes Fenster)
    case WM_MOUSELEAVE: if (v) { v->tracking = false; if (!v->down) send(v, 3, 0, 0); } return 0;
    case WM_MOUSEWHEEL: if (v) { POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)}; ScreenToClient(h, &p); LPARAM cl = MAKELPARAM(p.x, p.y);
        v->wheelAcc += GET_WHEEL_DELTA_WPARAM(wp) / (double)WHEEL_DELTA;
        while (v->wheelAcc >= 1) { v->wheelAcc -= 1; send(v, 0, cl, 4); } while (v->wheelAcc <= -1) { v->wheelAcc += 1; send(v, 0, cl, 5); } } return 0;
    case WM_TIMER: if (wp == kTimer && v && v->client && IsWindowVisible(h) && v->client->tick()) InvalidateRect(h, nullptr, FALSE); return 0;
    case WM_NCDESTROY: if (v) { KillTimer(h, kTimer); v->hw = nullptr; } SetWindowLongPtrW(h, GWLP_USERDATA, 0); break;   // z. B. mit dem Fenster des Hosts zerstört
  }
  return DefWindowProcW(h, msg, wp, lp); }
bool registerClass() { if (clsRefs > 0) { clsRefs++; return true; }
  std::swprintf(clsName, 64, L"tonestack_view_%llx", (unsigned long long)(uintptr_t)&clsRefs);
  WNDCLASSEXW wc{}; wc.cbSize = sizeof wc; wc.lpfnWndProc = proc; wc.hInstance = moduleOf(); wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.lpszClassName = clsName;
  if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false; clsRefs = 1; return true; }
void unregisterClass() { if (clsRefs > 0 && --clsRefs == 0) UnregisterClassW(clsName, moduleOf()); }
}

namespace tsmac {
void *create(void *parent, int w, int h, Client *c) {
  if (!parent || !registerClass()) return nullptr; View *v = new View(); v->client = c;
  HWND hw = CreateWindowExW(0, clsName, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, w, h, (HWND)parent, nullptr, moduleOf(), nullptr);
  if (!hw) { delete v; unregisterClass(); return nullptr; }
  v->hw = hw; SetWindowLongPtrW(hw, GWLP_USERDATA, (LONG_PTR)v); SetTimer(hw, kTimer, 33, nullptr); InvalidateRect(hw, nullptr, FALSE); return v; }
void setSize(void *view, int w, int h) { View *v = (View *)view; if (!v || !v->hw) return; SetWindowPos(v->hw, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE); InvalidateRect(v->hw, nullptr, FALSE); }
void show(void *view) { View *v = (View *)view; if (!v || !v->hw) return; ShowWindow(v->hw, SW_SHOWNA); InvalidateRect(v->hw, nullptr, FALSE); }
void hide(void *view) { View *v = (View *)view; if (v && v->hw) ShowWindow(v->hw, SW_HIDE); }
void redraw(void *view) { View *v = (View *)view; if (v && v->hw) InvalidateRect(v->hw, nullptr, FALSE); }
void destroy(void *view) { View *v = (View *)view; if (!v) return; v->client = nullptr;
  if (v->hw && IsWindow(v->hw) && viewOf(v->hw) == v) { KillTimer(v->hw, kTimer); if (GetCapture() == v->hw) ReleaseCapture(); DestroyWindow(v->hw); }
  if (v->buf) cairo_surface_destroy(v->buf); delete v; unregisterClass(); }
}
