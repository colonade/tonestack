#!/usr/bin/env python3
# Zeichnet die Frontplatte des Blech 140 (Hallplatte nach dem EMT 140). Eigene Gestaltung im Stil eines Studio-Bedienteils der Zeit:
# kein Foto, kein Logo, keine Nachbildung der EMT-Fernbedienung. Instrument NACHHALLZEIT (Sekunden bei 500 Hz, wie die Anzeige des Geräts),
# Knopf DECAY (fährt die Dämpfungsplatte), Wahlschalter BASS CUT 0–3 (wie am Verstärker EMT 162), Lampe MOTOR (Dämpfungsplatte fährt),
# Knöpfe MIX, PRE-DELAY, OUTPUT (Modell, nicht am Gerät).
# Schreibt engine/gen/blechpanel.jpg (2000 × 240, ohne Zeiger), blechgeo.json, blechpanel_jpg.h, blechgeo_gen.h und die Seitensymbole
#   python3 engine/tools/blech_panel.py [repo]
import json, os, sys, math
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
GEN = os.path.join(R, 'engine', 'gen')
W, H, S = 2000, 240, 2
FB = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
FN = '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
PANEL = (196, 199, 194); INK = (38, 40, 42); DIM = (92, 96, 98); RED = (168, 40, 32)
KY = 128; KR = 30; KC = 21
GAUGE = (520, 150, 112)   # Mitte x, y (Drehpunkt), Radius: Zeiger von 1 s (−50°) bis 4 s (+50°)
G0, G1, GA = 1.0, 4.0, 50.0
KNOBS = [('decay', 760, 'DECAY', 'short', 'long', 36), ('mix', 1310, 'MIX', 'dry', 'wet', 30), ('pre', 1450, 'PRE-DELAY', '0', '200 ms', 30), ('out', 1590, 'OUTPUT', '−24', '+24 dB', 30)]
BASS = (1000, 128)        # Wahlschalter BASS CUT, Stellungen 0–3 bei −45°, −15°, 15°, 45°
LAMP = (860, 70)
def font(px, f=FB): return ImageFont.truetype(f, int(round(px * S)))
def gang(t): return (-GA + 2 * GA * (t - G0) / (G1 - G0)) * math.pi / 180
def draw():
    im = Image.new('RGB', (W * S, H * S)); d = ImageDraw.Draw(im)
    for y in range(H * S):   # Hammerschlaglack, leichter Verlauf
        v = int(6 * (0.5 - y / (H * S))); d.line([(0, y), (W * S, y)], fill=(PANEL[0] + v, PANEL[1] + v, PANEL[2] + v))
    d.rectangle([0, 0, W * S - 1, H * S - 1], outline=(110, 112, 110), width=5 * S)
    for x in (24, W - 24):
        for y in (24, H - 24):
            d.ellipse([(x - 8) * S, (y - 8) * S, (x + 8) * S, (y + 8) * S], fill=(150, 152, 150), outline=(90, 92, 90), width=S); d.line([((x - 5) * S, y * S), ((x + 5) * S, y * S)], fill=(80, 80, 80), width=2 * S)
    def ctext(x, y, t, px=13, f=FB, col=INK):
        ff = font(px, f); w = d.textlength(t, font=ff); d.text((x * S - w / 2, y * S), t, font=ff, fill=col)
    # Name links
    ctext(150, 64, 'BLECH 140', 30, FB, INK); ctext(150, 104, 'HALLPLATTE · PLATE REVERB', 12, FB, DIM)
    ctext(150, 132, 'steel plate 2 m × 1 m × 0.5 mm', 10.5, FN, DIM); ctext(150, 147, 'physical model after the EMT 140', 10.5, FN, DIM)
    ctext(150, 170, 'own design', 10.5, FN, DIM)
    # Instrument NACHHALLZEIT
    gx, gy, gr = GAUGE; d.rounded_rectangle([(gx - 150) * S, 22 * S, (gx + 150) * S, 218 * S], radius=8 * S, fill=(30, 30, 28), outline=(70, 70, 66), width=2 * S)
    d.rounded_rectangle([(gx - 140) * S, 30 * S, (gx + 140) * S, 210 * S], radius=6 * S, fill=(236, 230, 210))
    for k in range(0, 31):
        t = G0 + (G1 - G0) * k / 30; a = gang(t); r1 = gr - (14 if k % 10 == 0 else 9 if k % 5 == 0 else 6)
        d.line([((gx + r1 * math.sin(a)) * S, (gy - r1 * math.cos(a)) * S), ((gx + gr * math.sin(a)) * S, (gy - gr * math.cos(a)) * S)], fill=INK, width=max(1, int((2.2 if k % 10 == 0 else 1.3) * S)))
    for t in (1, 2, 3, 4):
        a = gang(t); r = gr - 28; ff = font(16); s = str(t); w = d.textlength(s, font=ff); d.text(((gx + r * math.sin(a)) * S - w / 2, (gy - r * math.cos(a) - 10) * S), s, font=ff, fill=INK)
    d.arc([(gx - gr) * S, (gy - gr) * S, (gx + gr) * S, (gy + gr) * S], 270 - GA, 270 + GA, fill=INK, width=int(1.5 * S))
    ctext(gx, 168, 'NACHHALLZEIT  s', 13, FB, INK); ctext(gx, 186, 'reverberation time at 500 Hz', 10, FN, DIM)
    # Lampe MOTOR (Dämpfungsplatte fährt)
    x, y = LAMP; ctext(x, 34, 'MOTOR', 11, FB, DIM); d.ellipse([(x - 9) * S, (y - 9) * S, (x + 9) * S, (y + 9) * S], fill=(40, 30, 26)); d.ellipse([(x - 6) * S, (y - 6) * S, (x + 6) * S, (y + 6) * S], fill=(96, 50, 30))
    # Wahlschalter BASS CUT
    bx, by = BASS; ctext(bx, 36, 'BASS CUT', 13, FB)
    for i in range(4):
        a = (-45 + 30 * i) * math.pi / 180; r = 46; s = str(i); ff = font(14); w = d.textlength(s, font=ff); d.text(((bx + r * math.sin(a)) * S - w / 2, (by - r * math.cos(a) - 9) * S), s, font=ff, fill=INK)
        d.line([((bx + 33 * math.sin(a)) * S, (by - 33 * math.cos(a)) * S), ((bx + 38 * math.sin(a)) * S, (by - 38 * math.cos(a)) * S)], fill=INK, width=2 * S)
    sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(bx - 26 + 3) * S, (by - 26 + 6) * S, (bx + 26 + 3) * S, (by + 26 + 6) * S], fill=200); sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((90, 92, 90), mask=sh); d = ImageDraw.Draw(im)
    d.ellipse([(bx - 26) * S, (by - 26) * S, (bx + 26) * S, (by + 26) * S], fill=(30, 30, 30))
    ctext(bx, 190, '0 off · 1–3 less bass', 10, FN, DIM)
    # Trennlinie Modell
    d.line([(1220 * S, 30 * S), (1220 * S, (H - 30) * S)], fill=(120, 122, 120), width=2 * S)   # Schriftzug „MODEL (not on the unit)“ entfernt (19.11.2026)
    ctext(1830, 100, 'MONO IN', 12, FB, DIM); ctext(1830, 118, 'STEREO OUT', 12, FB, DIM); ctext(1830, 140, 'two pickups', 10, FN, DIM)
    # Knöpfe
    for k, x, lab, lft, rgt, kr in KNOBS:
        ctext(x, 52 if kr < 36 else 44, lab, 13, FB)
        for j in range(11):
            a = (-150 + 30 * j) * math.pi / 180; r1, r2 = kr + 5, kr + (11 if j % 5 == 0 else 8)
            d.line([((x + r1 * math.sin(a)) * S, (KY - r1 * math.cos(a)) * S), ((x + r2 * math.sin(a)) * S, (KY - r2 * math.cos(a)) * S)], fill=INK, width=max(1, int(1.5 * S)))
        ff = font(11)
        for j, t in [(0, lft), (10, rgt)]:
            a = (-150 + 30 * j) * math.pi / 180; tx, ty = x + (kr + 24) * math.sin(a), KY - (kr + 24) * math.cos(a); tw = d.textlength(t, font=ff); d.text((tx * S - tw / 2, (ty - 2) * S), t, font=ff, fill=DIM)
        sh = Image.new('L', (W * S, H * S), 0); ImageDraw.Draw(sh).ellipse([(x - kr + 3) * S, (KY - kr + 6) * S, (x + kr + 3) * S, (KY + kr + 6) * S], fill=200); sh = sh.filter(ImageFilter.GaussianBlur(5 * S)); im.paste((100, 102, 100), mask=sh); d = ImageDraw.Draw(im)
        d.ellipse([(x - kr) * S, (KY - kr) * S, (x + kr) * S, (KY + kr) * S], fill=(26, 26, 26))
        for j in range(36):
            a = j * math.pi / 18; c = 64 if math.sin(a - 2.4) > 0 else 40; kc = kr * 0.7
            d.line([((x + (kc + 1) * math.cos(a)) * S, (KY + (kc + 1) * math.sin(a)) * S), ((x + (kr - 1) * math.cos(a)) * S, (KY + (kr - 1) * math.sin(a)) * S)], fill=(c, c, c), width=max(1, int(1.4 * S)))
        kc = kr * 0.7; d.ellipse([(x - kc) * S, (KY - kc) * S, (x + kc) * S, (KY + kc) * S], fill=(48, 48, 48))
    return im
def main():
    im = draw().resize((W, H), Image.LANCZOS); os.makedirs(GEN, exist_ok=True); jp = os.path.join(GEN, 'blechpanel.jpg'); im.save(jp, quality=90)
    geo = {'W': W, 'H': H, 'knobs': [{'k': k, 'x': x, 'y': KY, 'r': kr} for k, x, _, _, _, kr in KNOBS], 'gauge': {'x': GAUGE[0], 'y': GAUGE[1], 'r': GAUGE[2], 't0': G0, 't1': G1, 'deg': GA},
           'bass': {'x': BASS[0], 'y': BASS[1]}, 'lamp': {'x': LAMP[0], 'y': LAMP[1]}}
    json.dump(geo, open(os.path.join(GEN, 'blechgeo.json'), 'w'))
    data = open(jp, 'rb').read()
    with open(os.path.join(GEN, 'blechpanel_jpg.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/blech_panel.py: gezeichnete Frontplatte Blech 140 (JPEG)\nstatic const unsigned char BLECHPANEL_JPG[] = {\n')
        for i in range(0, len(data), 24): f.write(','.join(str(b) for b in data[i:i + 24]) + ',\n')
        f.write('};\nstatic const unsigned int BLECHPANEL_JPG_LEN = %d;\n' % len(data))
    with open(os.path.join(GEN, 'blechgeo_gen.h'), 'w') as f:
        f.write('// erzeugt von engine/tools/blech_panel.py: Lage der Bedienelemente auf blechpanel.jpg (Bildpunkte)\n')
        f.write('static const int BLW = %d, BLH = %d;\n' % (W, H))
        f.write('static const double BLKNOB[%d][3] = {%s};   // DECAY, MIX, PRE-DELAY, OUTPUT\n' % (len(KNOBS), ', '.join('{%d, %d, %d}' % (x, KY, kr) for _, x, _, _, _, kr in KNOBS)))
        f.write('static const double BLGAUGE[6] = {%d, %d, %d, %g, %g, %g};   // Instrument: Drehpunkt x, y, Radius, Skala von/bis (s), halber Ausschlag (°)\n' % (GAUGE + (G0, G1, GA)))
        f.write('static const double BLBASS[2] = {%d, %d};   // Wahlschalter BASS CUT\nstatic const double BLLAMP[2] = {%d, %d};   // Lampe MOTOR\n' % (BASS + LAMP))
    crop = im.crop((380, 20, 660, 220)).resize((280, 200)); sq = Image.new('RGB', (280, 280), PANEL); sq.paste(crop, (0, 40))
    ICO = os.path.join(R, 'web', 'tonestack', 'assets', 'icons'); os.makedirs(ICO, exist_ok=True)
    for n in (180, 192, 512): sq.resize((n, n), Image.LANCZOS).save(os.path.join(ICO, 'blech-140-%d.png' % n), optimize=True)
    m = Image.new('RGB', (512, 512), PANEL); m.paste(sq.resize((410, 410), Image.LANCZOS), (51, 51)); m.save(os.path.join(ICO, 'blech-140-maskable-512.png'), optimize=True)
    print('ok', len(data), 'Bytes')
main()
