// Ausgabe einer dk-Netzliste als SPICE-Netzliste (ngspice, auch in LTspice lesbar): unabhängige Gegenrechnung desselben Netzes.
// Linear: Widerstände, Kondensatoren, gekoppelte Wicklungen (L + K), Quellen hinter Innenwiderstand, gesteuerte Stromquellen.
// Die nichtlinearen Tore (Röhren, Dioden) schreibt der Aufrufer über `devices` selbst, als Verhaltensquellen mit denselben Gleichungen wie im Kern
// oder mit fremden Modellen. Knotennamen wie im Kern (Masse = 0).
#pragma once
#include "dk.hpp"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
namespace dkspice {
inline std::string num(double v) { char b[40]; std::snprintf(b, sizeof b, "%.12g", v); return b; }
struct Names { std::vector<std::string> n; explicit Names(const dk::Net &net) : n(net.nn + 1) { n[0] = "0"; for (auto &kv : net.idx) n[kv.second] = kv.first; }
  const std::string &operator[](int i) const { return n[i]; } };
// inputs[k]: SPICE-Quellenbeschreibung für den Eingang k (z. B. "DC 482.14" oder "PWL(...)")
inline std::string netlist(const dk::Net &net, const std::vector<std::string> &inputs, const std::function<std::string(const Names &)> &devices, const std::string &title) {
  Names N(net); std::string s = "* " + title + "\n";
  for (size_t k = 0; k < net.R.size(); k++) s += "R" + std::to_string(k) + " " + N[net.R[k].a] + " " + N[net.R[k].b] + " " + num(net.R[k].v) + "\n";
  for (size_t k = 0; k < net.C.size(); k++) s += "C" + std::to_string(k) + " " + N[net.C[k].a] + " " + N[net.C[k].b] + " " + num(net.C[k].v) + "\n";
  for (size_t g = 0; g < net.L.size(); g++) { const auto &c = net.L[g]; int n = (int)c.w.size();
    for (int i = 0; i < n; i++) s += "L" + std::to_string(g) + "_" + std::to_string(i) + " " + N[c.w[i].first] + " " + N[c.w[i].second] + " " + num(c.L(i, i)) + "\n";
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) { double k = c.L(i, j) / std::sqrt(c.L(i, i) * c.L(j, j)); if (k == 0) continue;
      s += "K" + std::to_string(g) + "_" + std::to_string(i) + "_" + std::to_string(j) + " L" + std::to_string(g) + "_" + std::to_string(i) + " L" + std::to_string(g) + "_" + std::to_string(j) + " " + num(k) + "\n"; } }
  for (size_t k = 0; k < net.S.size(); k++) { std::string in = "src" + std::to_string(k);
    s += "V" + std::to_string(k) + " " + in + " 0 " + inputs.at(net.S[k].in) + "\n"; s += "RS" + std::to_string(k) + " " + in + " " + N[net.S[k].a] + " " + num(net.S[k].r) + "\n"; }
  for (size_t k = 0; k < net.G.size(); k++) s += "G" + std::to_string(k) + " " + N[net.G[k].a] + " " + N[net.G[k].b] + " " + N[net.G[k].c] + " " + N[net.G[k].d] + " " + num(net.G[k].g) + "\n";
  if (!net.I.empty()) s += "* (Stromquellen des Netzes nicht ausgegeben)\n";
  s += devices(N); return s; }
}
