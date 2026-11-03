// erzeugt aus geo.json (Foto der Frontplatte, 2430 x 358)
static const double FW = 2430, FH = 358, SEG0 = 0.884, SEG1 = 1.103, SEGHA = 18.5, DIG0 = 0.889, DIG1 = 1.094;
struct KnobGeo { const char *key; double rx, ry, R, px, py; int kind; double cap, skirt; };   // kind 0 schwarz, 1 rot, 2 lila
static const KnobGeo KG[9] = {
  {"v1", 434.8, 166.1, 65.5, 417.1, 156.1, 0, 0, 0},
  {"v2", 613.7, 167.1, 65.1, 602.2, 156.8, 0, 0, 0},
  {"pl", 790.7, 167.6, 64.6, 785.3, 157.2, 0, 0, 0},
  {"mv", 967.6, 168.3, 64.4, 973.4, 161.0, 1, 24.5, 42.7},
  {"t", 1145.3, 168.6, 64.5, 1152.1, 157.9, 0, 0, 0},
  {"m", 1321.3, 168.6, 64.1, 1334.2, 157.7, 0, 0, 0},
  {"b", 1495.9, 169.1, 64.1, 1514.9, 158.0, 0, 0, 0},
  {"pres", 1666.9, 167.0, 63.3, 1686.0, 161.4, 2, 23.8, 43.6},
  {"rev", 1839.8, 168.3, 63.5, 1870.6, 156.9, 0, 0, 0},
};
struct LabelGeo { const char *t; const char *k; double x, y, w, h; };
static const LabelGeo LG[20] = {
  {"PULL BOOST", "t", 1101, 48, 87, 13},
  {"PULL BOOST", "m", 1278, 47, 88, 14},
  {"PULL BOOST", "b", 1455, 48, 86, 13},
  {"INPUTS", "", 142, 64, 61, 14},
  {"PREAMP 1 VOLUME", "v1", 354, 64, 153, 16},
  {"PREAMP 2 VOLUME", "v2", 534, 64, 154, 16},
  {"PREAMP 1 LEVEL", "pl", 722, 65, 137, 16},
  {"MASTER VOLUME", "mv", 897, 66, 139, 16},
  {"TREBLE", "t", 1112, 66, 65, 15},
  {"MIDDLE", "m", 1291, 66, 63, 15},
  {"BASS", "b", 1477, 66, 43, 15},
  {"PRESENCE", "pres", 1628, 66, 85, 15},
  {"REVERB", "rev", 1811, 67, 65, 15},
  {"STANDBY", "", 1966, 67, 72, 15},
  {"POWER", "", 2120, 68, 60, 14},
  {"SENSITIVITY", "", 122, 102, 106, 15},
  {"HIGH", "jh", 78, 154, 43, 15},
  {"LOW", "jl", 235, 154, 36, 15},
  {"MADE IN ENGLAND", "", 117, 291, 134, 13},
  {"PULL ON", "v1", 401, 46, 57, 12},
};
static const double JACK_HIGH[3] = {116, 218, 44};
static const double JACK_LOW[3] = {272, 218, 42};
static const double LED[3] = {313.8, 62.9, 10.5};
static const double PULLLINE[4] = {334.5, 393.8, 52, 3};
static const double POWER[4] = {2115, 133, 95, 84};
