// GUI-Test (Yamaha): Plugin laden, Fenster in ein Elternfenster einbetten (Xvfb), Mausereignisse senden, Bild speichern
#include "clap-1.2.2/include/clap/clap.h"
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <initializer_list>
static bool ok_timer(const clap_host_t *, uint32_t, clap_id *id) { *id = 1; return true; }
static bool ok_untimer(const clap_host_t *, clap_id) { return true; }
static const clap_host_timer_support_t HT = {ok_timer, ok_untimer};
static void req_flush(const clap_host_t *) {} static void req_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void req_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static const clap_host_params_t HP = {req_rescan, req_clear, req_flush};
static const void *host_ext(const clap_host_t *, const char *id) { if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &HT; if (!strcmp(id, CLAP_EXT_PARAMS)) return &HP; return nullptr; }
static void host_nop(const clap_host_t *) {}
int main(int argc, char **argv) {
  Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  Window root = RootWindow(d, DefaultScreen(d)); Window par = XCreateSimpleWindow(d, root, 0, 0, 1200, 420, 0, 0, 0x202020); XMapWindow(d, par); XFlush(d);
  void *lib = dlopen(argv[1], RTLD_NOW); auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *desc = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "test", "", "", "1", host_ext, host_nop, host_nop, host_nop};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, desc->id); p->init(p); p->activate(p, 48000, 32, 512);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  printf("GUI x11: %d\n", gui->is_api_supported(p, CLAP_WINDOW_API_X11, false)); gui->create(p, CLAP_WINDOW_API_X11, false);
  uint32_t w, h; gui->get_size(p, &w, &h); printf("Größe %u x %u\n", w, h); XResizeWindow(d, par, w, h);
  clap_window_t cw{}; cw.api = CLAP_WINDOW_API_X11; cw.x11 = par; gui->set_parent(p, &cw); gui->show(p);
  for (int i = 0; i < 5; i++) { tim->on_timer(p, 1); usleep(30000); }
  Window r, pr; Window *ch; unsigned nch; XQueryTree(d, par, &r, &pr, &ch, &nch); Window win = nch ? ch[0] : 0; printf("Kindfenster: %lu\n", (unsigned long)win);
  auto send = [&](int type, int x, int y, int button) { XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = button; e.xbutton.time = 1000; e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; e.xmotion.window = win; } XSendEvent(d, win, True, type == MotionNotify ? PointerMotionMask : (type == ButtonPress ? ButtonPressMask : ButtonReleaseMask), &e); XFlush(d); };
  double s = w / 1800.0; auto kx = [&](double v) { return (int)(v * s); };
  // unterer Bereich in UI-Einheiten (1200 breit), beginnt unter dem Foto (Höhe 227 Foto-Einheiten)
  double uu = w / 1200.0; auto ux = [&](double v) { return (int)(v * uu); }; auto uy = [&](double v) { return (int)(227 * s + v * uu); };
  // Treble nach oben ziehen; LOW antippen; Endstufe, Lautsprecher antippen; über 60-Hz-Knopf schweben
  send(ButtonPress, kx(456.7), kx(119.3), 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, kx(456.7), kx(119.3) - i * 8, 0); tim->on_timer(p, 1); } send(ButtonRelease, kx(456.7), kx(39), 1); tim->on_timer(p, 1);
  send(ButtonPress, kx(217), kx(120), 1); send(ButtonRelease, kx(217), kx(120), 1); tim->on_timer(p, 1);
  // INPUT und OUTPUT nach oben ziehen (bis 1.3 nicht getroffen)
  { auto drag = [&](int x, int y) { send(ButtonPress, x, y, 1); tim->on_timer(p, 1); for (int i = 1; i <= 10; i++) { send(MotionNotify, x, y - i * 4, 0); tim->on_timer(p, 1); } send(ButtonRelease, x, y - 40, 1); tim->on_timer(p, 1); };
    drag(ux(830), uy(88)); drag(ux(1046), uy(88)); }
  // Kippschalter: Endstufe an, Lautsprecher an, Kennlinie eine Stufe nach unten (LOG 10 %)
  send(ButtonPress, ux(64), uy(88), 1); send(ButtonRelease, ux(64), uy(88), 1); tim->on_timer(p, 1);
  send(ButtonPress, ux(186), uy(88), 1); send(ButtonRelease, ux(186), uy(88), 1); tim->on_timer(p, 1);
  send(ButtonPress, kx(982.4), kx(119.8), 4); tim->on_timer(p, 1); send(ButtonPress, kx(982.4), kx(119.8), 4); tim->on_timer(p, 1);
  send(MotionNotify, kx(982.4), kx(119.8), 0); tim->on_timer(p, 1); usleep(200000); tim->on_timer(p, 1);
  char t[64]; for (uint32_t id = 0; id < params->count(p); id++) { double v; params->get_value(p, id, &v); clap_param_info_t inf; params->get_info(p, id, &inf); params->value_to_text(p, id, v, t, 64); printf("  %s = %s\n", inf.name, t); }
  cairo_surface_t *xs = cairo_xlib_surface_create(d, par, DefaultVisual(d, DefaultScreen(d)), w, h); cairo_surface_write_to_png(xs, argv[2]); cairo_surface_destroy(xs);
  gui->hide(p); gui->destroy(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d); return 0;
}
