#!/usr/bin/env python3
# Zeichnet die Frontplatte des Wien-EQ (5-Band-Stereo-Equalizer nach den veröffentlichten Angaben zum adt-audio TM205). Eigene Gestaltung:
# kein Foto, kein Logo, keine Nachbildung der ADT-Frontplatte; Bedienelemente wie in der ADT-Beschreibung (LOW mit zwei Tasten 60 / 100 Hz,
# beide = 160 Hz, keine = 40 Hz; MID 1/2/3 GAIN, Q, FREQ; HIGH mit Tasten 12k / 8k, beide = 4 kHz, keine = Kuhschwanz; EQ ON mit Anzeige).
# Schreibt
#   engine/gen/weqpanel.jpg   Bild 2000 × 240 (Knöpfe ohne Zeiger; Zeiger, Tasten und Anzeige zeichnen Seite und Plugin)
#   engine/gen/weqgeo.json    Lage von Knöpfen, Tasten und Anzeige in Bildpunkten
#   engine/gen/weqpanel_jpg.h, engine/gen/weqgeo_gen.h   für das Plugin
#   web/tonestack/assets/icons/wien-eq-{180,192,512,maskable-512}.png   Ausschnitt als Seitensymbol
#   python3 engine/tools/weq_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2000, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
KY = 140; KR = 30; KC = 21
PANEL = (44, 50, 60); INK = (232, 234, 238); DIM = (150, 156, 166)
CAP = {'low': (232, 150, 72), 'm1': (110, 196, 132), 'm2': (98, 160, 230), 'm3': (176, 140, 220), 'high': (236, 206, 92)}
# (Schlüssel, x, Beschriftung, Skala, Farbe): Skala 'g' = − 0 +, 'q' = Bandbreite, ('f', lo, hi, Marken) = Frequenz logarithmisch
KNOBS = [('lowg', 228, 'GAIN', 'g', 'low'),
         ('m1g', 452, 'GAIN', 'g', 'm1'), ('m1q', 552, 'Q', 'q', 'm1'), ('m1f', 656, 'FREQ', ('f', 20, 2000, [20, 50, 100, 200, 500, 1000, 2000]), 'm1'),
         ('m2g', 812, 'GAIN', 'g', 'm2'), ('m2q', 912, 'Q', 'q', 'm2'), ('m2f', 1016, 'FREQ', ('f', 60, 7000, [60, 100, 200, 500, 1000, 2000, 7000]), 'm2'),
         ('m3g', 1172, 'GAIN', 'g', 'm3'), ('m3q', 1272, 'Q', 'q', 'm3'), ('m3f', 1376, 'FREQ', ('f', 250, 25000, [250, 500, 1000, 2000, 5000, 10000, 25000]), 'm3'),
         ('highg', 1536, 'GAIN', 'g', 'high')]
SECT = [('LOW', 160, 372), ('MID 1', 388, 724), ('MID 2', 748, 1084), ('MID 3', 1108, 1444), ('HIGH', 1468, 1680)]
BTNS = [('low60', 322, 108, '60'), ('low100', 322, 166, '100'), ('high12', 1630, 108, '12k'), ('high8', 1630, 166, '8k'), ('on', 1772, 166, 'EQ ON')]
BW, BH = 48, 28
LED = (1772, 104)
def fmt(f): return ('%gk' % (f / 1000)) if f >= 1000 else '%g' % f
def font(px, f=FB, S=S): return ImageFont.truetype(f, int(round(px * S)))
def draw(S):
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):   # eloxiert, feiner Strich
        v = int(1.5 * math.sin(y * 0.7) * math.sin(y * 0.11)); d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(22, 25, 30), width=5 * S)
    for x in (18, W - 18):   # Befestigungsschrauben
        for y in (22, H - 22):
            d.ellipse([(x - 7) * S, (y - 7) * S, (x + 7) * S, (y + 7) * S], fill=(150, 154, 160)); d.line([((x - 5) * S, y * S), ((x + 5) * S, y * S)], fill=(70, 74, 80), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Typbezeichnung links (normale Schrift)
    ctext(88, 82, 'WIEN EQ', 21, FB); ctext(88, 112, '5-band stereo', 11, FN, DIM); ctext(88, 128, 'Wien-Robinson bridge', 11, FN, DIM); ctext(88, 154, 'after the published', 9.5, FN, DIM); ctext(88, 167, 'specs of adt-audio TM205', 9.5, FN, DIM)
    # Abschnitte mit Überschrift und Trennlinien
    for t, x0, x1 in SECT:
        ctext((x0 + x1) / 2, 18, t, 14, FB); d.line([(x0 * S, 40 * S), (x1 * S, 40 * S)], fill=(120, 126, 136), width=S)
    for x in (380, 736, 1096, 1456, 1692): d.line([(x * S, 20 * S), (x * S, (H - 20) * S)], fill=(28, 32, 38), width=3 * S); d.line([((x + 2) * S, 20 * S), ((x + 2) * S, (H - 20) * S)], fill=(84, 90, 100), width=S)
    ctext(322, 196, 'none = 40 Hz', 9.5, FN, DIM); ctext(322, 209, 'both = 160 Hz', 9.5, FN, DIM)
    ctext(1630, 196, 'none = shelf', 9.5, FN, DIM); ctext(1630, 209, 'both = 4 kHz', 9.5, FN, DIM)
    # Tasten (Ruhezustand; gedrückt/leuchtend zeichnen Seite und Plugin)
    for k, x, y, t in BTNS:
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).rounded_rectangle([(x - BW / 2 + 2) * S, (y - BH / 2 + 4) * S, (x + BW / 2 + 2) * S, (y + BH / 2 + 4) * S], radius=4 * S, fill=180)
        sh = sh.filter(ImageFilter.GaussianBlur(3 * S)); im.paste((16, 18, 22), mask=sh); d = ImageDraw.Draw(im)
        d.rounded_rectangle([(x - BW / 2) * S, (y - BH / 2) * S, (x + BW / 2) * S, (y + BH / 2) * S], radius=4 * S, fill=(196, 200, 206), outline=(30, 32, 36), width=S)
        d.rounded_rectangle([(x - BW / 2 + 3) * S, (y - BH / 2 + 3) * S, (x + BW / 2 - 3) * S, (y + BH / 2 - 6) * S], radius=3 * S, fill=(216, 220, 226))
        if k != 'on': ctext(x, y - BH / 2 - 16, t, 11, FB)   # EQ ON: Beschriftung über der Anzeige
    ctext(1772, 52, 'EQ ON', 12, FB)
    x, y = LED; d.ellipse([(x - 10) * S, (y - 10) * S, (x + 10) * S, (y + 10) * S], fill=(18, 20, 24)); d.ellipse([(x - 7) * S, (y - 7) * S, (x + 7) * S, (y + 7) * S], fill=(40, 70, 40))
    ctext(1900, 96, 'STEREO', 12, FB, DIM); ctext(1900, 114, 'one control set', 9.5, FN, DIM); ctext(1900, 127, 'for both channels', 9.5, FN, DIM)
    # Knöpfe
    for k, x, lab, scale, col in KNOBS:
        ctext(x, 52, lab, 13, FB)
        if scale == 'g': marks = [(j, '0' if j == 5 else '') for j in range(11)]
        elif scale == 'q': marks = [(j, '') for j in range(11)]
        else:
            _, lo, hi, fl = scale; marks = [(10 * math.log(f / lo) / math.log(hi / lo), fmt(f)) for f in fl]
        for j, t in marks:
            a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 6) * math.sin(a), KY - (KR + 6) * math.cos(a)
            d.ellipse([(px - 1.6) * S, (py - 1.6) * S, (px + 1.6) * S, (py + 1.6) * S], fill=INK)
            if t:
                tx, ty = x + (KR + 16) * math.sin(a), KY - (KR + 16) * math.cos(a); ff = font(8.5, FN, S); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 5.5 * S), t, font=ff, fill=INK)
        if scale == 'g':
            ff = font(13, FB, S); d.text(((x - KR - 16) * S, (KY + KR) * S), '−', font=ff, fill=INK); d.text(((x + KR + 8) * S, (KY + KR) * S), '+', font=ff, fill=INK)
        if scale == 'q':
            ff = font(8.5, FN, S)
            for j, t in [(0, '5 oct'), (10, '1/3')]:
                a = (-150 + 30 * j) * math.pi / 180; tx, ty = x + (KR + 18) * math.sin(a), KY - (KR + 18) * math.cos(a); tw = d.textlength(t, font=ff); d.text((tx * S - tw / 2, ty * S - 5.5 * S), t, font=ff, fill=INK)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 3) * S, (KY - KR + 6) * S, (x + KR + 3) * S, (KY + KR + 6) * S], fill=200)
        sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((14, 16, 20), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(20, 21, 24))
        for j in range(40):
            a = j * math.pi / 20; c = 58 if math.sin(a - 2.4) > 0 else 36
            d.line([((x + (KC + 1) * math.cos(a)) * S, (KY + (KC + 1) * math.sin(a)) * S), ((x + (KR - 1) * math.cos(a)) * S, (KY + (KR - 1) * math.sin(a)) * S)], fill=(c, c, c + 3), width=max(1, int(1.4 * S)))
        d.arc([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], 190, 350, fill=(96, 98, 104), width=2 * S)
        cap = CAP[col]; d.ellipse([(x - KC - 1) * S, (KY - KC - 1) * S, (x + KC + 1) * S, (KY + KC + 1) * S], fill=(12, 12, 14))
        for i in range(KC * S, 0, -1):
            t = i / (KC * S); f = 0.86 + 0.14 * (1 - t) ** 0.5; ox, oy = -0.25 * (KC - i / S), -0.3 * (KC - i / S)
            d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=tuple(min(255, int(c * f)) for c in cap))
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'weqpanel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': KY, 'r': KR} for k, x, *_ in KNOBS], 'buttons': [{'k': k, 'x': x, 'y': y, 'w': BW, 'h': BH} for k, x, y, _ in BTNS], 'led': {'x': LED[0], 'y': LED[1]}}
    json.dump(geo, open(os.path.join(GEN, 'weqgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'weqpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/weq_panel.py: gezeichnete Frontplatte Wien-EQ (JPEG)\nstatic const unsigned char WEQPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int WEQPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'weqgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/weq_panel.py: Lage der Bedienelemente auf weqpanel.jpg (Bildpunkte)\n')
        f.write('static const int WEQW = %d, WEQH = %d;\n' % (W, H))
        f.write('static const double WEQKNOB[%d][3] = {%s};   // LOW GAIN, MID 1 GAIN/Q/FREQ, MID 2 …, MID 3 …, HIGH GAIN\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, *_ in KNOBS)))
        f.write('static const double WEQBTN[%d][4] = {%s};   // Tasten LOW 60, LOW 100, HIGH 12k, HIGH 8k, EQ ON (Mitte x, y, Breite, Höhe)\n' % (len(BTNS), ', '.join('{%d, %d, %d, %d}' % (x, y, BW, BH) for _, x, y, _ in BTNS)))
        f.write('static const double WEQLED[2] = {%d, %d};   // Anzeige EQ ON\n' % LED)
    crop = im.crop((520, 30, 720, 230)).resize((200, 200))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'wien-eq-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'wien-eq-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
