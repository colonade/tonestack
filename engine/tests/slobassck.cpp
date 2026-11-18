// Schalter BASS CK im Loldano (ab 1.8): Kathodenkondensatoren V1B, V2A, V3B 3,9 µF statt 1 µF (Anregung des Kritikers für Bass).
// 1. Kleinsignal am Lautsprecher (Grundstellung, 1x, 48 kHz), 3,9 gegen 1 µF: Bezug aus der Nachrechnung vom 18.11.2026 (scratchpad ckresp.cpp), ±0,1 dB.
// 2. Arbeitspunkt: Ruhespannungen an den Kathoden gleich (der Kondensator führt keinen Gleichstrom).
// 3. Umschalten im Lauf (Amp 2x): in Stille kein Knacks (Ausgang bleibt unter −90 dBFS); mit Basston (82 Hz, 100 mV) gleicher Zustand wie ein
//    von Anfang an umgeschalteter Verstärker (nach 1 s Rest unter −50 dB) und kein Sprung über die größte Änderung des Signals hinaus.
#include "slo_core.hpp"
#include <cstdio>
using namespace slo;
int main() { int bad = 0;
  // 1. Kleinsignal
  struct Ref { bool od; double f, d; } R[] = {{true, 41, 8.3}, {true, 55, 9.2}, {true, 82, 8.9}, {true, 110, 7.7}, {true, 220, 3.5}, {true, 400, 1.2},
                                             {false, 41, 4.9}, {false, 82, 5.2}, {false, 220, 2.0}, {false, 400, 0.7}};
  Core c1[2], c2[2]; for (int od = 0; od < 2; od++) { Setting s; s.od = od; Setting b = s; b.bassCk = true; c1[od].init(48000, Parts(), s); c2[od].init(48000, Parts(), b); c1[od].settle(SETTLE); c2[od].settle(SETTLE); }
  for (auto &r : R) { int k = r.od ? 1 : 0; double d = 20 * std::log10(std::abs(c2[k].response(r.f)) / std::abs(c1[k].response(r.f))); bool ok = std::fabs(d - r.d) < 0.1; if (!ok) bad++;
    printf("%-9s %4.0f Hz: 3,9 uF gegen 1 uF %+5.2f dB (Bezug %+4.1f)%s\n", r.od ? "OVERDRIVE" : "NORMAL", r.f, d, r.d, ok ? "" : "   ABWEICHUNG"); }
  // 2. Arbeitspunkt
  for (const char *n : {"k1", "k2", "k4"}) { double a = dk::nodeVoltages(c1[1].net, c1[1].fs, c1[1].run.h, std::vector<double>{0, c1[1].v.dc_v - SI_U, c1[1].v.bias_v}, c1[1].run.ip)[c1[1].net.idx.at(n)];
    double b = dk::nodeVoltages(c2[1].net, c2[1].fs, c2[1].run.h, std::vector<double>{0, c2[1].v.dc_v - SI_U, c2[1].v.bias_v}, c2[1].run.ip)[c2[1].net.idx.at(n)];
    bool ok = std::fabs(a - b) < 2e-3; if (!ok) bad++; printf("Ruhe Kathode %s: %.4f V gegen %.4f V%s\n", n, a, b, ok ? "" : "   ABWEICHUNG"); }
  // 3. Umschalten im Lauf
  const double fs = 48000; const int N = (int)fs;
  { Amp a; Setting s; a.init(fs, 2, Parts(), s); std::vector<float> b(N, 0.0f); a.process(b.data(), N); double q0 = 0; for (float x : b) q0 = std::max(q0, (double)std::fabs(x));
    s.bassCk = true; a.setTarget(s); std::fill(b.begin(), b.end(), 0.0f); a.process(b.data(), N); double q1 = 0; for (float x : b) q1 = std::max(q1, (double)std::fabs(x));
    s.bassCk = false; a.setTarget(s); std::fill(b.begin(), b.end(), 0.0f); a.process(b.data(), N); double q2 = 0; for (float x : b) q2 = std::max(q2, (double)std::fabs(x));
    double w = 20 * std::log10(std::max(q1, q2) + 1e-30); bool ok = w < -90; if (!ok) bad++;
    printf("Stille: vorher %.1f dBFS, nach Umschalten auf 3,9 uF %.1f dBFS, zurück %.1f dBFS%s\n", 20 * std::log10(q0 + 1e-30), 20 * std::log10(q1 + 1e-30), 20 * std::log10(q2 + 1e-30), ok ? "" : "   ABWEICHUNG"); }
  for (int od = 1; od >= 0; od--) { Setting s; s.od = od; Setting sb = s; sb.bassCk = true; Amp a, r; a.vin = r.vin = 0.1; a.init(fs, 2, Parts(), s); r.init(fs, 2, Parts(), sb);
    std::vector<float> x(3 * N), ya, yr; for (int i = 0; i < 3 * N; i++) x[i] = (float)std::sin(2 * PI * 82 * i / fs); ya = x; yr = x;
    a.process(ya.data(), N); a.setTarget(sb); a.process(ya.data() + N, 2 * N); r.process(yr.data(), 3 * N);
    double dmax = 0, dsw = 0; for (int i = 1; i < N; i++) dmax = std::max(dmax, (double)std::fabs(ya[i] - ya[i - 1]));
    for (int i = N; i < N + 480; i++) dsw = std::max(dsw, (double)std::fabs(ya[i] - ya[i - 1]));
    double e = 0, p = 0; for (int i = 2 * N; i < 3 * N; i++) { e += (ya[i] - yr[i]) * (ya[i] - yr[i]); p += yr[i] * yr[i]; } double rest = 10 * std::log10(e / p + 1e-30);
    bool ok = rest < -50 && dsw <= 1.05 * dmax; if (!ok) bad++;
    printf("%-9s 82 Hz 100 mV: größter Schritt vor dem Umschalten %.4f, in 10 ms danach %.4f; Rest gegen von Anfang an 3,9 uF nach 1 s %.1f dB%s\n", od ? "OVERDRIVE" : "NORMAL", dmax, dsw, rest, ok ? "" : "   ABWEICHUNG"); }
  printf("%s\n", bad ? "FEHLER" : "fertig"); return bad ? 1 : 0; }
