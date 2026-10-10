# Klangregler-Simulatoren (Yamaha B100-II, Laney AOR 50)

Stand: 2026-10-05. Vollständige Quellen: Web, Apps, Plugins (JSFX und CLAP), C++-Rechenkerne, Skripte.

## Aufbau

| Ordner | Inhalt | bearbeitbar als |
|---|---|---|
| `web/tonestack/` | Apache-Fassung (Übersicht, Yamaha, Laney), so wie auf borg.fluxstatus.de | HTML/JS direkt; Rechenkerne als `*-worklet.js`, WebAssembly-Kerne als Base64 in den `index.html` eingebettet |
| `web/*_tonestack.html` | Einzeldatei-Fassungen (für claude.ai) | HTML/JS direkt |
| `app/laney/`, `app/yamaha/` | Android-Apps, mit apktool zerlegt (Smali-Code + `assets/index.html`) | Smali und HTML; Bau siehe unten |
| `reaper-jsfx/` | JSFX-Plugin (Quelltext = die `.jsfx`-Datei) | Text |
| `reaper-clap/` | CLAP-Plugin, ausgelieferte Fassung (Quellen in `engine/`) | – |
| `scripts/` | `install-tonestack.sh`, `update-tonestack.sh` | Shell |
| `docs/` | Bass-Umbau (Kondensatoren), PDF | – |

## Rechenkerne (C++): `engine/`

Ein gemeinsamer C++-Kern für alle Fassungen: CLAP-Plugin (nativ) und WebAssembly-Kerne für Web und Apps.

| Pfad | Inhalt |
|---|---|
| `engine/src/core.hpp` | Laney: lineares Netz, Röhrenkern (Koren 12AX7, Newton), Endstufe mit Presence, Überabtastung, Überblendung |
| `engine/src/yamaha_core.hpp` | Yamaha B100-II: PA-Board als lineares Netz (Transistoren, Gyrator-Bänder) |
| `engine/clap/` | CLAP-Plugin (`plugin.cpp`) und X11/Cairo-Oberfläche (`gui.hpp`) |
| `engine/wasm/` | C-Schnittstellen für WebAssembly (`wasm_api.cpp` Laney, `yamaha_wasm.cpp` Yamaha) |
| `engine/tools/` | `extract.js` (Bauteile, Geometrie, Foto aus den Web-Seiten), `gen_geo.py`, `embed_wasm.py`, `patch_tubes.py` (einmaliger Einbau der Röhrenauswahl in Web/App) |
| `engine/gen/` | erzeugte Dateien (nicht von Hand ändern; Quelle sind die Web-Seiten) |
| `engine/tests/` | `harness` (Kern), `hosttest` (CLAP-Host), `guitest` (Oberfläche unter Xvfb), `wasmcompare.js` |
| `engine/third_party/` | CLAP-Header 1.2.2 (MIT), stb_image (Public Domain/MIT) |

Bauen und prüfen:

    WASI=/opt/wasi-sdk bash engine/build.sh     # CLAP, WebAssembly, Testwerkzeuge → engine/build/
    bash engine/verify.sh                       # Vergleich mit den ausgelieferten Programmen
    python3 engine/tools/embed_wasm.py          # neue WebAssembly-Kerne in Web-Seiten und App-Inhalte einbetten

Voraussetzungen: g++ 13 (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; wasi-sdk 24 für WebAssembly.

Nachweis der Wiederherstellung (2026-10-05): Die Quelltexte wurden aus dem Arbeitsverlauf rekonstruiert.
Neu gebaut sind das CLAP-Plugin 2.9 und der Yamaha-WebAssembly-Kern **Byte für Byte identisch** mit den
ausgelieferten Programmen; der Laney-WebAssembly-Kern war vor der Yamaha-Ergänzung des gemeinsamen Kerns
ebenfalls Byte für Byte identisch und ist im heutigen Stand im Klang bitgleich (siehe `verify.sh`).

## Apps bauen

    java -jar apktool.jar b app/laney  -o laney_unsigned.apk
    java -jar apktool.jar b app/yamaha -o yamaha_unsigned.apk
    java -jar uber-apk-signer.jar -a laney_unsigned.apk -a yamaha_unsigned.apk \
         --ks tonestack-release.jks --ksAlias tonestack --ksPass PASSWORT --ksKeyPass PASSWORT -o signed

Vor jedem Bau `versionCode` und `versionName` in `app/*/apktool.yml` erhöhen (Werkzeuge: apktool 2.10, uber-apk-signer 1.3; Stand 2.9 = versionCode 20).
Der Signaturschlüssel gehört **nicht** in dieses Repository (siehe `.gitignore`); er liegt separat in `App-Signaturschluessel.zip`.

## Web einspielen

`tonestack/` als ZIP packen (`zip -qr9 tonestack-apache.zip tonestack` im Ordner `web/`) und mit `scripts/update-tonestack.sh` einspielen.
