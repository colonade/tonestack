// GUI-Test (Kaiserin Reverb): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   kaiguitest Kaiserin_Reverb.clap bild.png
// Prüft: DECAY ziehen, MIX kaum bewegt (Rastung bei 12 Uhr), Modustasten und Anzeigen, Mausrad auf den Modustasten, HOLD, THING 1 ziehen, Doppelklick, Kopfzeile ohne VALUE POPUP; Ausgang stereo.
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
  if (argc < 3) { printf("kaiguitest PLUGIN bild.png\n"); return 1; }
  // eigener HOME-Ordner: der Schalter VALUE POPUP schreiben eine Einstellungsdatei, die des Benutzers bleibt unberührt
  char home[] = "/tmp/kaiguitest-XXXXXX"; if (!mkdtemp(home)) { printf("kein Testordner\n"); return 1; } setenv("HOME", home, 1); unsetenv("XDG_CONFIG_HOME");
  Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  Window root = RootWindow(d, DefaultScreen(d)); Window par = XCreateSimpleWindow(d, root, 0, 0, 1400, 800, 0, 0, 0x202020); XMapWindow(d, par); XFlush(d);
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("%s\n", dlerror()); return 2; } auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_cb};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, desc->id); p->init(p); p->activate(p, 48000, 32, 512); p->start_processing(p);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  // Audio: 200-Hz-Ton, 256er-Blöcke
  std::vector<float> L(256), R(256), OL(256), OR(256); long pos = 0; double outPk = 0, lrDiff = 0;
  auto audio = [&](int blocks) { for (int b = 0; b < blocks; b++) { for (int i = 0; i < 256; i++) L[i] = R[i] = (float)(0.5 * std::sin(2 * M_PI * 200 * (pos + i) / 48000.0)); pos += 256;
      float *ins[2] = {L.data(), R.data()}, *outs[2] = {OL.data(), OR.data()}; clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
      clap_input_events_t ie = {nullptr, ev0, evg}; clap_output_events_t oe = {nullptr, push}; clap_process_t pr{}; pr.frames_count = 256; pr.steady_time = -1; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
      p->process(p, &pr); outPk = 0; for (int i = 0; i < 256; i++) { outPk = std::max(outPk, (double)std::fabs(OL[i])); lrDiff = std::max(lrDiff, (double)std::fabs(OL[i] - OR[i])); } }
    if (cbReq.exchange(false)) p->on_main_thread(p); };
  printf("GUI x11: %d\n", gui->is_api_supported(p, CLAP_WINDOW_API_X11, false)); gui->create(p, CLAP_WINDOW_API_X11, false);
  uint32_t w, h; gui->get_size(p, &w, &h); printf("Größe %u x %u\n", w, h); XResizeWindow(d, par, w, h);
  clap_window_t cw{}; cw.api = CLAP_WINDOW_API_X11; cw.x11 = par; gui->set_parent(p, &cw); gui->show(p);
  for (int i = 0; i < 5; i++) { audio(4); tim->on_timer(p, 1); usleep(30000); }
  Window r, pr; Window *ch; unsigned nch; XQueryTree(d, par, &r, &pr, &ch, &nch); Window win = nch ? ch[0] : 0; printf("Kindfenster: %lu\n", (unsigned long)win);
  auto send = [&](int type, int x, int y, int button) { XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = button; e.xbutton.time = 1000; e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; e.xmotion.window = win; } XSendEvent(d, win, True, type == MotionNotify ? PointerMotionMask : (type == ButtonPress ? ButtonPressMask : ButtonReleaseMask), &e); XFlush(d); };
  auto tick = [&]() { audio(2); tim->on_timer(p, 1); };
  double s = w / 2000.0; auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(240 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { for (int i = 0; i < 3; i++) { usleep(10000); tick(); } double v; params->get_value(p, id, &v); return v; };   // vorher kurz warten: Xvfb liefert Ereignisse manchmal erst nach dem nächsten Takt
  int bad = 0; auto chk = [&](const char *what, double v, bool ok) { printf("%s: %g %s\n", what, v, ok ? "ok" : "FEHLER"); if (!ok) bad++; };
  // Parameter: 0 MODE, 1 DECAY, 2 MIX, 3 OUTPUT, 4 LOW, 5 HI, 6 THING 1, 7 THING 2, 8 HOLD; Knöpfe bei x 640, 760, 880, 1050, 1170, 1340, 1460 (y 128)
  send(ButtonPress, kx(640), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(640), kx(128) - i * 8, 0); tick(); } send(ButtonRelease, kx(640), kx(48), 1); tick(); chk("DECAY gezogen (Soll > 5)", pv(1), pv(1) > 5);
  send(ButtonPress, kx(760), kx(128), 1); send(MotionNotify, kx(760), kx(127), 0); tick(); send(ButtonRelease, kx(760), kx(127), 1); tick(); chk("MIX kaum bewegt (Soll 5, Rastung)", pv(2), pv(2) == 5);
  click(kx(370), kx(102)); chk("Taste PLATE (Soll 1)", pv(0), pv(0) == 1);
  click(kx(486), kx(178)); chk("Taste SPARKLE (Soll 5)", pv(0), pv(0) == 5);
  click(kx(218), kx(150)); chk("Anzeige ROOM angeklickt (Soll 3)", pv(0), pv(0) == 3);
  send(ButtonPress, kx(254), kx(102), 5); tick(); chk("Mausrad auf den Modustasten (Soll 4)", pv(0), pv(0) == 4);
  click(kx(1640), kx(150)); chk("HOLD ein (Soll 1)", pv(8), pv(8) == 1); audio(60); click(kx(1640), kx(150)); chk("HOLD aus (Soll 0)", pv(8), pv(8) == 0);
  send(ButtonPress, kx(1340), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1340), kx(128) + i * 8, 0); tick(); } send(ButtonRelease, kx(1340), kx(208), 1); tick(); chk("THING 1 gezogen (Soll < 5)", pv(6), pv(6) < 5);
  click(kx(1460), kx(128)); click(kx(1460), kx(128)); chk("THING 2 Doppelklick (Soll 5)", pv(7), pv(7) == 5);
  send(ButtonPress, kx(1170), kx(128), 4); tick(); chk("HI Mausrad (Soll 5.2)", pv(5), std::fabs(pv(5) - 5.2) < 1e-9);
  click(kx(254), kx(102)); chk("zurück auf HALL (Soll 0)", pv(0), pv(0) == 0);
  audio(80); chk("Ausgang Spitze (Soll > 0,05)", outPk, outPk > 0.05); chk("links und rechts verschieden (Soll > 1e-3)", lrDiff, lrDiff > 1e-3);
  send(MotionNotify, kx(1340), kx(128), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  char t[96]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 96); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  { std::string cf = std::string(home) + "/.config/tonestack/ui.conf"; auto rd = [&]() { int tp = -1; FILE *f = fopen(cf.c_str(), "r"); if (!f) return tp; char k[32]; int v; while (fscanf(f, " %31[a-z]=%d", k, &v) == 2) if (!strcmp(k, "tips")) tp = v; fclose(f); return tp; };
    click(ux(460), uy(16)); int tp = rd(); chk("Kopfzeile ohne VALUE POPUP (ab 18.11.2026): Klick an die alte Stelle schreibt nichts (Soll -1)", tp, tp == -1);
    remove(cf.c_str()); rmdir((std::string(home) + "/.config/tonestack").c_str()); rmdir((std::string(home) + "/.config").c_str()); rmdir(home); }
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  if (bad) { printf("FEHLER: %d Prüfung(en)\n", bad); return 1; } printf("alle Prüfungen bestanden\n"); return 0;
}
