#!/usr/bin/env python3
# Zeichnet die Frontplatte des AmpFett SVT-CL (Bedienelemente in der Reihenfolge der Ampeg-Bedienungsanleitung: 0 dB / −15 dB INPUT, GAIN, ULTRA HI,
# ULTRA LO, BASS, MIDRANGE, FREQUENCY (fünf Stellungen), TREBLE, MASTER (ab SVT-CL 1.3 ohne den rechten Teil mit Anzeige, STANDBY, POWER); kein Foto, kein Logo;
# Schriftzug „AmpFett“ in Damion wie beim B-15NF, „SVT-CL“ in normaler Schrift) und schreibt
#   engine/gen/svtpanel.jpg   Bild 1390 × 260 (Knöpfe ohne Zeiger; Zeiger, Schalterhebel und Anzeige zeichnen Seite und Plugin)
#   engine/gen/svtgeo.json    Lage von Knöpfen, Buchsen, Schaltern und Anzeige in Bildpunkten
#   engine/gen/svtpanel_jpg.h, engine/gen/svtgeo_gen.h   für das Plugin
#   web/tonestack/assets/icons/ampfett-svtcl-{180,192,512,maskable-512}.png   Ausschnitt um GAIN als Seitensymbol
#   python3 engine/tools/svt_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1390, 260, 2   # bis 1.2: 1800 (rechts Anzeige STANDBY/POWER/FAULT, STANDBY, POWER)
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
FS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fonts', 'Damion-Regular.ttf')
KY = 138; KR = 40
# (Schlüssel, x, Beschriftung, Skala): 'n' = 0 … 10, 'f' = fünf Stellungen
KNOBS = [('gain', 330, 'GAIN', 'n'), ('bass', 690, 'BASS', 'n'), ('mid', 840, 'MIDRANGE', 'n'), ('freq', 990, 'FREQUENCY', 'f'),
         ('treb', 1140, 'TREBLE', 'n'), ('master', 1290, 'MASTER', 'n')]
FPOS = [(-60, '220'), (-30, '450'), (0, '800'), (30, '1.6K'), (60, '3K')]   # FREQUENCY: Winkel der fünf Stellungen, Beschriftung (Hz)
JY = 150; JR = 22
JACKS = [('in0', 100, '0 dB'), ('in15', 196, '−15 dB')]
SY = 140
SWITCHES = [('uhi', 470, 'ULTRA', 'HI'), ('ulo', 560, 'ULTRA', 'LO')]
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def main():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    # gebürstetes Aluminium: feine waagerechte Striche, leichter Verlauf
    import random; rnd = random.Random(7)
    for y in range(H * S):
        t = y / (H * S); v = 196 + 18 * math.cos(math.pi * (t - 0.35)) + rnd.uniform(-5, 5)
        d.line([(0, y), (W * S, y)], fill=(int(v), int(v) + 1, int(v) + 4))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(70, 72, 76), width=6 * S)
    ink = (28, 30, 34)
    def ctext(x, y, t, px=16, f=FB):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=ink)
    d.text((34 * S, 4 * S), 'AmpFett', font=font(44, FS), fill=ink, stroke_width=S, stroke_fill=ink)
    tw = d.textlength('SVT-CL', font=font(26)); d.text(((W - 36) * S - tw, 14 * S), 'SVT-CL', font=font(26), fill=ink)
    # Trennlinien
    for x in (250, 610):
        d.line([(x * S, 40 * S), (x * S, (H - 30) * S)], fill=(120, 122, 128), width=2 * S)
    # Buchsen
    ctext(148, 72, 'INPUT', 15)
    for k, x, lab in JACKS:
        ctext(x, JY + JR + 14, lab, 14)
        for r, c in [(JR + 6, (60, 62, 66)), (JR, (205, 208, 214)), (JR - 6, (120, 122, 128)), (JR - 10, (15, 15, 16))]:
            d.ellipse([(x - r) * S, (JY - r) * S, (x + r) * S, (JY + r) * S], fill=c)
    # Knöpfe: Skala, schwarze Riffelkappe mit Metallmitte (ohne Zeiger)
    for k, x, lab, sc in KNOBS:
        ctext(x, KY - KR - 50, lab, 16)
        if sc == 'n':
            for j in range(11):
                a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 8) * math.sin(a), KY - (KR + 8) * math.cos(a)
                d.ellipse([(px - 2) * S, (py - 2) * S, (px + 2) * S, (py + 2) * S], fill=ink)
                tx, ty = x + (KR + 19) * math.sin(a), KY - (KR + 19) * math.cos(a); ff = font(10, FN); t = str(j); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 6 * S), t, font=ff, fill=ink)
        else:
            for i, (deg, t) in enumerate(FPOS):
                a = deg * math.pi / 180; px, py = x + (KR + 8) * math.sin(a), KY - (KR + 8) * math.cos(a)
                d.ellipse([(px - 2.5) * S, (py - 2.5) * S, (px + 2.5) * S, (py + 2.5) * S], fill=ink)
                tx, ty = x + (KR + 22) * math.sin(a), KY - (KR + 22) * math.cos(a); ff = font(11, FB); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 7 * S), t, font=ff, fill=ink)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 5) * S, (KY - KR + 8) * S, (x + KR + 5) * S, (KY + KR + 8) * S], fill=150)
        sh = sh.filter(ImageFilter.GaussianBlur(7 * S)); im.paste((40, 40, 44), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(20, 20, 22))
        for j in range(30):
            a = 2 * math.pi * j / 30; c = 44 + int(24 * max(0, math.cos(a + 2.3)))
            d.line([((x + (KR - 2) * math.cos(a)) * S, (KY + (KR - 2) * math.sin(a)) * S), ((x + (KR - 10) * math.cos(a)) * S, (KY + (KR - 10) * math.sin(a)) * S)], fill=(c, c, c + 2), width=3 * S)
        r2 = KR - 13
        for i in range(r2 * S, 0, -1):   # Metallkappe mit Verlauf
            t = i / (r2 * S); v = int(150 + 70 * (1 - t) ** 0.6); ox, oy = -0.2 * (r2 - i / S), -0.25 * (r2 - i / S)
            d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=(v, v + 1, v + 4))
    # Schalter: Mutter; Hebel zeichnen Seite/Plugin. ULTRA HI/LO: Hebel oben = ein
    for k, x, top, bot in SWITCHES:
        ctext(x, SY - 92, top, 15)
        if bot: ctext(x, SY - 74, bot, 15)
        if k in ('uhi', 'ulo'): ctext(x, SY + 46, 'OFF', 11); ctext(x, SY - 56, 'ON', 11)
        else: ctext(x, SY - 56, 'ON', 11); ctext(x, SY + 46, 'OFF', 11)
        for r, c in [(18, (70, 72, 76)), (14, (210, 213, 220)), (9, (150, 152, 158))]:
            d.ellipse([(x - r) * S, (SY - r) * S, (x + r) * S, (SY + r) * S], fill=c)
    for x in (18, W - 18):
        d.ellipse([(x - 7) * S, (H / 2 - 7) * S, (x + 7) * S, (H / 2 + 7) * S], fill=(120, 122, 126))
    return im
def write():
    im = main().resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'svtpanel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR, 'label': lab, 'scale': sc} for k, x, lab, sc in KNOBS},
           'fpos': [a for a, _ in FPOS], 'jacks': {k: {'x': x, 'y': JY, 'r': JR, 'label': lab} for k, x, lab in JACKS},
           'switches': {k: {'x': x, 'y': SY} for k, x, _, _ in SWITCHES}}
    json.dump(geo, open(os.path.join(GEN, 'svtgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'svtpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/svt_panel.py: gezeichnete Frontplatte AmpFett SVT-CL (JPEG)\nstatic const unsigned char SVTPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int SVTPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'svtgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/svt_panel.py: Lage der Bedienelemente auf svtpanel.jpg (Bildpunkte)\n')
        f.write('static const int SVTW = %d, SVTH = %d;\n' % (W, H))
        f.write('static const double SVTKNOB[%d][3] = {%s};   // GAIN, BASS, MIDRANGE, FREQUENCY, TREBLE, MASTER\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _, _ in KNOBS)))
        f.write('static const double SVTFPOS[5] = {%s};   // FREQUENCY: Winkel der Stellungen (Grad)\n' % ', '.join(str(a) for a, _ in FPOS))
        f.write('static const double SVTJACK[2][3] = {%s};   // 0 dB, −15 dB\n' % ', '.join('{%d, %d, %d}' % (x, JY, JR) for _, x, _ in JACKS))
        f.write('static const double SVTSW[%d][2] = {%s};   // ULTRA HI, ULTRA LO\n' % (len(SWITCHES), ', '.join('{%d, %d}' % (x, SY) for _, x, _, _ in SWITCHES)))
    print('svtpanel.jpg', len(data), 'Bytes')
    S4 = 4; big = main_big(S4); x0 = KNOBS[0][1] - 90; crop = big.crop((x0 * S4, 46 * S4, (x0 + 180) * S4, 226 * S4))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'ampfett-svtcl-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), (200, 202, 206)); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'ampfett-svtcl-maskable-512.png'), optimize=True)
def main_big(s):
    global S; old = S; S = s
    try: return main()
    finally: S = old
if __name__ == '__main__': write()
