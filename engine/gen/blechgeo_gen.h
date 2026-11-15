// erzeugt von engine/tools/blech_panel.py: Lage der Bedienelemente auf blechpanel.jpg (Bildpunkte)
static const int BLW = 2000, BLH = 240;
static const double BLKNOB[4][3] = {{760, 128, 36}, {1310, 128, 30}, {1450, 128, 30}, {1590, 128, 30}};   // DECAY, MIX, PRE-DELAY, OUTPUT
static const double BLGAUGE[6] = {520, 150, 112, 1, 4, 50};   // Instrument: Drehpunkt x, y, Radius, Skala von/bis (s), halber Ausschlag (°)
static const double BLBASS[2] = {1000, 128};   // Wahlschalter BASS CUT
static const double BLLAMP[2] = {860, 70};   // Lampe MOTOR
