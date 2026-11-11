#!/usr/bin/env python3
# Zeichnet die Frontplatte des AmpFett 76 (Nachbau des UREI 1176LN nach dem Schaltplan, mit Erweiterungen nach Origin Effects Cali76).
# Eigene Gestaltung: kein Foto, kein Logo, keine Nachbildung der UREI-Frontplatte; Bedienelemente wie am Gerät (INPUT, OUTPUT, ATTACK mit OFF,
# RELEASE, vier RATIO-Tasten 20/12/8/4, VU-Meter, METER-Tasten GR/+8/+4/OFF).
# Schreibt
#   engine/gen/u76panel.jpg   Bild 2000 × 320 (Knöpfe ohne Zeiger; Zeiger, Tasten und Nadel zeichnen Seite und Plugin)
#   engine/gen/u76geo.json    Lage der Bedienelemente in Bildpunkten
#   engine/gen/u76panel_jpg.h, engine/gen/u76geo_gen.h   für das Plugin
#   web/tonestack/assets/icons/ampfett-76-{180,192,512,maskable-512}.png   Ausschnitt als Seitensymbol
#   python3 engine/tools/u76_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2000, 320, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
PANEL = (28, 30, 34); INK = (236, 238, 242); DIM = (150, 156, 166)
# Knöpfe: (Schlüssel, x, y, Radius, Beschriftung, Skala) – Skala 'n' 0 … 10, 'a' OFF + 1 … 7 (ATTACK), 'r' 1 … 7 (RELEASE)
KNOBS = [('input', 420, 178, 64, 'INPUT', 'n'), ('output', 610, 178, 64, 'OUTPUT', 'n'), ('attack', 790, 178, 42, 'ATTACK', 'a'), ('release', 930, 178, 42, 'RELEASE', 'r')]
BW, BH = 64, 38
RATIO = [('r20', 1070, 82, '20'), ('r12', 1070, 136, '12'), ('r8', 1070, 190, '8'), ('r4', 1070, 244, '4')]
METER = [('mgr', 1640, 82, 'GR'), ('m8', 1640, 136, '+8'), ('m4', 1640, 190, '+4'), ('moff', 1640, 244, 'OFF')]
VU = (1170, 40, 1540, 286)   # Fenster des Instruments (x0, y0, x1, y1)
PIV = (1355, 330)            # Drehpunkt der Nadel (unter dem Fenster)
NR = 240                     # Nadellänge
VUMARKS = [-20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3]
def vuAng(db):   # VU-Meter: Ausschlag proportional zur Spannung; −20 … +3 über ±42°
    x = 10 ** (db / 20); return -42 + 84 * (x - 0.1) / (10 ** 0.15 - 0.1)
def font(px, f=FB, S=S): return ImageFont.truetype(f, int(round(px * S)))
def draw(S):
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):   # gebürstet, dunkel eloxiert
        v = int(2 * math.sin(y * 0.9) * math.sin(y * 0.07)); d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v + 1))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(10, 11, 13), width=5 * S)
    for x in (22, W - 22):   # Rackschrauben
        for y in (30, H - 30):
            d.ellipse([(x - 9) * S, (y - 9) * S, (x + 9) * S, (y + 9) * S], fill=(160, 164, 170)); d.line([((x - 6) * S, y * S), ((x + 6) * S, y * S)], fill=(70, 74, 80), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Typbezeichnung (eigene, normale Schrift)
    ctext(185, 92, 'AMPFETT 76', 30, FB); ctext(185, 136, 'LIMITING AMPLIFIER', 14, FB, DIM)
    ctext(185, 176, 'after the UREI 1176LN schematic', 11, FN, DIM); ctext(185, 192, 'FET gain reduction, class AB output', 11, FN, DIM)
    ctext(185, 226, 'Cali76 extensions: model strip', 10, FN, DIM)
    # Trennlinien
    for x in (330, 1000, 1140, 1570, 1710):
        d.line([(x * S, 26 * S), (x * S, (H - 26) * S)], fill=(12, 13, 15), width=3 * S); d.line([((x + 2) * S, 26 * S), ((x + 2) * S, (H - 26) * S)], fill=(70, 74, 82), width=S)
    ctext(1070, 30, 'RATIO', 14, FB); ctext(1640, 30, 'METER', 14, FB)
    # Tasten im Ruhezustand
    for k, x, y, t in RATIO + METER:
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).rounded_rectangle([(x - BW / 2 + 2) * S, (y - BH / 2 + 4) * S, (x + BW / 2 + 2) * S, (y + BH / 2 + 4) * S], radius=5 * S, fill=180)
        sh = sh.filter(ImageFilter.GaussianBlur(3 * S)); im.paste((8, 9, 11), mask=sh); d = ImageDraw.Draw(im)
        d.rounded_rectangle([(x - BW / 2) * S, (y - BH / 2) * S, (x + BW / 2) * S, (y + BH / 2) * S], radius=5 * S, fill=(200, 203, 208), outline=(20, 22, 26), width=S)
        d.rounded_rectangle([(x - BW / 2 + 3) * S, (y - BH / 2 + 3) * S, (x + BW / 2 - 3) * S, (y + BH / 2 - 7) * S], radius=4 * S, fill=(222, 225, 230))
        ff = font(15, FB, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, (y - 11) * S), t, font=ff, fill=(30, 32, 36))
    # VU-Meter: Fenster, Skala (schwarz auf cremefarbenem Grund), rotes Feld über 0
    x0, y0, x1, y1 = VU
    d.rounded_rectangle([(x0 - 8) * S, (y0 - 8) * S, (x1 + 8) * S, (y1 + 8) * S], radius=10 * S, fill=(12, 13, 15))
    face = Image.new('RGB', ((x1 - x0) * S, (y1 - y0) * S)); fd = ImageDraw.Draw(face)
    for yy in range((y1 - y0) * S):
        t = yy / ((y1 - y0) * S); c = (int(242 - 30 * t), int(226 - 32 * t), int(176 - 40 * t)); fd.line([(0, yy), ((x1 - x0) * S, yy)], fill=c)
    px, py = (PIV[0] - x0) * S, (PIV[1] - y0) * S
    def pt(a, r): a = math.radians(a); return (px + r * S * math.sin(a), py - r * S * math.cos(a))
    fd.arc([px - 210 * S, py - 210 * S, px + 210 * S, py + 210 * S], 270 + vuAng(-20), 270 + vuAng(0), fill=(30, 30, 30), width=2 * S)
    fd.arc([px - 210 * S, py - 210 * S, px + 210 * S, py + 210 * S], 270 + vuAng(0), 270 + vuAng(3), fill=(190, 30, 24), width=7 * S)
    for m in VUMARKS:
        a = vuAng(m); col = (190, 30, 24) if m > 0 else (24, 24, 24)
        fd.line([pt(a, 210), pt(a, 226)], fill=col, width=2 * S)
        t = ('+%d' % m) if m > 0 else ('%d' % m); ff = font(13 if m in (-20, -10, 0, 3) else 11, FB, S); tx, ty = pt(a, 244); tw = fd.textlength(t, font=ff)
        fd.text((tx - tw / 2, ty - 8 * S), t, font=ff, fill=col)
    for m in [-15, -6, -4]: a = vuAng(m); fd.line([pt(a, 210), pt(a, 218)], fill=(24, 24, 24), width=S)
    ff = font(22, FB, S); tw = fd.textlength('VU', font=ff); fd.text((px - tw / 2, py - 150 * S), 'VU', font=ff, fill=(30, 30, 30))
    ff = font(10, FN, S); t = 'GR: 0 = no gain reduction'; tw = fd.textlength(t, font=ff); fd.text((px - tw / 2, py - 118 * S), t, font=ff, fill=(70, 66, 56))
    im.paste(face, (x0 * S, y0 * S)); d = ImageDraw.Draw(im)
    d.rounded_rectangle([x0 * S, y0 * S, x1 * S, y1 * S], radius=6 * S, outline=(60, 62, 66), width=2 * S)
    ctext(1840, 110, 'SOLID STATE', 12, FB, DIM); ctext(1840, 128, 'LIMITING AMPLIFIER', 12, FB, DIM); ctext(1840, 168, 'own panel design', 9.5, FN, DIM); ctext(1840, 182, 'circuit after UREI 1176LN', 9.5, FN, DIM)
    # Knöpfe: gerändelte schwarze Rockform mit Metallkappe
    for k, x, y, r, lab, scale in KNOBS:
        ctext(x, y - r - 52, lab, 15, FB)
        if scale == 'n': marks = [(-150 + 30 * j, str(j) if j % 2 == 0 else '') for j in range(11)]
        elif scale == 'a': marks = [(-150, 'OFF')] + [(-120 + 45 * (j - 1), str(j)) for j in range(1, 8)]
        else: marks = [(-150 + 50 * (j - 1), str(j)) for j in range(1, 8)]
        for a0, t in marks:
            a = math.radians(a0); mx, my = x + (r + 8) * math.sin(a), y - (r + 8) * math.cos(a)
            d.ellipse([(mx - 2) * S, (my - 2) * S, (mx + 2) * S, (my + 2) * S], fill=INK)
            if t:
                tx, ty = x + (r + 24) * math.sin(a), y - (r + 24) * math.cos(a); ff = font(11 if t != 'OFF' else 9.5, FB, S); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 7 * S), t, font=ff, fill=INK)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - r + 4) * S, (y - r + 8) * S, (x + r + 4) * S, (y + r + 8) * S], fill=210)
        sh = sh.filter(ImageFilter.GaussianBlur(6 * S)); im.paste((4, 5, 6), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=(16, 16, 18))
        n = 48 if r > 50 else 36; kc = r * 0.62
        for j in range(n):
            a = j * 2 * math.pi / n; c = 52 if math.sin(a - 2.4) > 0 else 30
            d.line([((x + (kc + 2) * math.cos(a)) * S, (y + (kc + 2) * math.sin(a)) * S), ((x + (r - 1) * math.cos(a)) * S, (y + (r - 1) * math.sin(a)) * S)], fill=(c, c, c + 3), width=max(1, int(1.6 * S)))
        d.arc([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], 190, 350, fill=(90, 92, 98), width=2 * S)
        d.ellipse([(x - kc - 1) * S, (y - kc - 1) * S, (x + kc + 1) * S, (y + kc + 1) * S], fill=(10, 10, 12))
        for i in range(int(kc * S), 0, -1):
            t = i / (kc * S); f = 0.80 + 0.2 * (1 - t) ** 0.5; ox, oy = -0.25 * (kc - i / S), -0.3 * (kc - i / S)
            v = int(200 * f); d.ellipse([(x + ox) * S - i, (y + oy) * S - i, (x + ox) * S + i, (y + oy) * S + i], fill=(v, v, min(255, v + 6)))
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'u76panel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': y, 'r': r} for k, x, y, r, *_ in KNOBS], 'buttons': [{'k': k, 'x': x, 'y': y, 'w': BW, 'h': BH} for k, x, y, _ in RATIO + METER],
           'vu': {'x0': VU[0], 'y0': VU[1], 'x1': VU[2], 'y1': VU[3], 'px': PIV[0], 'py': PIV[1], 'r': NR}}
    json.dump(geo, open(os.path.join(GEN, 'u76geo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'u76panel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/u76_panel.py: gezeichnete Frontplatte AmpFett 76 (JPEG)\nstatic const unsigned char U76PANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int U76PANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'u76geo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/u76_panel.py: Lage der Bedienelemente auf u76panel.jpg (Bildpunkte)\n')
        f.write('static const int U76W = %d, U76H = %d;\n' % (W, H))
        f.write('static const double U76KNOB[%d][3] = {%s};   // INPUT, OUTPUT, ATTACK, RELEASE (x, y, Radius)\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, y, r) for _, x, y, r, *_ in KNOBS)))
        f.write('static const double U76BTN[%d][4] = {%s};   // RATIO 20, 12, 8, 4; METER GR, +8, +4, OFF (Mitte x, y, Breite, Höhe)\n' % (len(RATIO + METER), ', '.join('{%d, %d, %d, %d}' % (x, y, BW, BH) for _, x, y, _ in RATIO + METER)))
        f.write('static const double U76VU[7] = {%d, %d, %d, %d, %d, %d, %d};   // Fenster x0, y0, x1, y1; Drehpunkt x, y; Nadellänge\n' % (VU + PIV + (NR,)))
    crop = im.crop((330, 70, 690, 290)).resize((360, 220))
    sq = Image.new('RGB', (360, 360), PANEL); sq.paste(crop, (0, 70)); crop = sq
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'ampfett-76-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'ampfett-76-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
