// erzeugt von engine/tools/viereck_panel.py: Lage der Bedienelemente auf viereckpanel.jpg (Bildpunkte)
static const int VRW = 1600, VRH = 240;
static const double VRKNOB[4][3] = {{380, 132, 30}, {580, 132, 30}, {770, 132, 30}, {1250, 132, 30}};   // BASS, TILT, BALANCE, TILT PIVOT (Mitte x, y, Radius)
static const double VRBTN[3][4] = {{920, 160, 52, 30}, {1000, 160, 52, 30}, {1080, 160, 52, 30}};   // Tasten F1, F2, SLOPE (Mitte x, y, Breite, Höhe); Leuchten bei y = 112
static const double VRLEDY = 112;
static const double VRMODE[2] = {1400, 132};   // Kippschalter STEPS/SMOOTH
static const double VRPIV[3] = {80, 4000, 847.13};   // TILT PIVOT: unten, oben (Hz, logarithmisch), Original
