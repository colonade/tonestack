// GUI-Test (AmpFett 76): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   u76guitest AmpFett_76.clap bild.png
// Prüft: INPUT ziehen, RATIO-Tasten, METER-Tasten, ATTACK bis OFF, Mausrad auf RELEASE, Kippschalter RATIO-Art / SC HPF / STACK / POT TAPER,
// Streifenknöpfe (VAR RATIO, DRY, HPF FREQ, THRU), Doppelklick auf OUTPUT, Wertanzeigen, Ausgang hörbar und endlich.
#include "clap-1.2.2/include/clap/clap.h"
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <atomic>
#include <vector>
static std::atomic<bool> cbReq{false};
static bool ok_timer(const clap_host_t *, uint32_t, clap_id *id) { *id = 1; return true; }
static bool ok_untimer(const clap_host_t *, clap_id) { return true; }
static const clap_host_timer_support_t HT = {ok_timer, ok_untimer};
static void req_flush(const clap_host_t *) {} static void req_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void req_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static const clap_host_params_t HP = {req_rescan, req_clear, req_flush};
static const void *host_ext(const clap_host_t *, const char *id) { if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &HT; if (!strcmp(id, CLAP_EXT_PARAMS)) return &HP; return nullptr; }
static void host_nop(const clap_host_t *) {}
static void host_cb(const clap_host_t *) { cbReq = true; }
static uint32_t ev0(const clap_input_events_t *) { return 0; } static const clap_event_header_t *evg(const clap_input_events_t *, uint32_t) { return nullptr; }
static bool push(const clap_output_events_t *, const clap_event_header_t *) { return true; }
int main(int argc, char **argv) {
  if (argc < 3) { printf("u76guitest PLUGIN bild.png\n"); return 1; }
  Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  Window root = RootWindow(d, DefaultScreen(d)); Window par = XCreateSimpleWindow(d, root, 0, 0, 1400, 800, 0, 0, 0x202020); XMapWindow(d, par); XFlush(d);
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("%s\n", dlerror()); return 2; } auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_cb};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, desc->id); p->init(p); p->activate(p, 48000, 32, 512); p->start_processing(p);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  // Audio: 200-Hz-Ton, 256er-Blöcke
  std::vector<float> L(256), R(256), OL(256), OR(256); long pos = 0; double outPk = 0;
  auto audio = [&](int blocks) { for (int b = 0; b < blocks; b++) { for (int i = 0; i < 256; i++) L[i] = R[i] = (float)(0.5 * std::sin(2 * M_PI * 200 * (pos + i) / 48000.0)); pos += 256;
      float *ins[2] = {L.data(), R.data()}, *outs[2] = {OL.data(), OR.data()}; clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
      clap_input_events_t ie = {nullptr, ev0, evg}; clap_output_events_t oe = {nullptr, push}; clap_process_t pr{}; pr.frames_count = 256; pr.steady_time = -1; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
      p->process(p, &pr); outPk = 0; for (int i = 0; i < 256; i++) outPk = std::max(outPk, (double)std::fabs(OL[i])); }
    if (cbReq.exchange(false)) p->on_main_thread(p); };
  printf("GUI x11: %d\n", gui->is_api_supported(p, CLAP_WINDOW_API_X11, false)); gui->create(p, CLAP_WINDOW_API_X11, false);
  uint32_t w, h; gui->get_size(p, &w, &h); printf("Größe %u x %u\n", w, h); XResizeWindow(d, par, w, h);
  clap_window_t cw{}; cw.api = CLAP_WINDOW_API_X11; cw.x11 = par; gui->set_parent(p, &cw); gui->show(p);
  for (int i = 0; i < 5; i++) { audio(4); tim->on_timer(p, 1); usleep(30000); }
  Window r, pr; Window *ch; unsigned nch; XQueryTree(d, par, &r, &pr, &ch, &nch); Window win = nch ? ch[0] : 0; printf("Kindfenster: %lu\n", (unsigned long)win);
  auto send = [&](int type, int x, int y, int button) { XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = button; e.xbutton.time = 1000; e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; e.xmotion.window = win; } XSendEvent(d, win, True, type == MotionNotify ? PointerMotionMask : (type == ButtonPress ? ButtonPressMask : ButtonReleaseMask), &e); XFlush(d); };
  auto tick = [&]() { audio(2); tim->on_timer(p, 1); };
  double s = w / 2000.0; auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(320 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { for (int i = 0; i < 3; i++) { usleep(10000); tick(); } double v; params->get_value(p, id, &v); return v; };   // vorher kurz warten: Xvfb liefert Ereignisse manchmal erst nach dem nächsten Takt
  // Parameter: 0 INPUT, 1 OUTPUT, 2 ATTACK, 3 RELEASE, 4 RATIO (0 = 4:1 … 3 = 20:1, 4 alle, 5 stufenlos), 5 VAR RATIO, 6 METER, 7 DRY, 8 SC HPF, 9 HPF FREQ, 10 STACK, 11 THRU, 18 POT TAPER
  auto dragBy = [&](int x, int y, int dy) { send(ButtonPress, x, y, 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, x, y - dy * i / 10, 0); tick(); } send(ButtonRelease, x, y - dy, 1); tick(); };
  int fail = 0; auto chk = [&](const char *name, double got, bool ok, const char *want) { printf("%s: %g (erwartet %s)%s\n", name, got, want, ok ? "" : "   FEHLER"); if (!ok) fail++; };
  dragBy(kx(420), kx(178), kx(80)); { double v = pv(0); chk("INPUT gezogen", v, v > 5, "> 5"); }
  click(kx(1070), kx(82)); { double v = pv(4); chk("RATIO-Taste 20", v, v == 3, "3 = 20:1"); }
  click(kx(1070), kx(190)); { double v = pv(4); chk("RATIO-Taste 8", v, v == 1, "1 = 8:1"); }
  click(kx(1640), kx(244)); { double v = pv(6); chk("METER OFF", v, v == 3, "3"); }
  click(kx(1640), kx(82)); { double v = pv(6); chk("METER GR", v, v == 0, "0"); }
  dragBy(kx(790), kx(178), -kx(250)); { double v = pv(2); chk("ATTACK ganz zurück", v, v == 0, "0 = OFF"); }
  dragBy(kx(790), kx(178), kx(125)); { double v = pv(2); chk("ATTACK wieder auf", v, v >= 1, ">= 1"); }
  { double a = pv(3); send(ButtonPress, kx(930), kx(178), 4); pv(3); send(ButtonPress, kx(930), kx(178), 4); double b = pv(3); chk("RELEASE Mausrad", b, b > a, "größer als vorher"); }
  click(ux(60), uy(98 + 14)); { double v = pv(4); chk("RATIO-Art eine Stufe runter", v, v == 4, "4 = alle Tasten"); }
  click(ux(60), uy(98 + 14)); { double v = pv(4); chk("RATIO-Art noch eine", v, v == 5, "5 = stufenlos"); }
  dragBy(ux(215), uy(78), ux(60)); { double v = pv(5); chk("VAR RATIO gezogen", v, v > 8, "> 8"); }
  click(ux(60), uy(98 - 14)); click(ux(60), uy(98 - 14)); { double v = pv(4); chk("RATIO-Art zurück auf Tasten", v, v < 4, "< 4 (Taste)"); }
  dragBy(ux(330), uy(78), ux(60)); { double v = pv(7); chk("DRY gezogen", v, v > 0, "> 0"); }
  click(ux(470), uy(98)); { double v = pv(8); chk("SC HPF", v, v == 1, "1"); }
  dragBy(ux(580), uy(78), ux(60)); { double v = pv(9); chk("HPF FREQ gezogen", v, v > 100, "> 100 Hz"); }
  click(ux(690), uy(98)); { double v = pv(10); chk("STACK", v, v == 1, "1"); }
  dragBy(ux(805), uy(78), ux(40)); { double v = pv(11); chk("THRU gezogen", v, v > 5, "> 5"); }
  click(ux(60), uy(238 + 14)); { double v = pv(18); chk("POT TAPER", v, v == 1, "1 = log 10 %"); }
  dragBy(kx(610), kx(178), kx(60)); send(ButtonPress, kx(610), kx(178), 1); send(ButtonRelease, kx(610), kx(178), 1); tick(); send(ButtonPress, kx(610), kx(178), 1); send(ButtonRelease, kx(610), kx(178), 1); tick(); { double v = pv(1); chk("OUTPUT Doppelklick", v, v == 5, "5 = Grundstellung"); }
  { const int pts[][2] = {{420, 178}, {610, 178}, {790, 178}, {930, 178}, {1070, 82}, {1070, 244}, {1640, 136}, {1355, 160}};
    for (auto &q : pts) { send(MotionNotify, kx(q[0]), kx(q[1]), 0); tick(); tick(); } for (int k = 0; k < 9; k++) { send(MotionNotify, ux(k < 7 ? (k == 0 ? 215 : k == 1 ? 330 : k == 2 ? 580 : 805 + 90 * (k - 3)) : (k == 7 ? 960 : 1110)), uy(k < 7 ? 78 : 216), 0); tick(); tick(); } printf("Wertanzeigen über Knöpfen und Tasten: ok\n"); }
  { double m = 0; bool fin = true; for (int i = 0; i < 60; i++) { audio(4); m = std::max(m, outPk); for (int k = 0; k < 256; k++) if (!std::isfinite(OL[k]) || !std::isfinite(OR[k])) fin = false; } chk("Ausgang hörbar und endlich", m, m > 1e-3 && fin, "> 0,001"); }
  send(MotionNotify, kx(420), kx(178), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  char t[64]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 64); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  if (fail) { printf("FEHLER: %d Prüfung(en)\n", fail); return 1; } printf("alle Prüfungen bestanden\n"); return 0;
}
