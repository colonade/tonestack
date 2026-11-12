#!/usr/bin/env python3
# Zeichnet die Frontplatte des Lametta-B100-III-Simulators nach der Zeichnung auf Seite 2 des Service Manuals (B100-115III): INPUT; VOLUME,
# TREBLE, MIDDLE, BASS; PARAMETRIC EQ 1 (Anzeige, LEVEL, Q, FREQUENCY, PRESET LEVEL); PARAMETRIC EQ 2; CROSSOVER (LPF LEVEL, FREQUENCY,
# HPF LEVEL – im Simulator ohne Wirkung); PHONES, FOOT SW, POWER-Anzeige und -Taste. Ab Plugin 1.1 Farben wie am Gerät (nach einem Foto der
# Frontplatte, das der Nutzer geschickt hat; das Foto selbst ist nicht im Repository): schwarze Platte mit heller Schrift, schwarze Riffelknöpfe mit farbiger
# Kappe (grün: Volume/Klang, grau-violett: EQ 1, creme: EQ 2, rot: Frequenzweiche), rote Anzeigen. Kein Foto, kein Logo, nur die Typbezeichnung in
# normaler Schrift. Schreibt
#   engine/gen/b3panel.jpg   Bild 2000 × 240 (Knöpfe ohne Zeiger; Zeiger und Anzeigen zeichnen Seite und Plugin)
#   engine/gen/b3geo.json    Lage von Knöpfen, Anzeigen und Buchsen in Bildpunkten
#   engine/gen/b3panel_jpg.h, engine/gen/b3geo_gen.h   für das Plugin
#   web/tonestack/assets/icons/lametta-b100iii-{180,192,512,maskable-512}.png   Ausschnitt um VOLUME als Seitensymbol
#   python3 engine/tools/b3_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2000, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
KY = 140; KR = 27; KC = 19   # Knopf: Riffelrand KR, farbige Kappe KC
CAP = {'vol': (98, 204, 140), 'treb': (98, 204, 140), 'mid': (98, 204, 140), 'bass': (98, 204, 140),
       'lev1': (152, 150, 186), 'q1': (152, 150, 186), 'f1': (152, 150, 186), 'pre1': (152, 150, 186),
       'lev2': (244, 212, 150), 'q2': (244, 212, 150), 'f2': (244, 212, 150), 'pre2': (244, 212, 150),
       'lpf': (234, 104, 94), 'xf': (234, 104, 94), 'hpf': (234, 104, 94)}
PANEL = (34, 34, 36); INK = (226, 226, 220); DIM = (138, 138, 136)
# (Schlüssel, x, Beschriftung (Zeilen), Skala, aktiv): Skala 'n' = 0 … 10, 'lvl' = −5 … +5, 'q' = Symbole, 'f' = 50 Hz … 2.5K, 'xf' = 100 Hz … 5K
KNOBS = [('vol', 214, ['VOLUME'], 'n', 1), ('treb', 306, ['TREBLE'], 'lvl', 1), ('mid', 404, ['MIDDLE'], 'lvl', 1), ('bass', 500, ['BASS'], 'lvl', 1),
         ('lev1', 606, ['LEVEL'], 'lvl', 1), ('q1', 696, ['Q'], 'q', 1), ('f1', 794, ['FREQUENCY'], 'f', 1), ('pre1', 886, ['PRESET', 'LEVEL'], 'lvl', 1),
         ('lev2', 990, ['LEVEL'], 'lvl', 1), ('q2', 1082, ['Q'], 'q', 1), ('f2', 1176, ['FREQUENCY'], 'f', 1), ('pre2', 1272, ['PRESET', 'LEVEL'], 'lvl', 1),
         ('lpf', 1370, ['LPF', 'LEVEL'], 'n', 0), ('xf', 1468, ['FREQUENCY'], 'xf', 0), ('hpf', 1566, ['HPF', 'LEVEL'], 'n', 0)]
SEPS = [158, 556, 942, 1322, 1616]            # Trennlinien zwischen den Gruppen
LEDS = [('eq1', 574, 42), ('eq2', 958, 42)]   # Anzeigen PARAMETRIC EQ 1 / 2 (Schalter wie der Fußschalter)
JACKS = [('input', 86, 140, 19), ('phones', 1676, 88, 15), ('footsw', 1676, 176, 15)]
POWER = {'lamp': (1790, 140, 7), 'switch': (1880, 132, 22)}
def font(px, f=FB, S=S): return ImageFont.truetype(f, int(round(px * S)))
def draw(S):   # Platte in S-facher Auflösung (Bildpunkte der Geometrie × S)
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):   # schwarze Platte, feiner Strich
        v = PANEL[0] + int(2 * math.sin(y * 0.9) * math.sin(y * 0.13)); d.line([(0, y), (W * S, y)], fill=(v, v, v + 2))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(14, 14, 15), width=5 * S)
    d.rectangle([0, (H - 18) * S, W * S, H * S], fill=(18, 18, 19)); d.rectangle([0, (H - 24) * S, W * S, (H - 17) * S], fill=(176, 172, 164))   # Zierleiste
    d.line([(0, (H - 24) * S), (W * S, (H - 24) * S)], fill=(214, 212, 206), width=S)
    ink = INK; dim = DIM
    def ctext(x, y, t, px=13, f=FB, col=ink):
        ff = font(px, f, S); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    for x in SEPS: d.line([(x * S, 18 * S), (x * S, (H - 26) * S)], fill=(150, 150, 148), width=2 * S)
    # Klammern unter den Entzerrern und der Weiche
    for x0, x1, t, col in [(596, 806, 'PARAMETRIC EQ 1', ink), (980, 1188, 'PARAMETRIC EQ 2', ink), (1356, 1580, 'CROSSOVER', dim)]:
        y = H - 34; d.line([(x0 * S, (y - 8) * S), (x0 * S, y * S), ((x0 + x1) / 2 * S - 80 * S, y * S)], fill=col, width=2 * S)
        d.line([((x0 + x1) / 2 * S + 80 * S, y * S), (x1 * S, y * S), (x1 * S, (y - 8) * S)], fill=col, width=2 * S); ctext((x0 + x1) / 2, y - 8, t, 12, FB, col)
    d.line([(1396 * S, 38 * S), (1440 * S, 38 * S)], fill=dim, width=2 * S); d.line([(1496 * S, 38 * S), (1540 * S, 38 * S)], fill=dim, width=2 * S)
    # Buchsen
    for k, x, y, r in JACKS:
        if k == 'input': ctext(x, y - r - 30, 'INPUT', 12)
        elif k == 'phones': ctext(x, y - r - 22, 'PHONES', 11)
        else: ctext(x, y - r - 22, 'FOOT SW', 11)
        ring = [(r + 5, (70, 68, 64)), (r + 1, (196, 192, 184)), (r - 4, (122, 120, 116)), (r - 8, (10, 10, 11))] if k == 'input' else \
               [(r + 5, (20, 20, 21)), (r + 1, (44, 44, 46)), (r - 4, (26, 26, 28)), (r - 8, (8, 8, 9))]   # INPUT verchromt, PHONES/FOOT SW schwarze Muttern
        for rr, c in ring: d.ellipse([(x - rr) * S, (y - rr) * S, (x + rr) * S, (y + rr) * S], fill=c)
    # Anzeigen der Entzerrer (Fassung; das Leuchten zeichnen Seite/Plugin)
    for k, x, y in LEDS:
        d.ellipse([(x - 9) * S, (y - 9) * S, (x + 9) * S, (y + 9) * S], fill=(16, 16, 17)); d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(70, 20, 18))
    # Knöpfe
    for k, x, lab, scale, act in KNOBS:
        col = ink if act else dim
        for i, t in enumerate(lab): ctext(x, (44 if len(lab) == 1 else 30) + i * 16, t, 13, FB, col)
        if scale == 'n': marks = [(j, str(j)) for j in range(11)]
        elif scale == 'lvl': marks = [(j, str(abs(j - 5))) for j in range(11)]
        elif scale == 'f': marks = [(0, '50'), (2.9, '100'), (5, '250'), (7.1, '500'), (10, '2.5K')]
        elif scale == 'xf': marks = [(0, '100'), (2.8, '200'), (5, '500'), (7.2, '1K'), (10, '5K')]
        else: marks = [(j, '') for j in range(11)]
        for j, t in marks:
            a = (-150 + 30 * j) * math.pi / 180; px, py = x + (KR + 6) * math.sin(a), KY - (KR + 6) * math.cos(a)
            d.ellipse([(px - 1.5) * S, (py - 1.5) * S, (px + 1.5) * S, (py + 1.5) * S], fill=col)
            if t:
                tx, ty = x + (KR + 14) * math.sin(a), KY - (KR + 14) * math.cos(a); ff = font(7.5, FN, S); tw = d.textlength(t, font=ff)
                d.text((tx * S - tw / 2, ty * S - 5 * S), t, font=ff, fill=col)
        if scale == 'lvl':
            ff = font(10, FB, S); d.text(((x - KR - 14) * S, (KY + KR + 6) * S), '−', font=ff, fill=col); d.text(((x + KR + 8) * S, (KY + KR + 6) * S), '+', font=ff, fill=col)
        if scale == 'q':
            for j, pts in [(0, [(-9, 4), (-5, -3), (0, -4), (5, -3), (9, 4)]), (10, [(-7, 5), (0, -6), (7, 5)])]:
                a = (-150 + 30 * j) * math.pi / 180; cx, cy = x + (KR + 19) * math.sin(a), KY - (KR + 19) * math.cos(a)
                d.line([((cx + px_) * S, (cy + py_) * S) for px_, py_ in pts], fill=col, width=max(1, int(1.6 * S)), joint='curve')
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - KR + 3) * S, (KY - KR + 6) * S, (x + KR + 3) * S, (KY + KR + 6) * S], fill=200)
        sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((8, 8, 9), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], fill=(16, 16, 18))   # Riffelrand
        for j in range(36):
            a = j * math.pi / 18; c = 54 if math.sin(a - 2.4) > 0 else 34
            d.line([((x + (KC + 1) * math.cos(a)) * S, (KY + (KC + 1) * math.sin(a)) * S), ((x + (KR - 1) * math.cos(a)) * S, (KY + (KR - 1) * math.sin(a)) * S)], fill=(c, c, c + 2), width=max(1, int(1.4 * S)))
        d.arc([(x - KR) * S, (KY - KR) * S, (x + KR) * S, (KY + KR) * S], 190, 350, fill=(90, 90, 94), width=2 * S)
        cap = CAP[k] if act else tuple(int(0.42 * a + 0.58 * b) for a, b in zip(CAP[k], PANEL))   # Frequenzweiche abgeblendet
        d.ellipse([(x - KC - 1) * S, (KY - KC - 1) * S, (x + KC + 1) * S, (KY + KC + 1) * S], fill=(10, 10, 11))
        for i in range(KC * S, 0, -1):   # Kappe mit leichtem Verlauf (Licht von oben links)
            t = i / (KC * S); f = 0.86 + 0.14 * (1 - t) ** 0.5; ox, oy = -0.25 * (KC - i / S), -0.3 * (KC - i / S)
            d.ellipse([(x + ox) * S - i, (KY + oy) * S - i, (x + ox) * S + i, (KY + oy) * S + i], fill=tuple(min(255, int(c * f)) for c in cap))
        if not act:   # Zeiger der Frequenzweiche fest in Mittelstellung (ohne Wirkung)
            d.line([(x * S, KY * S), (x * S, (KY - KC * 0.92) * S)], fill=(40, 40, 42), width=int(3 * S))
    # POWER: Anzeige, runde Taste, Typbezeichnung (normale Schrift)
    x, y, r = POWER['lamp']; d.ellipse([(x - r - 4) * S, (y - r - 4) * S, (x + r + 4) * S, (y + r + 4) * S], fill=(16, 16, 17)); d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=(225, 50, 36))
    x, y, r = POWER['switch']; ctext(x, 30, 'POWER', 12); ctext(x, 46, 'ON / OFF', 10, FN)
    for rr, c in [(r + 5, (14, 14, 15)), (r, (52, 52, 56)), (r - 6, (22, 22, 24))]: d.ellipse([(x - rr) * S, (y - rr) * S, (x + rr) * S, (y + rr) * S], fill=c)
    ctext(1836, 182, 'B100 III', 20, FB)
    return im
def main():
    im = draw(S).resize((W, H), Image.LANCZOS)
    os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'b3panel.jpg'); im.save(jp, quality=90)
    geo = {'w': W, 'h': H, 'knobs': {k: {'x': x, 'y': KY, 'r': KR, 'label': ' '.join(lab), 'scale': sc, 'active': bool(a)} for k, x, lab, sc, a in KNOBS},
           'leds': {k: {'x': x, 'y': y} for k, x, y in LEDS}, 'jacks': {k: {'x': x, 'y': y, 'r': r} for k, x, y, r in JACKS}}
    json.dump(geo, open(os.path.join(GEN, 'b3geo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'b3panel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/b3_panel.py: gezeichnete Frontplatte Lametta B100-III (JPEG)\nstatic const unsigned char B3PANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int B3PANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'b3geo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/b3_panel.py: Lage der Bedienelemente auf b3panel.jpg (Bildpunkte)\n')
        f.write('static const int B3W = %d, B3H = %d;\n' % (W, H))
        f.write('static const int B3NKNOB = %d;\nstatic const double B3KNOB[%d][3] = {%s};\n' % (len(KNOBS), len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, KR) for _, x, _, _, _ in KNOBS)))
        f.write('static const int B3ACTIVE[%d] = {%s};   // 0 = Frequenzweiche (ohne Wirkung)\n' % (len(KNOBS), ', '.join(str(a) for *_, a in KNOBS)))
        f.write('static const double B3LED[2][2] = {%s};   // Anzeigen PARAMETRIC EQ 1 / 2\n' % ', '.join('{%d, %d}' % (x, y) for _, x, y in LEDS))
        f.write('static const double B3JACK[3][3] = {%s};   // INPUT, PHONES, FOOT SW\n' % ', '.join('{%d, %d, %d}' % (x, y, r) for _, x, y, r in JACKS))
    print('b3panel.jpg', len(data), 'Bytes')
    S4 = 4; big = draw(S4); x0 = KNOBS[0][1] - 84; crop = big.crop((x0 * S4, 40 * S4, (x0 + 172) * S4, 212 * S4))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): crop.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'lametta-b100iii-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(crop.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'lametta-b100iii-maskable-512.png'), optimize=True)
if __name__ == '__main__': main()
