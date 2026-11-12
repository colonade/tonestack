# Klangregler-Simulatoren (Lametta B100-II, Laney AOR 50)

Vollständige Quellen: Web, Plugins (CLAP, VST3, Audio Unit für den Mac), C++-Rechenkerne, Skripte. Fertige Plugins stehen nicht im Repo, sondern im [GitHub-Release](https://github.com/colonade/tonestack/releases/latest) (nur die neueste Fassung; ältere lassen sich aus dem jeweiligen Commit neu bauen). Projektstand, Versionen und offene Punkte: `docs/PROJEKTSTAND.md`.

## Aufbau

| Ordner | Inhalt | bearbeitbar als |
|---|---|---|
| `web/tonestack/` | Apache-Fassung (Übersicht, Lametta, Laney) wie auf borg.fluxstatus.de; `download/` wird erst beim Packen (`scripts/pack-web.sh`) mit den Release-Dateien gefüllt; `LIESMICH.txt` beschreibt die Dateien | HTML/JS direkt; Rechenkerne als `*-worklet.js`, WebAssembly-Kerne als Base64 in den `index.html` eingebettet |
| `reaper-clap/` | READMEs der Plugins (gelten für CLAP, VST3 und Mac; kommen als `README.txt` in die ZIPs) und `THIRD-PARTY-LICENSES.txt`; Quellen in `engine/` | Text |
| `mac/` | Mac-Bau (läuft auf GitHub Actions, `.github/workflows/plugins.yml`): `package.sh`, `make_pkg.sh` (Installerpaket), `LIESMICH-Mac.txt`, Prüfbilder (`test/`) | Shell |
| `scripts/` | `install-tonestack.sh` (Apache), `update-tonestack.sh` (Web, Plugins), `pack-plugins.sh`, `release.sh`, `pack-web.sh`, `versions.sh` | Shell |
| `docs/` | `PROJEKTSTAND.md` (Übergabe), Bass-Umbau (Kondensatoren) als PDF | – |

## Rechenkerne (C++): `engine/`

Ein gemeinsamer C++-Kern für alle Fassungen: CLAP-Plugins (nativ, daraus VST3 und Audio Unit) und WebAssembly-Kerne für das Web.

| Pfad | Inhalt |
|---|---|
| `engine/src/core.hpp` | Laney: lineares Netz, Röhrenkern (Koren-Modelle, Newton), Endstufe mit Presence, Überabtastung, Überblendung; Lautsprechermodell (auch vom Lametta B100-II genutzt) |
| `engine/src/yamaha_core.hpp` | Lametta B100-II: PA-Board als lineares Netz (Transistoren, Gyrator-Bänder), Endstufe (MA-Board) mit Begrenzung |
| `engine/clap/` | CLAP-Plugins: Laney `plugin.cpp` + `gui.hpp`, Lametta B100-II `yplugin.cpp` + `ygui.hpp` (Oberflächen mit Cairo gezeichnet: unter Linux im X11-Fenster, auf dem Mac über `cairo_cg.hpp` mit CoreGraphics in `macview.mm`), gemeinsame Bedienelemente `widgets.hpp`, X11-Absicherung `x11safe.hpp`, `platform.hpp` |
| `engine/vst3/` | VST3- und (auf dem Mac) Audio-Unit-Bau: `CMakeLists.txt` holt clap-wrapper (fester Commit), VST3-SDK 3.8 (MIT) und AudioUnitSDK (Apache-2.0), baut die Plugin-Quellen als statische Bibliothek; Einstiegsdateien, `exports.map` |
| `engine/wasm/` | C-Schnittstellen für WebAssembly (`wasm_api.cpp` Laney, `yamaha_wasm.cpp` Lametta B100-II) |
| `engine/tools/` | `extract.js` (Bauteile, Geometrie, Fotos aus den Web-Seiten), `gen_geo.py`, `embed_wasm.py` |
| `engine/gen/` | erzeugte Dateien (nicht von Hand ändern; Quelle sind die Web-Seiten; werden von `build.sh` neu erzeugt und mit eingecheckt) |
| `engine/tests/` | `harness` (Kern), `hosttest` (CLAP-Host), `defaults`, `guitest`/`yguitest` (Oberflächen unter Xvfb), `clapstress` (Belastung wie in REAPER), `switchtest` (Moduswechsel, mit AddressSanitizer), `wasmcompare.js`/`ywasmcompare.js` |
| `engine/third_party/` | CLAP-Header 1.2.2 (MIT), stb_image (Public Domain/MIT) |

Bauen und prüfen:

    WASI=/opt/wasi-sdk bash engine/build.sh     # CLAP, WebAssembly, Testwerkzeuge → engine/build/
    VST3=1 bash engine/build.sh                 # zusätzlich VST3 → engine/build/vst3/ (cmake, ninja; lädt beim ersten Mal die SDKs aus GitHub)
    python3 engine/tools/embed_wasm.py          # neue WebAssembly-Kerne in die Web-Seiten einbetten
    bash engine/verify.sh                       # Vergleich mit den eingebetteten Kernen und den Plugin-Paketen in dist/

Voraussetzungen: g++ 13 (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; wasi-sdk 24 für WebAssembly; cmake ≥ 3.21 und ninja für VST3.

Neue Plugin-Fassung ausliefern:
1. Version `TS_VERSION` in `engine/clap/plugin.cpp` bzw. `yplugin.cpp`, in `engine/vst3/CMakeLists.txt` und im README (`reaper-clap/`) anheben; Links und Versionen in `web/tonestack/index.html`, `VERSION` in `web/tonestack/sw.js`.
2. Lokal prüfen: `VST3=1 bash engine/build.sh`, `bash scripts/pack-plugins.sh` (→ `dist/`), `bash engine/verify.sh`.
3. Committen und pushen. Der Bau auf GitHub (`.github/workflows/plugins.yml`) baut Linux (CLAP, VST3) und Mac (Audio Unit, VST3, Installerpaket), prüft und veröffentlicht alles als Release `laney-<V>_b100ii-<V>_…`; ältere Releases werden gelöscht (`scripts/release.sh`).
4. `bash scripts/pack-web.sh` → `web/tonestack-apache.zip` (Seiten plus Release-Dateien in `download/`).

Installation beim Nutzer: `update-tonestack.sh --nur plugins` (CLAP nach `~/.clap`, VST3 nach `~/.vst3`).

## Bau auf GitHub

`.github/workflows/plugins.yml` läuft bei jedem Push auf `main` mit Änderungen an `engine/`, `mac/`, `reaper-clap/` oder den Paket-Skripten (oder von Hand: Actions → Plugins → Run workflow). Linux: Ubuntu 24.04, `VST3=1 bash engine/build.sh`, kurze Prüfung, `scripts/pack-plugins.sh`. Mac: Plugins bauen, mit `auval` prüfen, ad hoc signieren, Installerpaket bauen und probehalber installieren. Die Bau-Jobs haben nur Leserechte; der Veröffentlichungsschritt prüft die SHA-256-Summen und lädt alles ins Release (ZIPs, `.pkg`, Prüfbilder `mac-*.png`, Summen). Das Mac-Paket ist nicht bei Apple beglaubigt; die Freigabe beschreibt `mac/LIESMICH-Mac.txt`.

Signaturschlüssel und andere Zugangsdaten gehören **nicht** in dieses Repository (siehe `.gitignore`).

## Web einspielen

`bash scripts/pack-web.sh` baut `web/tonestack-apache.zip`; einspielen mit `scripts/update-tonestack.sh --nur web` (erstmalig: `scripts/install-tonestack.sh`).
