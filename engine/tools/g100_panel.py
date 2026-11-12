#!/usr/bin/env python3
# Zeichnet die Frontplatte des Lametta-G100-212II-Simulators nach der Zeichnung auf Seite 1 des Service Manuals (Anordnung: Kanal A VOLUME (PULL FAT),
# TREBLE (PULL BRIGHT), MIDDLE, BASS; A/B-Taste mit Anzeige; INPUT HIGH/LOW; Kanal B GAIN (PULL FAT), MASTER VOL, TREBLE (PULL BRIGHT), MIDDLE, BASS;
# PARAMETRIC EQ LEVEL / Q / FREQUENCY; REVERB; POWER). Kein Foto, kein Logo, nur die Typbezeichnung in normaler Schrift. Schreibt
#   engine/gen/g100panel.jpg   Bild 1800 × 260 (Knöpfe ohne Zeiger; Zeiger, Stecker, Anzeigen zeichnen Seite und Plugin)
#   engine/gen/g100geo.json    Lage von Knöpfen, Zugschaltern, Buchsen, Taste und Anzeigen in Bildpunkten
#   engine/gen/g100panel_jpg.h, engine/gen/g100geo_gen.h   für das Plugin
#   web/tonestack/assets/icons/lametta-g100-{180,192,512,maskable-512}.png   Ausschnitt um VOLUME als Seitensymbol
#   python3 engine/tools/g100_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1800, 260, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
KY = 136; KR = 31
# (Schlüssel, x, Beschriftung, Zugschalter, Skala): Skala 'n' = 0 … 10, 'lvl' = −5 … +5, 'q' = Symbole, 'f' = 100 Hz … 5K
KNOBS = [('vola', 120, 'VOLUME', 'PULL FAT', 'n'), ('treba', 224, 'TREBLE', 'PULL BRIGHT', 'n'), ('mida', 328, 'MIDDLE', None, 'n'), ('bassa', 432, 'BASS', None, 'n'),
         ('gainb', 766, 'GAIN', 'PULL FAT', 'n'), ('masterb', 870, 'MASTER VOL', None, 'n'), ('trebb', 974, 'TREBLE', 'PULL BRIGHT', 'n'), ('midb', 1078, 'MIDDLE', None, 'n'), ('bassb', 1182, 'BASS', None, 'n'),
         ('level', 1294, 'LEVEL', None, 'lvl'), ('q', 1398, 'Q', None, 'q'), ('freq', 1502, 'FREQUENCY', None, 'f'), ('reverb', 1612, 'REVERB', None, 'n')]
SEPS = [492, 712, 1240, 1560, 1668]            # Trennlinien zwischen den Gruppen
BTN = ('ab', 540, 100, 14)                     # A/B-Taste (quadratisch, halbe Kantenlänge) mit Anzeige darüber
LED = ('ab', 540, 62)
JACKS = [('high', 606, 136, 19), ('low', 662, 136, 19)]
POWER = {'lamp': (1712, 136, 11), 'switch': (1762, 136)}
PULLY = 44                                     # Zugschalter-Beschriftung (Trefferfläche für Seite und Plugin)
def font(px, f=FB, S=S): return ImageFont.truetype(f, px * S)
def draw(S):   # Platte in S-facher Auflösung (Bildpunkte der Geometrie × S)
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    # schwarze Platte mit feinem Schliff, helle Zierleiste unten (wie in der Zeichnung des Manuals)
    for y in range(H * S):
        v = 24 + int(5 * math.sin(y * 0.7) * math.sin(y * 0.11)); d.line([(0, y), (W * S, y)], fill=(v, v, v + 1))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(70, 72, 76), width=6 * S)
    d.rectangle([0, (H - 16) * S, W * S, H * S], fill=(196, 198, 202)); d.rectangle([0, (H - 16) * S, W * S, (H - 13) * S], fill=(120, 122, 126))
    ink = (236, 236, 234)
    def ctext(x, y, t, px=13, f=FB, col=ink):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Typbezeichnung (normale Schrift, kein Logo) und Hinweis
    d.text((30 * S, 204 * S), 'G100-212 II', font=font(22, FB, S), fill=ink)
    d.text((190 * S, 212 * S), 'GUITAR AMPLIFIER SIMULATION', font=font(10, FN, S), fill=(150, 152, 156))
    for x in SEPS: d.line([(x * S, 40 * S), (x * S, 220 * S)], fill=(130, 132, 136), width=2 * S)
    # Klammer PARAMETRIC EQ
    d.line([(1262 * S, 36 * S), (1534 * S, 36 * S)], fill=(190, 192, 196), width=2 * S); ctext(1398, 20, 'PARAMETRIC EQ', 12, FN, (210, 212, 216))
    # Buchsen
    ctext((JACKS[0][1] + JACKS[1][1]) / 2, 176, 'INPUT', 12)
    for k, x, y, r in JACKS:
        ctext(x, y - r - 40, k.upper(), 12)
        for rr, c in [(r + 5, (110, 112, 116)), (r, (205, 208, 214)), (r - 5, (120, 122, 128)), (r - 9, (15, 15, 16))]:
            d.ellipse([(x - rr) * S, (y - rr) * S, (x + rr) * S, (y + rr) * S], fill=c)
    # A/B-Taste (Rahmen; den gedrückten Zustand und die Anzeige zeichnen Seite/Plugin)
    k, x, y, h = BTN
    ctext(x, 24, 'A/B', 12)
    d.rectangle([(x - h - 3) * S, (y - h - 3) * S, (x + h + 3) * S, (y + h + 3) * S], fill=(70, 72, 76))
    d.rectangle([(x - h) * S, (y - h) * S, (x + h) * S, (y + h) * S], fill=(28, 28, 30))
    d.rectangle([(LED[1] - 10) * S, (LED[2] - 7) * S, (LED[1] + 10) * S, (LED[2] + 7) * S], fill=(60, 62, 66))
    # Knöpfe: Beschriftung und Zugschalter-Hinweis oben, Skala, schwarze Kappe mit heller Kante, ohne Zeiger
    for k, x, lab, pull, scale in KNOBS:
        ctext(x, 60, lab, 13)
        if pull: ctext(x, PULLY, pull, 10, FN, (200, 202, 206))
        if scale == 'n': marks = [(j, str(j)) for j in range(11)]
        elif scale == 'lvl': marks = [(j, str(abs(j - 5))) for j in range(11)]
        elif scale == 'f': marks = [(0, '100'), (2.8, '200'), (5, '500'), (7.2, '1K'), (10, '5K')]
        else: marks = [(0, ''), (10, '')]
        for j, t in marks:
            a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 8) * math.sin(a), KY - (KR + 8) * math.cos(a)
            d.ellipse([(px - 1.6) * S, (py - 1.6) * S, (px + 1.6) * S, (py + 1.6) * S], fill=(200, 200, 204))
            if t:
                tx, ty = x + (KR + 17) * math.sin(a), KY - (KR + 17) * math.cos(a); ff = font(8, FN, S); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 5 * S), t, font=ff, fill=(214, 214, 218))
        if scale == 'q':   # Symbole: breit (links) und spitz (rechts)
            for j, pts in [(0, [(-9, 4), (-5, -4), (0, -5), (5, -4), (9, 4)]), (10, [(-7, 5), (0, -6), (7, 5)])]:
                a = (-150 + 30 * j) * math.pi / 180; cx, cy = x + (KR + 19) * math.sin(a), KY - (KR + 19) * math.cos(a)
                d.line([((cx + px_) * S, (cy + py_) * S) for px_, py_ in pts], fill=(214, 214, 218), width=max(1, int(1.5 * S)), joint='curve')
        if scale == 'f': ctext(x, KY + KR + 18, 'Hz', 9, FN, (214, 214, 218))
        if scale == 'lvl':
            ff = font(10, FN, S); d.text(((x - KR - 18) * S, (KY + KR + 2) * S), '-', font=ff, fill=(214, 214, 218)); d.text(((x + KR + 12) * S, (KY + KR + 2) * S), '+', font=ff, fill=(214, 214, 218))
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 4) * S, (KY - KR + 7) * S, (x + KR + 4) * S, (KY + KR + 7) * S], fill=160)
        sh = sh.filter(ImageFilter.GaussianBlur(6 * S)); im.paste((6, 6, 7), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(16, 16, 18))
        d.arc([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], 190, 350, fill=(92, 92, 98), width=3 * S)
        r2 = KR - 11; d.ellipse([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], fill=(30, 30, 33))
        d.arc([(x - r2) * S, (KY - r2) * S, (x + r2) * S, (KY + r2) * S], 200, 300, fill=(74, 74, 80), width=2 * S)
    # POWER: Lampe und Kippschalter (nur Zierde)
    x, y, r = POWER['lamp']; ctext((x + POWER['switch'][0]) / 2, 60, 'POWER', 13)
    d.ellipse([(x - r - 4) * S, (y - r - 4) * S, (x + r + 4) * S, (y + r + 4) * S], fill=(70, 72, 76)); d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=(230, 60, 40))
    x, y = POWER['switch']; ctext(x, y - 46, 'ON', 10, FN, (200, 202, 206)); ctext(x, y + 32, 'OFF', 10, FN, (200, 202, 206))
    for rr, c in [(17, (70, 72, 76)), (13, (200, 203, 210)), (8, (150, 152, 158))]:
        d.ellipse([(x - rr) * S, (y - rr) * S, (x + rr) * S, (y + rr) * S], fill=c)
    d.polygon([((x - 5) * S, y * S), ((x - 6) * S, (y - 30) * S), ((x + 6) * S, (y - 30) * S), ((x + 5) * S, y * S)], fill=(216, 218, 223), outline=(74, 77, 82))
    # Schrauben
    for x in (18, W - 18):
        for y in (22, H - 34):
            d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(120, 122, 126)); d.line([((x - 4) * S, y * S), ((x + 4) * S, y * S)], fill=(60, 60, 64), width=2 * S)
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'g100panel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR, 'label': lab, 'pull': pull, 'scale': sc} for k, x, lab, pull, sc in KNOBS},
           'pully': PULLY, 'jacks': {k: {'x': x, 'y': y, 'r': r} for k, x, y, r in JACKS}, 'button': {'x': BTN[1], 'y': BTN[2], 'h': BTN[3]}, 'led': {'x': LED[1], 'y': LED[2]}}
    json.dump(geo, open(os.path.join(GEN, 'g100geo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'g100panel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/g100_panel.py: gezeichnete Frontplatte Lametta G100-212II (JPEG)\nstatic const unsigned char G100PANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int G100PANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'g100geo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/g100_panel.py: Lage der Bedienelemente auf g100panel.jpg (Bildpunkte)\n')
        f.write('static const int G100W = %d, G100H = %d;\n' % (W, H))
        f.write('static const int G100NKNOB = %d;\nstatic const double G100KNOB[%d][3] = {%s};\n' % (len(KNOBS), len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _, _, _ in KNOBS)))
        f.write('static const int G100PULL[%d] = {%s};   // 1 = Zugschalter (Beschriftung bei y = G100PULLY)\nstatic const double G100PULLY = %d;\n' % (len(KNOBS), ', '.join('1' if p else '0' for _, _, _, p, _ in KNOBS), PULLY))
        f.write('static const double G100JACK[2][3] = {%s};   // HIGH, LOW\n' % ', '.join('{%d, %d, %d}' % (x, y, r) for _, x, y, r in JACKS))
        f.write('static const double G100BTN[3] = {%d, %d, %d};   // A/B-Taste: Mitte, halbe Kantenlänge\nstatic const double G100LED[2] = {%d, %d};\n' % (BTN[1], BTN[2], BTN[3], LED[1], LED[2]))
    print('g100panel.jpg', len(data), 'Bytes')
    # Seitensymbole: quadratischer Ausschnitt um VOLUME, in 4-facher Auflösung gezeichnet
    S4 = 4; big = draw(S4); x0 = KNOBS[0][1] - 80; crop = big.crop((x0 * S4, 52 * S4, (x0 + 168) * S4, 220 * S4))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'lametta-g100-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), (24, 24, 25)); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'lametta-g100-maskable-512.png'), optimize=True)
if __name__ == '__main__': main()
