// erzeugt von engine/tools/wisent_panel.py: Lage der Bedienelemente auf wisentpanel.jpg (Bildpunkte)
static const int WSW = 2000, WSH = 240;
static const double WSKNOB[7][3] = {{470, 128, 30}, {590, 128, 30}, {710, 128, 30}, {830, 128, 30}, {950, 128, 30}, {1640, 128, 30}, {1760, 128, 30}};   // INPUT, LENGTH OF SWELL, VOLUME, BASS, TREBLE, LEVEL, OUTPUT
static const double WSEYE[4] = {330, 128, 120, 30};   // magisches Auge: Mitte x, y, Breite, Höhe
static const double WSMODE[3][2] = {{1085, 76}, {1085, 128}, {1085, 180}}; static const double WSBTN[2] = {74, 34};   // ECHO, REPEAT, SWELL
static const double WSHEADX[4] = {1262, 1342, 1422, 1502}; static const double WSHFB = 100, WSHOUT = 172; static const double WSHBTN[2] = {56, 30};   // Kopftasten
static const double WSMAINS[2] = {1885, 128};   // Netzfrequenz
