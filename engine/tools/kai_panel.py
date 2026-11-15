#!/usr/bin/env python3
# Zeichnet die Frontplatte des Kaiserin Reverb (Hall nach den veröffentlichten Angaben zum Empress Effects Reverb). Eigene Gestaltung:
# kein Foto, kein Logo, keine Nachbildung des Empress-Gehäuses; Bedienelemente wie in der Empress-Anleitung beschrieben (DECAY, MIX, OUTPUT,
# LOW, HI, THING 1, THING 2, Moduswahl, Halten am Fußschalter SELECT), Moduswahl hier als sechs Tasten.
# Schreibt
#   engine/gen/kaipanel.jpg   Bild 2000 × 240 (Knöpfe ohne Zeiger; Zeiger, Tasten, Anzeigen und die Bedeutung von THING 1/2 zeichnen Seite und Plugin)
#   engine/gen/kaigeo.json    Lage von Knöpfen, Tasten und Anzeigen in Bildpunkten
#   engine/gen/kaipanel_jpg.h, engine/gen/kaigeo_gen.h   für das Plugin
#   web/tonestack/assets/icons/kaiserin-reverb-{180,192,512,maskable-512}.png   Ausschnitt als Seitensymbol
#   python3 engine/tools/kai_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2000, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
KY = 128; KR = 32; KC = 22
PANEL = (52, 30, 48); INK = (236, 222, 196); DIM = (178, 160, 150); GOLD = (214, 178, 102)
# (Schlüssel, x, Beschriftung, links, rechts)
KNOBS = [('decay', 640, 'DECAY', 'short', 'long'), ('mix', 760, 'MIX', 'dry', 'wet'), ('out', 880, 'OUTPUT', '', ''),
         ('low', 1050, 'LOW', '', ''), ('hi', 1170, 'HI', '', ''),
         ('t1', 1340, 'THING 1', '', ''), ('t2', 1460, 'THING 2', '', '')]
SECT = [('MODE', 196, 540), ('REVERB', 580, 940), ('TONE', 990, 1230), ('THINGS', 1280, 1520)]
MODES = ['HALL', 'PLATE', 'SPRING', 'ROOM', 'MOD', 'SPARKLE']
BW, BH = 92, 34
BTNS = [('m%d' % i, 254 + 116 * (i % 3), 102 + 76 * (i // 3), MODES[i]) for i in range(6)] + [('hold', 1640, 150, 'HOLD')]   # Reihenfolge = Tastennummern
LEDS = [(254 + 116 * (i % 3) - BW / 2 + 10, 102 + 76 * (i // 3) - BH / 2 - 11) for i in range(6)] + [(1640, 100)]
def font(px, f=FB, S=S): return ImageFont.truetype(f, int(round(px * S)))
def draw(S):
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):   # Lack mit leichtem Verlauf
        t = y / (H * S); v = int(10 * (0.5 - abs(t - 0.4))); d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v // 2, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(24, 12, 22), width=5 * S)
    d.rectangle([12 * S, 12 * S, (W - 12) * S, (H - 12) * S], outline=GOLD, width=max(1, int(1.2 * S)))
    for x in (28, W - 28):
        for y in (28, H - 28):
            d.ellipse([(x - 7) * S, (y - 7) * S, (x + 7) * S, (y + 7) * S], fill=(168, 160, 150)); d.line([((x - 5) * S, y * S), ((x + 5) * S, y * S)], fill=(80, 74, 70), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Name links
    ctext(108, 70, 'KAISERIN', 24, FB, GOLD); ctext(108, 100, 'REVERB', 17, FB, INK)
    ctext(108, 136, 'after the published', 9.5, FN, DIM); ctext(108, 149, 'specs of the Empress', 9.5, FN, DIM); ctext(108, 162, 'Effects Reverb', 9.5, FN, DIM)
    ctext(108, 184, 'own design', 9.5, FN, DIM)
    for t, x0, x1 in SECT:
        ctext((x0 + x1) / 2, 22, t, 13, FB, GOLD); d.line([(x0 * S, 42 * S), (x1 * S, 42 * S)], fill=(140, 110, 90), width=S)
    for x in (560, 965, 1255, 1545): d.line([(x * S, 24 * S), (x * S, (H - 24) * S)], fill=(30, 16, 28), width=3 * S); d.line([((x + 2) * S, 24 * S), ((x + 2) * S, (H - 24) * S)], fill=(98, 70, 90), width=S)
    # Tasten (Ruhezustand)
    for k, x, y, t in BTNS:
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).rounded_rectangle([(x - BW / 2 + 2) * S, (y - BH / 2 + 4) * S, (x + BW / 2 + 2) * S, (y + BH / 2 + 4) * S], radius=5 * S, fill=180)
        sh = sh.filter(ImageFilter.GaussianBlur(3 * S)); im.paste((18, 10, 16), mask=sh); d = ImageDraw.Draw(im)
        d.rounded_rectangle([(x - BW / 2) * S, (y - BH / 2) * S, (x + BW / 2) * S, (y + BH / 2) * S], radius=5 * S, fill=(206, 198, 186), outline=(30, 20, 26), width=S)
        d.rounded_rectangle([(x - BW / 2 + 3) * S, (y - BH / 2 + 3) * S, (x + BW / 2 - 3) * S, (y + BH / 2 - 6) * S], radius=4 * S, fill=(226, 220, 210))
        ff = font(12.5, FB, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, (y - 9) * S), t, font=ff, fill=(44, 30, 40))
    for x, y in LEDS:
        d.ellipse([(x - 7) * S, (y - 7) * S, (x + 7) * S, (y + 7) * S], fill=(20, 12, 18)); d.ellipse([(x - 5) * S, (y - 5) * S, (x + 5) * S, (y + 5) * S], fill=(70, 40, 30))
    ctext(1640, 42, 'HOLD', 13, FB, GOLD); ctext(1640, 180, 'infinite hold', 9.5, FN, DIM)
    ctext(1640, 194, '(footswitch SELECT)', 9.5, FN, DIM)
    ctext(1840, 96, 'MONO IN', 12, FB, DIM); ctext(1840, 114, 'STEREO OUT', 12, FB, DIM)
    ctext(1840, 140, 'all knobs at 12 o’clock', 9.5, FN, DIM); ctext(1840, 153, 'sound good', 9.5, FN, DIM)
    # Knöpfe mit Skala 0 … 10
    for k, x, lab, lft, rgt in KNOBS:
        ctext(x, 52, lab, 13, FB)
        for j in range(11):
            a = (-150 + 30 * j) * math.pi / 180; r1, r2 = KR + 5, KR + (11 if j % 5 == 0 else 8)
            d.line([((x + r1 * math.sin(a)) * S, (KY - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (KY - r2 * math.cos(a)) * S)], fill=INK, width=max(1, int(1.5 * S)))
        ff = font(11, FB, S)
        for j, t in [(0, lft or '0'), (10, rgt or '10')]:
            a = (-150 + 30 * j) * math.pi / 180; tx, ty = x + (KR + 22) * math.sin(a), KY - (KR + 22) * math.cos(a); tw = d.textlength(t, font=ff); d.text((tx * S - tw / 2, (ty - 2) * S), t, font=ff, fill=DIM)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 3) * S, (KY - KR + 6) * S, (x + KR + 3) * S, (KY + KR + 6) * S], fill=200)
        sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((16, 8, 14), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(22, 20, 22))
        for j in range(40):
            a = j * math.pi / 20; c = 62 if math.sin(a - 2.4) > 0 else 38
            d.line([((x + (KC + 1) * math.cos(a)) * S, (KY + (KC + 1) * math.sin(a)) * S), ((x + (KR - 1) * math.cos(a)) * S, (KY + (KR - 1) * math.sin(a)) * S)], fill=(c, c, c + 2), width=max(1, int(1.4 * S)))
        d.ellipse([(x - KC - 1) * S, (KY - KC - 1) * S, (x + KC + 1) * S, (KY + KC + 1) * S], fill=(12, 10, 12))
        for i in range(KC * S, 0, -1):   # Kappe aus hellem Metall
            t = i / (KC * S); f = 0.80 + 0.20 * (1 - t) ** 0.5; ox, oy = -0.25 * (KC - i / S), -0.3 * (KC - i / S)
            d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=tuple(min(255, int(c * f)) for c in (210, 200, 186)))
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'kaipanel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': KY, 'r': KR} for k, x, *_ in KNOBS], 'buttons': [{'k': k, 'x': x, 'y': y, 'w': BW, 'h': BH} for k, x, y, _ in BTNS],
           'leds': [{'x': x, 'y': y} for x, y in LEDS]}
    json.dump(geo, open(os.path.join(GEN, 'kaigeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'kaipanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/kai_panel.py: gezeichnete Frontplatte Kaiserin Reverb (JPEG)\nstatic const unsigned char KAIPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int KAIPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'kaigeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/kai_panel.py: Lage der Bedienelemente auf kaipanel.jpg (Bildpunkte)\n')
        f.write('static const int KAIW = %d, KAIH = %d;\n' % (W, H))
        f.write('static const double KAIKNOB[%d][3] = {%s};   // DECAY, MIX, OUTPUT, LOW, HI, THING 1, THING 2\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, *_ in KNOBS)))
        f.write('static const double KAIBTN[%d][4] = {%s};   // Tasten HALL, PLATE, SPRING, ROOM, MOD, SPARKLE, HOLD (Mitte x, y, Breite, Höhe)\n' % (len(BTNS), ', '.join('{%d, %d, %d, %d}' % (x, y, BW, BH) for _, x, y, _ in BTNS)))
        f.write('static const double KAILED[%d][2] = {%s};   // Anzeigen der Modi und HOLD\n' % (len(LEDS), ', '.join('{%g, %g}' % (x, y) for x, y in LEDS)))
    crop = im.crop((20, 20, 220, 220)).resize((200, 200))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'kaiserin-reverb-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'kaiserin-reverb-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
