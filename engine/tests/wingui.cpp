// Windows-Prüfung der Plugin-Oberflächen (CLAP-DLL, wie ein Host mit echtem Fenster; übersetzt mit MinGW in engine/win/build.sh,
// läuft im GitHub-Bau auf einem Windows-Rechner).
//   wingui PLUGIN.clap bild.bmp [breite]
// 1. Oberfläche anlegen (API „win32“), in ein Hauptfenster setzen, zeigen, 1,5 s Nachrichten verarbeiten (eigener Takt der Oberfläche)
// 2. Bild aufnehmen (WM_PRINTCLIENT an die Ansicht) → bild.bmp; Kennzahlen: Anteil nicht schwarzer Pixel, Zahl der Farben
// 3. 300 Mausaktionen an Zufallsstellen (Klick, Ziehen, Mausrad) per Nachricht an die Ansicht; Parameteränderungen über flush gezählt
// 4. Mausrad als echte Eingabe (SendInput) mit Zustellung „unter dem Zeiger“ und „an das Fokusfenster“ (dort leitet der Maus-Hook der Ansicht um);
//    ohne echte Eingabe (kein Desktop) übersprungen
// 5. Anzeigeskalierung bei offenem Fenster: set_scale muss beim Host eine neue Größe anfordern; Größengrenzen wachsen mit der Skalierung
// 6. Größe ändern (1,5-fach) und Bild prüfen; 20-mal verbergen/zeigen und zerstören/neu anlegen
// Ausgabe: Zeilen „RESULT …“, Rückgabe 0 = alles bestanden.
#include "clap-1.2.2/include/clap/clap.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>

static int fails = 0;
static void result(const char *what, bool ok, const std::string &detail) { std::printf("RESULT %-28s %s %s\n", what, ok ? "ok" : "FEHLER", detail.c_str()); if (!ok) fails++; }
static const clap_plugin_t *P = nullptr; static const clap_plugin_params_t *PAR = nullptr; static long flushes = 0, gestures = 0, values = 0;
static void host_flush_req(const clap_host_t *) { flushes++; }
static void host_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void host_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static const clap_host_params_t HP = {host_rescan, host_clear, host_flush_req};
static long resizeReq = 0; static uint32_t reqW = 0, reqH = 0;
static void g_hints(const clap_host_t *) {} static bool g_resize(const clap_host_t *, uint32_t w, uint32_t h) { resizeReq++; reqW = w; reqH = h; return true; } static bool g_show(const clap_host_t *) { return true; }
static bool g_hide(const clap_host_t *) { return true; } static void g_closed(const clap_host_t *, bool) {}
static const clap_host_gui_t HG = {g_hints, g_resize, g_show, g_hide, g_closed};
static const void *host_ext(const clap_host_t *, const char *id) { if (!std::strcmp(id, CLAP_EXT_PARAMS)) return &HP; if (!std::strcmp(id, CLAP_EXT_GUI)) return &HG; return nullptr; }
static void host_nop(const clap_host_t *) {}
static clap_host_t HOST = {CLAP_VERSION_INIT, nullptr, "wingui", "tonestack", "", "1", host_ext, host_nop, host_nop, host_nop};
static uint32_t in_size(const clap_input_events_t *) { return 0; } static const clap_event_header_t *in_get(const clap_input_events_t *, uint32_t) { return nullptr; }
static bool out_push(const clap_output_events_t *, const clap_event_header_t *e) { if (e->type == CLAP_EVENT_PARAM_GESTURE_BEGIN) gestures++; if (e->type == CLAP_EVENT_PARAM_VALUE) values++; return true; }
static void flush() { if (!PAR) return; clap_input_events_t in{nullptr, in_size, in_get}; clap_output_events_t out{nullptr, out_push}; PAR->flush(P, &in, &out); }
static void pump(int ms) { auto t0 = std::chrono::steady_clock::now(); MSG m;
  while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(ms)) { while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); } flush(); Sleep(5); } }
static LRESULT CALLBACK topProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) { return DefWindowProcW(h, msg, wp, lp); }
static HWND child(HWND top) { return GetWindow(top, GW_CHILD); }
// Bild der Ansicht über WM_PRINTCLIENT; Kennzahlen und BMP
static bool snap(HWND top, const char *file, int &w, int &h, double &filled, int &colors) {
  HWND c = child(top); if (!c) return false; RECT r; GetClientRect(c, &r); w = r.right; h = r.bottom; if (w <= 0 || h <= 0) return false;
  HDC sdc = GetDC(c); HDC mdc = CreateCompatibleDC(sdc); BITMAPINFO bi{}; bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
  bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB; void *bits = nullptr; HBITMAP bm = CreateDIBSection(mdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  HGDIOBJ old = SelectObject(mdc, bm); std::memset(bits, 0, (size_t)w * h * 4); SendMessageW(c, WM_PRINTCLIENT, (WPARAM)mdc, PRF_CLIENT); GdiFlush();
  const uint32_t *px = (const uint32_t *)bits; long nz = 0; std::vector<uint32_t> seen; seen.reserve(4096);
  for (long i = 0; i < (long)w * h; i++) { uint32_t v = px[i] & 0xffffff; if (v) nz++; if (seen.size() < 4096 && (i % 7) == 0) { bool f = false; for (uint32_t s : seen) if (s == v) { f = true; break; } if (!f) seen.push_back(v); } }
  filled = nz / (double)((long)w * h); colors = (int)seen.size();
  if (file) { FILE *f = std::fopen(file, "wb"); if (f) { uint32_t sz = 54 + (uint32_t)w * h * 4; unsigned char hd[54] = {'B', 'M'}; std::memcpy(hd + 2, &sz, 4); uint32_t off = 54; std::memcpy(hd + 10, &off, 4);
      uint32_t ih = 40; std::memcpy(hd + 14, &ih, 4); int32_t ww = w, hh = -h; std::memcpy(hd + 18, &ww, 4); std::memcpy(hd + 22, &hh, 4); uint16_t pl = 1, bc = 32; std::memcpy(hd + 26, &pl, 2); std::memcpy(hd + 28, &bc, 2);
      std::fwrite(hd, 1, 54, f); std::fwrite(bits, 4, (size_t)w * h, f); std::fclose(f); } }
  SelectObject(mdc, old); DeleteObject(bm); DeleteDC(mdc); ReleaseDC(c, sdc); return true; }

int main(int argc, char **argv) {
  if (argc < 3) { std::printf("wingui PLUGIN.clap bild.bmp [breite]\n"); return 2; }
  SetProcessDPIAware();
  HMODULE lib = LoadLibraryA(argv[1]); if (!lib) { result("laden", false, "LoadLibrary " + std::to_string(GetLastError())); return 1; }
  auto *entry = (const clap_plugin_entry_t *)GetProcAddress(lib, "clap_entry"); if (!entry || !entry->init(argv[1])) { result("laden", false, "clap_entry"); return 1; }
  auto *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  P = fac->create_plugin(fac, &HOST, d->id); P->init(P); PAR = (const clap_plugin_params_t *)P->get_extension(P, CLAP_EXT_PARAMS);
  std::printf("Plugin: %s %s\n", d->name, d->version);
  P->activate(P, 48000, 1, 512); P->start_processing(P);
  auto *gui = (const clap_plugin_gui_t *)P->get_extension(P, CLAP_EXT_GUI);
  result("gui-erweiterung", gui != nullptr, ""); if (!gui) return 1;
  result("api win32", gui->is_api_supported(P, CLAP_WINDOW_API_WIN32, false), "");
  const char *api = nullptr; bool fl = true; gui->get_preferred_api(P, &api, &fl); result("bevorzugte api", api && !std::strcmp(api, CLAP_WINDOW_API_WIN32) && !fl, api ? api : "-");
  WNDCLASSW wc{}; wc.lpfnWndProc = topProc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"winguiTop"; wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); RegisterClassW(&wc);
  HWND top = CreateWindowW(L"winguiTop", L"wingui", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 1700, 1100, nullptr, nullptr, wc.hInstance, nullptr);
  bool ok = gui->create(P, CLAP_WINDOW_API_WIN32, false); result("create", ok, ""); if (!ok) return 1;
  uint32_t w = 0, h = 0; gui->get_size(P, &w, &h); if (argc > 3) { uint32_t want = (uint32_t)std::atoi(argv[3]), hh = want; if (gui->can_resize(P) && gui->adjust_size(P, &want, &hh)) { gui->set_size(P, want, hh); w = want; h = hh; } }
  result("groesse", w >= 300 && h >= 100, std::to_string(w) + "x" + std::to_string(h));
  clap_window_t win{CLAP_WINDOW_API_WIN32, {nullptr}}; win.win32 = top; ok = gui->set_parent(P, &win); result("set_parent", ok && child(top) != nullptr, ""); if (!ok) return 1;
  SetWindowPos(top, HWND_TOP, 0, 0, (int)w + 40, (int)h + 80, SWP_NOMOVE); gui->show(P); pump(1500);
  int iw, ih, colors; double filled; ok = snap(top, argv[2], iw, ih, filled, colors);
  char b[200]; std::snprintf(b, sizeof b, "%dx%d gefuellt %.1f %% Farben>=%d", iw, ih, filled * 100, colors);
  result("bild", ok && iw == (int)w && ih == (int)h && filled > 0.9 && colors > 50, b);
  // Maus: Zufallsstellen, Klick/Ziehen/Rad
  std::mt19937 rng(11); HWND c = child(top); std::vector<double> v0; uint32_t np = PAR ? PAR->count(P) : 0;
  for (uint32_t i = 0; i < np; i++) { clap_param_info_t inf; PAR->get_info(P, i, &inf); double v; PAR->get_value(P, inf.id, &v); v0.push_back(v); }
  for (int k = 0; k < 300; k++) { int x = (int)(rng() % w), y = (int)(rng() % h); LPARAM lp = MAKELPARAM(x, y); int kind = (int)(rng() % 3);
    if (kind == 0) { SendMessageW(c, WM_LBUTTONDOWN, MK_LBUTTON, lp); SendMessageW(c, WM_LBUTTONUP, 0, lp); }
    else if (kind == 1) { SendMessageW(c, WM_LBUTTONDOWN, MK_LBUTTON, lp); for (int s = 1; s <= 6; s++) SendMessageW(c, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x, y - 12 * s)); SendMessageW(c, WM_LBUTTONUP, 0, MAKELPARAM(x, y - 72)); }
    else { POINT p{x, y}; ClientToScreen(c, &p); SendMessageW(c, WM_MOUSEWHEEL, MAKEWPARAM(0, (rng() & 1) ? WHEEL_DELTA : -WHEEL_DELTA), MAKELPARAM(p.x, p.y)); }
    if (k % 20 == 0) pump(30); }
  pump(300); int changed = 0;
  for (uint32_t i = 0; i < np; i++) { clap_param_info_t inf; PAR->get_info(P, i, &inf); double v; PAR->get_value(P, inf.id, &v); if (v != v0[i]) changed++; }
  std::snprintf(b, sizeof b, "Parameter geaendert %d von %u, Gesten %ld, Werte %ld, flush-Anfragen %ld", changed, np, gestures, values, flushes);
  result("maus", changed > 0 && gestures > 0 && values > 0, b);
  // Mausrad über echte Eingabe
  { const UINT GETR = 0x201C, SETR = 0x201D;   // SPI_GET/SETMOUSEWHEELROUTING (Windows 10): 0 Fokus, 2 unter dem Zeiger
    DWORD old = 2; bool haveR = SystemParametersInfoW(GETR, 0, &old, 0) != 0;
    auto setR = [&](DWORD r) { DWORD now = 99; SystemParametersInfoW(SETR, 0, (PVOID)(UINT_PTR)r, 0); SystemParametersInfoW(GETR, 0, &now, 0);
      if (now != r) { SystemParametersInfoW(SETR, 0, &r, 0); SystemParametersInfoW(GETR, 0, &now, 0); } return now == r; };
    for (int k = 0; k < 5 && GetForegroundWindow() != top; k++) { ShowWindow(top, SW_RESTORE); SetForegroundWindow(top); BringWindowToTop(top); pump(50); }
    SetFocus(top);   // Fokus beim Hauptfenster (wie im Host), nicht bei der Ansicht
    RECT cr; GetWindowRect(c, &cr); int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    auto spin = [&]() { long before = values; for (int gy = 1; gy <= 5; gy++) for (int gx = 1; gx <= 8; gx++) {
        int x = cr.left + (int)((cr.right - cr.left) * gx / 9.0), y = cr.top + (int)((cr.bottom - cr.top) * gy / 6.0); if (x < 0 || y < 0 || x >= sw || y >= sh) continue;
        SetCursorPos(x, y); INPUT in{}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_WHEEL; in.mi.mouseData = (DWORD)(gx & 1 ? WHEEL_DELTA : -WHEEL_DELTA); SendInput(1, &in, sizeof in); pump(25); }
      pump(100); return values - before; };
    bool fg = GetForegroundWindow() == top; long hover = haveR && setR(2) ? spin() : -1;
    if (!fg || hover <= 0) result("mausrad", true, std::string("uebersprungen (") + (!fg ? "Fenster nicht im Vordergrund" : hover < 0 ? "Einstellung nicht lesbar" : "keine echte Eingabe") + ")");
    else { long focus = setR(0) ? spin() : -1; result("mausrad", focus > 0, "Werte unter dem Zeiger " + std::to_string(hover) + ", ueber den Fokus " + (focus < 0 ? std::string("(Einstellung nicht setzbar)") : std::to_string(focus))); }
    if (haveR) setR(old); }
  // Anzeigeskalierung bei offenem Fenster
  { uint32_t aw = 100000, ah = 100000; gui->adjust_size(P, &aw, &ah); long r0 = resizeReq; bool sOk = gui->set_scale(P, 1.25); long r1 = resizeReq;
    uint32_t ew = (uint32_t)std::lround(w * 1.25); bool reqOk = sOk && r1 == r0 + 1 && (reqW + 2 >= ew && reqW <= ew + 2);
    uint32_t qW = reqW, qH = reqH; if (reqOk) { gui->set_size(P, qW, qH); pump(150); }
    RECT rr; GetClientRect(c, &rr); bool sizeOk = reqOk && rr.right == (LONG)qW && rr.bottom == (LONG)qH;
    uint32_t bw = 100000, bh = 100000; gui->adjust_size(P, &bw, &bh);   // Obergrenze bei 1,25 = 1,25 × Obergrenze bei 1
    bool limOk = bw + 2 >= aw * 5 / 4 && bw <= aw * 5 / 4 + 2;
    gui->set_scale(P, 1.0); bool backOk = resizeReq == r1 + 1 && reqW + 2 >= w && reqW <= w + 2; if (backOk) { gui->set_size(P, reqW, reqH); pump(150); }
    std::snprintf(b, sizeof b, "1,25: angefordert %ux%u (erwartet Breite %u), Fenster %ldx%ld, Obergrenze %u -> %u, zurueck %ux%u", qW, qH, ew, rr.right, rr.bottom, aw, bw, reqW, reqH);
    result("skalierung", sOk && reqOk && sizeOk && limOk && backOk, b); }
  // Größe 1,5-fach
  uint32_t w2 = w * 3 / 2, h2 = h * 3 / 2; gui->adjust_size(P, &w2, &h2); gui->set_size(P, w2, h2); SetWindowPos(top, nullptr, 0, 0, (int)w2 + 40, (int)h2 + 80, SWP_NOMOVE | SWP_NOZORDER); pump(500);
  ok = snap(top, nullptr, iw, ih, filled, colors); std::snprintf(b, sizeof b, "%dx%d gefuellt %.1f %%", iw, ih, filled * 100);
  result("groesse aendern", ok && iw == (int)w2 && ih == (int)h2 && filled > 0.9, b);
  // verbergen/zeigen, zerstören/neu
  bool all = true; for (int k = 0; k < 20; k++) { all &= gui->hide(P); pump(20); all &= gui->show(P); pump(20); gui->destroy(P); pump(10); all &= gui->create(P, CLAP_WINDOW_API_WIN32, false); all &= gui->set_parent(P, &win); all &= gui->show(P); pump(40); }
  int n = 0; for (HWND x = child(top); x; x = GetWindow(x, GW_HWNDNEXT)) n++;
  result("auf/zu 20x", all && n == 1, "Kindfenster " + std::to_string(n));
  gui->destroy(P); pump(50); result("zerstoert", child(top) == nullptr, "");
  P->stop_processing(P); P->deactivate(P); P->destroy(P); entry->deinit(); DestroyWindow(top); FreeLibrary(lib);
  std::printf("%s\n", fails ? "FEHLER" : "alle Pruefungen bestanden"); return fails ? 1 : 0; }
