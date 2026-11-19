// GUI-Test (Wisent Echoreck): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   wisentguitest Wisent_Echoreck.clap bild.png
// Prüft: magisches Auge (Stille / lauter Ton), INPUT ziehen und Doppelklick, Betriebstasten (Klick, Mausrad), Kopftasten beider Reihen, Kippschalter 50/60 Hz,
// Mausrad OUTPUT, LEVEL, DRY und VARISPEED, Doppelklick VARISPEED, Kopfzeile ohne VALUE POPUP; Ausgang mono auf beiden Kanälen.
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
  if (argc < 3) { printf("wisentguitest PLUGIN bild.png\n"); return 1; }
  // eigener HOME-Ordner: der Schalter VALUE POPUP schreibt eine Einstellungsdatei, die des Benutzers bleibt unberührt
  char home[] = "/tmp/wisentguitest-XXXXXX"; if (!mkdtemp(home)) { printf("kein Testordner\n"); return 1; } setenv("HOME", home, 1); unsetenv("XDG_CONFIG_HOME");
  Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  Window root = RootWindow(d, DefaultScreen(d)); Window par = XCreateSimpleWindow(d, root, 0, 0, 1400, 800, 0, 0, 0x202020); XMapWindow(d, par); XFlush(d);
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("%s\n", dlerror()); return 2; } auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_cb};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, desc->id); p->init(p); p->activate(p, 48000, 32, 512); p->start_processing(p);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  // Audio: 200-Hz-Ton (Pegel amp), 256er-Blöcke
  std::vector<float> L(256), R(256), OL(256), OR(256); long pos = 0; double outPk = 0, lrDiff = 0, amp = 0;
  auto audio = [&](int blocks) { for (int b = 0; b < blocks; b++) { for (int i = 0; i < 256; i++) L[i] = R[i] = (float)(amp * std::sin(2 * M_PI * 200 * (pos + i) / 48000.0)); pos += 256;
      float *ins[2] = {L.data(), R.data()}, *outs[2] = {OL.data(), OR.data()}; clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
      clap_input_events_t ie = {nullptr, ev0, evg}; clap_output_events_t oe = {nullptr, push}; clap_process_t pr{}; pr.frames_count = 256; pr.steady_time = -1; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
      p->process(p, &pr); for (int i = 0; i < 256; i++) { outPk = std::max(outPk, (double)std::fabs(OL[i])); lrDiff = std::max(lrDiff, (double)std::fabs(OL[i] - OR[i])); } }
    if (cbReq.exchange(false)) p->on_main_thread(p); };
  printf("GUI x11: %d\n", gui->is_api_supported(p, CLAP_WINDOW_API_X11, false)); gui->create(p, CLAP_WINDOW_API_X11, false);
  uint32_t w, h; gui->get_size(p, &w, &h); printf("Größe %u x %u\n", w, h); XResizeWindow(d, par, w, h);
  clap_window_t cw{}; cw.api = CLAP_WINDOW_API_X11; cw.x11 = par; gui->set_parent(p, &cw); gui->show(p);
  for (int i = 0; i < 5; i++) { audio(4); tim->on_timer(p, 1); usleep(30000); }
  Window r, pr; Window *ch; unsigned nch; XQueryTree(d, par, &r, &pr, &ch, &nch); Window win = nch ? ch[0] : 0; printf("Kindfenster: %lu\n", (unsigned long)win);
  auto send = [&](int type, int x, int y, int button) { XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = button; e.xbutton.time = 1000; e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; e.xmotion.window = win; } XSendEvent(d, win, True, type == MotionNotify ? PointerMotionMask : (type == ButtonPress ? ButtonPressMask : ButtonReleaseMask), &e); XFlush(d); };
  auto tick = [&]() { audio(2); tim->on_timer(p, 1); };
  double s = w / 2850.0; auto kx = [&](double v) { return (int)(v * s); };
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(240 * s + v * uu); };
  auto click = [&](int x, int y) { send(ButtonPress, x, y, 1); tick(); send(ButtonRelease, x, y, 1); tick(); };
  auto pv = [&](int id) { for (int i = 0; i < 3; i++) { usleep(10000); tick(); } double v; params->get_value(p, id, &v); return v; };   // vorher kurz warten: Xvfb liefert Ereignisse manchmal erst nach dem nächsten Takt
  int bad = 0; auto chk = [&](const char *what, double v, bool ok) { printf("%s: %g %s\n", what, v, ok ? "ok" : "FEHLER"); if (!ok) bad++; };
  // Parameter: 0 INPUT, 1 SWELL, 2 VOLUME, 3 BASS, 4 TREBLE, 5 MODE, 6–9 Wiedergabeköpfe, 10–13 Rückkopplungsköpfe, 14 LEVEL (mV), 15 OUTPUT (dB), 16 MAINS, 17 DRY, 18 VARISPEED, 19 WOBBLE, 20–23 Kopfstellungen (ms bei 50 Hz, ab 1.4), 24 LINK (ab 1.5)
  // Frontplatte (2850 breit, ab 1.5): Knöpfe INPUT 470, HEAD 1–4 1640 … 1910, LINK 1995, DRY 2130, VARISPEED 2250, WOBBLE 2370, LEVEL 2490, OUTPUT 2610 (y 128); Auge 330/128 (120 × 30); Betriebstasten 1085 / 76, 128, 180; Kopftasten x 1262 + 80·k, Rückkopplung y 100, Wiedergabe y 172; Kippschalter 2125/128
  // Auge: Grünanteil bei 20 % der Breite von links (leuchtet ab etwa −27 dB re Sättigung)
  auto eyeG = [&]() { for (int i = 0; i < 3; i++) { usleep(20000); tick(); } XImage *im = XGetImage(d, win, kx(330 - 60 + 24), kx(128), 1, 1, AllPlanes, ZPixmap); unsigned long px = XGetPixel(im, 0, 0); XDestroyImage(im); return (int)((px >> 8) & 255); };
  chk("Auge dunkel bei Stille (Grün Soll < 120)", eyeG(), eyeG() < 120);
  amp = 0.5; audio(40); chk("Auge leuchtet bei lautem Ton (Grün Soll > 180)", eyeG(), eyeG() > 180);
  send(ButtonPress, kx(470), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(470), kx(128) - i * 6, 0); tick(); } send(ButtonRelease, kx(470), kx(68), 1); tick(); chk("INPUT gezogen (Soll > 2,5)", pv(0), pv(0) > 2.5);
  click(kx(470), kx(128)); click(kx(470), kx(128)); chk("INPUT Doppelklick (Soll 2,5)", pv(0), pv(0) == 2.5);
  click(kx(1085), kx(180)); chk("Taste SWELL (Soll 2)", pv(5), pv(5) == 2);
  click(kx(1085), kx(76)); chk("Taste ECHO (Soll 0)", pv(5), pv(5) == 0);
  send(ButtonPress, kx(1085), kx(76), 5); tick(); chk("Betriebstasten Mausrad (Soll 1)", pv(5), pv(5) == 1);
  click(kx(1262), kx(100)); chk("Rückkopplungskopf 1 ein (Soll 1)", pv(10), pv(10) == 1);
  click(kx(1342), kx(100)); chk("Rückkopplungskopf 2 aus (Soll 0)", pv(11), pv(11) == 0);
  click(kx(1422), kx(172)); chk("Wiedergabekopf 3 ein (Soll 1)", pv(8), pv(8) == 1);
  click(kx(2735), kx(128)); chk("Kippschalter 60 Hz (Soll 1)", pv(16), pv(16) == 1);
  send(ButtonPress, kx(2610), kx(128), 4); tick(); chk("OUTPUT Mausrad (Soll +1 dB)", pv(15), pv(15) == 1);
  send(ButtonPress, kx(2490), kx(128), 4); tick(); chk("LEVEL Mausrad (Soll 219 mV)", pv(14), pv(14) == 219);
  send(ButtonPress, kx(2130), kx(128), 5); tick(); chk("DRY Mausrad (Soll 6,5)", pv(17), std::fabs(pv(17) - 6.5) < 1e-9);
  send(ButtonPress, kx(2250), kx(128), 4); tick(); chk("VARISPEED Mausrad (Soll 1,04)", pv(18), std::fabs(pv(18) - 1.04) < 1e-9);
  click(kx(2250), kx(128)); click(kx(2250), kx(128)); chk("VARISPEED Doppelklick (Soll 1)", pv(18), pv(18) == 1);
  send(ButtonPress, kx(2370), kx(128), 4); tick(); send(ButtonPress, kx(2370), kx(128), 4); tick(); chk("WOBBLE Mausrad (Soll 0,4)", pv(19), std::fabs(pv(19) - 0.4) < 1e-9);
  // Kopfknöpfe (ab 1.4) auf der Frontplatte: HEAD 1–4 bei x 1640/1730/1820/1910, y 128; Parameter 20–23 in ms bei 50 Hz
  { const double h0 = 1000 * 34e-3 / 0.457; chk("HEAD 2 Grundstellung (Soll 148,8 ms)", pv(21), pv(21) == 2 * h0);
    send(ButtonPress, kx(1730), kx(128), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1730), kx(128) - i * 8, 0); tick(); } send(ButtonRelease, kx(1730), kx(48), 1); tick(); chk("HEAD 2 gezogen (Soll > 300 ms)", pv(21), pv(21) > 300);
    send(ButtonPress, kx(1910), kx(128), 4); tick(); chk("HEAD 4 Mausrad (Soll 312,5 ms: +2 % des Wegs, auf 0,5 ms)", pv(23), std::fabs(pv(23) - 312.5) < 1e-9);
    click(kx(1730), kx(128)); click(kx(1730), kx(128)); chk("HEAD 2 Doppelklick (Soll genau das Original)", pv(21), pv(21) == 2 * h0);
    click(kx(1910), kx(128)); click(kx(1910), kx(128)); chk("HEAD 4 Doppelklick (Soll genau das Original)", pv(23), pv(23) == 4 * h0); }
  // LINK (ab 1.5, Parameter 24): HEAD 2 ziehen dreht alle vier im Verhältnis; Doppelklick HEAD 3 stellt alle auf das Original; LINK wieder aus
  { const double h0 = 1000 * 34e-3 / 0.457; click(kx(1995), kx(100)); chk("LINK an (Soll 1)", pv(24), pv(24) == 1);
    send(ButtonPress, kx(1730), kx(128), 1); for (int i = 1; i <= 6; i++) { send(MotionNotify, kx(1730), kx(128) - i * 8, 0); tick(); } send(ButtonRelease, kx(1730), kx(80), 1); tick();
    const double f = pv(21) / (2 * h0); bool rat = std::fabs(pv(20) - h0 * f) < 1e-9 && std::fabs(pv(22) - 3 * h0 * f) < 1e-9 && std::fabs(pv(23) - 4 * h0 * f) < 1e-9;
    chk("LINK: HEAD 2 gezogen, alle im Verhältnis 1:2:3:4 (Faktor)", f, f > 1.2 && rat);
    for (int i = 0; i < 6; i++) { send(ButtonPress, kx(1910), kx(128), 4); tick(); } chk("LINK: Mausrad HEAD 4 bis an die Grenze, HEAD 4 höchstens 770 ms", pv(23), pv(23) <= 770 + 1e-9 && std::fabs(pv(20) * 4 - pv(23)) < 1e-6);
    click(kx(1820), kx(128)); click(kx(1820), kx(128)); bool org = true; for (int j = 0; j < 4; j++) if (pv(20 + j) != h0 * (j + 1)) org = false; chk("LINK: Doppelklick HEAD 3 stellt alle auf das Original", org ? 1 : 0, org);
    click(kx(1995), kx(150)); chk("LINK wieder angeklickt: aus (Soll 0)", pv(24), pv(24) == 0);
    send(ButtonPress, kx(1640), kx(128), 4); tick(); chk("ohne LINK: Mausrad HEAD 1 bewegt nur HEAD 1 (HEAD 2 bleibt Original)", pv(21), pv(20) != h0 && pv(21) == 2 * h0);
    click(kx(1640), kx(128)); click(kx(1640), kx(128)); }
  outPk = 0; lrDiff = 0; audio(200); chk("Ausgang Spitze (Soll > 0,05)", outPk, outPk > 0.05); chk("links und rechts gleich (Soll < 1e-7)", lrDiff, lrDiff < 1e-7);
  send(MotionNotify, kx(1342), kx(172), 0); tick(); usleep(200000); for (int i = 0; i < 6; i++) tick();
  char t[96]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 96); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  { std::string cf = std::string(home) + "/.config/tonestack/ui.conf"; auto rd = [&]() { int tp = -1; FILE *f = fopen(cf.c_str(), "r"); if (!f) return tp; char k[32]; int v; while (fscanf(f, " %31[a-z]=%d", k, &v) == 2) if (!strcmp(k, "tips")) tp = v; fclose(f); return tp; };
    click(ux(460), uy(16)); int tp = rd(); chk("Kopfzeile ohne VALUE POPUP (ab 18.11.2026): Klick an die alte Stelle schreibt nichts (Soll -1)", tp, tp == -1);
    remove(cf.c_str()); rmdir((std::string(home) + "/.config/tonestack").c_str()); rmdir((std::string(home) + "/.config").c_str()); rmdir(home); }
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  if (bad) { printf("FEHLER: %d Prüfung(en)\n", bad); return 1; } printf("alle Prüfungen bestanden\n"); return 0;
}
