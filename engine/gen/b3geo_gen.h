// erzeugt von engine/tools/b3_panel.py: Lage der Bedienelemente auf b3panel.jpg (Bildpunkte)
static const int B3W = 2000, B3H = 240;
static const int B3NKNOB = 15;
static const double B3KNOB[15][3] = {{214, 140, 27}, {306, 140, 27}, {404, 140, 27}, {500, 140, 27}, {606, 140, 27}, {696, 140, 27}, {794, 140, 27}, {886, 140, 27}, {990, 140, 27}, {1082, 140, 27}, {1176, 140, 27}, {1272, 140, 27}, {1370, 140, 27}, {1468, 140, 27}, {1566, 140, 27}};
static const int B3ACTIVE[15] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0};   // 0 = Frequenzweiche (ohne Wirkung)
static const double B3LED[2][2] = {{574, 42}, {958, 42}};   // Anzeigen PARAMETRIC EQ 1 / 2
static const double B3JACK[3][3] = {{86, 140, 19}, {1676, 88, 15}, {1676, 176, 15}};   // INPUT, PHONES, FOOT SW
