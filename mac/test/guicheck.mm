// Oberflächen-Prüfung am echten Mac-Fenster: guicheck <Plugin-Binärdatei im .clap-Bündel> [Sekunden]
// Lädt das CLAP, rechnet in einem Audio-Thread in Echtzeit (Bass-Signal, Zufalls-Automation), öffnet die Oberfläche in einem NSWindow und misst:
//  - Rechenzeit des Haupt-Threads (Zeichnen, Takt) in % eines Kerns: offen ohne Bedienung, mit Zufallsklicks/Ziehen/Mausrad
//  - längste Blockade des Haupt-Threads (ein 100-Hz-Taktgeber misst, wie verspätet er dran kommt)
//  - wiederholtes Öffnen/Schließen und Größenänderungen während der Audio-Thread rechnet (darf nicht abstürzen)
// Ausgabe: Zeilen "RESULT ...". Läuft nativ und unter Rosetta (arch -x86_64).
#import <Cocoa/Cocoa.h>
#include <mach/mach.h>
#include <dlfcn.h>
#include "clap-1.2.2/include/clap/clap.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>
using Clock = std::chrono::steady_clock;
static double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }
static unsigned rs = 777; static double urand() { rs = rs * 1664525u + 1013904223u; return (rs >> 8) / 16777216.0; }
static double threadCpu() { thread_basic_info_data_t i; mach_msg_type_number_t n = THREAD_BASIC_INFO_COUNT; mach_port_t t = mach_thread_self();
  thread_info(t, THREAD_BASIC_INFO, (thread_info_t)&i, &n); mach_port_deallocate(mach_task_self(), t); return i.user_time.seconds + i.user_time.microseconds * 1e-6 + i.system_time.seconds + i.system_time.microseconds * 1e-6; }
static std::atomic<int> flushReq{0};
static void req_rescan(const clap_host_t *, clap_param_rescan_flags) {} static void req_clear(const clap_host_t *, clap_id, clap_param_clear_flags) {} static void req_flush(const clap_host_t *) { flushReq++; }
static const clap_host_params_t HP = {req_rescan, req_clear, req_flush};
static const void *host_ext(const clap_host_t *, const char *id) { if (!strcmp(id, CLAP_EXT_PARAMS)) return &HP; return nullptr; }
static void host_nop(const clap_host_t *) {}
static std::mutex qm; static std::vector<clap_event_param_value_t> pending, Q;
static uint32_t q_size(const clap_input_events_t *) { return (uint32_t)Q.size(); } static const clap_event_header_t *q_get(const clap_input_events_t *, uint32_t i) { return &Q[i].header; }
static std::atomic<int> outEv{0}; static bool out_push(const clap_output_events_t *, const clap_event_header_t *) { outEv++; return true; }
static double bass(double t) { double env = 0.5 * std::exp(-2.5 * std::fmod(t, 0.8)); return env * (std::sin(2 * M_PI * 55 * t) + 0.3 * std::sin(2 * M_PI * 110 * t + 0.4)); }

@interface Stall : NSObject { @public Clock::time_point last; double maxGap; bool armed; } - (void)tick:(NSTimer *)t; @end
@implementation Stall - (void)tick:(NSTimer *)t { auto now = Clock::now(); if (armed) maxGap = std::max(maxGap, std::chrono::duration<double>(now - last).count()); last = now; armed = true; } @end

static void runFor(double s) { auto t0 = Clock::now(); while (since(t0) < s) @autoreleasepool { [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]]; } }
static void sendMouse(NSWindow *w, NSEventType t, NSPoint p, int clicks = 1) {
  NSEvent *e = [NSEvent mouseEventWithType:t location:p modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:[w windowNumber] context:nil eventNumber:0 clickCount:clicks pressure:1];
  [w sendEvent:e]; }
static void sendWheel(NSWindow *w, NSPoint p, int dy) {
  CGEventRef ce = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitLine, 1, dy); CGEventSetLocation(ce, CGPointMake(p.x, [w frame].size.height - p.y));
  NSEvent *e = [NSEvent eventWithCGEvent:ce]; CFRelease(ce); NSView *v = [[w contentView] hitTest:p]; if (v) [v scrollWheel:e]; }

int main(int argc, char **argv) { @autoreleasepool {
  if (argc < 2) { std::printf("guicheck <binary> [Sekunden]\n"); return 2; }
  double secs = argc > 2 ? atof(argv[2]) : 8;
  [NSApplication sharedApplication]; [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory]; [NSApp finishLaunching];
  void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL); if (!lib) { std::printf("RESULT kind=gui_error what=dlopen msg=\"%s\"\n", dlerror()); return 1; }
  auto *entry = (const clap_plugin_entry_t*)dlsym(lib, "clap_entry"); entry->init(argv[1]);
  auto *fac = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID); const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  clap_host_t host = {CLAP_VERSION_INIT, nullptr, "guicheck", "tonestack", "", "1", host_ext, host_nop, host_nop, host_nop};
  const clap_plugin_t *p = fac->create_plugin(fac, &host, d->id); p->init(p);
  auto *par = (const clap_plugin_params_t*)p->get_extension(p, CLAP_EXT_PARAMS); auto *gui = (const clap_plugin_gui_t*)p->get_extension(p, CLAP_EXT_GUI);
  std::vector<clap_param_info_t> info; for (uint32_t i = 0; i < par->count(p); i++) { clap_param_info_t in; par->get_info(p, i, &in); info.push_back(in); }
  double fs = 48000; const int B = 128; p->activate(p, fs, 32, 1024); p->start_processing(p);
  std::atomic<bool> stop{false}, automate{false}; std::atomic<double> audioMax{0};
  std::thread audio([&]() { std::vector<float> L(B), R(B), OL(B), OR(B); long pos = 0; auto next = Clock::now(); long nextEv = 0;
    while (!stop) { for (int i = 0; i < B; i++) { double t = (pos + i) / fs; L[i] = (float)bass(t); R[i] = L[i]; }
      { std::lock_guard<std::mutex> lk(qm); Q.swap(pending); pending.clear(); }
      if (automate && pos >= nextEv) { const clap_param_info_t &pi = info[(size_t)(urand() * info.size())]; if (!(pi.flags & CLAP_PARAM_IS_HIDDEN)) { double v = pi.min_value + urand() * (pi.max_value - pi.min_value); if (pi.flags & CLAP_PARAM_IS_STEPPED) v = std::round(v);
          clap_event_param_value_t e{}; e.header.size = sizeof e; e.header.space_id = CLAP_CORE_EVENT_SPACE_ID; e.header.type = CLAP_EVENT_PARAM_VALUE; e.param_id = pi.id; e.value = v; e.note_id = -1; e.port_index = -1; e.channel = -1; e.key = -1; Q.push_back(e); } nextEv = pos + (long)(0.3 * fs); }
      float *ins[2] = {L.data(), R.data()}, *outs[2] = {OL.data(), OR.data()}; clap_audio_buffer_t ib{}; ib.data32 = ins; ib.channel_count = 2; clap_audio_buffer_t ob{}; ob.data32 = outs; ob.channel_count = 2;
      clap_input_events_t ie = {nullptr, q_size, q_get}; clap_output_events_t oe = {nullptr, out_push};
      clap_process_t pr{}; pr.frames_count = B; pr.steady_time = pos; pr.audio_inputs = &ib; pr.audio_outputs = &ob; pr.audio_inputs_count = 1; pr.audio_outputs_count = 1; pr.in_events = &ie; pr.out_events = &oe;
      auto t0 = Clock::now(); p->process(p, &pr); double r = since(t0) / (B / fs); if (r > audioMax) audioMax = r; Q.clear(); pos += B;
      next += std::chrono::microseconds((long)(1e6 * B / fs)); std::this_thread::sleep_until(next); } });
  Stall *st = [Stall new]; NSTimer *tm = [NSTimer timerWithTimeInterval:0.01 target:st selector:@selector(tick:) userInfo:nil repeats:YES]; [[NSRunLoop currentRunLoop] addTimer:tm forMode:NSRunLoopCommonModes];
  uint32_t W = 1200, H = 0; NSWindow *win = nil;
  auto openGui = [&](uint32_t w) { if (!gui->create(p, CLAP_WINDOW_API_COCOA, false)) { std::printf("RESULT kind=gui_error what=create\n"); return false; }
    uint32_t ww = w, hh = 0; gui->adjust_size(p, &ww, &hh); gui->set_size(p, ww, hh); gui->get_size(p, &ww, &hh); W = ww; H = hh;
    if (!win) { win = [[NSWindow alloc] initWithContentRect:NSMakeRect(40, 40, W, H) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO]; [win setReleasedWhenClosed:NO]; }
    [win setContentSize:NSMakeSize(W, H)]; [win orderFrontRegardless];
    clap_window_t cw{}; cw.api = CLAP_WINDOW_API_COCOA; cw.cocoa = (__bridge void *)[win contentView]; if (!gui->set_parent(p, &cw)) { std::printf("RESULT kind=gui_error what=set_parent\n"); return false; }
    gui->show(p); return true; };
  auto measure = [&](const char *what, double s, bool clicks) { runFor(0.3); st->maxGap = 0; double c0 = threadCpu(); auto t0 = Clock::now(); int nClick = 0;
    while (since(t0) < s) { if (clicks) { NSPoint pt = NSMakePoint(urand() * W, urand() * H); double k = urand();
        if (k < 0.5) { sendMouse(win, NSEventTypeLeftMouseDown, pt); runFor(0.03); sendMouse(win, NSEventTypeLeftMouseUp, pt); }
        else if (k < 0.75) { sendMouse(win, NSEventTypeLeftMouseDown, pt); for (int j = 0; j < 6; j++) { pt.y += 8; sendMouse(win, NSEventTypeLeftMouseDragged, pt); runFor(0.016); } sendMouse(win, NSEventTypeLeftMouseUp, pt); }
        else if (k < 0.9) sendWheel(win, pt, urand() < 0.5 ? 1 : -1);
        else { sendMouse(win, NSEventTypeLeftMouseDown, pt, 2); sendMouse(win, NSEventTypeLeftMouseUp, pt, 2); }
        nClick++; }
      runFor(clicks ? 0.08 : 0.1); }
    double cpu = (threadCpu() - c0) / since(t0);
    std::printf("RESULT kind=gui what=%s width=%u main_cpu=%.1f max_main_stall_ms=%.1f clicks=%d audio_max=%.0f flushes=%d out_events=%d\n", what, W, cpu * 100, st->maxGap * 1000, nClick, audioMax.load() * 100, flushReq.load(), outEv.load()); std::fflush(stdout); };
  if (!openGui(1200)) return 1;
  measure("offen_ohne_bedienung", secs, false);
  automate = true; measure("offen_mit_automation", secs, false);
  measure("zufallsklicks", secs * 2, true);
  automate = false;
  gui->destroy(p); if (!openGui(2000)) return 1; measure("gross_2000_offen", secs, false);
  // Öffnen/Schließen und Größe ändern, während der Audio-Thread rechnet
  auto t0 = Clock::now(); int cycles = 0; automate = true;
  for (int i = 0; i < 30; i++) { gui->destroy(p); runFor(0.02); if (!openGui(700 + (uint32_t)(urand() * 1500))) return 1; runFor(0.05 + urand() * 0.2);
    for (int j = 0; j < 3; j++) { uint32_t w = 600 + (uint32_t)(urand() * 2000), h = 0; gui->adjust_size(p, &w, &h); gui->set_size(p, w, h); [win setContentSize:NSMakeSize(w, h)]; runFor(0.03); }
    if (i % 3 == 0) { gui->hide(p); runFor(0.05); gui->show(p); } cycles++; }
  std::printf("RESULT kind=gui what=oeffnen_schliessen cycles=%d sek=%.1f audio_max=%.0f\n", cycles, since(t0), audioMax.load() * 100);
  gui->destroy(p); runFor(0.2); [win close];
  stop = true; audio.join(); p->stop_processing(p); p->deactivate(p); p->destroy(p); entry->deinit();
  std::printf("RESULT kind=gui what=ende ok=1\n"); [tm invalidate]; return 0; } }
