#!/usr/bin/env python3
# Zeichnet die Frontplatte des Wisent Echoreck (Röhren-Echo mit Magnettrommel nach dem Binson Echorec PE 603 T). Eigene Gestaltung im Stil
# italienischer Geräte der Sechziger: kein Foto, kein Logo, keine Nachbildung der Binson-Frontplatte. Magisches Auge (Aufnahmepegel, EM84),
# Knöpfe INPUT, LENGTH OF SWELL, VOLUME, BASS, TREBLE, Tasten ECHO/REPEAT/SWELL, zwei Reihen Kopftasten (oben Rückkopplung, unten Wiedergabe),
# dazu DRY (Trimmer TD, im Gerät innen), VARISPEED (späterer Umbau), LEVEL, OUTPUT und Netzfrequenz 50/60 Hz (Modell, nicht am Gerät).
# Schreibt engine/gen/wisentpanel.jpg (2240 × 240, ohne Zeiger und Tasten), wisentgeo.json, wisentpanel_jpg.h, wisentgeo_gen.h und die Seitensymbole
#   python3 engine/tools/wisent_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2240, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
PANEL = (58, 64, 62); INK = (232, 226, 206); DIM = (170, 168, 152); GOLD = (206, 172, 98)
KY = 128; KR = 30
KNOBS = [('input', 470, 'INPUT', '0', '10'), ('swell', 590, 'LENGTH OF SWELL', '0', '10'), ('volume', 710, 'VOLUME', '0', '10'),
         ('bass', 830, 'BASS', '−', '+'), ('treble', 950, 'TREBLE', '−', '+'), ('level', 1880, 'LEVEL', '20 mV', '2 V'), ('out', 2000, 'OUTPUT', '−24', '+24 dB'),
         ('dry', 1640, 'DRY (TD)', '0', '10'), ('speed', 1760, 'VARISPEED', '¼×', '2×')]   # Reihenfolge = Parameter der Oberfläche (KPARAM)
EYE = (330, 128, 120, 30)             # Mitte x, y, Breite, Höhe des Leuchtschirms
MODE = [(1085, 76), (1085, 128), (1085, 180)]   # ECHO, REPEAT, SWELL (Mitte; Taste 74 × 34)
BW, BH = 74, 34
HEADX = [1262, 1342, 1422, 1502]; HFB, HOUT = 100, 172   # Kopftasten: obere Reihe Rückkopplung, untere Wiedergabe (Taste 56 × 30)
HW, HH = 56, 30
MAINS = (2125, 128)                   # Kippschalter 50/60 Hz
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def draw():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):
        v = int(8 * (0.5 - y / (H * S))); d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(30, 32, 31), width=5 * S)
    d.rectangle([10 * S, 10 * S, (W - 10) * S, (H - 10) * S], outline=GOLD, width=S)
    for x in (24, W - 24):
        for y in (24, H - 24):
            d.ellipse([(x - 8) * S, (y - 8) * S, (x + 8) * S, (y + 8) * S], fill=(150, 150, 140), outline=(80, 80, 72), width=S); d.line([((x - 5) * S, y * S), ((x + 5) * S, y * S)], fill=(70, 70, 64), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Name links
    ctext(150, 50, 'WISENT', 34, FB, GOLD); ctext(150, 92, 'ECHORECK', 20, FB, INK)
    ctext(150, 126, 'magnetic drum echo · 4 heads', 10.5, FN, DIM); ctext(150, 141, 'circuit model after the', 10.5, FN, DIM); ctext(150, 156, 'Binson Echorec PE 603 T', 10.5, FN, DIM)
    ctext(150, 180, 'own design', 10, FN, DIM)
    # Magisches Auge
    ex, ey, ew, eh = EYE; ctext(ex, 52, 'RECORD', 12, FB); d.rounded_rectangle([(ex - ew / 2 - 10) * S, (ey - eh / 2 - 10) * S, (ex + ew / 2 + 10) * S, (ey + eh / 2 + 10) * S], radius=8 * S, fill=(22, 24, 22), outline=GOLD, width=2 * S)
    d.rectangle([(ex - ew / 2) * S, (ey - eh / 2) * S, (ex + ew / 2) * S, (ey + eh / 2) * S], fill=(12, 26, 16))
    ctext(ex, 160, 'level', 10.5, FN, DIM)
    # Knöpfe
    for k, x, lab, lft, rgt in KNOBS:
        ctext(x, 52, lab, 12.5 if len(lab) < 12 else 11.5, FB)
        for j in range(11):
            a = (-150 + 30 * j) * math.pi / 180; r1, r2 = KR + 5, KR + (11 if j % 5 == 0 else 8)
            d.line([((x + r1 * math.sin(a)) * S, (KY - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (KY - r2 * math.cos(a)) * S)], fill=INK, width=max(1, int(1.5 * S)))
        ff = font(11, FN)
        for j, t in [(0, lft), (10, rgt)]:
            a = (-150 + 30 * j) * math.pi / 180; tx, ty = x + (KR + 22) * math.sin(a), KY - (KR + 22) * math.cos(a); tw = d.textlength(t, font=ff); d.text((tx * S - tw / 2, (ty - 2) * S), t, font=ff, fill=DIM)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 3) * S, (KY - KR + 6) * S, (x + KR + 3) * S, (KY + KR + 6) * S], fill=200); sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((14, 16, 15), (0, 0), sh)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(232, 224, 200))
        for j in range(36):
            a = j * math.pi / 18; c = 210 if math.sin(a - 2.4) > 0 else 186; kc = KR * 0.72
            d.line([((x + (kc + 1) * math.cos(a)) * S, (KY + (kc + 1) * math.sin(a)) * S), ((x + (KR - 1) * math.cos(a)) * S, (KY + (KR - 1) * math.sin(a)) * S)], fill=(c, c - 6, c - 22), width=max(1, int(1.4 * S)))
        kc = KR * 0.72; d.ellipse([(x - kc) * S, (KY - kc) * S, (x + kc) * S, (KY + kc) * S], fill=(222, 214, 190))
        if k == 'speed':   # 1× (Stellung log(4)/log(8) der logarithmischen Skala)
            a = (-150 + 300 * math.log(4) / math.log(8)) * math.pi / 180; tx, ty = x + (KR + 22) * math.sin(a), KY - (KR + 22) * math.cos(a); tw = d.textlength('1×', font=ff); d.text((tx * S - tw / 2, (ty - 2) * S), '1×', font=ff, fill=GOLD)
    # Tasten: Betriebsart und Köpfe (Schächte; die Tasten zeichnet die Oberfläche)
    for (x, y) in MODE:
        d.rounded_rectangle([(x - BW / 2 - 4) * S, (y - BH / 2 - 4) * S, (x + BW / 2 + 4) * S, (y + BH / 2 + 4) * S], radius=5 * S, fill=(26, 28, 27))
    for y, lab in [(HFB, 'FEEDBACK'), (HOUT, 'PLAYBACK')]:
        ctext(1178, y - 8, lab, 10, FB, DIM)
        for x in HEADX: d.rounded_rectangle([(x - HW / 2 - 4) * S, (y - HH / 2 - 4) * S, (x + HW / 2 + 4) * S, (y + HH / 2 + 4) * S], radius=5 * S, fill=(26, 28, 27))
    ctext(1382, 22, 'HEADS', 12, FB)
    for i, x in enumerate(HEADX): ctext(x, 44, str(i + 1), 14, FB)   # Zeiten darunter zeichnet die Oberfläche (50/60 Hz)
    ctext(1085, 22, 'MODE', 12, FB)
    # Trennlinie Modell
    d.line([(1560 * S, 30 * S), (1560 * S, (H - 30) * S)], fill=GOLD, width=2 * S); ctext(1900, 22, 'MODEL  (not on the unit)', 11, FB, DIM)
    mx, my = MAINS; ctext(mx, 52, 'MAINS', 12.5, FB); ctext(mx, 80, '60 Hz', 11, FN, DIM); ctext(mx, 164, '50 Hz', 11, FN, DIM)
    d.ellipse([(mx - 16) * S, (my - 16) * S, (mx + 16) * S, (my + 16) * S], fill=(200, 200, 190), outline=(90, 90, 84), width=2 * S)
    ctext(mx, 196, 'drum speed', 10, FN, DIM)
    return im
def main():
    im = draw().resize((W, H), Image.LANCZOS); os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'wisentpanel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': KY, 'r': KR} for k, x, _, _, _ in KNOBS], 'eye': {'x': EYE[0], 'y': EYE[1], 'w': EYE[2], 'h': EYE[3]},
           'mode': [{'x': x, 'y': y, 'w': BW, 'h': BH} for x, y in MODE], 'fb': [{'x': x, 'y': HFB, 'w': HW, 'h': HH} for x in HEADX], 'out': [{'x': x, 'y': HOUT, 'w': HW, 'h': HH} for x in HEADX],
           'mains': {'x': MAINS[0], 'y': MAINS[1]}}
    json.dump(geo, open(os.path.join(GEN, 'wisentgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'wisentpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/wisent_panel.py: gezeichnete Frontplatte Wisent Echoreck (JPEG)\nstatic const unsigned char WISENTPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int WISENTPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'wisentgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/wisent_panel.py: Lage der Bedienelemente auf wisentpanel.jpg (Bildpunkte)\n')
        f.write('static const int WSW = %d, WSH = %d;\n' % (W, H))
        f.write('static const double WSKNOB[%d][3] = {%s};   // INPUT, LENGTH OF SWELL, VOLUME, BASS, TREBLE, LEVEL, OUTPUT, DRY, VARISPEED\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _, _, _ in KNOBS)))
        f.write('static const double WSEYE[4] = {%d, %d, %d, %d};   // magisches Auge: Mitte x, y, Breite, Höhe\n' % EYE)
        f.write('static const double WSMODE[3][2] = {%s}; static const double WSBTN[2] = {%d, %d};   // ECHO, REPEAT, SWELL\n' % (', '.join('{%d, %d}' % m for m in MODE), BW, BH))
        f.write('static const double WSHEADX[4] = {%s}; static const double WSHFB = %d, WSHOUT = %d; static const double WSHBTN[2] = {%d, %d};   // Kopftasten\n' % (', '.join(str(x) for x in HEADX), HFB, HOUT, HW, HH))
        f.write('static const double WSMAINS[2] = {%d, %d};   // Netzfrequenz\n' % MAINS)
    crop = im.crop((240, 30, 520, 230)).resize((280, 200)); sq = Image.new('RGB', (280, 280), PANEL); sq.paste(crop, (0, 40))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): sq.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'wisent-echoreck-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(sq.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'wisent-echoreck-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
