# Klangregler-Simulatoren (Yamaha B100-II, Laney AOR 50)

Vollständige Quellen: Web, Apps, Plugins (CLAP, VST3, JSFX), C++-Rechenkerne, Skripte. Projektstand, Versionen und offene Punkte: `docs/PROJEKTSTAND.md`.

## Aufbau

| Ordner | Inhalt | bearbeitbar als |
|---|---|---|
| `web/tonestack/` | Apache-Fassung (Übersicht, Yamaha, Laney, `download/` mit den CLAP-Paketen), so wie auf borg.fluxstatus.de; `LIESMICH.txt` beschreibt die Dateien | HTML/JS direkt; Rechenkerne als `*-worklet.js`, WebAssembly-Kerne als Base64 in den `index.html` eingebettet |
| `app/laney/`, `app/yamaha/` | Android-Apps, mit apktool zerlegt (Smali-Code + `assets/index.html`), eingefroren auf Stand 2.10 | Smali und HTML; Bau siehe unten |
| `reaper-clap/` | CLAP-Plugins, ausgelieferte Fassungen (`Laney_AOR50.clap`, `Yamaha_B100II.clap`), die READMEs (gelten für CLAP und VST3) und `THIRD-PARTY-LICENSES.txt`; Quellen in `engine/` | – |
| `reaper-vst3/` | VST3-Fassungen derselben Plugins (Ordner-Bundles `*.vst3`), gebaut aus denselben Quellen über clap-wrapper | – |
| `reaper-jsfx/` | JSFX-Plugin (eingefrorener Rückfall; Quelltext = die `.jsfx`-Datei) | Text |
| `scripts/` | `install-tonestack.sh` (Apache), `update-tonestack.sh` (Web, Apps, JSFX, CLAP) | Shell |
| `docs/` | `PROJEKTSTAND.md` (Übergabe), Bass-Umbau (Kondensatoren) als PDF | – |

## Rechenkerne (C++): `engine/`

Ein gemeinsamer C++-Kern für alle Fassungen: CLAP-Plugins (nativ) und WebAssembly-Kerne für Web und Apps.

| Pfad | Inhalt |
|---|---|
| `engine/src/core.hpp` | Laney: lineares Netz, Röhrenkern (Koren-Modelle, Newton), Endstufe mit Presence, Überabtastung, Überblendung; Lautsprechermodell (auch vom Yamaha genutzt) |
| `engine/src/yamaha_core.hpp` | Yamaha B100-II: PA-Board als lineares Netz (Transistoren, Gyrator-Bänder), Endstufe (MA-Board) mit Begrenzung |
| `engine/clap/` | CLAP-Plugins: Laney `plugin.cpp` + `gui.hpp`, Yamaha `yplugin.cpp` + `ygui.hpp` (X11/Cairo-Oberflächen) |
| `engine/vst3/` | VST3-Bau: `CMakeLists.txt` holt clap-wrapper (fester Commit) und VST3-SDK 3.8 (MIT), baut die Plugin-Quellen mit `TONESTACK_CLAP_FIRST` als statische Bibliothek; Einstiegsdateien, `exports.map` |
| `engine/wasm/` | C-Schnittstellen für WebAssembly (`wasm_api.cpp` Laney, `yamaha_wasm.cpp` Yamaha) |
| `engine/tools/` | `extract.js` (Bauteile, Geometrie, Fotos aus den Web-Seiten), `gen_geo.py`, `embed_wasm.py` |
| `engine/gen/` | erzeugte Dateien (nicht von Hand ändern; Quelle sind die Web-Seiten; werden von `build.sh` neu erzeugt und mit eingecheckt) |
| `engine/tests/` | `harness` (Kern), `hosttest` (CLAP-Host), `defaults`, `guitest`/`yguitest` (Oberflächen unter Xvfb), `wasmcompare.js`/`ywasmcompare.js` |
| `engine/third_party/` | CLAP-Header 1.2.2 (MIT), stb_image (Public Domain/MIT) |

Bauen und prüfen:

    WASI=/opt/wasi-sdk bash engine/build.sh     # CLAP, WebAssembly, Testwerkzeuge → engine/build/
    VST3=1 bash engine/build.sh                 # zusätzlich VST3 → engine/build/vst3/ (cmake, ninja; lädt beim ersten Mal die SDKs aus GitHub)
    bash engine/verify.sh                       # Vergleich mit den ausgelieferten Programmen und Download-Paketen
    python3 engine/tools/embed_wasm.py          # neue WebAssembly-Kerne in Web-Seiten und App-Inhalte einbetten

Voraussetzungen: g++ 13 (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; wasi-sdk 24 für WebAssembly; cmake ≥ 3.21 und ninja für VST3.
`embed_wasm.py` schreibt auch in `app/*/assets/index.html`; solange die Apps nicht neu gebaut werden, diese Änderung mit `git checkout app/laney/assets/index.html app/yamaha/assets/index.html` zurücknehmen.

Neue Plugin-Fassung ausliefern: Version in `DESC` (plugin.cpp/yplugin.cpp), in `engine/vst3/CMakeLists.txt` und im README anheben; `VST3=1 bash engine/build.sh`; `engine/build/*.clap` nach `reaper-clap/`, `engine/build/vst3/*.vst3` nach `reaper-vst3/`; ZIP je Verstärker (`<Name>.clap`, Ordner `<Name>.vst3`, README als `README.txt`, `THIRD-PARTY-LICENSES.txt`) als `<Name>_Plugin-<Version>.zip` nach `web/tonestack/download/`; Link und Version in `web/tonestack/index.html`, `VERSION` in `web/tonestack/sw.js` erhöhen; `bash engine/verify.sh`. Installation beim Nutzer: `update-tonestack.sh --nur plugins` (CLAP nach `~/.clap`, VST3 nach `~/.vst3`).

## Apps bauen

    java -jar apktool.jar b app/laney  -o laney_unsigned.apk
    java -jar apktool.jar b app/yamaha -o yamaha_unsigned.apk
    java -jar uber-apk-signer.jar -a laney_unsigned.apk -a yamaha_unsigned.apk \
         --ks tonestack-release.jks --ksAlias tonestack --ksPass PASSWORT --ksKeyPass PASSWORT -o signed

Vor jedem Bau `versionCode` und `versionName` in `app/*/apktool.yml` erhöhen (Werkzeuge: apktool 2.10, uber-apk-signer 1.3; Stand 2.10 = versionCode 21).
Der Signaturschlüssel gehört **nicht** in dieses Repository (siehe `.gitignore`); er liegt separat in `App-Signaturschluessel.zip`.

## Web einspielen

`tonestack/` als ZIP packen (`zip -qr9 tonestack-apache.zip tonestack` im Ordner `web/`) und mit `scripts/update-tonestack.sh --nur web` einspielen (erstmalig: `scripts/install-tonestack.sh`).
