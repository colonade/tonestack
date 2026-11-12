// erzeugt aus ygeo.json (Foto der Frontplatte des Lametta B100-II, 1800 x 227)
static const double YFW = 1800, YFH = 227, YPW = 4.44, YPIN = 0.19, YPOUT = 0.82;
struct YKnobGeo { const char *key; double x, y, r; };
static const YKnobGeo YKG[9] = {
  {"vol", 326, 122.3, 32.5},
  {"t", 456.7, 119.3, 33},
  {"m", 589.3, 119.3, 34.5},
  {"b", 722, 119.3, 33.5},
  {"e0", 982.4, 119.8, 33.5},
  {"e1", 1112.5, 118.4, 31.6},
  {"e2", 1243.2, 116.9, 33},
  {"e3", 1374.9, 116.4, 31.6},
  {"e4", 1501.6, 116.4, 32.5},
};
static const double YJACK_HIGH[3] = {136, 120, 26};
static const double YJACK_LOW[3] = {217, 120, 26};
