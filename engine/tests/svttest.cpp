// Prüfstand Ampeg SVT-CL (svt_core.hpp) gegen Plan und Herstellerdaten (Owner's Guide 1997 / Owner's Manual 2018):
// Ruhespannungen der Pläne, Signalpegel der Kästchen im Vorverstärker, Klangregelung, ULTRA LO/HI, Frequenzgang, Leistung und Klirr, Stabilität.
// Rückgabe ≠ 0 bei einer Abweichung (bekannte Abweichungen werden gemeldet, zählen aber nicht).
#include "svt_core.hpp"
#include <cstdio>
using namespace svt;
static int bad = 0, known = 0;
static void chk(const char *what, double v, double soll, double tol, const char *unit = "", bool bek = false) {
  bool ok = std::isfinite(v) && std::fabs(v - soll) <= tol; if (!ok) { if (bek) known++; else bad++; }
  printf("  %-58s %8.2f %-4s (Soll %.2f ± %.2f) %s\n", what, v, unit, soll, tol, ok ? "ok" : bek ? "BEKANNT" : "ABWEICHUNG"); }
static double FS = 96000;
static Core mk(Setting s, const std::vector<std::string> *dbg = nullptr) { Core c; c.dbg = dbg; c.init(FS, Parts(), s); c.settle(1.2); return c; }
static double db(std::complex<double> z) { return 20 * std::log10(std::abs(z)); }
// Sinus am Eingang (V rms, f): Ausgangsleistung an der Last und Klirrfaktor (Oberwellen 2–15)
static void sine(Core &c, double vin, double f, double &watt, double &thd, bool &fin) {
  int per = (int)std::lround(FS / f); long n0 = (long)(0.4 * FS / per) * per, n1 = 10L * per * std::max(1, (int)(f / 200)); double y[O_NO + 8]; fin = true;
  for (long i = 0; i < n0; i++) { c.step(vin * std::sqrt(2.0) * std::sin(2 * PI * f * i / FS), y); if (!std::isfinite(y[O_SPK])) fin = false; }
  std::vector<double> o(n1); for (long i = 0; i < n1; i++) { c.step(vin * std::sqrt(2.0) * std::sin(2 * PI * f * (i + n0) / FS), y); o[i] = y[O_SPK]; if (!std::isfinite(y[O_SPK])) fin = false; }
  auto harm = [&](int hn) { double re = 0, im = 0; for (long i = 0; i < n1; i++) { double ph = 2 * PI * f * hn * (i + n0) / FS; re += o[i] * std::cos(ph); im += o[i] * std::sin(ph); } return 2 * std::sqrt(re * re + im * im) / n1; };
  double a1 = harm(1), hs = 0; for (int hn = 2; hn <= 15; hn++) { double a = harm(hn); hs += a * a; } watt = a1 * a1 / 2 / loadOhm(c.cur); thd = 100 * std::sqrt(hs) / a1; }
int main() {
  printf("Ruhespannungen (Plan 07S519-12 / 07S419-12 / Röhrenplatine; Grundstellung)\n");
  { std::vector<std::string> N = {"A1", "A2", "A3", "K4", "pa", "pb", "dpA", "B1", "SCR", "kA", "kB", "gA"}; Setting s; Core c = mk(s, &N); double y[O_NO + 16]; c.step(0, y); auto V = [&](int i) { return y[O_NO + i]; };
    printf("  Knoten: %d, Zustände: %d, nichtlineare Tore: %d\n", c.net.nn, c.run.s.nh, c.run.s.m);
    chk("V1B Anode (235 V)", V(0), 235, 15, "V"); chk("V1A Anode (240 V)", V(1), 240, 15, "V"); chk("V2B Anode (240 V)", V(2), 240, 15, "V"); chk("V2A Kathode (100 V)", V(3), 100, 15, "V");
    chk("Endstufe V1A Anode (200 V)", V(4), 200, 15, "V"); chk("Phasenumkehr V1B Anode (250 V)", V(5), 250, 15, "V");
    chk("Treiber V2 Anode (180 V; Treiberwicklung geschätzt)", V(6), 180, 30, "V");
    chk("Hochspannung (660 V)", V(7), 660, 5, "V"); chk("Schirmgitter (345 V)", V(8), 345, 5, "V");
    chk("6550 Ruhestrom Seite A (0,24 V an 10 Ω)", V(9) / 10 * 1000, 24, 1, "mA"); chk("6550 Ruhestrom Seite B", V(10) / 10 * 1000, 24, 1, "mA");
    chk("6550 Gittervorspannung (etwa −45 V)", V(11), -45, 8, "V"); }
  printf("Signalpegel bei 1 kHz, 15 mV Eingang, GAIN und MASTER Maximum (Kästchen im Plan)\n");
  { std::vector<std::string> N = {"X", "P", "A3", "PO"}; Setting s; s.gain = 1; s.master = 1; Core c = mk(s, &N);
    std::vector<std::vector<double>> J; std::vector<std::pair<int,int>> dp;
    for (auto &d : c.run.dev) { std::vector<double> b((size_t)d.np * d.np); for (int a = 0; a < d.np; a++) for (int k = 0; k < d.np; k++) b[a * d.np + k] = c.run.Jd[(size_t)(d.p0 + a) * 16 + k]; J.push_back(b); dp.push_back({d.p0, d.np}); }
    auto lv = [&](const char *n) { return 0.015 * std::abs(dk::acResponse(c.net, J, dp, 1000, 0, c.net.idx.at(n), 0)); };
    chk("nach V1B (0,9 V)", lv("X"), 0.9, 0.1, "V"); chk("nach V1A (7 V)", lv("P"), 7, 0.7, "V"); chk("Anode V2B (8 V)", lv("A3"), 8, 0.8, "V"); chk("Vorverstärker-Ausgang (0,4 V)", lv("PO"), 0.4, 0.04, "V");
    double g1k = db(c.response(1000));
    chk("Gesamtverstärkung 1 kHz (Spez. 67 dB)", g1k, 67, 1.5, "dB", true);
    chk("Frequenzgang 40 Hz rel. 1 kHz (Spez. −3 dB)", db(c.response(40)) - g1k, -3, 1, "dB", true);
    chk("Frequenzgang 15 kHz rel. 1 kHz (Spez. −3 dB)", db(c.response(15000)) - g1k, -3, 1, "dB");
    double mx = -99; for (double f = 1000; f < 60000; f *= 1.03) mx = std::max(mx, db(c.response(f)) - g1k); chk("keine Überhöhung über 1 kHz (höchster Wert)", mx, 0, 0.5, "dB"); }
  printf("Klangregelung (Abstand zur Mittelstellung aller Regler; GAIN und MASTER Mitte)\n");
  { Setting r; Core ref = mk(r); auto rel = [&](Setting t, double f) { Core x = mk(t); return db(x.response(f)) - db(ref.response(f)); };
    Setting t = r; t.bass = 1; chk("BASS 10 bei 40 Hz (+12 dB)", rel(t, 40), 12, 1.5, "dB"); t.bass = 0; chk("BASS 0 bei 40 Hz (−12 dB)", rel(t, 40), -12, 1.5, "dB");
    t = r; t.treb = 1; chk("TREBLE 10 bei 4 kHz (+15 dB)", rel(t, 4000), 15, 1.5, "dB"); t.treb = 0; chk("TREBLE 0 bei 4 kHz (−20 dB)", rel(t, 4000), -20, 3, "dB");
    const double ff[5] = {220, 450, 800, 1600, 3000};
    for (int f = 0; f < 5; f++) { char b[80]; t = r; t.freq = f;
      t.mid = 1; std::snprintf(b, sizeof b, "MIDDLE 10, FREQUENCY %.0f Hz (+10 dB)", ff[f]); chk(b, rel(t, ff[f]), 10, 1.5, "dB");
      t.mid = 0; std::snprintf(b, sizeof b, "MIDDLE 0, FREQUENCY %.0f Hz (−20 dB)", ff[f]); chk(b, rel(t, ff[f]), -20, 2, "dB"); }
    t = r; t.ulo = true; chk("ULTRA LO bei 40 Hz (+2 dB)", rel(t, 40), 2, 1, "dB"); chk("ULTRA LO bei 500 Hz (−10 dB)", rel(t, 500), -10, 1.5, "dB");
    t = r; t.uhi = true; chk("ULTRA HI bei 8 kHz (+9 dB)", rel(t, 8000), 9, 1.5, "dB");
    t = r; t.jack = 1; chk("Eingang −15 dB", rel(t, 1000), -15, 0.5, "dB"); }
  printf("Leistung (1 kHz, GAIN und MASTER Maximum; Spez. 300 W an 2 oder 4 Ω bei < 3 %% Klirr, 0,4 V Eingang)\n");
  for (int tap = 0; tap < 2; tap++) { Setting s; s.gain = 1; s.master = 1; s.tap = tap; double lo = 0.004, hi = 0.02, w = 0, thd = 0; bool fin;
    for (int it = 0; it < 12; it++) { double m = std::sqrt(lo * hi); Core c = mk(s); sine(c, m, 1000, w, thd, fin); if (thd < 3) lo = m; else hi = m; }
    Core c = mk(s); sine(c, lo, 1000, w, thd, fin); char b[80]; std::snprintf(b, sizeof b, "Leistung bei 3 %% Klirr an %d Ω (≥ 300 W; Netzteil geschätzt)", tap ? 2 : 4); chk(b, w, 360, 60, "W");   /* ab 1.10 (Knie der Strahltetrode): 400 W am geschätzten Netzteil; Spezifikation nur „≥ 300 W“, Fenster deshalb 300 … 420 W */
    std::vector<std::string> N = {"PO"}; Core c2 = mk(s, &N); double y[O_NO + 2], sq = 0; long n = 9600; for (long i = 0; i < 48000 + n; i++) { c2.step(lo * std::sqrt(2.0) * std::sin(2 * PI * 1000 * i / FS), y); if (i >= 48000) sq += y[O_NO] * y[O_NO]; }
    std::snprintf(b, sizeof b, "  Vorverstärker-Ausgang dabei (Spez. 0,4 V für 300 W)"); chk(b, std::sqrt(sq / n), 0.4, 0.05, "V", true); }
  printf("Netzteil nach Plan (Gleichrichter, Siebkette R54/R55/R58; Grundstellung, Mittel über 0,2 s)\n");
  for (int psu : {1, 2}) { std::vector<std::string> N = {"B1", "SCR", "NEG", "kA", "kB", "B470", "B345", "B343", "pa", "pb", "A1"}; Setting s; s.psu = psu; Core c = mk(s, &N);
    double y[O_NO + 16], m[11] = {}; int n = (int)(0.2 * FS); double mn = 1e9, mx = -1e9;
    for (int i = 0; i < n; i++) { c.step(0, y); for (int k = 0; k < 11; k++) m[k] += y[O_NO + k] / n; mn = std::min(mn, y[O_SPK]); mx = std::max(mx, y[O_SPK]); }
    printf("  %d Hz Netz:\n", psu == 2 ? 60 : 50);
    chk("  Hochspannung (660 V)", m[0], 660, 5, "V"); chk("  Schirmgitter (345 V)", m[1], 345, 5, "V"); chk("  −180 V", m[2], -180, 2, "V");
    chk("  6550 Ruhestrom (24 mA)", (m[3] + m[4]) / 2 / 10 * 1000, 24, 1, "mA");
    chk("  +343 V Vorstufe (R55 47 k)", m[7], 343, 10, "V"); chk("  +470 V (R54 15 k; Lasten laut Plan ergeben mehr)", m[5], 470, 10, "V", true);
    chk("  +345 V Endstufen-Eingang (R58 47 k)", m[6], 345, 10, "V", true); chk("  Endstufe V1A Anode (200 V)", m[8], 200, 15, "V", true); chk("  Phasenumkehr Anode (250 V)", m[9], 250, 15, "V", true);
    chk("  Brumm am Abgriff in Ruhe, Spitze-Spitze (unter 10 mV)", (mx - mn) * 1000, 0, 10, "mV"); }
  printf("BAL OUT (T1 geschätzt: 10 H, 50 Ω, 20 kΩ; GAIN und MASTER Maximum, Klang Mitte), Abstand zu ohne T1\n");
  { Setting a; a.gain = 1; a.master = 1; Setting b = a; b.bal = 1; Core ca = mk(a), cb = mk(b); auto rel = [](Core &c, double f) { return db(c.response(f)) - db(c.response(1000)); };
    for (double f : {30.0, 40.0, 60.0, 80.0, 100.0, 200.0}) { char t[80]; std::snprintf(t, sizeof t, "  %.0f Hz", f); printf("  %-58s %+8.2f dB\n", t + 2, rel(cb, f) - rel(ca, f)); }
    chk("  mit T1: 40 Hz rel. 1 kHz (Spez. −3 dB)", rel(cb, 40), -3, 1, "dB");
    chk("  1 kHz absolut gegenüber ohne T1", db(cb.response(1000)) - db(ca.response(1000)), 0, 0.5, "dB"); }
  printf("TUBE MISMATCH (Ruhestrom-Unterschied Seite A gegen B)\n");
  { std::vector<std::string> N = {"kA", "kB"}; double h2[2] = {0, 0}, r0 = 100;
    for (int k = 0; k < 2; k++) { Setting s; s.mis = k ? 0.08 : 0; Core c = mk(s, &N); double y[O_NO + 4]; c.step(0, y); double ia = y[O_NO], ib = y[O_NO + 1];
      if (k) chk("  8 %: Ruhestrom A/B, Unterschied zu 0 %", 100 * (ia / ib) - r0, 8, 0.8, "%"); else { r0 = 100 * (ia / ib); chk("  0 %: Ruhestrom A/B − 1 (Rest des Einschwingens nach 1,2 s)", r0 - 100, 0, 1, "%"); }
      Core c2 = mk(s); double w, thd; bool fin; sine(c2, 0.05, 100, w, thd, fin);
      int per = (int)std::lround(FS / 100); long n1 = 20L * per; std::vector<double> o(n1); for (long i = 0; i < n1; i++) { c2.step(0.05 * std::sqrt(2.0) * std::sin(2 * PI * 100 * i / FS), y); o[i] = y[O_SPK]; }
      auto hm = [&](int hn) { double re = 0, im = 0; for (long i = 0; i < n1; i++) { double ph = 2 * PI * 100 * hn * i / FS; re += o[i] * std::cos(ph); im += o[i] * std::sin(ph); } return std::sqrt(re * re + im * im); };
      h2[k] = 20 * std::log10(hm(2) / hm(1)); printf("  %-58s %+8.2f dB\n", k ? "8 %: 2. Oberwelle (50 mV, 100 Hz)" : "0 %: 2. Oberwelle (50 mV, 100 Hz)", h2[k]); }
    chk("  2. Oberwelle mit 8 % stärker als ohne", h2[1] - h2[0] > 3 ? 1 : 0, 1, 0, ""); }
  printf("Stabilität (harte Übersteuerung, 48 und 96 kHz)\n");
  for (int var = 0; var < 3; var++) for (double fs : {48000.0, 96000.0}) { if (var && fs > 50000) continue; double sv = FS; FS = fs; Setting s; s.gain = 1; s.master = 1; s.bass = 1; s.uhi = true; s.psu = var; s.bal = var > 0; s.mis = var ? 0.1 : 0; Core c = mk(s); double w, thd; bool fin; sine(c, 1.0, 82.4, w, thd, fin); FS = sv;
    char b[80]; std::snprintf(b, sizeof b, "1 V bei 82 Hz, %.0f kHz%s: endlich, Leistung", fs / 1000, var == 1 ? ", Netzteil 50 Hz + T1 + 10 %" : var == 2 ? ", Netzteil 60 Hz + T1 + 10 %" : ""); chk(b, fin ? w : NAN, 450, 450, "W"); }
  printf("\n%d Abweichungen, %d bekannte\n", bad, known); return bad ? 1 : 0; }
