// Kernwechsel ohne Knacks und ohne Speicherfreigabe im Audio-Faden (ab AmpFett 1.20, GK 1.5, G100 1.8, B100-III 1.5, SVT-CL 1.7)
// Bisher tauschte das Plugin beim Umschalten (Überabtastung, Netzteil, BAL OUT …) den Kern hart aus und löschte den alten im Audio-Faden.
// Bei Puffern um 128 Abtastwerte reichte das für einen Aussetzer, dazu sprang der Ausgang. Jetzt: alter Kern wird 3 ms ausgeblendet,
// dann getauscht und der neue 3 ms eingeblendet (kein Überblenden: zwei Kerne gleichzeitig würden genau dann doppelt rechnen);
// der alte Kern kommt in den Papierkorb und wird im Hauptfaden gelöscht.
#pragma once
#include <atomic>
#include <cmath>
#include <algorithm>

namespace tsfx {
// Papierkorb: Audio-Faden legt hinein (ohne Sperre, ohne Speicheranforderung), Hauptfaden leert ihn
template <class P> struct Trash {
  std::atomic<P*> head{nullptr};
  void push(P *q) { if (!q) return; q->next = head.load(); while (!head.compare_exchange_weak(q->next, q)) {} }
  void freeAll() { P *q = head.exchange(nullptr); while (q) { P *n = q->next; delete q->a; delete q->proto; delete q; q = n; } }
};
// Aus-/Einblenden mit halber Kosinuskurve
struct Fade {
  int len = 0, pos = 0, dir = 0;   // dir −1 ausblenden (bleibt danach bei 0), +1 einblenden, 0 nichts
  void start(int d, double fs, double sec = 0.003) { dir = d; len = std::max(1, (int)std::lround(sec * fs)); pos = 0; }
  bool outDone() const { return dir < 0 && pos >= len; }
  double next() { if (dir == 0) return 1; double x = std::min(1.0, (double)pos / len); if (pos < len) pos++;
    double c = 0.5 - 0.5 * std::cos(3.14159265358979323846 * x); if (dir < 0) return 1 - c; if (pos >= len) dir = 0; return c; }
};
}
