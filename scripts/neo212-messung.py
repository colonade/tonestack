#!/usr/bin/env python3
# Messung der Box GK Neo 212-II (oder jeder anderen Box) für das Lautsprechermodell im Simulator.
# Rechner: Ubuntu, Interface Zoom AMS-24, Mikrofon Shure SM7. Alles läuft aus diesem Skript, REAPER wird nicht gebraucht.
#
#   python3 neo212-messung.py              Menü mit allen Schritten (jeder Schritt erklärt vorher die Verdrahtung)
#   python3 neo212-messung.py --geraete    Audiogeräte auflisten
#   python3 neo212-messung.py --test       Rechnung an erfundenen Signalen prüfen (ohne Interface)
#
# Was gemessen wird (alles mit einem logarithmischen Sinus-Sweep 10 Hz … 20 kHz, 10 s, 48 kHz):
#   A  Impedanzkurve der Box      Kopfhörerausgang des Interface → Vorwiderstand → Box. Eingang 1 misst die Spannung vor dem Widerstand,
#                                 Eingang 2 die Spannung an der Box. Vorher einmal mit einem bekannten Widerstand statt der Box
#                                 (Kalibrierung der beiden Eingangsverstärkungen). Kein Verstärker nötig, keine Lautstärke.
#   B  Schalldruck                Verstärker → Box; Eingang 2 misst die Spannung an den Boxenklemmen über einen Spannungsteiler,
#                                 Eingang 1 das SM7. Nahfeld Membran, Nahfeld Port, 1 m auf Achse (Horn an / aus).
# Ergebnis: Ordner ~/neo212-messung/<Datum>/ mit Rohaufnahmen (WAV, 2 Kanäle), Impulsantworten (WAV), Kurven (CSV) und Bildern (PNG)
# sowie messung.json (Widerstände, Pegel, Abstände). Den ganzen Ordner als ZIP schicken.
#
# Benötigt: python3-numpy, python3-scipy, python3-matplotlib, python3-sounddevice  (sudo apt install …; sounddevice notfalls pip)
import argparse, datetime, json, os, sys, time
import numpy as np
try:
    import sounddevice as sd
except (ImportError, OSError):   # Modul oder PortAudio (libportaudio2) fehlt
    sd = None
from scipy import signal
from scipy.io import wavfile

FS = 48000
F0, F1, T_SWEEP = 10.0, 20000.0, 10.0
VOR, NACH = 0.5, 2.0            # Stille vor dem Sweep, Nachlauf (Raum, Nachhall)
HOME = os.path.expanduser('~')

# ---------- Signale ----------
def sweep():
    """Logarithmischer Sweep (Farina) mit Ein-/Ausblendung und Stille davor/danach; zurück: Signal, Startindex des Sweeps."""
    n = int(T_SWEEP * FS); t = np.arange(n) / FS
    k = np.log(F1 / F0); s = np.sin(2 * np.pi * F0 * T_SWEEP / k * (np.exp(t * k / T_SWEEP) - 1))
    fade = int(0.02 * FS); w = np.hanning(2 * fade)
    s[:fade] *= w[:fade]; s[-fade:] *= w[fade:]
    x = np.concatenate([np.zeros(int(VOR * FS)), s, np.zeros(int(NACH * FS))])
    return x.astype(np.float32), int(VOR * FS)

def inverse_sweep():
    """Inverses Filter zum Sweep: zeitlich gespiegelt, mit 6 dB/Oktave fallender Hüllkurve (Farina)."""
    n = int(T_SWEEP * FS); t = np.arange(n) / FS; k = np.log(F1 / F0)
    s = np.sin(2 * np.pi * F0 * T_SWEEP / k * (np.exp(t * k / T_SWEEP) - 1))
    env = np.exp(-t * k / T_SWEEP)   # 6 dB/Oktave fallend über die Zeit des gespiegelten Sweeps (der mit den Höhen beginnt)
    inv = s[::-1] * env
    inv /= np.max(np.abs(np.fft.rfft(s, 4 * n) * np.fft.rfft(inv, 4 * n)))  # Normierung: Sweep * inv ≈ 1 im Band
    return inv

PRE = 0.2   # Sekunden vor dem Zeitnullpunkt, die in jeder Impulsantwort mitgeführt werden (Vorschwingen der Bandbegrenzung)
def impulsantwort(aufnahme, start):
    """Impulsantwort eines aufgenommenen Kanals: Faltung mit dem inversen Sweep; der Zeitnullpunkt liegt bei Index PRE·FS."""
    inv = inverse_sweep()
    y = signal.fftconvolve(aufnahme, inv)
    t0 = start + len(inv) - 1
    return y[t0 - int(PRE * FS):]

def spektrum(ir, fenster_s, fs=FS):
    """Spektrum (komplex) einer Impulsantwort: Fenster von −PRE bis +fenster_s um den Zeitnullpunkt, hinten halbes Hann-Fenster
    (letzte Hälfte des Nachlaufs), auf dem Raster einer festen FFT-Länge."""
    n = int((PRE + fenster_s) * fs); seg = ir[:n].copy()
    post = n - int(PRE * fs); m = n - post // 2; w = np.ones(n); w[m:] = np.hanning(2 * (n - m))[(n - m):]
    seg *= w[:len(seg)]
    nfft = 1 << 18   # feste Länge für alle Fenster (Spektren müssen zueinander passen)
    H = np.fft.rfft(seg, nfft); f = np.fft.rfftfreq(nfft, 1 / fs)
    return f, H

def glaetten(f, mag_db, oktave=1 / 6):
    """Glättung in Bruchteilen einer Oktave (für die Darstellung)."""
    out = np.empty_like(mag_db)
    for i, fi in enumerate(f):
        if fi <= 0: out[i] = mag_db[i]; continue
        lo, hi = fi * 2 ** (-oktave / 2), fi * 2 ** (oktave / 2)
        sel = (f >= lo) & (f <= hi); out[i] = np.mean(mag_db[sel]) if np.any(sel) else mag_db[i]
    return out

# ---------- Audio ----------
def geraet_finden(name=None):
    if sd is None: sys.exit('python3-sounddevice oder libportaudio2 fehlt: sudo apt install python3-sounddevice libportaudio2')
    devs = sd.query_devices()
    if name is None:
        for i, d in enumerate(devs):
            n = d['name'].lower()
            if ('ams' in n or 'zoom' in n) and d['max_input_channels'] >= 2 and d['max_output_channels'] >= 2: return i
        return None
    for i, d in enumerate(devs):
        if name.lower() in d['name'].lower(): return i
    sys.exit(f'Gerät "{name}" nicht gefunden (--geraete zeigt alle)')

def aufnehmen(dev, x, pegel):
    """Spielt x auf Ausgang L+R und nimmt gleichzeitig 2 Kanäle auf; zurück: Array (n, 2) float."""
    out = np.stack([x * pegel, x * pegel], axis=1)
    try:
        rec = sd.playrec(out, samplerate=FS, channels=2, device=dev, dtype='float32', blocking=True)
        sd.wait()
    except Exception as e:
        print(f'  Audiogerät lässt sich nicht öffnen: {e}')
        print('  Meist hält PipeWire/PulseAudio das Interface. Dann: in den Ubuntu-Toneinstellungen das AMS-24 als Ein- und Ausgabe wählen')
        print('  und das Skript mit  --geraet pipewire  (oder --geraet pulse / --geraet default) starten.')
        raise
    return rec

def pegelcheck(dev, pegel, dauer=2.0):
    """Kurzer Testton (200 Hz + 2 kHz), zeigt die Spitzenpegel beider Eingänge in dBFS."""
    t = np.arange(int(dauer * FS)) / FS
    x = (0.5 * np.sin(2 * np.pi * 200 * t) + 0.5 * np.sin(2 * np.pi * 2000 * t)).astype(np.float32)
    rec = aufnehmen(dev, x, pegel); pk = np.max(np.abs(rec[int(0.3 * FS):]), axis=0)
    db = 20 * np.log10(np.maximum(pk, 1e-6))
    print(f'    Spitzen: Eingang 1 {db[0]:6.1f} dBFS   Eingang 2 {db[1]:6.1f} dBFS   (gut: −30 … −6; über −1 = übersteuert)')
    return db

def wav_schreiben(pfad, data):
    wavfile.write(pfad, FS, np.asarray(data, dtype=np.float32))

# ---------- Auswertung ----------
def impedanz_auswerten(rec_cal, rec_box, start, r_vor, r_cal):
    """Impedanz aus zwei Aufnahmen: V1 = Spannung vor dem Vorwiderstand (Eingang 1), V2 = an der Last (Eingang 2).
    Kalibrierung: mit bekanntem Widerstand r_cal ist V2/V1 = r_cal/(r_vor + r_cal); daraus der Verstärkungsfaktor k der Eingänge."""
    def ratio(rec):
        h1 = impulsantwort(rec[:, 0], start); h2 = impulsantwort(rec[:, 1], start)
        f, H1 = spektrum(h1, 0.5); _, H2 = spektrum(h2, 0.5)
        return f, H2 / np.where(np.abs(H1) > 1e-12, H1, 1e-12)
    f, m_cal = ratio(rec_cal); _, m_box = ratio(rec_box)
    sel = (f >= 20) & (f <= 20000)
    k = m_cal / (r_cal / (r_vor + r_cal))           # komplexer Korrekturfaktor je Frequenz (Eingangsverstärkungen, Phasen)
    m = m_box / k
    z = r_vor * m / (1 - m)
    return f[sel], z[sel]

def impedanz_bericht(f, z, ordner):
    mag = np.abs(z); ph = np.degrees(np.angle(z))
    lo = (f >= 20) & (f <= 300); fl, ml = f[lo], mag[lo]
    peaks, _ = signal.find_peaks(glaetten(fl, 20 * np.log10(ml), 1 / 12), prominence=1.5)
    text = []
    if len(peaks) >= 2:
        p1, p2 = peaks[0], peaks[1]; seg = slice(p1, p2 + 1); imin = p1 + int(np.argmin(ml[seg]))
        text.append(f'Bassreflex: Impedanzhöcker bei {fl[p1]:.1f} Hz ({ml[p1]:.1f} Ω) und {fl[p2]:.1f} Hz ({ml[p2]:.1f} Ω), Minimum dazwischen = Abstimmung Fb ≈ {fl[imin]:.1f} Hz ({ml[imin]:.1f} Ω)')
    elif len(peaks) == 1:
        text.append(f'Ein Impedanzhöcker bei {fl[peaks[0]]:.1f} Hz ({ml[peaks[0]]:.1f} Ω) – geschlossene Box oder Port verschlossen?')
    else:
        text.append('Keine Höcker unter 300 Hz gefunden – Verdrahtung prüfen (Kalibrierung mit richtigem Widerstandswert?)')
    hf = (f >= 150) & (f <= 400); text.append(f'Gleichstromnahe Impedanz (Minimum 150–400 Hz): {np.min(mag[hf]):.2f} Ω; bei 1 kHz {mag[np.argmin(np.abs(f - 1000))]:.2f} Ω, bei 10 kHz {mag[np.argmin(np.abs(f - 10000))]:.2f} Ω')
    np.savetxt(os.path.join(ordner, 'impedanz.csv'), np.column_stack([f, mag, ph]), delimiter=';', header='Hz;Ohm;Grad', comments='', fmt='%.4f')
    try:
        import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
        fig, ax = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
        ax[0].semilogx(f, mag); ax[0].set_ylabel('|Z| (Ω)'); ax[0].grid(True, which='both'); ax[0].set_title('Impedanz der Box')
        ax[1].semilogx(f, ph); ax[1].set_ylabel('Phase (°)'); ax[1].set_xlabel('Hz'); ax[1].grid(True, which='both')
        fig.savefig(os.path.join(ordner, 'impedanz.png'), dpi=110); plt.close(fig)
    except Exception as e: text.append(f'(kein Bild: {e})')
    return text

def akustik_auswerten(rec, start, teiler, fenster_s, ordner, name):
    """Übertragung Mikrofon / Klemmenspannung: Impulsantworten beider Kanäle, Quotient der Spektren, gefenstert."""
    h_mic = impulsantwort(rec[:, 0], start); h_ref = impulsantwort(rec[:, 1], start)
    wav_schreiben(os.path.join(ordner, f'{name}-ir-mikro.wav'), h_mic[:int((PRE + 2) * FS)] / max(np.max(np.abs(h_mic)), 1e-9) * 0.9)   # Nullpunkt bei 0,2 s
    f, Hm = spektrum(h_mic, fenster_s); _, Hr = spektrum(h_ref, 0.5)
    H = Hm / np.where(np.abs(Hr) > 1e-12, Hr, 1e-12) * teiler   # je Volt an den Klemmen: Klemmenspannung = Referenzkanal / Teiler
    sel = (f >= 20) & (f <= 20000); f, H = f[sel], H[sel]
    mag = 20 * np.log10(np.maximum(np.abs(H), 1e-12)); sm = glaetten(f, mag)
    np.savetxt(os.path.join(ordner, f'{name}.csv'), np.column_stack([f, mag, sm, np.degrees(np.angle(H))]), delimiter=';', header='Hz;dB_roh;dB_1/6okt;Grad', comments='', fmt='%.4f')
    try:
        import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(10, 5)); ax.semilogx(f, mag, alpha=0.3); ax.semilogx(f, sm); ax.set_xlabel('Hz'); ax.set_ylabel('dB (Mikro je Volt an der Box, unkalibriert)')
        ax.grid(True, which='both'); ax.set_title(f'{name} (Fenster {fenster_s * 1000:.0f} ms)'); fig.savefig(os.path.join(ordner, f'{name}.png'), dpi=110); plt.close(fig)
    except Exception: pass
    i1k = np.argmin(np.abs(f - 1000)); ref = sm[i1k]
    return [f'{name}: bezogen auf 1 kHz: ' + ', '.join(f'{fx:g} Hz {sm[np.argmin(np.abs(f - fx))] - ref:+.1f} dB' for fx in (40, 60, 100, 200, 500, 2000, 5000, 10000))]

# ---------- Ablauf ----------
def frage(text, standard=None):
    s = input(f'{text}' + (f' [{standard}]' if standard is not None else '') + ': ').strip()
    return s if s else standard

def warte(text):
    input(f'\n{text}\n→ Enter, wenn es so weit ist (Strg+C bricht ab) ')

def zahl(text, standard):
    while True:
        s = frage(text, str(standard))
        try: return float(s.replace(',', '.'))
        except ValueError: print('  bitte eine Zahl')

def sweep_messen(dev, pegel, ordner, name, notizen):
    x, start = sweep()
    while True:
        print('  Pegelprobe (2 s Testton) …'); db = pegelcheck(dev, pegel)
        if np.all(db < -1) and np.all(db > -35): break
        if np.any(db >= -1): print('  übersteuert: Gain am Interface (oder Lautstärke am Verstärker) kleiner')
        else: print('  sehr leise: Gain am Interface größer (bei der Impedanz-Kalibrierung liegt Eingang 2 bauartbedingt etwa 20 dB unter Eingang 1; mit der Box kommen bis zu 12 dB dazu)')
        if frage('  nochmal prüfen? (j = nach Änderung erneut messen, n = so weitermachen)', 'j') != 'j': break
    print(f'  Sweep läuft ({T_SWEEP:.0f} s + Nachlauf) …'); rec = aufnehmen(dev, x, pegel)
    pk = 20 * np.log10(np.maximum(np.max(np.abs(rec), axis=0), 1e-6)); print(f'  aufgenommen, Spitzen {pk[0]:.1f} / {pk[1]:.1f} dBFS')
    if np.any(pk > -0.5): print('  ACHTUNG: übersteuert – bitte wiederholen')
    wav_schreiben(os.path.join(ordner, f'{name}-roh.wav'), rec)
    notizen.setdefault('aufnahmen', {})[name] = {'spitzen_dbfs': [float(pk[0]), float(pk[1])], 'ausgangspegel': pegel, 'zeit': datetime.datetime.now().isoformat(timespec='seconds')}
    return rec, start

def main():
    ap = argparse.ArgumentParser(description='Messung einer Lautsprecherbox (Impedanz, Schalldruck) mit Sweeps')
    ap.add_argument('--geraete', action='store_true', help='Audiogeräte auflisten'); ap.add_argument('--geraet', help='Teil des Gerätenamens (Standard: AMS/Zoom)')
    ap.add_argument('--test', action='store_true', help='Rechnung mit erfundenen Signalen prüfen'); ap.add_argument('--ordner', help='Zielordner')
    a = ap.parse_args()
    if a.test: return selbsttest()
    if a.geraete:
        if sd is None: sys.exit('python3-sounddevice fehlt'); print(sd.query_devices()); return
    dev = geraet_finden(a.geraet)
    if dev is None:
        print('Zoom AMS-24 nicht gefunden. Geräte:'); print(sd.query_devices()); dev = int(frage('Nummer des Geräts', '0'))
    print(f'Gerät: {sd.query_devices(dev)["name"]}')
    ordner = a.ordner or os.path.join(HOME, 'neo212-messung', datetime.datetime.now().strftime('%Y-%m-%d_%H%M')); os.makedirs(ordner, exist_ok=True)
    npf = os.path.join(ordner, 'messung.json'); notizen = json.load(open(npf)) if os.path.exists(npf) else {'box': 'GK Neo 212-II', 'mikrofon': 'Shure SM7', 'interface': 'Zoom AMS-24', 'fs': FS}
    def speichern(): json.dump(notizen, open(npf, 'w'), indent=1, ensure_ascii=False)
    print(f'Ordner: {ordner}\n')
    print('AM INTERFACE ZUERST: Schalter LOOPBACK aus (sonst landet der Sweep direkt in der Aufnahme), Modus MUSIC (nicht STREAMING),')
    print('Phantomspeisung 48 V aus (SM7 braucht keine; bei der Impedanzmessung hängen Widerstände an den Eingängen!), Direct-Monitor egal.')
    print('Beide Eingänge auf LINE (Eingang 2 nicht Hi-Z) für die Impedanzmessung; für die Schalldruckmessung Eingang 1 = Mikro.\n')
    rec_cal = None
    while True:
        print('\nSchritte:')
        print('  1  Impedanz: Kalibrierung mit bekanntem Widerstand statt Box')
        print('  2  Impedanz: Box messen und auswerten (braucht 1)')
        print('  3  Schalldruck: Nahfeld Membran (Mikro 1–2 cm vor der Membran eines Zwölfers, mittig zwischen Kalotte und Rand)')
        print('  4  Schalldruck: Nahfeld Port (Mikro in der Portöffnung, mittig)')
        print('  5  Schalldruck: 1 m auf Achse des Horns, Horn-Regler voll auf (Full-Range-Stellung)')
        print('  6  Schalldruck: 1 m auf Achse, Horn-Regler ganz zu bzw. Horn aus')
        print('  7  Schalldruck: beliebige weitere Stellung (eigener Name)')
        print('  0  Ende')
        w = frage('Schritt', '0')
        if w == '0': speichern(); print(f'\nFertig. Bitte den ganzen Ordner packen und schicken:\n  cd {os.path.dirname(ordner)} && zip -r {os.path.basename(ordner)}.zip {os.path.basename(ordner)}'); return
        try:
            if w == '1':
                print('\nVERDRAHTUNG IMPEDANZ (kein Verstärker!):')
                print('  Interface KOPFHÖRERAUSGANG (treibt die kleine Last besser als OUTPUT; Klinke: Spitze = links, Schaft = Masse; Kopfhörerregler etwa halb)')
                print('  → Vorwiderstand R_vor (z. B. 100 Ω, 1/4 W reicht) → Last → Masse (Schaft).')
                print('  Eingang 1 (TS): Spitze an die Seite des Widerstands, die am Interface-Ausgang hängt.   Eingang 2 (TS): Spitze an die Last-Seite.')
                print('  Beide Schäfte an Masse (Schaft des Ausgangskabels). Last jetzt: ein bekannter Widerstand R_cal (5 … 20 Ω, z. B. 10 Ω; Wert mit dem Multimeter messen).')
                r_vor = zahl('R_vor in Ohm (gemessen)', notizen.get('r_vor', 100)); r_cal = zahl('R_cal in Ohm (gemessen)', notizen.get('r_cal', 10))
                notizen['r_vor'] = r_vor; notizen['r_cal'] = r_cal
                warte('Widerstände verdrahtet, Gain beider Eingänge auf etwa Mitte')
                rec_cal, start = sweep_messen(dev, 0.5, ordner, 'impedanz-kalibrierung', notizen); speichern(); print('  Kalibrierung gespeichert.')
            elif w == '2':
                if rec_cal is None:
                    p = os.path.join(ordner, 'impedanz-kalibrierung-roh.wav')
                    if os.path.exists(p): rec_cal = wavfile.read(p)[1].astype(np.float32); start = int(VOR * FS); print('  Kalibrierung aus Datei geladen')
                    else: print('  erst Schritt 1'); continue
                warte('Statt R_cal jetzt die Box anschließen (Klinke/Speakon FULL RANGE; Horn-Regler voll auf). Gain NICHT verändern!')
                rec_box, start = sweep_messen(dev, 0.5, ordner, 'impedanz-box', notizen)
                f, z = impedanz_auswerten(rec_cal, rec_box, start, notizen['r_vor'], notizen['r_cal'])
                for t in impedanz_bericht(f, z, ordner): print('  ' + t)
                notizen['impedanz'] = 'impedanz.csv'; speichern()
                if frage('  Noch einmal mit Horn-Regler ganz zu? (j/n)', 'n') == 'j':
                    warte('Horn-Regler ganz zu'); rec2, start = sweep_messen(dev, 0.5, ordner, 'impedanz-box-horn-zu', notizen)
                    f, z = impedanz_auswerten(rec_cal, rec2, start, notizen['r_vor'], notizen['r_cal'])
                    o2 = os.path.join(ordner, 'horn-zu'); os.makedirs(o2, exist_ok=True)
                    for t in impedanz_bericht(f, z, o2): print('  ' + t)
            elif w in ('3', '4', '5', '6', '7'):
                name = {'3': 'nahfeld-membran', '4': 'nahfeld-port', '5': '1m-horn-voll', '6': '1m-horn-zu'}.get(w) or frage('Name der Messung (ohne Leerzeichen)', 'eigene')
                if 'teiler' not in notizen:
                    print('\nVERDRAHTUNG SCHALLDRUCK:')
                    print('  Verstärker-Lautsprecherausgang → Box (Full Range). Dein Yamaha B100: Minus-Klemme liegt an Masse, das ist in Ordnung.')
                    print('  (Brückenverstärker/Class-D mit zwei heißen Klemmen: NICHT so anschließen – vorher fragen.)')
                    print('  Spannungsteiler an den Boxenklemmen: Plus → 10 kΩ → Abgriff → 1 kΩ → Minus. Abgriff an Eingang 2 (TS, Spitze), Minus an den Schaft.')
                    print('  Teilerverhältnis 1 kΩ/(10 kΩ + 1 kΩ) = 0,0909 (mit gemessenen Werten eintragen).')
                    print('  Interface OUTPUT L → Verstärkereingang (Instrumenteneingang, alle Klangregler auf Mitte/aus, Lautstärke zuerst ganz zu).')
                    print('  SM7 an Eingang 1 (XLR): Schalter am SM7 beide flach (Bass-Rolloff aus, Presence aus). Kein 48 V.')
                    print('  Lautstärke am Verstärker langsam so weit auf, dass der Testton deutlich hörbar ist (Zimmerlautstärke+); Eingang 2 soll −20 … −6 dBFS zeigen.')
                    r_hi = zahl('Teiler: oberer Widerstand in Ohm', 10000); r_lo = zahl('Teiler: unterer Widerstand in Ohm', 1000)
                    notizen['teiler'] = r_lo / (r_hi + r_lo); notizen['teiler_widerstaende'] = [r_hi, r_lo]
                    notizen['verstaerker'] = frage('Welcher Verstärker', 'Yamaha B100'); speichern()
                tipp = {'3': 'Mikro 1–2 cm vor der Membran eines Zwölfers (halber Weg zwischen Staubkappe und Sicke), Horn-Regler egal',
                        '4': 'Mikro mittig in der Portöffnung, etwa bündig mit der Frontplatte',
                        '5': 'Mikro 1,00 m vor der Frontplatte, auf Höhe und Achse des Horns; Horn-Regler voll auf; Box möglichst frei im Raum (vom Boden heben oder kippen)',
                        '6': 'wie 5, Horn-Regler ganz zu (bzw. Horn-Schalter aus)'}.get(w, 'Aufstellung nach Wunsch')
                abstand = frage('Abstand/Position (Notiz)', tipp)
                warte(f'{tipp}\nLautstärke am Verstärker unverändert lassen zwischen den Messungen 3–7')
                rec, start = sweep_messen(dev, 0.5, ordner, name, notizen)
                notizen['aufnahmen'][name]['position'] = abstand
                fenster = 0.3 if w in ('3', '4') else 0.02
                for t in akustik_auswerten(rec, start, notizen['teiler'], fenster, ordner, name): print('  ' + t)
                speichern()
            else: print('  ?')
        except KeyboardInterrupt:
            print('\n  abgebrochen'); speichern()

def selbsttest():
    """Erfundene Box: Impedanz eines Bassreflex-Zweihöckers und ein Übertragungsweg; prüft Kalibrierung, Deconvolution, Auswertung."""
    print('Selbsttest ohne Interface …'); x, start = sweep()
    r_vor, r_cal = 100.0, 10.0; g1, g2 = 0.8, 1.3    # unterschiedliche Eingangsverstärkungen, die die Kalibrierung herausrechnen muss
    def zbox(f):   # zwei Resonanzen (Bassreflex: Höcker bei 35 und 75 Hz) + Schwingspule
        s = 2j * np.pi * f
        z = 6.2 + s * 0.5e-3
        for f0, q, rmax in ((35, 4, 40), (75, 4, 50)): w0 = 2 * np.pi * f0; z = z + rmax * (s / (q * w0)) / (1 + s / (q * w0) + (s / w0) ** 2)
        return z
    def durch(xsig, Hf):   # lineares System im Frequenzbereich
        n = len(xsig); X = np.fft.rfft(xsig, 2 * n); f = np.fft.rfftfreq(2 * n, 1 / FS); return np.fft.irfft(X * Hf(f), 2 * n)[:n]
    v1 = x * g1 * 0.5
    rec_cal = np.column_stack([v1, x * g2 * 0.5 * r_cal / (r_vor + r_cal)])
    rec_box = np.column_stack([v1, durch(x * g2 * 0.5, lambda f: zbox(f) / (r_vor + zbox(f)))])
    f, z = impedanz_auswerten(rec_cal, rec_box, start, r_vor, r_cal)
    soll = zbox(f); err = np.max(np.abs(np.abs(z) - np.abs(soll))[(f > 20) & (f < 10000)])
    o = '/tmp/neo212-selbsttest'; os.makedirs(o, exist_ok=True)
    for t in impedanz_bericht(f, z, o): print('  ' + t)
    print(f'  Impedanz: größter Fehler 20 Hz … 10 kHz {err:.3f} Ω (soll < 0,5)')
    # Akustik: Hochpass 2. Ordnung 50 Hz + Tiefpass 4 kHz, Mikrofon mit 2 ms Laufzeit, Teiler 1/11
    teiler = 1 / 11
    Hak = lambda f: ((2j * np.pi * f / (2 * np.pi * 50)) ** 2 / (1 + 2j * np.pi * f / (2 * np.pi * 50) / 0.7 + (2j * np.pi * f / (2 * np.pi * 50)) ** 2)) / (1 + 2j * np.pi * f / (2 * np.pi * 4000)) * np.exp(-2j * np.pi * f * 0.002)
    klemm = x * 0.3; rec = np.column_stack([durch(klemm, Hak) * 0.5, klemm * teiler * 0.9])
    for t in akustik_auswerten(rec, start, teiler, 0.3, o, 'selbsttest'): print('  ' + t)
    d = np.loadtxt(os.path.join(o, 'selbsttest.csv'), delimiter=';', skiprows=1); fz, mag = d[:, 0], d[:, 1]
    soll_db = 20 * np.log10(np.abs(Hak(fz)) * 0.5 / 0.9)   # Mikro = 0,5·H·Klemmenspannung, Referenzkanal = 0,9·Teiler·Klemmenspannung
    e2 = np.max(np.abs(mag - soll_db)[(fz > 30) & (fz < 15000)])
    print(f'  Schalldruck: größter Fehler 30 Hz … 15 kHz {e2:.2f} dB (soll < 0,3)')
    print(f'  Bilder/CSV in {o}')

if __name__ == '__main__': main()
