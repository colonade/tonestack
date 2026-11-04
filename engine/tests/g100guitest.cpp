// GUI-Test (Yamaha G100-212II): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   g100guitest Yamaha_G100.clap bild.png
// Prüft: PULL FAT A, Treble A ziehen, A/B-Taste, Buchse LOW, PULL BRIGHT B, Überabtastung 4x (Kernwechsel im Hilfsfaden), POWER AMP linear, Mausrad auf LEVEL, REVERB ziehen (ab 1.1), Regler REVERB TANK (ab 1.4), INPUT-Knopf.
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
  if (argc < 3) { printf("g100guitest PLUGIN bild.png\n"); return 1; }
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
  double s = w / 1800.0; auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(260 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { double v; params->get_value(p, id, &v); return v; };
  click(kx(120), kx(48)); printf("Pull Fat A: %g (erwartet 1)\n", pv(4));
  send(ButtonPress, kx(224), kx(136), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(224), kx(136) - i * 8, 0); tick(); } send(ButtonRelease, kx(224), kx(56), 1); tick(); printf("Treble A: %g (erwartet > 5)\n", pv(1));
  click(kx(540), kx(100)); printf("Kanal: %g (erwartet 1 = B)\n", pv(6)); audio(40); printf("Ausgang Kanal B: %.3f\n", outPk);
  click(kx(662), kx(136)); printf("Buchse: %g (erwartet 1 = LOW)\n", pv(17)); click(kx(606), kx(136)); printf("Buchse: %g (erwartet 0 = HIGH)\n", pv(17));
  click(kx(974), kx(48)); printf("Pull Bright B: %g (erwartet 1)\n", pv(13));
  click(ux(60), uy(100)); printf("Überabtastung: %g (erwartet 2 = 4x)\n", pv(20));
  for (int i = 0; i < 400; i++) { tick(); usleep(5000); }
  click(ux(172), uy(100)); printf("Endstufe: %g (erwartet 0 = linear)\n", pv(18)); click(ux(172), uy(70)); printf("Endstufe: %g (erwartet 1)\n", pv(18));
  click(ux(290), uy(100)); printf("Poti-Kennlinie: %g (erwartet 1)\n", pv(22));
  send(ButtonPress, ux(420), uy(72), 1); for (int i = 1; i <= 12; i++) { send(MotionNotify, ux(420), uy(72) + i * 10, 0); tick(); } send(ButtonRelease, ux(420), uy(192), 1); tick(); printf("Federtank: %g (erwartet 0 = linear wie 1.1)\n", pv(24));
  send(ButtonPress, ux(420), uy(72), 1); for (int i = 1; i <= 12; i++) { send(MotionNotify, ux(420), uy(72) - i * 10, 0); tick(); } send(ButtonRelease, ux(420), uy(-48), 1); tick(); printf("Federtank: %g (erwartet 5: zweiter Druck auf denselben Regler gilt im Test als Doppelklick = Grundstellung)\n", pv(24));
  send(ButtonPress, ux(1020), uy(72), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, ux(1020), uy(72) - i * 4, 0); tick(); } send(ButtonRelease, ux(1020), uy(32), 1); tick(); printf("Instrument: %g (erwartet > 100)\n", pv(19));
  send(ButtonPress, kx(1294), kx(136), 4); tick(); send(ButtonPress, kx(1294), kx(136), 4); tick(); printf("Level: %g (erwartet 0.4)\n", pv(14));
  send(ButtonPress, kx(1612), kx(136), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1612), kx(136) - i * 8, 0); tick(); } send(ButtonRelease, kx(1612), kx(56), 1); tick(); printf("Reverb: %g (erwartet > 2)\n", pv(23));
  click(kx(540), kx(100)); printf("Kanal: %g (erwartet 0 = A)\n", pv(6));
  send(MotionNotify, kx(328), kx(136), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  printf("Ausgang Spitze: %.3f\n", outPk);
  char t[64]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 64); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d); return 0;
}
