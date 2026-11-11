// erzeugt von engine/tools/u76_panel.py: Lage der Bedienelemente auf u76panel.jpg (Bildpunkte)
static const int U76W = 2000, U76H = 320;
static const double U76KNOB[4][3] = {{420, 178, 64}, {610, 178, 64}, {790, 178, 42}, {930, 178, 42}};   // INPUT, OUTPUT, ATTACK, RELEASE (x, y, Radius)
static const double U76BTN[8][4] = {{1070, 82, 64, 38}, {1070, 136, 64, 38}, {1070, 190, 64, 38}, {1070, 244, 64, 38}, {1640, 82, 64, 38}, {1640, 136, 64, 38}, {1640, 190, 64, 38}, {1640, 244, 64, 38}};   // RATIO 20, 12, 8, 4; METER GR, +8, +4, OFF (Mitte x, y, Breite, Höhe)
static const double U76VU[7] = {1170, 40, 1540, 286, 1355, 330, 240};   // Fenster x0, y0, x1, y1; Drehpunkt x, y; Nadellänge
