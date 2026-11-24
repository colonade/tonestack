// GUI-Test (Cultec EQ-1A): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Audio rechnen, Mausereignisse senden, Bild speichern.
//   cultecguitest Cultec_EQ1A.clap bild.png
// Prüft: Drehregler ziehen, Mausrad, Doppelklick (0 bzw. Mitte), Wahlschalter (Klick rechts/links, Rad, Ziehen), IN-Schalter, LEVEL, OUTPUT, OVERSAMPLING
// (Umbau im Hauptfaden, danach weiter Ton), ab 1.1 MODEL EQP-1R (zweites Bild …_r.png). Rückgabe 1 bei Abweichung.
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
  if (argc < 3) { printf("cultecguitest PLUGIN bild.png\n"); return 1; }
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
  // Parameter: 0 LB, 1 LA, 2 LF, 3 HB, 4 BW, 5 HF, 6 HC, 7 HCS, 8 IN, 9 LEVEL, 10 OS, 11 OUT. Frontplatte 1600 × 420: LB 300/160, LA 530/160, HB 830/160, HC 1090/160,
  // BW 680/330, CPS 415/330, KCS 960/330, ATTEN SEL 1320/160, IN 150/330
  auto drag = [&](int x, int y, int dy) { send(ButtonPress, x, y, 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, x, y - i * dy / 10, 0); tick(); } send(ButtonRelease, x, y - dy, 1); tick(); };
  drag(kx(300), kx(160), 80); { double v = pv(0); bool ok = v > 2; if (!ok) bad++; printf("LOW BOOST nach oben gezogen: %g (Soll > 2)%s\n", v, ok ? "" : "   ABWEICHUNG"); }
  dbl(kx(300), kx(160)); chk("LOW BOOST Doppelklick: 0", pv(0), 0);
  send(ButtonPress, kx(830), kx(160), 4); tick(); chk("HIGH BOOST Mausrad hoch", pv(3), 0.2, 1e-9);
  drag(kx(1090), kx(160), 60); { double v = pv(6); bool ok = v > 1; if (!ok) bad++; printf("HIGH ATTEN gezogen: %g (Soll > 1)%s\n", v, ok ? "" : "   ABWEICHUNG"); }
  drag(kx(680), kx(330), -40); { double v = pv(4); bool ok = v < 5; if (!ok) bad++; printf("BANDWIDTH nach unten (schmaler): %g (Soll < 5)%s\n", v, ok ? "" : "   ABWEICHUNG"); }
  dbl(kx(680), kx(330)); chk("BANDWIDTH Doppelklick: Mitte", pv(4), 5);
  click(kx(415) + 10, kx(330)); chk("CPS Klick rechts: 100 Hz", pv(2), 3); click(kx(415) - 10, kx(330)); chk("CPS Klick links: 60 Hz", pv(2), 2);
  send(ButtonPress, kx(415), kx(330), 5); tick(); chk("CPS Mausrad runter: 30 Hz", pv(2), 1);
  drag(kx(960), kx(330), 120); { double v = pv(5); bool ok = v >= 4; if (!ok) bad++; printf("KCS nach oben gezogen: %g (Soll ≥ 4)%s\n", v, ok ? "" : "   ABWEICHUNG"); }
  click(kx(1320) + 10, kx(160)); chk("ATTEN SEL Klick rechts: 20 kHz", pv(7), 2);
  click(kx(150), kx(330)); chk("IN-Schalter: OUT", pv(8), 0); click(kx(150), kx(330)); chk("IN-Schalter: IN", pv(8), 1);
  { auto uy4 = [&](double v) { return (int)(420 * s + v * uu); };
    send(ButtonPress, ux(700), uy4(62), 1); for (int i = 1; i <= 5; i++) { send(MotionNotify, ux(700), uy4(62) - i * 6, 0); tick(); } send(ButtonRelease, ux(700), uy4(32), 1); tick(); { double l = pv(9); bool ok = l > 18; if (!ok) bad++; printf("LEVEL hoch: %g (Soll > 18)%s\n", l, ok ? "" : "   ABWEICHUNG"); }
    send(ButtonPress, ux(1060), uy4(62), 1); for (int i = 1; i <= 5; i++) { send(MotionNotify, ux(1060), uy4(62) - i * 6, 0); tick(); } send(ButtonRelease, ux(1060), uy4(32), 1); tick(); { double o = pv(11); bool ok = o > 0; if (!ok) bad++; printf("OUTPUT hoch: %g (Soll > 0)%s\n", o, ok ? "" : "   ABWEICHUNG"); }
    click(ux(880), uy4(68)); chk("OVERSAMPLING Klick: 4x → 1x", pv(10), 0); for (int i = 0; i < 20; i++) tick(); }
  send(ButtonPress, kx(300), kx(160), 4); tick(); send(ButtonPress, kx(300), kx(160), 4); tick(); send(ButtonPress, kx(830), kx(160), 4); tick();
  audio(40); printf("Ausgang Spitze: %.3f\n", outPk); if (!(outPk > 0.01 && outPk < 5)) bad++;
  send(MotionNotify, kx(830), kx(160), 0); for (int i = 0; i < 6; i++) tick(); usleep(200000);
  char t[96]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 96); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  // ab 1.1: MODEL EQP-1R (eigene Frontplatte, eigene Wahlschalter-Parameter 13/14; ATTEN SEL ohne Wirkung), Umbau im Hauptfaden, danach weiter Ton
  { auto uy4 = [&](double v) { return (int)(420 * s + v * uu); };
    const double lf0 = pv(2), hf0 = pv(5), hcs0 = pv(7);
    click(ux(545), uy4(96)); chk("MODEL Klick unten: EQP-1R", pv(12), 1); for (int i = 0; i < 20; i++) tick();
    click(kx(415) + 10, kx(330)); chk("EQP-1R CPS Klick rechts: 100 Hz", pv(13), 2); click(kx(415) - 10, kx(330)); click(kx(415) - 10, kx(330)); chk("EQP-1R CPS zweimal links: 30 Hz", pv(13), 0);
    send(ButtonPress, kx(960), kx(330), 4); tick(); send(ButtonPress, kx(960), kx(330), 4); tick(); send(ButtonPress, kx(960), kx(330), 4); tick(); send(ButtonPress, kx(960), kx(330), 4); tick();
    chk("EQP-1R KCS Mausrad 4× hoch: 12 kHz (Ende)", pv(14), 4);
    click(kx(1320) + 10, kx(160)); chk("EQP-1R: Schild statt ATTEN SEL (ohne Wirkung)", pv(7), hcs0);
    chk("EQP-1A-Wahlschalter unverändert: CPS", pv(2), lf0); chk("EQP-1A-Wahlschalter unverändert: KCS", pv(5), hf0);
    audio(60); printf("Ausgang Spitze (EQP-1R): %.3f\n", outPk); if (!(outPk > 0.01 && outPk < 5)) bad++;
    send(MotionNotify, kx(830), kx(160), 0); for (int i = 0; i < 6; i++) tick(); usleep(200000);
    std::string p2 = argv[2]; size_t dot = p2.rfind('.'); p2 = (dot == std::string::npos ? p2 : p2.substr(0, dot)) + "_r.png";
    cairo_surface_t *xs2 = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs2, p2.c_str()); cairo_surface_destroy(xs2);
    click(ux(545), uy4(52)); chk("MODEL Klick oben: zurück EQP-1A", pv(12), 0); for (int i = 0; i < 20; i++) tick(); audio(40); if (!(outPk > 0.01 && outPk < 5)) { bad++; printf("kein Ton nach Rückwechsel\n"); } }
  // ab 1.2: Host stellt MODEL um, während an ATTEN SEL gezogen wird → Ziehen endet, kein anderer Parameter (OUTPUT, OVERSAMPLING) verändert;
  // nicht endliche Werte vom Host werden verworfen
  { static clap_event_param_value_t hev; auto hostSet = [&](int id, double v) { hev = clap_event_param_value_t{}; hev.header.size = sizeof hev; hev.header.type = CLAP_EVENT_PARAM_VALUE; hev.header.space_id = CLAP_CORE_EVENT_SPACE_ID; hev.param_id = id; hev.value = v; hev.note_id = -1; hev.port_index = -1; hev.channel = -1; hev.key = -1;
      clap_input_events_t ie = {nullptr, [](const clap_input_events_t *) -> uint32_t { return 1; }, [](const clap_input_events_t *, uint32_t) -> const clap_event_header_t * { return &hev.header; }}; clap_output_events_t oe = {nullptr, push}; params->flush(p, &ie, &oe); };
    const double out0 = pv(11), os0 = pv(10), hcs0 = pv(7);
    send(ButtonPress, kx(1320), kx(160), 1); tick(); hostSet(12, 1); for (int i = 0; i < 5; i++) tick();
    for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(1320), kx(160) - i * 12, 0); tick(); } send(ButtonRelease, kx(1320), kx(160) - 120, 1); tick();
    chk("Modellwechsel beim Ziehen: OUTPUT unverändert", pv(11), out0); chk("Modellwechsel beim Ziehen: OVERSAMPLING unverändert", pv(10), os0); chk("Modellwechsel beim Ziehen: ATTEN SEL unverändert", pv(7), hcs0);
    for (int i = 0; i < 20; i++) tick(); audio(40); if (!(outPk > 0.01 && outPk < 5)) { bad++; printf("kein Ton nach Modellwechsel durch den Host\n"); }
    hostSet(12, 0); for (int i = 0; i < 20; i++) tick();
    const double lb0 = pv(0); hostSet(0, std::nan("")); chk("NaN vom Host verworfen (LOW BOOST)", pv(0), lb0); hostSet(11, INFINITY); chk("+∞ vom Host verworfen (OUTPUT)", pv(11), out0);
    audio(40); if (!(outPk > 0.01 && outPk < 5)) { bad++; printf("kein Ton nach NaN\n"); } }
  gui->hide(p); gui->destroy(p); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  printf("%s\n", bad ? "FEHLER" : "alle Prüfungen bestanden"); return bad ? 1 : 0;
}
