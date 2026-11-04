// erzeugt von engine/tools/g100_panel.py: Lage der Bedienelemente auf g100panel.jpg (Bildpunkte)
static const int G100W = 1800, G100H = 260;
static const int G100NKNOB = 13;
static const double G100KNOB[13][3] = {{120, 136, 31}, {224, 136, 31}, {328, 136, 31}, {432, 136, 31}, {766, 136, 31}, {870, 136, 31}, {974, 136, 31}, {1078, 136, 31}, {1182, 136, 31}, {1294, 136, 31}, {1398, 136, 31}, {1502, 136, 31}, {1612, 136, 31}};
static const int G100PULL[13] = {1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0};   // 1 = Zugschalter (Beschriftung bei y = G100PULLY)
static const double G100PULLY = 44;
static const double G100JACK[2][3] = {{606, 136, 19}, {662, 136, 19}};   // HIGH, LOW
static const double G100BTN[3] = {540, 100, 14};   // A/B-Taste: Mitte, halbe Kantenlänge
static const double G100LED[2] = {540, 62};
