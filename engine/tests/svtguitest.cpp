// GUI-Test (AmpFett SVT-CL): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   svtguitest AmpFett_SVTCL.clap bild.png
// Prüft: Netzteil- und BAL-OUT-Schalter (ab 1.5), Buchsen −15 dB / 0 dB, GAIN ziehen, FREQUENCY bis 3 kHz, ULTRA HI/LO, STANDBY und POWER verborgen (ab 1.3 nicht mehr auf der Frontplatte), Ausgang 2 Ω, POT TAPER, Mausrad auf MASTER (Überabtastung ab 1.1 entfernt).
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
  if (argc < 3) { printf("svtguitest PLUGIN bild.png\n"); return 1; }
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
  double s = w / 1390.0; /* ab 1.3 Frontplatte 1390 breit (ohne STANDBY/POWER) */ auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(260 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { double v; params->get_value(p, id, &v); return v; };
  click(kx(196), kx(150)); printf("Buchse -15 dB: %g (erwartet 1)\n", pv(8)); click(kx(100), kx(150)); printf("Buchse 0 dB: %g (erwartet 0)\n", pv(8));
  send(ButtonPress, kx(330), kx(138), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(330), kx(138) - i * 8, 0); tick(); } send(ButtonRelease, kx(330), kx(58), 1); tick(); printf("Gain: %g (erwartet > 5)\n", pv(0));
  send(ButtonPress, kx(990), kx(138), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(990), kx(138) - i * 10, 0); tick(); } send(ButtonRelease, kx(990), kx(38), 1); tick(); printf("Frequency: %g (erwartet 4 = 3 kHz)\n", pv(3));
  click(kx(470), kx(140)); printf("Ultra Hi: %g (erwartet 1)\n", pv(6)); click(kx(560), kx(140)); printf("Ultra Lo: %g (erwartet 1)\n", pv(7));
  { clap_param_info_t a, b; params->get_info(p, 10, &a); params->get_info(p, 11, &b); printf("Standby/Power verborgen: %d %d (erwartet 1 1)\n", (a.flags & CLAP_PARAM_IS_HIDDEN) ? 1 : 0, (b.flags & CLAP_PARAM_IS_HIDDEN) ? 1 : 0); }
  audio(40); printf("Ausgang hörbar: %d (erwartet 1)\n", outPk > 1e-3 ? 1 : 0);
  click(ux(64), uy(100)); printf("Ausgang: %g (erwartet 1 = 2 Ohm)\n", pv(9));
  click(ux(186), uy(100)); printf("Pot taper: %g (erwartet 1 = log 10 %%)\n", pv(15));
  click(ux(308), uy(100)); printf("Netzteil: %g (erwartet 1 = nach Plan 50 Hz)\n", pv(16)); click(ux(430), uy(100)); printf("BAL OUT: %g (erwartet 1 = T1 geschätzt)\n", pv(17));
  for (int i = 0; i < 300; i++) { tick(); usleep(10000); }   // neuer Kern im Hilfsfaden (Netzteil nach Plan, T1)
  { double m = 0; for (int i = 0; i < 40; i++) { audio(4); m = std::max(m, outPk); } printf("Ausgang hörbar nach Umbau: %d (erwartet 1)\n", m > 1e-3 && std::isfinite(m) ? 1 : 0); }
  for (int i = 0; i < 400; i++) { tick(); usleep(5000); }
  send(ButtonPress, kx(1290), kx(138), 4); tick(); send(ButtonPress, kx(1290), kx(138), 4); tick(); printf("Master: %g (erwartet 5.4)\n", pv(5));
  // Wertanzeige über allen Bedienelementen (1.0 stürzte beim Zeigen auf eine Buchse ab: falscher Zweig, Zugriff außerhalb der Schalterliste)
  { const int pts[][2] = {{100, 150}, {196, 150}, {470, 140}, {560, 140}, {1340, 140}, {1380, 200}, {330, 138}, {990, 138}};
    for (auto &q : pts) { send(MotionNotify, kx(q[0]), kx(q[1]), 0); tick(); tick(); } printf("Wertanzeigen über Buchsen, Schaltern, Knöpfen: ok\n"); }
  send(MotionNotify, kx(840), kx(138), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  printf("Ausgang Spitze: %.3f\n", outPk);
  char t[64]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 64); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d); return 0;
}
