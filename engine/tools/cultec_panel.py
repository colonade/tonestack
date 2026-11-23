#!/usr/bin/env python3
# Zeichnet die Frontplatte des Cultec EQ-1A (Schaltungsmodell nach dem Pultec EQP-1A). Eigene Gestaltung: kein Foto, kein Logo, kein Pultec-Schriftzug;
# nur die Anordnung der Bedienelemente folgt dem Gerät (obere Reihe BOOST/ATTEN tief, BOOST/ATTEN hoch, ATTEN SEL; untere Reihe IN, CPS, BANDWIDTH, KCS, Lampe).
# Schreibt engine/gen/cultecpanel.jpg und cultecpanel_r.jpg (EQ-1R; 1600 × 420, ohne Zeiger und Hebel), cultecgeo.json, cultecpanel_jpg.h, cultecgeo_gen.h und die Seitensymbole
# web/tonestack/assets/icons/cultec-eq1a-*.png
#   python3 engine/tools/cultec_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1600, 420, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
PANEL = (112, 128, 142); INK = (240, 238, 228); DIM = (206, 212, 216); LINE = (226, 230, 232)
Y1, Y2 = 160, 330
# Drehregler 0 … 10 über ±150°: (Schlüssel, x, y, Radius, Beschriftung, Skala links, Skala rechts)
KNOBS = [('lb', 300, Y1, 42, 'BOOST', None, None), ('la', 530, Y1, 42, 'ATTEN', None, None), ('hb', 830, Y1, 42, 'BOOST', None, None),
         ('hc', 1090, Y1, 42, 'ATTEN', None, None), ('bw', 680, Y2, 34, 'BANDWIDTH', 'SHARP', 'BROAD')]
# Wahlschalter: (Schlüssel, x, y, Radius, Beschriftung, Stellungen, Winkelbereich in Grad)
SELS = [('lf', 415, Y2, 32, 'CPS', ['20', '30', '60', '100'], 90), ('hf', 960, Y2, 32, 'KCS', ['3', '4', '5', '8', '10', '12', '16'], 150),
        ('hcs', 1320, Y1, 32, 'ATTEN SEL', ['5', '10', '20'], 60)]
# Modell EQP-1R (ab 1.1): 30/60/100 Hz, 3/5/8/10/12 kHz (gleicher Winkelschritt), Höhen-Atten fest bei 10 kHz (kein Wahlschalter)
SELS_R = [('lf', 415, Y2, 32, 'CPS', ['30', '60', '100'], 60), ('hf', 960, Y2, 32, 'KCS', ['3', '5', '8', '10', '12'], 100)]
INSW = (150, Y2); LAMP = (1200, Y2)
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def selang(n, span, i): return (-span / 2 + span * i / (n - 1)) * math.pi / 180
def draw(model='1A'):
    sels = SELS if model == '1A' else SELS_R
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(0, H * S):   # Einbrennlack: leicht gesprenkelt, oben etwas heller
        v = int(4 * (1 - y / (H * S)) + 1.5 * math.sin(y * 0.11) * math.sin(y * 0.013))
        d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(70, 82, 92), width=4 * S)
    for x in (34, W - 34):   # Befestigungslöcher des 19-Zoll-Rahmens (Langlöcher)
        for y in (54, H - 54):
            d.rounded_rectangle([(x - 14) * S, (y - 8) * S, (x + 14) * S, (y + 8) * S], radius=8 * S, fill=(40, 46, 52), outline=(150, 160, 168), width=S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    def hline(x0, x1, y): d.line([(x0 * S, y * S), (x1 * S, y * S)], fill=LINE, width=2 * S)
    # Gruppen: Bögen der Beschriftung wie gravierte Linien
    ctext(415, 30, 'LOW FREQUENCY', 17); hline(250, 330, 42); hline(500, 580, 42)
    ctext(960, 30, 'HIGH FREQUENCY', 17); hline(780, 870, 42); hline(1050, 1140, 42)
    # Name (eigener Schriftzug, unten rechts)
    ctext(1440, 262, 'CULTEC', 30, FB); ctext(1440, 300, 'EQ-' + model, 22, FB); ctext(1440, 334, 'PROGRAM EQUALIZER', 11.5, FB, DIM)
    ctext(1440, 356, 'circuit model after the EQP-' + model, 10, FN, DIM)
    if model == '1R':   # Höhen-Atten fest: Schild statt Wahlschalter
        x, y = 1320, Y1; d.rounded_rectangle([(x - 46) * S, (y - 22) * S, (x + 46) * S, (y + 22) * S], radius=6 * S, outline=LINE, width=2 * S)
        ctext(x, y - 15, 'ATTEN AT', 11, FB, DIM); ctext(x, y + 1, '10 KCS', 15, FB)
    def shadow(x, y, r):
        nonlocal d
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - r + 3) * S, (y - r + 7) * S, (x + r + 3) * S, (y + r + 7) * S], fill=170)
        sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((52, 60, 68), mask=sh); d = ImageDraw.Draw(im)
    def knobbody(x, y, r):
        shadow(x, y, r)
        d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=(14, 14, 14))
        for i in range(int(r * S), 0, -1):   # schwarze Kappe, Glanz oben links, Rand mit Riffelung angedeutet
            t = i / (r * S); f = 1.0 + 0.9 * (1 - t) ** 0.8; ox, oy = -0.2 * (r - i / S), -0.25 * (r - i / S)
            c = tuple(min(255, int(v * f)) for v in (26, 26, 27)); d.ellipse([(x + ox) * S - i, (y + oy) * S - i, (x + ox) * S + i, (y + oy) * S + i], fill=c)
        for k in range(36):
            a = k * math.pi / 18; d.line([((x + (r - 3) * math.sin(a)) * S, (y - (r - 3) * math.cos(a)) * S), ((x + r * math.sin(a)) * S, (y - r * math.cos(a)) * S)], fill=(48, 48, 50), width=S)
        d.arc([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], 200, 330, fill=(96, 96, 98), width=2 * S)
    for k, x, y, r, lab, l0, l1 in KNOBS:
        for j in range(11):   # Skala 0 … 10 über ±150°
            a = (-150 + 30 * j) * math.pi / 180; r1, r2 = r + 6, r + (14 if j % 5 == 0 else 10)
            d.line([((x + r1 * math.sin(a)) * S, (y - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (y - r2 * math.cos(a)) * S)], fill=INK, width=int(1.6 * S))
            if l0 is None or j in (0, 10) and False:
                pass
            t = str(j); ff = font(12 if r > 38 else 10.5, FB); tw = d.textlength(t, font=ff); rr = r + 24 + 0.3 * tw / S
            d.text(((x + rr * math.sin(a)) * S - tw / 2, (y - rr * math.cos(a) - 8) * S), t, font=ff, fill=INK)
        ctext(x, y + r + 26, lab, 13.5 if r > 38 else 12, FB)
        if l0: ctext(x - r - 34, y + r + 4, l0, 10.5, FB, DIM); ctext(x + r + 34, y + r + 4, l1, 10.5, FB, DIM)
        knobbody(x, y, r)
    for k, x, y, r, lab, pos, span in sels:
        n = len(pos)
        for i, t in enumerate(pos):
            a = selang(n, span, i); r1, r2 = r + 6, r + 12
            d.line([((x + r1 * math.sin(a)) * S, (y - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (y - r2 * math.cos(a)) * S)], fill=INK, width=int(1.6 * S))
            ff = font(12.5, FB); tw = d.textlength(t, font=ff); rr = r + 24 + 0.35 * tw / S * abs(math.sin(a))
            d.text(((x + rr * math.sin(a)) * S - tw / 2, (y - rr * math.cos(a) - 8) * S), t, font=ff, fill=INK)
        ctext(x, y + r + 22, lab, 13.5, FB)
        knobbody(x, y, r)
    # IN-Schalter: Platte und Mutter (Hebel zeichnet die Oberfläche); oben IN, unten OUT
    x, y = INSW; ctext(x, y - 66, 'IN', 15, FB); ctext(x, y + 52, 'OUT', 12, FB, DIM)
    shadow(x, y, 16); d.ellipse([(x - 17) * S, (y - 17) * S, (x + 17) * S, (y + 17) * S], fill=(196, 198, 200), outline=(60, 64, 70), width=S)
    d.ellipse([(x - 8) * S, (y - 8) * S, (x + 8) * S, (y + 8) * S], fill=(30, 30, 32))
    # Netzlampe (Fassung; Glas zeichnet die Oberfläche)
    x, y = LAMP; shadow(x, y, 20); d.ellipse([(x - 21) * S, (y - 21) * S, (x + 21) * S, (y + 21) * S], fill=(170, 172, 176), outline=(60, 64, 70), width=S)
    d.ellipse([(x - 14) * S, (y - 14) * S, (x + 14) * S, (y + 14) * S], fill=(90, 30, 20)); ctext(x, y + 34, 'POWER', 11, FB, DIM)
    return im
def main():
    os.makedirs(GEN, exist_ok=True); data = {}
    for model, fn in (('1A', 'cultecpanel.jpg'), ('1R', 'cultecpanel_r.jpg')):
        im = draw(model).resize((W, H), Image.LANCZOS); jp = os.path.join(GEN, fn); im.save(jp, quality=90); data[model] = open(jp, 'rb').read()
        if model == '1A': im1 = im
    sj = lambda L: [{'k': k, 'x': x, 'y': y, 'r': r, 'pos': p, 'span': sp} for k, x, y, r, _, p, sp in L]
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': y, 'r': r} for k, x, y, r, *_ in KNOBS], 'sels': sj(SELS), 'sels_r': sj(SELS_R),
           'insw': {'x': INSW[0], 'y': INSW[1]}, 'lamp': {'x': LAMP[0], 'y': LAMP[1]}}
    json.dump(geo, open(os.path.join(GEN, 'cultecgeo.json'), 'w'))
    with open(os.path.join(GEN, 'cultecpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/cultec_panel.py: gezeichnete Frontplatten Cultec EQ-1A und EQ-1R (JPEG)\n')
        for nm, model in (('CTPANEL', '1A'), ('CTPANEL_R', '1R')):
            f.write('static const unsigned char %s_JPG[] = {\n' % nm); b = data[model]
            for i in range(0, len(b), 24): f.write(','.join(str(x) for x in b[i:i + 24]) + ',\n')
            f.write('};\nstatic const unsigned int %s_JPG_LEN = %d;\n' % (nm, len(b)))
    with open(os.path.join(GEN, 'cultecgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/cultec_panel.py: Lage der Bedienelemente auf cultecpanel.jpg / cultecpanel_r.jpg (Bildpunkte)\n')
        f.write('static const int CTW = %d, CTH = %d;\n' % (W, H))
        f.write('static const double CTKNOB[%d][3] = {%s};   // LOW BOOST, LOW ATTEN, HIGH BOOST, HIGH ATTEN, BANDWIDTH (Mitte x, y, Radius), 0 … 10 über ±150°\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, y, r) for _, x, y, r, *_ in KNOBS)))
        f.write('static const double CTSEL[%d][5] = {%s};   // CPS, KCS, ATTEN SEL (Mitte x, y, Radius, Stellungen, Winkelbereich in Grad)\n' % (len(SELS), ', '.join('{%d, %d, %d, %d, %d}' % (x, y, r, len(p), sp) for _, x, y, r, _, p, sp in SELS)))
        f.write('static const double CTSEL_R[%d][5] = {%s};   // EQP-1R: CPS, KCS\n' % (len(SELS_R), ', '.join('{%d, %d, %d, %d, %d}' % (x, y, r, len(p), sp) for _, x, y, r, _, p, sp in SELS_R)))
        f.write('static const double CTIN[2] = {%d, %d}, CTLAMP[2] = {%d, %d};   // IN/OUT-Schalter, Netzlampe\n' % (INSW + LAMP))
    im = im1; crop = im.crop((210, 40, 650, 480)) if H >= 480 else im.crop((190, 20, 410, 240))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'cultec-eq1a-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'cultec-eq1a-maskable-512.png'), optimize=True)
    print('ok', len(data['1A']), len(data['1R']), 'Bytes')
main()
