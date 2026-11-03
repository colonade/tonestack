// Belastungstest für die CLAP-Fassung wie in REAPER: Audio-Thread rechnet dauernd, Oberfläche wird mehrfach geöffnet und
// geschlossen, dazwischen zufällige Klicks (Schalter, Röhren, Buchsen, Knöpfe), Ziehen, Mausrad.   clapstress plugin.clap [runden]
#include "clap-1.2.2/include/clap/clap.h"
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
static std::atomic<bool> flushReq{false};
static bool ok_timer(const clap_host_t *, uint32_t, clap_id *id) { *id = 1; return true; }
static bool ok_untimer(const clap_host_t *, clap_id) { return true; }
static const clap_host_timer_support_t HT = {ok_timer, ok_untimer};
static void req_flush(const clap_host_t *) { flushReq = true; } static void req_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void req_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {}
static const clap_host_params_t HP = {req_rescan, req_clear, req_flush};
static const void *host_ext(const clap_host_t *, const char *id) { if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &HT; if (!strcmp(id, CLAP_EXT_PARAMS)) return &HP; return nullptr; }
static void host_nop(const clap_host_t *) {}
static std::atomic<int> outEvents{0};
static uint32_t ev0(const clap_input_events_t *) { return 0; } static const clap_event_header_t *evg(const clap_input_events_t *, uint32_t) { return nullptr; }
static bool push(const clap_output_events_t *, const clap_event_header_t *) { outEvents++; return true; }
int main(int argc, char **argv) {
  int rounds = argc > 2 ? atoi(argv[2]) : 8; Display *d = XOpenDisplay(nullptr); if (!d) { printf("kein Display\n"); return 1; }
  void *lib = dlopen(argv[1], RTLD_NOW); if (!lib) { printf("%s\n", dlerror()); return 2; } auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); clap_host_t host = {CLAP_VERSION_INIT, nullptr, "stress", "", "", "1", host_ext, host_nop, host_nop, host_nop};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, fac->get_plugin_descriptor(fac, 0)->id); p->init(p); p->activate(p, 48000, 16, 512); p->start_processing(p);
  auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI); auto *tim = (const clap_plugin_timer_support_t*)p->get_extension(p, CLAP_EXT_TIMER_SUPPORT); auto *params = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS);
  std::mutex mainMx;   // CLAP: params.flush nie gleichzeitig mit process
  std::atomic<bool> run{true}; std::atomic<long> blocks{0}; std::atomic<int> bad{0};
  std::thread audio([&] { std::mt19937 rng(1); std::vector<float> l(512), r(512), ol(512), orr(512); double ph = 0; clap_input_events_t in{nullptr, ev0, evg}; clap_output_events_t out{nullptr, push};
    while (run) { int n = 16 + (int)(rng() % 496); for (int i = 0; i < n; i++) { l[i] = r[i] = 0.4f * (float)std::sin(ph); ph += 2 * M_PI * 82.4 / 48000; }
      float *ins[2] = {l.data(), r.data()}, *outs[2] = {ol.data(), orr.data()}; clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
      clap_process_t pr{}; pr.frames_count = n; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &in; pr.out_events = &out;
      { std::lock_guard<std::mutex> lk(mainMx); p->process(p, &pr); } for (int i = 0; i < n; i++) if (!std::isfinite(ol[i])) bad++; blocks++; usleep(n * 1000000 / 48000 / 4); } });
  std::mt19937 rng(42); Window root = RootWindow(d, DefaultScreen(d));
  for (int rd = 0; rd < rounds; rd++) {
    gui->create(p, CLAP_WINDOW_API_X11, false); uint32_t w, h; gui->get_size(p, &w, &h);
    Window par = XCreateSimpleWindow(d, root, 0, 0, w, h, 0, 0, 0); XMapWindow(d, par); XSync(d, False);
    clap_window_t cw{}; cw.api = CLAP_WINDOW_API_X11; cw.x11 = par; gui->set_parent(p, &cw); gui->show(p);
    auto tick = [&](int ms) { for (int i = 0; i < ms / 10 + 1; i++) { tim->on_timer(p, 1); if (flushReq.exchange(false)) { clap_input_events_t in{nullptr, ev0, evg}; clap_output_events_t out{nullptr, push}; std::lock_guard<std::mutex> lk(mainMx); params->flush(p, &in, &out); } usleep(10000); } };
    tick(60); Window rr, pp, *ch; unsigned nch = 0; XQueryTree(d, par, &rr, &pp, &ch, &nch); Window win = nch ? ch[0] : 0; if (ch) XFree(ch);
    auto send = [&](int type, int x, int y, int btn) { if (!win) return; XEvent e{}; e.type = type; e.xbutton.window = win; e.xbutton.display = d; e.xbutton.x = x; e.xbutton.y = y; e.xbutton.button = btn; e.xbutton.time = (Time)(rng() % 100000); e.xbutton.same_screen = True; if (type == MotionNotify) { e.xmotion.x = x; e.xmotion.y = y; e.xmotion.window = win; }
      XSendEvent(d, win, True, type == MotionNotify ? PointerMotionMask : (type == ButtonPress ? ButtonPressMask : ButtonReleaseMask), &e); XFlush(d); };
    for (int k = 0; k < 80; k++) { int x = rng() % w, y = rng() % h, a = rng() % 4;
      if (a <= 1) { send(ButtonPress, x, y, 1); send(ButtonRelease, x, y, 1); } else if (a == 2) { send(ButtonPress, x, y, 1); for (int i = 1; i <= 5; i++) { send(MotionNotify, x, y - i * 7, 0); tick(3); } send(ButtonRelease, x, y - 35, 1); } else send(ButtonPress, x, y, rng() % 2 ? 4 : 5);
      tick(rng() % 12); }
    if (rd % 2) { uint32_t w2 = w * 3 / 2, h2 = 0; gui->adjust_size(p, &w2, &h2); XResizeWindow(d, par, w2, h2); XSync(d, False); gui->set_size(p, w2, h2); tick(40); }
    if (rd % 3 == 2) { XDestroyWindow(d, par); XSync(d, False); tick(60); gui->hide(p); gui->destroy(p); }   // Container zuerst weg
    else { gui->hide(p); gui->destroy(p); XDestroyWindow(d, par); XSync(d, False); }
    double v; params->get_value(p, 16, &v);
    printf("Runde %d ok, Audio-Blöcke %ld, Ausgabe-Ereignisse %d\n", rd + 1, blocks.load(), outEvents.load()); fflush(stdout); }
  run = false; audio.join(); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit(); dlclose(lib); XCloseDisplay(d);
  printf("fertig: %d nicht-endliche Ausgabewerte\n", bad.load()); return bad ? 5 : 0;
}
