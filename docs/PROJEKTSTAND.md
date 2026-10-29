# Klangregler-Simulatoren: Projektstand und Übergabe (2026-10-10)

Lies diese Datei zuerst, wenn das Projekt in einem neuen Gespräch weitergeht. Sie fasst alles zusammen, was aus dem bisherigen Verlauf wichtig ist.

## Was es ist
Simulation zweier Bassverstärker von Nutzer: **Yamaha B100-115II** (Transistor-Combo 1979) und **Laney AOR 50** (Röhrenverstärker 1985, Schaltplan DRG 1054). Drei Fassungen, die alle denselben C++-Rechenkern benutzen:
- **Web** auf https://borg.fluxstatus.de/tonestack/ (Apache, Debian, Pfad /var/www/html/tonestack; Skripte `scripts/install-tonestack.sh`, `scripts/update-tonestack.sh` liegen auf dem Server im Heimatordner). Einspielen: ZIP in den Heimatordner, `bash ~/update-tonestack.sh --nur web`.
- **Android-Apps** (`app/laney`, `app/yamaha`; apktool-Projekte): WebView mit eingebetteter Seite, Audio-Fokus, eigener Signaturschlüssel (**nicht im Repository**; der Nutzer hat `App-Signaturschluessel.zip` mit Passwort). Stand 2.10 = versionCode 21.
- **CLAP-Plugin** für REAPER 7.81 unter Linux x86-64 (`reaper-clap/Laney_AOR50.clap`, Stand 3.4): X11/Cairo-Oberfläche mit Foto der Frontplatte, Stereo, Aussteuerungsanzeige, Frequenzgang, Schlafzustand bei Stille, Röhrenwahl. Installation: nach ~/.clap, REAPER neu scannen. Das ältere **JSFX-Plugin** (`reaper-jsfx/`) ist eingefrorener Rückfall (zu langsam für > 2-fach).

## Arbeitsweise
- Alles liegt in **Git-Repository des Nutzers** `~/git/tonestack`. Zu Beginn jeder Runde lädt er ein Bundle hoch (`git -C ~/git/tonestack bundle create ~/tonestack.bundle --all`), bei App-Änderungen zusätzlich `App-Signaturschluessel.zip`. Am Ende liefere ich ein Bundle (`git bundle create tonestack.bundle <letzter bekannter Commit>..main`), er übernimmt mit `git pull ~/Downloads/tonestack.bundle main`.
- Meine Arbeitsumgebung wird zwischen Gesprächen zurückgesetzt; nichts darf nur dort liegen.
- Bauen: `WASI=/pfad/wasi-sdk bash engine/build.sh` (g++ 13, node, python3, libcairo2-dev, libx11-dev; wasi-sdk 24 für WebAssembly), `bash engine/verify.sh` vergleicht mit den ausgelieferten Programmen. `engine/tools/embed_wasm.py` bettet neue WebAssembly-Kerne in Web und App ein. Apps: apktool 2.10 bauen, uber-apk-signer 1.3 signieren; vorher versionCode/versionName in `app/*/apktool.yml` erhöhen.
- Tests: `engine/tests/` (Kern-Testprogramm `harness`, CLAP-Host `hosttest`, Oberfläche `guitest` unter Xvfb, `defaults`, WebAssembly-Klangvergleiche). Web-Seiten prüfe ich mit Playwright (Chromium) über einen lokalen http.server.
- Sprache: der Nutzer schreibt Deutsch; Oberfläche und README des CLAP-Plugins sind auf Englisch, Web und Apps auf Deutsch.

## Modell (Laney), Kurzfassung
- Lineares Netz (Knotenanalyse, Trapezregel) des ganzen Vorverstärkers V101A…V103B mit Klangregler, Zugschaltern, Kanalumschaltung (VT1/VT2-Optokoppler), Preamp 1 Level, Master; Endstufe (Phasenumkehr V104, Gegenkopplung R38/R105/VR108/C110, Presence) linear mit Schätzwert Gp = 0,45 (am Gerät eichbar: Sinus 5 kHz, Presence 0 gegen 10, Differenz in dB).
- Röhrenkern: Koren-Modell, Schaltung in 5 Abschnitte zerlegt, Newton je Abschnitt, Überabtastung 1/2/4/8-fach. **Röhrenwahl je Fassung** V101/V102/V103: 12AX7, 12AT7, 12AU7 (Koren-Parameter), 5751 und 12AY7 (an Datenblatt-Arbeitspunkt angepasst, als "angepasst" gekennzeichnet). Arbeitspunkt per Fortsetzung von der 12AX7; Kleinsignalwerte für lineare Betriebsart und Diagramm aus dem Gleichstromlöser.
- Bass-Umbau (PDF in `docs/`): C3 680 nF → 22 µF, C14 22 nF → 220 nF, C2 22 nF → 100 nF.
- Grundstellung überall: HIGH, Preamp 1 (PULL ON), alle Regler 5, Verzerrung an, 8-fach, Bass-Umbau an, Effektweg + Endstufe an, 3 × ECC83.
- Offen/unsicher: Gp (Übertragerdaten fehlen), Yamaha 60-Hz-Band rechnet 43 Hz, Yamaha Middle-Hub 3,3 dB gegen 6 ± 3 dB im SM.

## Prüfung der Verzerrung (2026-10-29)
Anlass: Ein Nutzer hörte bei einer Gitarrendatei „nur die Spitzen stark verzerrt, der Rest sauber". Gemessen am C++-Kern: Löser konvergiert in jedem Teilschritt, keine numerischen Sprünge; die steilen Flanken entstehen in der Schaltung (V102A aus der Sperrung, V102B kippt, Kathodenfolger). Übergang sauber → stark verzerrt liegt innerhalb von 6 bis 12 dB Eingangspegel (Preamp 2, alles 5: Körper −12 dB THD −36 dB, Spitze 100 mV THD −15 dB; Preamp 1 Volume 1/1: −28 / −12 dB; LOW-Eingang −67 dB). Normalisierung der Datei ist unerheblich, weil der Spitzenwert immer dem Regler „Spitzenpegel" gleichgesetzt wird. Unsicher bleiben VT1-Durchlasswiderstand und fehlende Netzteil-Nachgiebigkeit. Werkzeuge dazu im Verlauf: Sinus-THD-Tabelle und Burst-Test mit `engine/build/harness`.

Gitterstopper vor V102B/V103B wurden am 2026-10-29 im Kern eingebaut, gemessen (wirkungslos: < 0,3 dB THD, weil 470 kΩ/1 MΩ schon vor den Gittern liegen; die Spitzen entstehen durch Sperren der Röhren) und auf Wunsch wieder entfernt (Commit f84a11a, Revert fbbbc0b). Wirksam gegen den harten Einsatz: C3 an V101B weglassen (Preamp 2: Spitze −16 → −31 dB THD, −2 dB Pegel), 5751/12AY7 in V101, LOW-Eingang. Vorschlag für den echten Umbau: SPST-Schalter in Reihe mit C3, 1 MΩ parallel zum Schalter gegen Knacken; optional Rpad 2,2 MΩ vor VR102 (−10 dB, verschiebt nur den Einsatz; Sanft + Später in Preamp 2 alles 5: Spitze −45 dB THD bei −10,7 dB Pegel).
Mit der Datei des Nutzers (Strum, Crest 24 dB, Spitzen = gedämpfte Anschläge, 74 % der Zeit unter −20 dB): Preamp 2 alles 5 folgt der Hüllkurve fast exakt, nur die obersten 6 dB jedes Anschlags werden begrenzt; Preamp 1 Volume 2/2 komprimiert die Ausklingphase auf −20…−30 dB. Verhalten der Schaltung, kein Fehler.

## Ideen, die besprochen, aber nicht gebaut wurden
- CLAP-Plugin für den Yamaha (Kern vorhanden, Parameter und Oberfläche fehlen).
- Logic (AU) über ein JUCE-Projekt, zu bauen auf einem Mac; Pro Tools (AAX) wegen Avid-Signierung nicht sinnvoll.
- Farben für die Röhrentasten im CLAP-Plugin (Web/App haben sie: grau, grün, blau, orange, violett).
- Englische Fassung von Web und App.
- Apps (Laney, Yamaha) auf den Web-Stand 2026-10-29-3 bringen (Bypass, Download, Voreinstellungen, Yamaha-Buchsen); braucht Signaturschlüssel. Yamaha-CLAP: Eingangsfaktor ING wäre dort als Parameter zu ergänzen.

## Versionsstand
Web 2026-10-29-6 (Polarität: Ausgang beider Simulatoren jetzt gleichphasig zum Eingang, vorher invertiert (Hinweis eines Mac-Nutzers); Vorzeichen in Engine::block, YCore::block und den JS-Hüllen; Laney: Umbauideen als Schalter „Sanft" = C3 an V101B abgeschaltet (UI-seitig C3 → 1 nF) und „Später" = Vorwiderstand Rpad vor Preamp 2 Volume (Bauteil `rpad`, Kern-Standard 0 über p[7] in LANEY_PARTS, 2,2 MΩ in der Seite), beide in Voreinstellungen und Schalter-Tabelle; Laney und Yamaha: Seiten in Tafeln gegliedert (Frequenzgang, Anhören, Einstellungen, Bauteile) mit Sprungleiste und Farbsystem der Tasten: grün = Abspielen, rot = Stopp/Bypass/Löschen, blau = Dateien, ocker = Merken/Voreinstellungen, grau = neutral; Bypass-Taste B mit Pegelangleich, WAV-Download durch die Schaltung (auch Firefox), Voreinstellungen mit Namen inkl. JSON-Export/-Import, Bauteilwerte eingeklappt; Yamaha zusätzlich HIGH/LOW-Buchsen auf dem Foto, LOW = −12,2 dB nach Schaltplan JK-Board und Service Manual Tabelle 3; Apps noch auf Stand 2026-10-10) · Apps 2.10 (versionCode 21) · CLAP 3.5 (Polarität wie Eingang, Latenz 31 Abtastwerte an den Host gemeldet; sonst bitgleich zu 3.4) · JSFX eingefroren · letzter Commit siehe `git log`.
