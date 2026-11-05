// erzeugt von engine/tools/svt_panel.py: Lage der Bedienelemente auf svtpanel.jpg (Bildpunkte)
static const int SVTW = 1800, SVTH = 260;
static const double SVTKNOB[6][3] = {{330, 138, 40}, {690, 138, 40}, {840, 138, 40}, {990, 138, 40}, {1140, 138, 40}, {1290, 138, 40}};   // GAIN, BASS, MIDRANGE, FREQUENCY, TREBLE, MASTER
static const double SVTFPOS[5] = {-60, -30, 0, 30, 60};   // FREQUENCY: Winkel der Stellungen (Grad)
static const double SVTJACK[2][3] = {{100, 150, 22}, {196, 150, 22}};   // 0 dB, −15 dB
static const double SVTSW[4][2] = {{470, 140}, {560, 140}, {1560, 140}, {1680, 140}};   // ULTRA HI, ULTRA LO, STANDBY, POWER
static const double SVTLED[2] = {1440, 140};
