// Audio-Unit-Prüfung wie in Logic (über die AU-Hülle von clap-wrapper): aucheck <Subtyp> [Sekunden] [Block]
// Lädt aufx/<Subtyp>/Tnst, meldet die Parameterliste (Logic sieht nur diese), misst Instanz/Initialize/Uninitialize/Dispose und je Render-Aufruf
// die Rechenzeit relativ zur Blockdauer: Grundstellung, Zufalls-Automation (AudioUnitSetParameter aus einem zweiten Thread, wie Logic),
// Transportlücken, Zustand (ClassInfo) speichern und laden während des Rechnens, Abtastratenwechsel. Ausgabe: Zeilen "RESULT ...".
// Läuft nativ (arm64) und unter Rosetta (arch -x86_64).
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
using Clock = std::chrono::steady_clock;
static double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }
static unsigned rs = 20261103; static double urand() { rs = rs * 1664525u + 1013904223u; return (rs >> 8) / 16777216.0; }
static double fs = 48000; static long pos = 0; static std::atomic<int> sigMode{0};   // 0 Bass, 1 Stille
static double bass(double t) { double env = 0.5 * std::exp(-2.5 * std::fmod(t, 0.8)); return env * (std::sin(2 * M_PI * 55 * t) + 0.3 * std::sin(2 * M_PI * 110 * t + 0.4)); }
static OSStatus input(void *, AudioUnitRenderActionFlags *, const AudioTimeStamp *ts, UInt32, UInt32 n, AudioBufferList *io) {
  for (UInt32 b = 0; b < io->mNumberBuffers; b++) { float *d = (float*)io->mBuffers[b].mData; for (UInt32 i = 0; i < n; i++) { double t = (ts->mSampleTime + i) / fs; d[i] = sigMode ? 0.f : (float)bass(t + b * 0.0007); } }
  return noErr; }
struct Stat { std::vector<double> r; int nan = 0; int err = 0; double worstMs = 0; long worstAt = -1; };
static void summary(const char *ph, Stat &S) {
  if (S.r.empty()) return; std::vector<double> v = S.r; std::sort(v.begin(), v.end()); double m = 0; for (double x : v) m += x; m /= v.size(); int over = 0; for (double x : v) if (x > 1) over++;
  std::printf("RESULT kind=au phase=%s fs=%.0f blocks=%zu mean=%.1f p999=%.1f max=%.1f over100=%d nan=%d err=%d worst_ms=%.2f worst_block=%ld\n", ph, fs, v.size(), m * 100,
    v[std::min(v.size() - 1, (size_t)(v.size() * 0.999))] * 100, v.back() * 100, over, S.nan, S.err, S.worstMs, S.worstAt); std::fflush(stdout); }
int main(int argc, char **argv) {
  if (argc < 2) { std::printf("aucheck <Subtyp> [Sekunden] [Block]\n"); return 2; }
  OSType sub = ((OSType)argv[1][0] << 24) | ((OSType)argv[1][1] << 16) | ((OSType)argv[1][2] << 8) | (OSType)argv[1][3];
  double secs = argc > 2 ? std::atof(argv[2]) : 20; UInt32 B = argc > 3 ? (UInt32)std::atoi(argv[3]) : 128;
  AudioComponentDescription d = {kAudioUnitType_Effect, sub, 'Tnst', 0, 0}; AudioComponent comp = AudioComponentFindNext(nullptr, &d);
  if (!comp) { std::printf("RESULT kind=au_error what=component_not_found\n"); return 1; }
  auto t0 = Clock::now(); AudioComponentInstance au = nullptr; OSStatus e = AudioComponentInstanceNew(comp, &au); std::printf("RESULT kind=au_main what=instance_new ms=%.1f status=%d\n", since(t0) * 1000, (int)e); if (e) return 1;
  // Parameterliste (was Logic anzeigt und automatisiert)
  UInt32 sz = 0; AudioUnitGetPropertyInfo(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, &sz, nullptr); std::vector<AudioUnitParameterID> ids(sz / sizeof(AudioUnitParameterID));
  if (sz) AudioUnitGetProperty(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, ids.data(), &sz);
  std::vector<AudioUnitParameterInfo> pinf(ids.size()); std::string plist;
  for (size_t i = 0; i < ids.size(); i++) { UInt32 s2 = sizeof(AudioUnitParameterInfo); AudioUnitGetProperty(au, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, ids[i], &pinf[i], &s2);
    char nm[128] = ""; if (pinf[i].cfNameString) CFStringGetCString(pinf[i].cfNameString, nm, sizeof nm, kCFStringEncodingUTF8); else std::snprintf(nm, sizeof nm, "%s", pinf[i].name);
    char b[260]; std::snprintf(b, sizeof b, "%u:%s[%g..%g]%s; ", (unsigned)ids[i], nm, pinf[i].minValue, pinf[i].maxValue, pinf[i].unit == kAudioUnitParameterUnit_Indexed ? "idx" : ""); plist += b; }
  std::printf("RESULT kind=au_params count=%zu list=\"%s\"\n", ids.size(), plist.c_str());
  std::vector<float> L(8192), R(8192); struct { AudioBufferList l; AudioBuffer extra; } abl;
  auto setup = [&](double rate) { fs = rate; AudioStreamBasicDescription f = {}; f.mSampleRate = rate; f.mFormatID = kAudioFormatLinearPCM; f.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    f.mBytesPerPacket = 4; f.mFramesPerPacket = 1; f.mBytesPerFrame = 4; f.mChannelsPerFrame = 2; f.mBitsPerChannel = 32;
    OSStatus a = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f), b = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &f, sizeof f);
    UInt32 mx = 1024; AudioUnitSetProperty(au, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &mx, sizeof mx);
    AURenderCallbackStruct cb = {input, nullptr}; AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof cb);
    auto t1 = Clock::now(); OSStatus c = AudioUnitInitialize(au); std::printf("RESULT kind=au_main what=initialize@%.0f ms=%.1f status=%d/%d/%d\n", rate, since(t1) * 1000, (int)a, (int)b, (int)c); std::fflush(stdout); };
  auto uninit = [&]() { auto t1 = Clock::now(); AudioUnitUninitialize(au); std::printf("RESULT kind=au_main what=uninitialize ms=%.1f\n", since(t1) * 1000); };
  auto render = [&](Stat &S, UInt32 n) { abl.l.mNumberBuffers = 2; abl.l.mBuffers[0] = {1, n * 4, L.data()}; abl.extra = {1, n * 4, R.data()};
    AudioUnitRenderActionFlags fl = 0; AudioTimeStamp ts = {}; ts.mSampleTime = pos; ts.mFlags = kAudioTimeStampSampleTimeValid;
    auto t1 = Clock::now(); OSStatus r = AudioUnitRender(au, &fl, &ts, 0, n, &abl.l); double dt = since(t1); pos += n;
    S.r.push_back(dt / (n / fs)); if (dt * 1000 > S.worstMs) { S.worstMs = dt * 1000; S.worstAt = (long)S.r.size() - 1; } if (r) S.err++;
    for (int c = 0; c < 2; c++) { float *p = c ? R.data() : L.data(); for (UInt32 i = 0; i < n; i++) if (!std::isfinite(p[i])) { S.nan++; break; } } };
  auto nb = [&](double s) { return (long)(s * fs / B); };
  setup(48000);
  { Stat S; for (long k = 0; k < nb(secs / 2); k++) render(S, B); summary("grundstellung", S); }
  // Automation aus einem zweiten Thread (Logic setzt Parameter aus seinem Automations-/Hauptthread)
  { Stat S; std::atomic<bool> stop{false}; std::thread aut([&]() { while (!stop) { size_t i = (size_t)(urand() * ids.size()); const AudioUnitParameterInfo &pi = pinf[i];
        double v = pi.minValue + urand() * (pi.maxValue - pi.minValue); if (pi.unit == kAudioUnitParameterUnit_Indexed || pi.unit == kAudioUnitParameterUnit_Boolean) v = std::round(v);
        AudioUnitSetParameter(au, ids[i], kAudioUnitScope_Global, 0, (AudioUnitParameterValue)v, 0); std::this_thread::sleep_for(std::chrono::milliseconds(20 + (int)(urand() * 280))); } });
    for (long k = 0; k < nb(secs * 3); k++) { sigMode = (k / nb(4)) % 3 == 2; render(S, B); }
    stop = true; aut.join(); sigMode = 0; summary("zufall_automation", S); }
  // Transportlücken (Logic ruft Spuren ohne Wiedergabe nicht auf)
  { Stat S, F; for (int r = 0; r < 6; r++) { for (long k = 0; k < nb(1.5); k++) render(k == 0 && r > 0 ? F : S, B); std::this_thread::sleep_for(std::chrono::milliseconds(350)); }
    summary("transport_laufend", S); summary("transport_erster_block", F); }
  // Zustand speichern/laden (Projekt sichern, Preset wählen) im zweiten Thread während des Rechnens
  { Stat S; std::atomic<bool> stop{false}; double maxSet = 0, maxGet = 0; int n = 0;
    std::thread st([&]() { while (!stop) { CFPropertyListRef pl = nullptr; UInt32 s2 = sizeof pl; auto a = Clock::now(); OSStatus g = AudioUnitGetProperty(au, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &pl, &s2); maxGet = std::max(maxGet, since(a));
        if (!g && pl) { auto b = Clock::now(); AudioUnitSetProperty(au, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &pl, sizeof pl); maxSet = std::max(maxSet, since(b)); CFRelease(pl); n++; }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); } });
    for (long k = 0; k < nb(secs); k++) render(S, B); stop = true; st.join(); summary("zustand_parallel", S);
    std::printf("RESULT kind=au_main what=classinfo get_max_ms=%.1f set_max_ms=%.1f rounds=%d\n", maxGet * 1000, maxSet * 1000, n); }
  // Abtastraten wie in Logic-Projekten
  for (double r : {44100.0, 96000.0, 48000.0}) { uninit(); setup(r); Stat S; for (long k = 0; k < nb(secs / 4); k++) render(S, B); char ph[32]; std::snprintf(ph, sizeof ph, "rate_%.0f", r); summary(ph, S); }
  // größere Blöcke (Logic: Spuren ohne Live-Eingang rechnen mit 1024)
  { Stat S; for (long k = 0; k < nb(secs / 2) / 8; k++) render(S, 1024); summary("block_1024", S); }
  uninit(); auto t2 = Clock::now(); AudioComponentInstanceDispose(au); std::printf("RESULT kind=au_main what=dispose ms=%.1f\n", since(t2) * 1000);
  return 0; }
