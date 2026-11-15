// erzeugt von engine/tools/kai_panel.py: Lage der Bedienelemente auf kaipanel.jpg (Bildpunkte)
static const int KAIW = 2000, KAIH = 240;
static const double KAIKNOB[7][3] = {{640, 128, 32}, {760, 128, 32}, {880, 128, 32}, {1050, 128, 32}, {1170, 128, 32}, {1340, 128, 32}, {1460, 128, 32}};   // DECAY, MIX, OUTPUT, LOW, HI, THING 1, THING 2
static const double KAIBTN[7][4] = {{254, 102, 92, 34}, {370, 102, 92, 34}, {486, 102, 92, 34}, {254, 178, 92, 34}, {370, 178, 92, 34}, {486, 178, 92, 34}, {1640, 150, 92, 34}};   // Tasten HALL, PLATE, SPRING, ROOM, MOD, SPARKLE, HOLD (Mitte x, y, Breite, Höhe)
static const double KAILED[7][2] = {{218, 74}, {334, 74}, {450, 74}, {218, 150}, {334, 150}, {450, 150}, {1640, 100}};   // Anzeigen der Modi und HOLD
