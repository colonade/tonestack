// erzeugt von engine/tools/cultec_panel.py: Lage der Bedienelemente auf cultecpanel.jpg (Bildpunkte)
static const int CTW = 1600, CTH = 420;
static const double CTKNOB[5][3] = {{300, 160, 42}, {530, 160, 42}, {830, 160, 42}, {1090, 160, 42}, {680, 330, 34}};   // LOW BOOST, LOW ATTEN, HIGH BOOST, HIGH ATTEN, BANDWIDTH (Mitte x, y, Radius), 0 … 10 über ±150°
static const double CTSEL[3][5] = {{415, 330, 32, 4, 90}, {960, 330, 32, 7, 150}, {1320, 160, 32, 3, 60}};   // CPS, KCS, ATTEN SEL (Mitte x, y, Radius, Stellungen, Winkelbereich in Grad)
static const double CTIN[2] = {150, 330}, CTLAMP[2] = {1200, 330};   // IN/OUT-Schalter, Netzlampe
