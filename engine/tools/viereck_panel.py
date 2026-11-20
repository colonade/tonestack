#!/usr/bin/env python3
# Zeichnet die Frontplatte des Viereck 34 (Klangteil nach dem Schaltplan des Quad 34: BASS Step/Lift, TILT, BALANCE, Filter F1/F2/SLOPE).
# Eigene Gestaltung: kein Foto, kein Logo, keine Nachbildung der Quad-Frontplatte. Rechts neben einer Trennlinie der Modellteil:
# Drehpunkt des Tilt (TILT PIVOT, goldener Strich an der Stellung des Originals) und Kippschalter STEPS/SMOOTH (Rasten wie Original oder stufenlos).
# Schreibt engine/gen/viereckpanel.jpg (1600 × 240, ohne Zeiger, Leuchten und Hebel), viereckgeo.json, viereckpanel_jpg.h, viereckgeo_gen.h
# und die Seitensymbole web/tonestack/assets/icons/viereck-34-*.png
#   python3 engine/tools/viereck_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 1600, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
PANEL = (184, 180, 170); INK = (40, 38, 34); DIM = (92, 88, 80); GOLD = (168, 124, 40)
KY = 132; KR = 30
PIV0, PLO, PHI = 847.13, 80.0, 4000.0   # Drehpunkt des Originals (viereck::pivot0) und Bereich des Modellreglers (logarithmisch)
# (Schlüssel, x, Beschriftung, Art): 'bass'/'tilt' 7 Rasten über ±135°, 'bal' und 'piv' stufenlos über ±150°
KNOBS = [('bass', 380, 'BASS', 'bass'), ('tilt', 580, 'TILT', 'tilt'), ('bal', 770, 'BALANCE', 'bal'), ('piv', 1250, 'TILT PIVOT', 'piv')]   # Reihenfolge = Knöpfe der Oberfläche
BTNS = [('f1', 920, 160, 'F1'), ('f2', 1000, 160, 'F2'), ('sl', 1080, 160, 'SLOPE')]   # Tasten (Mitte), Leuchten darüber
BW, BH = 52, 30; LEDY = 112
MODE = (1400, 132)   # Kippschalter STEPS (oben, Original) / SMOOTH (unten)
def det(i): return (-135 + 45 * i) * math.pi / 180   # Rasten 0 … 6
def pivpos(f): return math.log(f / PLO) / math.log(PHI / PLO)   # 0 … 1
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def draw():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for x in range(0, W * S, 1):   # gebürstet, leicht ungleichmäßig
        v = int(3 * math.sin(x * 0.037) * math.sin(x * 0.0071) + 2 * math.sin(x * 0.19))
        d.line([(x, 0), (x, H * S)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(90, 86, 78), width=4 * S)
    d.line([(0, 30 * S), (W * S, 30 * S)], fill=(150, 146, 136), width=S); d.line([(0, (H - 30) * S), (W * S, (H - 30) * S)], fill=(150, 146, 136), width=S)
    for x in (16, W - 16):
        for y in (16, H - 16):
            d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(128, 124, 116), outline=(80, 76, 70), width=S); d.line([((x - 4) * S, y * S), ((x + 4) * S, y * S)], fill=(70, 66, 60), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    ctext(150, 70, 'VIERECK 34', 27, FB); ctext(150, 110, 'tone section · tilt', 11.5, FN, DIM)
    ctext(150, 132, 'circuit model after the', 10.5, FN, DIM); ctext(150, 147, 'Quad 34 preamplifier', 10.5, FN, DIM); ctext(150, 176, 'own design', 10, FN, DIM)
    for x in (268, 1150): d.line([(x * S, 44 * S), (x * S, (H - 44) * S)], fill=(120, 116, 106), width=2 * S)
    d.line([(1150 * S, 44 * S), (1150 * S, (H - 44) * S)], fill=GOLD, width=2 * S)
    # Knöpfe
    for k, x, lab, kind in KNOBS:
        ctext(x, 48, lab, 13, FB)
        ff = font(11.5, FB); fn = font(10.5, FN)
        def mark(a, r1, r2, col=INK, w=1.6):
            d.line([((x + r1 * math.sin(a)) * S, (KY - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (KY - r2 * math.cos(a)) * S)], fill=col, width=max(1, int(w * S)))
        def label(a, t, rr, f=ff, col=INK):   # Abstand nach Textbreite (seitlich) und Schrifthöhe (oben/unten)
            tw = d.textlength(t, font=f); rr = KR + 14 + 0.5 * tw / S * abs(math.sin(a)) + 6 * abs(math.cos(a)); tx, ty = x + rr * math.sin(a), KY - rr * math.cos(a); d.text((tx * S - tw / 2, (ty - 7) * S), t, font=f, fill=col)
        if kind == 'bass':
            for i, t in enumerate(['300', '170', '100', '0', '3', '6', '9']): mark(det(i), KR + 4, KR + 9); label(det(i), t, KR + 20)
            ctext(x - 50, 196, 'STEP Hz', 10, FN, DIM); ctext(x + 50, 196, 'LIFT dB', 10, FN, DIM)
        elif kind == 'tilt':
            for i, t in enumerate(['3', '2', '1', '0', '1', '2', '3']): mark(det(i), KR + 4, KR + 9); label(det(i), t, KR + 18)
            ctext(x - 52, 196, 'bright', 10, FN, DIM); ctext(x + 52, 196, 'warm', 10, FN, DIM)
        elif kind == 'bal':
            for j in range(11): a = (-150 + 30 * j) * math.pi / 180; mark(a, KR + 4, KR + (10 if j in (0, 5, 10) else 7))
            label((-150) * math.pi / 180, 'L', KR + 19); label(150 * math.pi / 180, 'R', KR + 19)
        else:
            for f in (200, 500, 1000, 2000, 4000):
                a = (-150 + 300 * pivpos(f)) * math.pi / 180; mark(a, KR + 4, KR + 9); label(a, ('%gk' % (f / 1000)) if f >= 1000 else '%g' % f, KR + 20, fn)
            for f in (80,): a = (-150 + 300 * pivpos(f)) * math.pi / 180; mark(a, KR + 4, KR + 9); label(a, '80', KR + 20, fn)
            a = (-150 + 300 * pivpos(PIV0)) * math.pi / 180; mark(a, KR + 3, KR + 12, GOLD, 2.6)   # Stellung des Originals
            ctext(x, 196, 'Hz · gold = original', 10, FN, DIM)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 3) * S, (KY - KR + 6) * S, (x + KR + 3) * S, (KY + KR + 6) * S], fill=190)
        sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((96, 92, 84), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(44, 40, 36))
        for i in range(int(KR * S), 0, -1):   # matte, dunkle Kappe mit Glanz oben links
            t = i / (KR * S); f = 0.75 + 0.35 * (1 - t) ** 0.6; ox, oy = -0.18 * (KR - i / S), -0.22 * (KR - i / S)
            c = tuple(min(255, int(v * f)) for v in (62, 58, 52)); d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=c)
        d.arc([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], 200, 340, fill=(110, 104, 96), width=2 * S)
    # Filtertasten und Leuchten
    ctext(1000, 48, 'FILTER', 13, FB)
    for k, x, y, t in BTNS:
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).rounded_rectangle([(x - BW / 2 + 2) * S, (y - BH / 2 + 4) * S, (x + BW / 2 + 2) * S, (y + BH / 2 + 4) * S], radius=4 * S, fill=170)
        sh = sh.filter(ImageFilter.GaussianBlur(3 * S)); im.paste((110, 106, 98), mask=sh); d = ImageDraw.Draw(im)
        d.rounded_rectangle([(x - BW / 2) * S, (y - BH / 2) * S, (x + BW / 2) * S, (y + BH / 2) * S], radius=4 * S, fill=(70, 66, 60), outline=(40, 38, 34), width=S)
        d.rounded_rectangle([(x - BW / 2 + 3) * S, (y - BH / 2 + 3) * S, (x + BW / 2 - 3) * S, (y + BH / 2 - 6) * S], radius=3 * S, fill=(92, 88, 80))
        ctext(x, 78, t, 12, FB)
        d.ellipse([(x - 7) * S, (LEDY - 7) * S, (x + 7) * S, (LEDY + 7) * S], fill=(60, 56, 50)); d.ellipse([(x - 5) * S, (LEDY - 5) * S, (x + 5) * S, (LEDY + 5) * S], fill=(80, 40, 30))
    ctext(960, 196, '6 dB/oct · SLOPE 12 dB/oct', 10, FN, DIM)
    mx, my = MODE; ctext(mx, 48, 'TILT', 13, FB); ctext(mx, 78, 'steps', 11, FN, DIM); ctext(mx, 178, 'smooth', 11, FN, DIM)
    ctext(1520, 104, 'STEREO', 12, FB, DIM); ctext(1520, 124, 'one control set', 9.5, FN, DIM); ctext(1520, 137, 'for both channels', 9.5, FN, DIM)
    return im
def main():
    im = draw().resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'viereckpanel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': KY, 'r': KR} for k, x, *_ in KNOBS], 'buttons': [{'k': k, 'x': x, 'y': y, 'w': BW, 'h': BH} for k, x, y, _ in BTNS], 'ledy': LEDY, 'mode': {'x': MODE[0], 'y': MODE[1]},
           'piv': {'lo': PLO, 'hi': PHI, 'orig': PIV0}}
    json.dump(geo, open(os.path.join(GEN, 'viereckgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'viereckpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/viereck_panel.py: gezeichnete Frontplatte Viereck 34 (JPEG)\nstatic const unsigned char VRPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int VRPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'viereckgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/viereck_panel.py: Lage der Bedienelemente auf viereckpanel.jpg (Bildpunkte)\n')
        f.write('static const int VRW = %d, VRH = %d;\n' % (W, H))
        f.write('static const double VRKNOB[%d][3] = {%s};   // BASS, TILT, BALANCE, TILT PIVOT (Mitte x, y, Radius)\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, *_ in KNOBS)))
        f.write('static const double VRBTN[%d][4] = {%s};   // Tasten F1, F2, SLOPE (Mitte x, y, Breite, Höhe); Leuchten bei y = %d\n' % (len(BTNS), ', '.join('{%d, %d, %d, %d}' % (x, y, BW, BH) for _, x, y, _ in BTNS), LEDY))
        f.write('static const double VRLEDY = %d;\nstatic const double VRMODE[2] = {%d, %d};   // Kippschalter STEPS/SMOOTH\n' % (LEDY, MODE[0], MODE[1]))
        f.write('static const double VRPIV[3] = {%g, %g, %g};   // TILT PIVOT: unten, oben (Hz, logarithmisch), Original\n' % (PLO, PHI, PIV0))
    crop = im.crop((480, 32, 680, 232))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'viereck-34-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'viereck-34-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
