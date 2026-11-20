// GUI-Test (Viereck 34): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   viereckguitest Viereck_34.clap bild.png
// Prüft: TILT ziehen (Rasten), Mausrad auf BASS, Filtertasten F1/F2/SLOPE (wie am Gerät: schließen sich aus, SLOPE nur mit Filter),
// Kippschalter STEPS/SMOOTH, TILT stufenlos, PIVOT (Rastung am Original, Doppelklick), BALANCE (Doppelklick Mitte), LEVEL, OUTPUT. Rückgabe 1 bei Abweichung.
#include "clap-1.2.2/include/clap/clap.h"
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <dlfcn.h>
#include <cmath>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
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
  if (argc < 3) { printf("viereckguitest PLUGIN bild.png\n"); return 1; }
  Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  Window root = RootWindow(d, DefaultScreen(d)); Window par = XCreateSimpleWindow(d, root, 0, 0, 1400, 800, 0, 0, 0x202020); XMapWindow(d, par); XFlush(d);
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("%s\n", dlerror()); return 2; } auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_cb};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, desc->id); p->init(p); p->activate(p, 48000, 32, 512); p->start_processing(p);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
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
  double tms = 1000; auto send = [&](int type, int x, int y, int button) { XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = button; tms += 500; e.xbutton.time = (Time)tms; e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; } XSendEvent(d, win, False, 0, &e); XFlush(d); };
  auto tick = [&]() { audio(2); tim->on_timer(p, 1); };
  double s = w / 1600.0; auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(240 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto dbl = [&](int x, int y) { XEvent e{}; for (int k = 0; k < 2; k++) { e = XEvent{}; e.type = ButtonPress; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = 1; e.xbutton.time = (Time)(tms + 100 + k * 100); e.xbutton.same_screen = True; XSendEvent(d, win, False, 0, &e); XFlush(d); tick();
      e.type = ButtonRelease; XSendEvent(d, win, False, 0, &e); XFlush(d); tick(); } tms += 1000; };
  auto pv = [&](int id) { for (int i = 0; i < 3; i++) { usleep(10000); tick(); } double v; params->get_value(p, id, &v); return v; };
  int bad = 0; auto chk = [&](const char *what, double got, double want, double tol = 1e-6) { bool ok = std::fabs(got - want) <= tol; if (!ok) bad++; printf("%-46s %g (Soll %g)%s\n", what, got, want, ok ? "" : "   ABWEICHUNG"); };
  // Parameter: 0 TILT, 1 BASS, 2 BALANCE, 3 FILTER, 4 SLOPE, 5 TILT-Art, 6 PIVOT, 7 LEVEL, 8 OUTPUT; Knöpfe BASS 380, TILT 580, BALANCE 770, PIVOT 1250 (y 132)
  send(ButtonPress, kx(580), kx(132), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(580), kx(132) - i * 10, 0); tick(); } send(ButtonRelease, kx(580), kx(32), 1); tick();
  { double t = pv(0); bool ok = t < 0 && t == std::round(t); if (!ok) bad++; printf("TILT nach oben gezogen (rechts herum, wärmer): %g (Soll ganze Zahl < 0)%s\n", t, ok ? "" : "   ABWEICHUNG"); }
  dbl(kx(580), kx(132)); chk("TILT Doppelklick", pv(0), 0);
  send(ButtonPress, kx(380), kx(132), 4); tick(); chk("BASS Mausrad hoch: Lift +3", pv(1), 4); send(ButtonPress, kx(380), kx(132), 5); tick(); send(ButtonPress, kx(380), kx(132), 5); tick(); chk("BASS Mausrad 2x runter: Step 100 Hz", pv(1), 2);
  click(kx(1080), kx(160)); chk("SLOPE ohne Filter: bleibt aus", pv(4), 0);
  click(kx(920), kx(160)); chk("F1", pv(3), 1); click(kx(1080), kx(160)); chk("SLOPE mit F1", pv(4), 1); click(kx(1000), kx(160)); chk("F2 löst F1 ab", pv(3), 2); chk("SLOPE bleibt", pv(4), 1);
  click(kx(1000), kx(160)); chk("F2 nochmals: Filter aus", pv(3), 0); chk("SLOPE mit Filter aus gelöscht", pv(4), 0);
  click(kx(1400), kx(132)); chk("Kippschalter: SMOOTH", pv(5), 1);
  send(ButtonPress, kx(580), kx(132), 1); for (int i = 1; i <= 3; i++) { send(MotionNotify, kx(580), kx(132) + i * 7, 0); tick(); } send(ButtonRelease, kx(580), kx(153), 1); tick();
  { double t = pv(0); bool ok = t > 0 && t != std::round(t); if (!ok) bad++; printf("TILT stufenlos nach unten gezogen: %g (Soll > 0, nicht ganzzahlig)%s\n", t, ok ? "" : "   ABWEICHUNG"); }
  click(kx(1400), kx(132)); chk("Kippschalter: STEPS", pv(5), 0);
  send(ButtonPress, kx(1250), kx(132), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1250), kx(132) - i * 8, 0); tick(); } send(ButtonRelease, kx(1250), kx(52), 1); tick();
  { double f = pv(6); bool ok = f > 1500; if (!ok) bad++; printf("PIVOT nach oben: %g Hz (Soll > 1500)%s\n", f, ok ? "" : "   ABWEICHUNG"); }
  dbl(kx(1250), kx(132)); chk("PIVOT Doppelklick: Original", pv(6), 847.13, 0.01);
  send(ButtonPress, kx(770), kx(132), 4); tick(); { double b = pv(2); bool ok = b > 0; if (!ok) bad++; printf("BALANCE Mausrad: %g (Soll > 0)%s\n", b, ok ? "" : "   ABWEICHUNG"); } dbl(kx(770), kx(132)); chk("BALANCE Doppelklick: Mitte", pv(2), 0);
  send(ButtonPress, ux(760), uy(62), 1); for (int i = 1; i <= 5; i++) { send(MotionNotify, ux(760), uy(62) - i * 6, 0); tick(); } send(ButtonRelease, ux(760), uy(32), 1); tick(); { double l = pv(7); bool ok = l > -10; if (!ok) bad++; printf("LEVEL hoch: %g (Soll > -10)%s\n", l, ok ? "" : "   ABWEICHUNG"); }
  send(ButtonPress, ux(1120), uy(62), 1); for (int i = 1; i <= 5; i++) { send(MotionNotify, ux(1120), uy(62) - i * 6, 0); tick(); } send(ButtonRelease, ux(1120), uy(32), 1); tick(); { double o = pv(8); bool ok = o > 0; if (!ok) bad++; printf("OUTPUT hoch: %g (Soll > 0)%s\n", o, ok ? "" : "   ABWEICHUNG"); }
  click(kx(920), kx(160)); click(kx(1080), kx(160)); send(ButtonPress, kx(380), kx(132), 4); tick(); send(ButtonPress, kx(380), kx(132), 4); tick(); send(ButtonPress, kx(580), kx(132), 5); tick();
  audio(40); printf("Ausgang Spitze: %.3f\n", outPk); if (!(outPk > 0.01 && outPk < 5)) bad++;
  send(MotionNotify, kx(580), kx(132), 0); for (int i = 0; i < 6; i++) tick(); usleep(200000);
  char t[96]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 96); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  printf("%s\n", bad ? "FEHLER" : "alle Prüfungen bestanden"); return bad ? 1 : 0;
}
