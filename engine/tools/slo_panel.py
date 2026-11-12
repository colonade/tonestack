#!/usr/bin/env python3
# Zeichnet die Frontplatte des Loldano SLO-100 (Bedienelemente in der Reihenfolge der Bedienungsanleitung des Soldano SLO-100: INPUT, BRIGHT, CLEAN/CRUNCH,
# Kanalwahl NORMAL/OVERDRIVE, PREAMP NORMAL, PREAMP OVERDRIVE, BASS, MIDDLE, TREBLE, MASTER NORMAL, MASTER OVERDRIVE, PRESENCE; ohne STANDBY/POWER;
# kein Foto, kein Logo; Schriftzug „Loldano“ in Damion) und schreibt
#   engine/gen/slopanel.jpg   Bild 1390 × 260 (Knöpfe ohne Zeiger; Zeiger und Schalterhebel zeichnen Seite und Plugin)
#   engine/gen/slogeo.json    Lage von Knöpfen, Buchse und Schaltern in Bildpunkten
#   engine/gen/slopanel_jpg.h, engine/gen/slogeo_gen.h   für das Plugin
#   web/tonestack/assets/icons/loldano-slo100-{180,192,512,maskable-512}.png   Ausschnitt um PREAMP OVERDRIVE als Seitensymbol
#   python3 engine/tools/slo_panel.py [repo]
import json, os, sys, math, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1390, 260, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
FS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fonts', 'Damion-Regular.ttf')
KY = 146; KR = 34
# (Schlüssel, x, obere Zeile, untere Zeile)
KNOBS = [('ngain', 452, 'PREAMP', 'NORMAL'), ('gain', 574, 'PREAMP', 'OVERDRIVE'), ('bass', 696, 'BASS', ''), ('mid', 818, 'MIDDLE', ''), ('treb', 940, 'TREBLE', ''),
         ('nmaster', 1062, 'MASTER', 'NORMAL'), ('master', 1184, 'MASTER', 'OVERDRIVE'), ('pres', 1306, 'PRESENCE', '')]
JY = 150; JR = 22
JACKS = [('in', 78, 'INPUT')]
SY = 146
# (Schlüssel, x, Beschriftung oben = Hebel oben, Beschriftung unten = Hebel unten)
SWITCHES = [('bright', 170, 'BRIGHT', 'NORMAL'), ('crunch', 250, 'CRUNCH', 'CLEAN'), ('od', 340, 'OVERDRIVE', 'NORMAL')]
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def main():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im); rnd = random.Random(11)
    for y in range(H * S):   # schwarze, leicht glänzende Platte
        t = y / (H * S); v = 20 + 9 * math.cos(math.pi * (t - 0.3)) + rnd.uniform(-1.2, 1.2)
        d.line([(0, y), (W * S, y)], fill=(int(v), int(v), int(v) + 2))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(92, 94, 98), width=5 * S)
    ink = (236, 236, 232)
    def ctext(x, y, t, px=16, f=FB, col=ink):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    d.text((30 * S, 6 * S), 'Loldano', font=font(44, FS), fill=ink, stroke_width=S, stroke_fill=ink)
    tw = d.textlength('SLO-100', font=font(24)); d.text(((W - 34) * S - tw, 16 * S), 'SLO-100', font=font(24), fill=ink)
    # Rahmen um die Gruppen: Schalter des Normal-Kanals, Kanalwahl
    d.line([(134 * S, 228 * S), (286 * S, 228 * S)], fill=(150, 152, 156), width=S); ctext(210, 231, 'NORMAL CHANNEL', 11, FN)
    d.line([(392 * S, 50 * S), (392 * S, (H - 30) * S)], fill=(80, 82, 86), width=2 * S)
    for k, x, lab in JACKS:
        ctext(x, JY - JR - 34, lab, 15)
        for r, c in [(JR + 6, (120, 122, 128)), (JR, (205, 208, 214)), (JR - 6, (120, 122, 128)), (JR - 10, (8, 8, 9))]:
            d.ellipse([(x - r) * S, (JY - r) * S, (x + r) * S, (JY + r) * S], fill=c)
    for k, x, top, bot in KNOBS:
        if bot: ctext(x, KY - KR - 66, top, 15); ctext(x, KY - KR - 49, bot, 12, FN)
        else: ctext(x, KY - KR - 56, top, 15)
        for j in range(11):
            a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 7) * math.sin(a), KY - (KR + 7) * math.cos(a)
            d.ellipse([(px - 1.8) * S, (py - 1.8) * S, (px + 1.8) * S, (py + 1.8) * S], fill=ink)
            tx, ty = x + (KR + 17) * math.sin(a), KY - (KR + 17) * math.cos(a); ff = font(10, FN); t = str(j); tw = d.textlength(t, font=ff)
            d.text((tx * S - tw / 2, ty * S - 6 * S), t, font=ff, fill=ink)
        # schwarzer Zeigerknopf: Schürze mit hellem Rand, gewölbte Kappe
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(70, 72, 76))
        d.ellipse([(x - KR + 2) * S, (KY - KR + 2) * S, (x + KR - 2) * S, (KY + KR - 2) * S], fill=(26, 26, 28))
        r2 = KR - 9
        for i in range(r2 * S, 0, -1):
            t = i / (r2 * S); v = int(34 + 30 * (1 - t) ** 0.7); ox, oy = -0.22 * (r2 - i / S), -0.28 * (r2 - i / S)
            d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=(v, v, v + 2))
        d.ellipse([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], outline=(8, 8, 9), width=S)
    for k, x, top, bot in SWITCHES:
        ctext(x, SY - 76, top, 12); ctext(x, SY + 58, bot, 12)
        for r, c in [(18, (120, 122, 128)), (14, (210, 213, 220)), (9, (150, 152, 158))]:
            d.ellipse([(x - r) * S, (SY - r) * S, (x + r) * S, (SY + r) * S], fill=c)
    for x in (16, W - 16):
        d.ellipse([(x - 6) * S, (H / 2 - 6) * S, (x + 6) * S, (H / 2 + 6) * S], fill=(120, 122, 126))
    return im
def write():
    im = main().resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'slopanel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR} for k, x, _, _ in KNOBS}, 'jacks': {k: {'x': x, 'y': JY, 'r': JR} for k, x, _ in JACKS},
           'switches': {k: {'x': x, 'y': SY} for k, x, _, _ in SWITCHES}}
    json.dump(geo, open(os.path.join(GEN, 'slogeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'slopanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/slo_panel.py: gezeichnete Frontplatte Loldano SLO-100 (JPEG)\nstatic const unsigned char SLOPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int SLOPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'slogeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/slo_panel.py: Lage der Bedienelemente auf slopanel.jpg (Bildpunkte)\n')
        f.write('static const int SLOW = %d, SLOH = %d;\n' % (W, H))
        f.write('static const double SLOKNOB[%d][3] = {%s};   // PREAMP NORMAL, PREAMP OVERDRIVE, BASS, MIDDLE, TREBLE, MASTER NORMAL, MASTER OVERDRIVE, PRESENCE\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _, _ in KNOBS)))
        f.write('static const double SLOJACK[1][3] = {%s};   // INPUT\n' % ', '.join('{%d, %d, %d}' % (x, JY, JR) for _, x, _ in JACKS))
        f.write('static const double SLOSW[%d][2] = {%s};   // BRIGHT, CRUNCH/CLEAN, OVERDRIVE/NORMAL (Hebel oben = erstgenannt)\n' % (len(SWITCHES), ', '.join('{%d, %d}' % (x, SY) for _, x, _, _ in SWITCHES)))
    print('slopanel.jpg', len(data), 'Bytes')
    S4 = 4; big = main_big(S4); x0 = KNOBS[1][1] - 90; crop = big.crop((x0 * S4, 56 * S4, (x0 + 180) * S4, 236 * S4))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'loldano-slo100-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), (24, 24, 26)); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'loldano-slo100-maskable-512.png'), optimize=True)
def main_big(s):
    global S; old = S; S = s
    try: return main()
    finally: S = old
if __name__ == '__main__': write()
