// erzeugt von engine/tools/weq_panel.py: Lage der Bedienelemente auf weqpanel.jpg (Bildpunkte)
static const int WEQW = 2000, WEQH = 240;
static const double WEQKNOB[11][3] = {{228, 140, 30}, {452, 140, 30}, {552, 140, 30}, {656, 140, 30}, {812, 140, 30}, {912, 140, 30}, {1016, 140, 30}, {1172, 140, 30}, {1272, 140, 30}, {1376, 140, 30}, {1536, 140, 30}};   // LOW GAIN, MID 1 GAIN/Q/FREQ, MID 2 …, MID 3 …, HIGH GAIN
static const double WEQBTN[5][4] = {{322, 108, 48, 28}, {322, 166, 48, 28}, {1630, 108, 48, 28}, {1630, 166, 48, 28}, {1772, 166, 48, 28}};   // Tasten LOW 60, LOW 100, HIGH 12k, HIGH 8k, EQ ON (Mitte x, y, Breite, Höhe)
static const double WEQLED[2] = {1772, 104};   // Anzeige EQ ON
