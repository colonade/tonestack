// Erzeugt die Datendateien des C++-Kerns aus den Web-Seiten (eine Quelle für alle Fassungen)
//   node extract.js <repo>  →  engine/gen/parts_gen.h, yparts_gen.h, yparts_order.json, geo.json, panel.jpg, ygeo.json, ypanel.jpg
const fs = require('fs'), path = require('path'), vm = require('vm');
const R = process.argv[2] || path.join(__dirname, '..', '..'); const OUT = path.join(R, 'engine', 'gen'); fs.mkdirSync(OUT, {recursive: true});
const scriptOf = f => { const h = fs.readFileSync(f, 'utf8'); return h.split('<script>').slice(1).map(p => p.split('</script>')[0]).sort((a, b) => b.length - a.length)[0]; };   // größter Skriptblock
const fmt = x => String(x);
// Laney: LANEY_PARTS / LANEY_FIXED aus der Seite (parts.js ist dort eingebettet)
{ const js = scriptOf(path.join(R, 'web/tonestack/laney-aor50/index.html'));
  const i = js.indexOf('const LANEY_PARTS'), j = js.indexOf('if (typeof module', i);
  const ctx = {}; vm.runInNewContext(js.slice(i, j).replace(/^const /gm, 'var '), ctx);
  const P = ctx.LANEY_PARTS, F = ctx.LANEY_FIXED, v = {...F}, vu = {...F};
  for (const p of P) { const k0 = p.length > 7 ? p[7] : p[4]; v[p[0]] = k0 * p[3]; vu[p[0]] = (p[5] !== null ? p[5] : k0) * p[3]; }   // p[7]: abweichender Standardwert für Plugin/Kern (Umbauideen, die nur mit Häkchen gelten)
  let out = '// erzeugt von engine/tools/extract.js aus web/tonestack/laney-aor50/index.html (LANEY_PARTS)\n#define PARTS_LIST(X) \\\n'; for (const k of Object.keys(v)) out += '  X(' + k + ', ' + fmt(v[k]) + ', ' + fmt(vu[k]) + ') \\\n'; out += '\n';
  fs.writeFileSync(path.join(OUT, 'parts_gen.h'), out);
  const m = js.match(/const FRONT = (\{.*?\});\n/); fs.writeFileSync(path.join(OUT, 'geo.json'), m[1]);
  const h = fs.readFileSync(path.join(R, 'web/tonestack/laney-aor50/index.html'), 'utf8'); const b64 = h.match(/data:image\/jpeg;base64,([A-Za-z0-9+/=]+)/)[1];
  fs.writeFileSync(path.join(OUT, 'panel.jpg'), Buffer.from(b64, 'base64'));
  // Gegenprobe: Reihenfolge wie im Worklet (PORDER)
  const w = fs.readFileSync(path.join(R, 'web/tonestack/laney-aor50/laney-worklet.js'), 'utf8'); const po = JSON.parse(w.match(/const PORDER = (\[[^\]]*\]);/)[1]);
  console.log('Laney: ' + Object.keys(v).length + ' Bauteile, Reihenfolge wie im Worklet: ' + (JSON.stringify(po) === JSON.stringify(Object.keys(v)))); }
// Yamaha: PARTS aus der Seite
{ const js = scriptOf(path.join(R, 'web/tonestack/yamaha-b100ii/index.html'));
  const i = js.indexOf('const PARTS = ['); const PARTS = vm.runInNewContext(js.slice(i + 14, js.indexOf('];', i) + 1));
  const keys = PARTS.map(p => p[0]);
  fs.writeFileSync(path.join(OUT, 'yparts_gen.h'), '// erzeugt von engine/tools/extract.js aus web/tonestack/yamaha-b100ii/index.html (Yamaha B100-II): Bauteilschlüssel in Übergabe-Reihenfolge, Standardwerte (SI) für das Plugin\n#define YPARTS_LIST(X) \\\n' + keys.map(k => '  X(' + k + ') \\\n').join('') + '\n'
    + '#define YPARTS_DEFAULTS(X) \\\n' + PARTS.map(p => '  X(' + p[0] + ', ' + fmt(p[4] * p[3]) + ') \\\n').join('') + '\n');
  const ym = js.match(/const FRONT = (\{.*?\});\n/), yj = js.match(/const JACKS = (\{.*?\}), FJACK/);   // Frontplatte (Foto 1800 x 227): Kappen und Buchsen
  fs.writeFileSync(path.join(OUT, 'ygeo.json'), JSON.stringify({front: vm.runInNewContext('(' + ym[1] + ')'), jacks: vm.runInNewContext('(' + yj[1] + ')')}));
  const yh = fs.readFileSync(path.join(R, 'web/tonestack/yamaha-b100ii/index.html'), 'utf8'); fs.writeFileSync(path.join(OUT, 'ypanel.jpg'), Buffer.from(yh.match(/data:image\/jpeg;base64,([A-Za-z0-9+/=]+)/)[1], 'base64'));
  const w = fs.readFileSync(path.join(R, 'web/tonestack/yamaha-b100ii/b100ii-worklet.js'), 'utf8'); const po = JSON.parse(w.match(/const PORDER = (\[[^\]]*\]);/)[1]);
  console.log('Yamaha: ' + keys.length + ' Bauteile, Reihenfolge wie im Worklet: ' + (JSON.stringify(po) === JSON.stringify(keys))); }
