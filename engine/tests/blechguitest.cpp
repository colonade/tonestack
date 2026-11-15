// GUI-Test (Blech 140): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   blechguitest Blech_140.clap bild.png
// Prüft: DECAY ziehen, Lampe MOTOR beim Umbau der Platte an und danach aus, Wahlschalter BASS CUT (Klick, Mitte, Mausrad), MIX und PRE-DELAY ziehen,
// Doppelklicks, Mausrad OUTPUT, VALUE POPUP; Ausgang stereo.
#include "clap-1.2.2/include/clap/clap.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
  if (argc < 3) { printf("blechguitest PLUGIN bild.png\n"); return 1; }
  // eigener HOME-Ordner: der Schalter VALUE POPUP schreiben eine Einstellungsdatei, die des Benutzers bleibt unberührt
  char home[] = "/tmp/blechguitest-XXXXXX"; if (!mkdtemp(home)) { printf("kein Testordner\n"); return 1; } setenv("HOME", home, 1); unsetenv("XDG_CONFIG_HOME");
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
  // Parameter: 0 DECAY (s), 1 BASS CUT, 2 MIX (%), 3 PRE-DELAY (ms), 4 OUTPUT (dB); Knöpfe DECAY 760, MIX 1310, PRE 1450, OUTPUT 1590 (y 128), BASS CUT 1000/128, Lampe 860/70
  auto lamp = [&]() { XImage *im = XGetImage(d, win, kx(860), kx(70), 1, 1, AllPlanes, ZPixmap); unsigned long px = XGetPixel(im, 0, 0); XDestroyImage(im); return (int)((px >> 16) & 255); };   // Rotanteil: an 255, aus 96
  chk("Lampe MOTOR aus nach dem Start (Rot Soll < 160)", lamp(), lamp() < 160);
  send(ButtonPress, kx(760), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(760), kx(128) - i * 8, 0); tick(); } send(ButtonRelease, kx(760), kx(48), 1); tick(); double dec = pv(0); chk("DECAY gezogen (Soll > 3 s)", dec, dec > 3);
  { int on = 0, n = 0; for (; n < 400; n++) { audio(4); tim->on_timer(p, 1); usleep(20000); int r = lamp(); if (r > 200) on++; if (on && r < 160) break; }   // Platte im Umbau: Lampe an, danach wieder aus
    chk("Lampe MOTOR leuchtete beim Umbau (Takte)", on, on > 0); chk("Lampe MOTOR wieder aus (Takte bis dahin, Soll < 400)", n, n < 400); }
  click(kx(1021), kx(107)); chk("BASS CUT rechts angeklickt (Soll 3)", pv(1), pv(1) == 3);
  click(kx(1000), kx(128)); chk("BASS CUT Mitte: weiterschalten (Soll 0)", pv(1), pv(1) == 0);
  send(ButtonPress, kx(1000), kx(128), 4); tick(); chk("BASS CUT Mausrad (Soll 1)", pv(1), pv(1) == 1);
  send(ButtonPress, kx(1310), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1310), kx(128) - i * 6, 0); tick(); } send(ButtonRelease, kx(1310), kx(68), 1); tick(); chk("MIX gezogen (Soll > 30 %)", pv(2), pv(2) > 30);
  send(ButtonPress, kx(1450), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1450), kx(128) - i * 6, 0); tick(); } send(ButtonRelease, kx(1450), kx(68), 1); tick(); chk("PRE-DELAY gezogen (Soll > 0 ms)", pv(3), pv(3) > 0);
  click(kx(1450), kx(128)); click(kx(1450), kx(128)); chk("PRE-DELAY Doppelklick (Soll 0)", pv(3), pv(3) == 0);
  send(ButtonPress, kx(1590), kx(128), 4); tick(); chk("OUTPUT Mausrad (Soll +1 dB)", pv(4), pv(4) == 1);
  click(kx(760), kx(128)); click(kx(760), kx(128)); chk("DECAY Doppelklick (Soll 2 s)", pv(0), pv(0) == 2);
  audio(200); chk("Ausgang Spitze (Soll > 0,05)", outPk, outPk > 0.05); chk("links und rechts verschieden (Soll > 1e-3)", lrDiff, lrDiff > 1e-3);
  send(MotionNotify, kx(760), kx(128), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  char t[96]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 96); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  { std::string cf = std::string(home) + "/.config/tonestack/ui.conf"; auto rd = [&]() { int tp = -1; FILE *f = fopen(cf.c_str(), "r"); if (!f) return tp; char k[32]; int v; while (fscanf(f, " %31[a-z]=%d", k, &v) == 2) if (!strcmp(k, "tips")) tp = v; fclose(f); return tp; };
    click(ux(460), uy(16)); int tp = rd(); chk("VALUE POPUP aus (tips Soll 0)", tp, tp == 0); click(ux(460), uy(16)); tp = rd(); chk("VALUE POPUP wieder ein (Soll 1)", tp, tp == 1);
    remove(cf.c_str()); rmdir((std::string(home) + "/.config/tonestack").c_str()); rmdir((std::string(home) + "/.config").c_str()); rmdir(home); }
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  if (bad) { printf("FEHLER: %d Prüfung(en)\n", bad); return 1; } printf("alle Prüfungen bestanden\n"); return 0;
}
