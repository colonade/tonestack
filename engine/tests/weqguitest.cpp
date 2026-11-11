// GUI-Test (Wien-EQ): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   weqguitest Wien_EQ.clap bild.png
// Prüft: MID-1-GAIN ziehen (Rastung bei 0), Tasten LOW 60/100 und HIGH 12k/8k, EQ ON (Taste und Anzeige), Mausrad auf FREQ, Q ziehen, Regelbereich, OUTPUT, Doppelklick.
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
  if (argc < 3) { printf("weqguitest PLUGIN bild.png\n"); return 1; }
  // eigener HOME-Ordner: die Schalter VALUE POPUP und BAND LIST schreiben eine Einstellungsdatei, die des Benutzers bleibt unberührt
  char home[] = "/tmp/weqguitest-XXXXXX"; if (!mkdtemp(home)) { printf("kein Testordner\n"); return 1; } setenv("HOME", home, 1); unsetenv("XDG_CONFIG_HOME");
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
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(240 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { for (int i = 0; i < 3; i++) { usleep(10000); tick(); } double v; params->get_value(p, id, &v); return v; };   // vorher kurz warten: Xvfb liefert Ereignisse manchmal erst nach dem nächsten Takt
  // Parameter: 0 LOW GAIN, 1 LOW FREQ, 2 MID1 GAIN, 3 MID1 Q, 4 MID1 FREQ, 5 MID2 GAIN, 8 MID3 GAIN, 10 MID3 FREQ, 11 HIGH GAIN, 12 HIGH FREQ, 13 EQ ON, 14 RANGE, 15 OUTPUT
  send(ButtonPress, kx(452), kx(140), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(452), kx(140) - i * 8, 0); tick(); } send(ButtonRelease, kx(452), kx(60), 1); tick(); printf("MID 1 GAIN: %g (erwartet > 0)\n", pv(2));
  send(ButtonPress, kx(812), kx(140), 1); send(MotionNotify, kx(812), kx(138), 0); tick(); send(ButtonRelease, kx(812), kx(138), 1); tick(); printf("MID 2 GAIN kaum bewegt: %g (erwartet 0, Rastung)\n", pv(5));
  click(kx(322), kx(108)); printf("LOW-Taste 60: %g (erwartet 1 = 60 Hz)\n", pv(1)); click(kx(322), kx(166)); printf("LOW-Taste 100 dazu: %g (erwartet 3 = 160 Hz)\n", pv(1)); click(kx(322), kx(108)); printf("LOW 60 wieder raus: %g (erwartet 2 = 100 Hz)\n", pv(1));
  click(kx(1630), kx(166)); printf("HIGH-Taste 8k: %g (erwartet 2)\n", pv(12));
  click(kx(1772), kx(166)); printf("EQ ON: %g (erwartet 0 = aus)\n", pv(13)); click(kx(1772), kx(104)); printf("EQ ON über die Anzeige: %g (erwartet 1)\n", pv(13));
  send(ButtonPress, kx(1376), kx(140), 4); pv(10); send(ButtonPress, kx(1376), kx(140), 4); tick(); printf("MID 3 FREQ: %g (erwartet 5.4)\n", pv(10));
  send(ButtonPress, kx(552), kx(140), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(552), kx(140) + i * 8, 0); tick(); } send(ButtonRelease, kx(552), kx(220), 1); tick(); printf("MID 1 Q: %g (erwartet < 5)\n", pv(3));
  click(ux(70), uy(100)); printf("Regelbereich: %g (erwartet 2 = +/-20 dB)\n", pv(14));
  send(ButtonPress, ux(1130), uy(72), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, ux(1130), uy(72) - i * 4, 0); tick(); } send(ButtonRelease, ux(1130), uy(32), 1); tick(); printf("OUTPUT: %g (erwartet > 0)\n", pv(15));
  send(ButtonPress, kx(228), kx(140), 1); for (int i = 1; i <= 6; i++) { send(MotionNotify, kx(228), kx(140) - i * 8, 0); tick(); } send(ButtonRelease, kx(228), kx(92), 1); tick(); printf("LOW GAIN: %g (erwartet > 0)\n", pv(0));
  audio(40); printf("Ausgang: %.3f\n", outPk);
  send(MotionNotify, kx(1016), kx(140), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  printf("Ausgang Spitze: %.3f\n", outPk);
  char t[64]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 64); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  // Schalter der Kopfzeile: VALUE POPUP und BAND LIST aus und wieder ein; Stand in der Einstellungsdatei; Bild ohne beide daneben
  int bad = 0; { std::string cf = std::string(home) + "/.config/tonestack/ui.conf"; auto rd = [&](int &tp, int &bd) { tp = bd = -1; FILE *f = fopen(cf.c_str(), "r"); if (!f) return; char k[32]; int v; while (fscanf(f, " %31[a-z]=%d", k, &v) == 2) { if (!strcmp(k, "tips")) tp = v; if (!strcmp(k, "bands")) bd = v; } fclose(f); };
    int tp, bd; click(ux(460), uy(16)); rd(tp, bd); printf("VALUE POPUP aus: tips=%d (Soll 0)\n", tp); if (tp != 0) bad++;
    click(ux(630), uy(16)); rd(tp, bd); printf("BAND LIST aus: bands=%d (Soll 0)\n", bd); if (bd != 0) bad++;
    send(MotionNotify, kx(1016), kx(140), 0); for (int i = 0; i < 4; i++) tick(); std::string f2 = std::string(argv[2]); size_t dot = f2.rfind('.'); f2 = f2.substr(0, dot) + "-aus" + f2.substr(dot);
    cairo_surface_t *x2 = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(x2, f2.c_str()); cairo_surface_destroy(x2);
    click(ux(460), uy(16)); click(ux(630), uy(16)); rd(tp, bd); printf("beide wieder ein: tips=%d bands=%d (Soll 1 1)\n", tp, bd); if (tp != 1 || bd != 1) bad++;
    remove(cf.c_str()); rmdir((std::string(home) + "/.config/tonestack").c_str()); rmdir((std::string(home) + "/.config").c_str()); rmdir(home); }
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  if (bad) { printf("FEHLER: %d Schalterprüfung(en)\n", bad); return 1; } return 0;
}
