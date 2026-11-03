// Umschalttest (Rechenkern Laney): während eines laufenden Moduswechsels (Nachrechnen, replay) erneut umschalten.
// Fand den Speicherfehler in Engine::setTarget (bis 3.10); mit -fsanitize=address bauen, um Speicherfehler sicher zu sehen.
#include "core.hpp"
#include <cstdio>
using namespace aor;
int main() { double fs = 48000; Engine E(fs); E.pad = 31; Target t; t.v = Parts::orig(); t.s.v1 = t.s.v2 = t.s.pl = t.s.t = t.s.b = t.s.mv = 0.5; t.s.m = taper(0.5, 0); t.s.pres = taper(0.5, 0); t.pa = true; t.vin = 0.1; t.os = 8;
  std::vector<float> buf(128); double ph = 0; int hits = 0;
  for (int blk = 0; blk < 4000; blk++) {
    if (blk % 7 == 0) { bool during = E.b && E.replay; t.nl = !t.nl; t.os = (blk / 7) % 2 ? 8 : 2; if (during) hits++; E.setTarget(t); }   // alle 7 Blöcke umschalten, oft mitten im Nachrechnen
    for (auto &x : buf) { x = 0.4f * std::sin(ph); ph += 2 * PI * 82.4 / fs; } E.block(buf.data(), 128);
    for (float x : buf) if (!std::isfinite(x)) { printf("nicht endlich!\n"); return 1; } }
  printf("Umschaltungen mitten im Moduswechsel: %d, kein Fehler\n", hits); return 0; }
