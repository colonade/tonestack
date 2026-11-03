#!/usr/bin/env python3
# Zeichnet die Frontplatte des AmpFett B-15NF (Anordnung nach Fotos des Ampeg B-15NF, kein Foto, kein Logo;
# Schriftzug „AmpFett“ in Damion, SIL OFL, engine/tools/fonts) und schreibt
#   engine/gen/b15panel.jpg   Bild 1800 × 260 (Knöpfe ohne Zeiger; Zeiger, Schalterhebel und Markierungen zeichnen Seite und Plugin)
#   engine/gen/b15geo.json    Lage von Knöpfen, Buchsen und Schaltern in Bildpunkten
#   engine/gen/b15panel_jpg.h, engine/gen/b15geo_gen.h   für das Plugin
#   python3 engine/tools/b15_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1800, 260, 2                       # Bildgröße, Überabtastung beim Zeichnen
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FI = '/usr/share/fonts/truetype/dejavu/DejaVuSans-BoldOblique.ttf'
FS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fonts', 'Damion-Regular.ttf')   # Schreibschrift für den Namen
KY = 112; KR = 50                            # Knöpfe: Mittelpunkt y, Radius
KNOBS = [('vol1', 505, 'VOLUME 1'), ('treb1', 655, 'TREBLE 1'), ('bass1', 805, 'BASS 1'),
         ('vol2', 965, 'VOLUME 2'), ('treb2', 1115, 'TREBLE 2'), ('bass2', 1265, 'BASS 2')]
JY = 128; JR = 25
JACKS = [('guitar', 165, 'GUITAR'), ('bass', 265, 'BASS'), ('inst', 370, 'INST.')]
SY = 122
SWITCHES = [('power', 1530, 'ON', 'OFF')]   # ab 1.15 nur noch POWER (STANDBY und GROUND entfernt)

def font(px, f=FB): return ImageFont.truetype(f, px * S)
def main():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    # Chromplatte: senkrechter Verlauf (oben hell, Mitte dunkler Streifen, unten hell), leichtes Rauschen
    for y in range(H * S):
        t = y / (H * S); v = 205 + 35 * math.cos(2 * math.pi * (t - 0.18)) - 25 * math.exp(-((t - 0.62) / 0.12) ** 2)
        v = max(120, min(250, v)); d.line([(0, y), (W * S, y)], fill=(int(v), int(v) + 2, int(v) + 6))
    # Rahmen
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(80, 82, 86), width=6 * S)
    # waagerechte Zierlinien hinter den Knöpfen
    for i, yy in enumerate([62, 82, 102, 122, 142, 162, 182]):
        x0 = 430 + (abs(3 - i) * 18); d.line([(x0 * S, yy * S), (1360 * S, yy * S)], fill=(250, 250, 252), width=3 * S); d.line([(x0 * S, (yy + 2) * S), (1360 * S, (yy + 2) * S)], fill=(150, 152, 158), width=1 * S)
    # Schriftzüge (normale Schrift, kein Logo)
    ink = (40, 42, 46)
    d.text((36 * S, -8 * S), 'AmpFett', font=font(56, FS), fill=ink, stroke_width=S, stroke_fill=ink)
    tw = d.textlength('Portaflex', font=font(30, FI)); d.text(((W - 40) * S - tw, 14 * S), 'Portaflex', font=font(30, FI), fill=ink)
    d.line([(40 * S, 54 * S), (300 * S, 54 * S)], fill=ink, width=2 * S); d.line([((W - 300) * S, 54 * S), ((W - 40) * S, 54 * S)], fill=ink, width=2 * S)
    def ctext(x, y, t, px=17):
        f = font(px); w = d.textlength(t, font=f); d.text((x * S - w / 2, y * S), t, font=f, fill=ink)
    # Buchsen
    for k, x, lab in JACKS:
        ctext(x, JY - JR - 34, lab)
        for r, c in [(JR + 6, (60, 62, 66)), (JR, (205, 208, 214)), (JR - 6, (120, 122, 128)), (JR - 11, (15, 15, 16))]:
            d.ellipse([(x - r) * S, (JY - r) * S, (x + r) * S, (JY + r) * S], fill=c)
    # Klammer CHANNEL 1 unter GUITAR/BASS, CHANNEL 2 unter INST.
    by = JY + JR + 22
    d.line([(150 * S, by * S), (280 * S, by * S)], fill=ink, width=2 * S); ctext(215, by + 4, 'CHANNEL 1', 15)
    ctext(370, by + 4, 'CHANNEL 2', 15)
    # Knöpfe: schwarze, geriffelte Kappen mit Lichtkante, ohne Zeiger
    for k, x, lab in KNOBS:
        ctext(x, KY + KR + 22, lab)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 6) * S, (KY - KR + 10) * S, (x + KR + 6) * S, (KY + KR + 10) * S], fill=150)
        sh = sh.filter(ImageFilter.GaussianBlur(8 * S)); im.paste((30, 30, 32), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(22, 22, 24))
        for j in range(24):   # Rippen am Rand
            a = 2 * math.pi * j / 24; c = 46 + int(26 * max(0, math.cos(a + 2.3)))
            x1, y1 = x + (KR - 2) * math.cos(a), KY + (KR - 2) * math.sin(a); x2, y2 = x + (KR - 13) * math.cos(a), KY + (KR - 13) * math.sin(a)
            d.line([(x1 * S, y1 * S), (x2 * S, y2 * S)], fill=(c, c, c + 2), width=4 * S)
        r2 = KR - 15; d.ellipse([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], fill=(30, 30, 33))
        d.arc([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], 200, 300, fill=(95, 95, 100), width=3 * S)
    # Schalter: Sockel (Mutter); Hebel zeichnen Seite/Plugin
    for k, x, top, bot in SWITCHES:
        ctext(x, SY - 62, top); ctext(x, SY + 44, bot)
        for r, c in [(20, (70, 72, 76)), (16, (200, 203, 210)), (10, (150, 152, 158))]:
            d.ellipse([(x - r) * S, (SY - r) * S, (x + r) * S, (SY + r) * S], fill=c)
    # Schrauben links/rechts
    for x in (18, W - 18):
        d.ellipse([(x - 7) * S, (H / 2 - 7) * S, (x + 7) * S, (H / 2 + 7) * S], fill=(110, 90, 70))
    im = im.resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'b15panel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR, 'label': lab} for k, x, lab in KNOBS},
           'jacks': {k: {'x': x, 'y': JY, 'r': JR, 'label': lab} for k, x, lab in JACKS},
           'switches': {k: {'x': x, 'y': SY, 'top': t, 'bottom': b} for k, x, t, b in SWITCHES}}
    json.dump(geo, open(os.path.join(GEN, 'b15geo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'b15panel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/b15_panel.py: gezeichnete Frontplatte B-15NF (JPEG)\nstatic const unsigned char B15PANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int B15PANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'b15geo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/b15_panel.py: Lage der Bedienelemente auf b15panel.jpg (Bildpunkte)\n')
        f.write('static const int B15W = %d, B15H = %d;\n' % (W, H))
        f.write('static const double B15KNOB[6][3] = {%s};\n' % ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _ in KNOBS))
        f.write('static const double B15JACK[3][3] = {%s};\n' % ', '.join('{%d, %d, %d}' % (x, JY, JR) for _, x, _ in JACKS))
        f.write('static const int B15NSW = %d;\nstatic const double B15SW[%d][2] = {%s};\n' % (len(SWITCHES), len(SWITCHES), ', '.join('{%d, %d}' % (x, SY) for _, x, _, _ in SWITCHES)))
    print('b15panel.jpg', len(data), 'Bytes')
if __name__ == '__main__': main()
