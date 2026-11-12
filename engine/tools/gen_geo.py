# Erzeugt geo_gen.h/panel_jpg.h (Laney) und ygeo_gen.h/ypanel_jpg.h (Yamaha) für die Oberflächen der CLAP-Plugins aus engine/gen/*.json und *.jpg
import json, os, sys
G_DIR = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'gen')
g = json.load(open(os.path.join(G_DIR, 'geo.json'))); G = g['_']; L = []
L.append('// erzeugt aus geo.json (Foto der Frontplatte, 2430 x 358)')
L.append(f'static const double FW = {G["w"]}, FH = {G["h"]}, SEG0 = {G["seg0"]}, SEG1 = {G["seg1"]}, SEGHA = {G["segha"]}, DIG0 = {G["dig0"]}, DIG1 = {G["dig1"]};')
L.append('struct KnobGeo { const char *key; double rx, ry, R, px, py; int kind; double cap, skirt; };   // kind 0 schwarz, 1 rot, 2 lila')
L.append('static const KnobGeo KG[9] = {')
for k in ['v1','v2','pl','mv','t','m','b','pres','rev']:
    d = g[k]; kind = {'black':0,'red':1,'purple':2}[d['kind']]
    L.append(f'  {{"{k}", {d["rx"]}, {d["ry"]}, {d["R"]}, {d["px"]}, {d["py"]}, {kind}, {d.get("cap",0)}, {d.get("skirt",0)}}},')
L.append('};')
L.append('struct LabelGeo { const char *t; const char *k; double x, y, w, h; };')
L.append('static const LabelGeo LG[%d] = {' % len(g['labels']))
for l in g['labels']: L.append(f'  {{"{l["t"]}", "{l["k"]}", {l["x"]}, {l["y"]}, {l["w"]}, {l["h"]}}},')
L.append('};')
j = g['jack_high']; L.append(f'static const double JACK_HIGH[3] = {{{j["x"]}, {j["y"]}, {j["r"]}}};'); j = g['jack_low']; L.append(f'static const double JACK_LOW[3] = {{{j["x"]}, {j["y"]}, {j["r"]}}};')
d = g['led']; L.append(f'static const double LED[3] = {{{d["x"]}, {d["y"]}, {d["r"]}}};'); p = g['pullline']; L.append(f'static const double PULLLINE[4] = {{{p["x1"]}, {p["x2"]}, {p["y"]}, {p["w"]}}};')
L.append('static const double POWER[4] = {2115, 133, 95, 84};')
open(os.path.join(G_DIR, 'geo_gen.h'), 'w').write('\n'.join(L) + '\n')
data = open(os.path.join(G_DIR, 'panel.jpg'), 'rb').read()
open(os.path.join(G_DIR, 'panel_jpg.h'), 'w').write('static const unsigned char PANEL_JPG[] = {' + ','.join(str(b) for b in data) + '};\nstatic const unsigned int PANEL_JPG_LEN = %d;\n' % len(data))
print('geo_gen.h', len(L), 'Zeilen; panel_jpg.h', len(data), 'Bytes')
# Yamaha: Kappen (vol, t, m, b, e0..e4) und Buchsen auf dem Foto
y = json.load(open(os.path.join(G_DIR, 'ygeo.json'))); F = y['front']; U = F['_']; L = []
L.append('// erzeugt aus ygeo.json (Foto der Frontplatte des Lametta B100-II, %s x %s)' % (U['w'], U['h']))
L.append(f'static const double YFW = {U["w"]}, YFH = {U["h"]}, YPW = {U["pw"]}, YPIN = {U["pin"]}, YPOUT = {U["pout"]};')
L.append('struct YKnobGeo { const char *key; double x, y, r; };')
L.append('static const YKnobGeo YKG[9] = {')
for k in ['vol','t','m','b','e0','e1','e2','e3','e4']:
    d = F[k]; L.append(f'  {{"{k}", {d["x"]}, {d["y"]}, {d["r"]}}},')
L.append('};')
j = y['jacks']['high']; L.append(f'static const double YJACK_HIGH[3] = {{{j["x"]}, {j["y"]}, {j["r"]}}};'); j = y['jacks']['low']; L.append(f'static const double YJACK_LOW[3] = {{{j["x"]}, {j["y"]}, {j["r"]}}};')
open(os.path.join(G_DIR, 'ygeo_gen.h'), 'w').write('\n'.join(L) + '\n')
data = open(os.path.join(G_DIR, 'ypanel.jpg'), 'rb').read()
open(os.path.join(G_DIR, 'ypanel_jpg.h'), 'w').write('static const unsigned char YPANEL_JPG[] = {' + ','.join(str(b) for b in data) + '};\nstatic const unsigned int YPANEL_JPG_LEN = %d;\n' % len(data))
print('ygeo_gen.h', len(L), 'Zeilen; ypanel_jpg.h', len(data), 'Bytes')
