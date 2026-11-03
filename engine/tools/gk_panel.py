#!/usr/bin/env python3
# Zeichnet die Frontplatte des GK-800RB-Simulators (Anordnung nach dem Gerät: Eingang, −10 dB, VOLUME, LO CUT / MID CONTOUR / HI BOOST,
# TREBLE / HI MID / LO MID / BASS, BOOST mit LED und Schalter, BI-AMP mit FREQ., HI MASTER / LO MASTER; kein Foto, kein Logo,
# nur die Typbezeichnung in normaler Schrift) und schreibt
#   engine/gen/gkpanel.jpg   Bild 1800 × 260 (Knöpfe ohne Zeiger; Zeiger, Schalterhebel und LEDs zeichnen Seite und Plugin)
#   engine/gen/gkgeo.json    Lage von Knöpfen, Buchse, Schaltern und LEDs in Bildpunkten
#   engine/gen/gkpanel_jpg.h, engine/gen/gkgeo_gen.h   für das Plugin
#   web/tonestack/assets/icons/gk-800rb-{180,192,512,maskable-512}.png   Ausschnitt um VOLUME als Seitensymbol
#   python3 engine/tools/gk_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1800, 260, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
KY = 118; KR = 42
KNOBS = [('vol', 290, 'VOLUME'), ('treb', 690, 'TREBLE'), ('himid', 800, 'HI MID'), ('lomid', 910, 'LO MID'), ('bass', 1020, 'BASS'),
         ('boost', 1150, 'BOOST'), ('freq', 1420, 'FREQ.'), ('hima', 1545, 'HI MASTER'), ('loma', 1660, 'LO MASTER')]
JACK = ('input', 80, 128, 23)                # INPUT (Klinke, immer belegt)
SY = 122
SWITCHES = [('pad', 175, '-10 dB'), ('locut', 405, 'LO CUT'), ('contour', 495, 'CONTOUR'), ('hiboost', 585, 'HI BOOST'), ('boostsw', 1245, 'BOOST'), ('biamp', 1335, 'BI-AMP')]   # Druckschalter des Geräts als Kippschalter
LEDS = [('boost', 1150, 48), ('power', 1760, 128)]
def font(px, f=FB, S=S): return ImageFont.truetype(f, px * S)
def draw(S):   # Platte in S-facher Auflösung (Bildpunkte der Geometrie × S)
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    # schwarze Platte mit feinem Bürstschliff
    for y in range(H * S):
        v = 28 + int(6 * math.sin(y * 0.9) * math.sin(y * 0.13)); d.line([(0, y), (W * S, y)], fill=(v, v, v + 1))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(70, 72, 76), width=6 * S)
    ink = (232, 232, 230)
    def ctext(x, y, t, px=15, f=FB, col=ink):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Typbezeichnung und Hinweis (normale Schrift, kein Logo)
    d.text((30 * S, 12 * S), '800RB', font=font(44, FB, S), fill=ink)
    d.text((32 * S, 226 * S), 'BASS AMPLIFIER SIMULATION', font=font(12, FN, S), fill=(150, 152, 156))
    # Linie über der Klangregelung
    for x0, x1, lab in [(640, 1070, 'EQUALIZATION'), (1495, 1710, 'MASTERS')]:
        d.line([(x0 * S, 46 * S), (x1 * S, 46 * S)], fill=(120, 122, 126), width=2 * S); ctext((x0 + x1) / 2, 28, lab, 13, FN, (190, 192, 196))
    # Buchse
    k, x, y, r = JACK
    ctext(x, y - r - 36, 'INPUT')
    for rr, c in [(r + 6, (110, 112, 116)), (r, (205, 208, 214)), (r - 6, (120, 122, 128)), (r - 11, (15, 15, 16))]:
        d.ellipse([(x - rr) * S, (y - rr) * S, (x + rr) * S, (y + rr) * S], fill=c)
    # Knöpfe: schwarze Kappen mit heller Kante, Skalenpunkte 0 … 10, ohne Zeiger
    for k, x, lab in KNOBS:
        ctext(x, KY + KR + 20, lab, 15)
        for j in range(11):
            a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 9) * math.sin(a), KY - (KR + 9) * math.cos(a)
            d.ellipse([(px - 2) * S, (py - 2) * S, (px + 2) * S, (py + 2) * S], fill=(200, 200, 204))
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 5) * S, (KY - KR + 8) * S, (x + KR + 5) * S, (KY + KR + 8) * S], fill=160)
        sh = sh.filter(ImageFilter.GaussianBlur(7 * S)); im.paste((8, 8, 9), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(16, 16, 18))
        d.arc([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], 190, 350, fill=(88, 88, 94), width=3 * S)
        r2 = KR - 12; d.ellipse([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], fill=(26, 26, 29))
        d.arc([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], 200, 300, fill=(70, 70, 76), width=2 * S)
    # Schalter: Beschriftung oben, Sockel (Mutter); Hebel zeichnen Seite/Plugin
    for k, x, lab in SWITCHES:
        ctext(x, SY - 64, lab, 13)
        for r, c in [(19, (70, 72, 76)), (15, (200, 203, 210)), (9, (150, 152, 158))]:
            d.ellipse([(x - r) * S, (SY - r) * S, (x + r) * S, (SY + r) * S], fill=c)
    # LEDs: Fassung (Leuchten zeichnen Seite/Plugin)
    for k, x, y in LEDS:
        d.ellipse([(x - 9) * S, (y - 9) * S, (x + 9) * S, (y + 9) * S], fill=(60, 62, 66)); d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(40, 10, 10))
        if k == 'power': ctext(x, y + 14, 'POWER', 11, FN, (190, 192, 196))
    # Schrauben
    for x in (18, W - 18):
        for y in (22, H - 22):
            d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(120, 122, 126)); d.line([((x - 4) * S, y * S), ((x + 4) * S, y * S)], fill=(60, 60, 64), width=2 * S)
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'gkpanel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR, 'label': lab} for k, x, lab in KNOBS},
           'jack': {'x': JACK[1], 'y': JACK[2], 'r': JACK[3]},
           'switches': {k: {'x': x, 'y': SY, 'label': lab} for k, x, lab in SWITCHES}, 'leds': {k: {'x': x, 'y': y} for k, x, y in LEDS}}
    json.dump(geo, open(os.path.join(GEN, 'gkgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'gkpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/gk_panel.py: gezeichnete Frontplatte GK 800RB (JPEG)\nstatic const unsigned char GKPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int GKPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'gkgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/gk_panel.py: Lage der Bedienelemente auf gkpanel.jpg (Bildpunkte)\n')
        f.write('static const int GKW = %d, GKH = %d;\n' % (W, H))
        f.write('static const int GKNKNOB = %d;\nstatic const double GKKNOB[%d][3] = {%s};\n' % (len(KNOBS), len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _ in KNOBS)))
        f.write('static const double GKJACK[3] = {%d, %d, %d};\n' % (JACK[1], JACK[2], JACK[3]))
        f.write('static const int GKNSW = %d;\nstatic const double GKSW[%d][2] = {%s};\n' % (len(SWITCHES), len(SWITCHES), ', '.join('{%d, %d}' % (x, SY) for _, x, _ in SWITCHES)))
        f.write('static const double GKLED[%d][2] = {%s};\n' % (len(LEDS), ', '.join('{%d, %d}' % (x, y) for _, x, y in LEDS)))
    print('gkpanel.jpg', len(data), 'Bytes')
    # Seitensymbole: quadratischer Ausschnitt um VOLUME, in 4-facher Auflösung gezeichnet
    S4 = 4; big = draw(S4); x0 = KNOBS[0][1] - 92; crop = big.crop((x0 * S4, 28 * S4, (x0 + 184) * S4, 212 * S4))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'gk-800rb-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), (28, 28, 29)); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'gk-800rb-maskable-512.png'), optimize=True)   # Sicherheitsrand 10 %
if __name__ == '__main__': main()
