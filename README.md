# Klangregler-Simulatoren (Lametta B100-II, Laney AOR 50)

Vollständige Quellen: Web, Plugins (CLAP und VST3 für Linux und Windows, VST3 und Audio Unit für den Mac, Debian/Ubuntu-Paket), C++-Rechenkerne, Skripte. Fertige Plugins stehen nicht im Repo, sondern im [GitHub-Release](https://github.com/colonade/tonestack/releases/latest) (nur die neueste Fassung; ältere lassen sich aus dem jeweiligen Commit neu bauen). Projektstand, Versionen und offene Punkte: `docs/PROJEKTSTAND.md`.

## Aufbau

| Ordner | Inhalt | bearbeitbar als |
|---|---|---|
| `web/tonestack/` | Webseite (Übersicht und alle Simulatoren), veröffentlicht auf GitHub Pages unter https://tonestack.fluxstatus.de/; `download/` wird erst beim Packen (`scripts/pack-web.sh`) mit den Release-Dateien gefüllt; `LIESMICH.txt` beschreibt die Dateien | HTML/JS direkt; Rechenkerne als `*-worklet.js`, WebAssembly-Kerne als Base64 in den `index.html` eingebettet |
| `reaper-clap/` | READMEs der Plugins (gelten für CLAP, VST3, Windows und Mac; kommen als `README.txt` in die ZIPs), `THIRD-PARTY-LICENSES.txt`, für Windows zusätzlich `INSTALL-Windows.txt` und `THIRD-PARTY-LICENSES-Windows.txt` (Cairo, pixman, MinGW-Laufzeit); Quellen in `engine/` | Text |
| `mac/` | Mac-Bau (läuft auf GitHub Actions, `.github/workflows/plugins.yml`): `package.sh`, `make_pkg.sh` (Installerpaket), `LIESMICH-Mac.txt`, Prüfbilder (`test/`) | Shell |
| `scripts/` | `update-tonestack.sh` (Plugins; Web nur für einen eigenen Apache), `install-tonestack.sh` (eigener Apache), `borg-umleitung.sh` (alte Apache-Adresse auf die neue umleiten), `plugins.txt` (Liste der Plugins für die Paket- und Prüfskripte), `pack-plugins.sh` (Linux-ZIPs), `pack-deb.sh` (Debian/Ubuntu-Paket), `pack-windows.sh` (Windows-ZIPs und NSIS-Installer), `release.sh` (prüft vor dem Hochladen: gleiche Namen im Release müssen gleiche Bytes haben), `pack-web.sh`, `versions.sh` | Shell |
| `docs/` | `PROJEKTSTAND.md` (Übergabe), Bass-Umbau (Kondensatoren) als PDF | – |

## Rechenkerne (C++): `engine/`

Ein gemeinsamer C++-Kern für alle Fassungen: CLAP-Plugins (nativ, daraus VST3 und Audio Unit) und WebAssembly-Kerne für das Web.

| Pfad | Inhalt |
|---|---|
| `engine/src/core.hpp` | Laney: lineares Netz, Röhrenkern (Koren-Modelle, Newton), Endstufe mit Presence, Überabtastung, Überblendung; Lautsprechermodell (auch vom Lametta B100-II genutzt) |
| `engine/src/yamaha_core.hpp` | Lametta B100-II: PA-Board als lineares Netz (Transistoren, Gyrator-Bänder), Endstufe (MA-Board) mit Begrenzung |
| `engine/clap/` | CLAP-Plugins: Laney `plugin.cpp` + `gui.hpp`, Lametta B100-II `yplugin.cpp` + `ygui.hpp` (Oberflächen mit Cairo gezeichnet: unter Linux im X11-Fenster, auf dem Mac über `cairo_cg.hpp` mit CoreGraphics in `macview.mm`, unter Windows mit echtem Cairo über `cairo_win.hpp` in einem HWND aus `winview.cpp`), gemeinsame Bedienelemente `widgets.hpp`, X11-Absicherung `x11safe.hpp`, `platform.hpp` |
| `engine/vst3/` | VST3- und (auf dem Mac) Audio-Unit-Bau: `CMakeLists.txt` holt clap-wrapper (fester Commit), VST3-SDK 3.8 (MIT) und AudioUnitSDK (Apache-2.0), baut die Plugin-Quellen als statische Bibliothek; Einstiegsdateien, `exports.map`; unter Windows (MinGW) zusätzlich VST3-Bundle mit `Contents/x86_64-win` |
| `engine/win/` | Windows-Bau unter Linux mit MinGW-w64: `build.sh` (alle Plugins, Prüfung von Importen und Exporten), `build-cairo.sh` (statisches Cairo 1.18.0 und pixman 0.42.2 aus den Ubuntu-Quellpaketen), `mingw64.cmake`, `vst3.def`/`clap.def`, `mkrefs.py` und `wincheck.py` (Prüfung auf dem Windows-Rechner von GitHub: Validator, Rechnung gegen Linux, `wingui`, VST3-Oberfläche in `editorhost` aus dem VST3-SDK) |
| `engine/wasm/` | C-Schnittstellen für WebAssembly (`wasm_api.cpp` Laney, `yamaha_wasm.cpp` Lametta B100-II) |
| `engine/tools/` | `extract.js` (Bauteile, Geometrie, Fotos aus den Web-Seiten), `gen_geo.py`, `embed_wasm.py` |
| `engine/gen/` | erzeugte Dateien (nicht von Hand ändern; Quelle sind die Web-Seiten; werden von `build.sh` neu erzeugt und mit eingecheckt) |
| `engine/tests/` | `harness` (Kern), `hosttest` (CLAP-Host), `defaults`, `guitest`/`yguitest` (Oberflächen unter Xvfb), `clapstress` (Belastung wie in REAPER), `switchtest` (Moduswechsel, mit AddressSanitizer), `wasmcompare.js`/`ywasmcompare.js`, `plugtest` (Klang aus der fertigen Plugin-Datei, auch als Windows-Programm), `wingui.cpp` (Oberflächen-Prüfung unter Windows) |
| `engine/third_party/` | CLAP-Header 1.2.2 (MIT), stb_image (Public Domain/MIT) |

Bauen und prüfen:

    WASI=/opt/wasi-sdk bash engine/build.sh     # CLAP, WebAssembly, Testwerkzeuge → engine/build/
    VST3=1 bash engine/build.sh                 # zusätzlich VST3 → engine/build/vst3/ (cmake, ninja; lädt beim ersten Mal die SDKs aus GitHub)
    python3 engine/tools/embed_wasm.py          # neue WebAssembly-Kerne in die Web-Seiten einbetten
    bash engine/win/build.sh                    # Windows-Fassungen (MinGW) → engine/build/win/ (baut beim ersten Mal Cairo, braucht deb-src)
    bash engine/verify.sh                       # Vergleich mit den eingebetteten Kernen und den Plugin-Paketen in dist/ (ZIPs, DEB, Windows-ZIPs)

Voraussetzungen: g++ 13 (C++17), node, python3, pkg-config, libcairo2-dev, libx11-dev; wasi-sdk 24 für WebAssembly; cmake ≥ 3.21 und ninja für VST3; für Windows g++-mingw-w64-x86-64-posix, meson und nsis; für das DEB-Paket dpkg-deb.

Neue Plugin-Fassung ausliefern:
1. Version `TS_VERSION` in `engine/clap/plugin.cpp` bzw. `yplugin.cpp`, in `engine/vst3/CMakeLists.txt` und im README (`reaper-clap/`) anheben; Links und Versionen in `web/tonestack/index.html`, `VERSION` in `web/tonestack/sw.js`.
2. Lokal prüfen: `VST3=1 bash engine/build.sh`, `bash engine/win/build.sh`, `bash scripts/pack-plugins.sh`, `bash scripts/pack-deb.sh`, `bash scripts/pack-windows.sh` (→ `dist/`), `bash engine/verify.sh`. Die DEB-Versionsnummer (`DEBVER` = `SUMVER-DEBREV` aus `scripts/versions.sh`) steckt im Link in `web/tonestack/plugins/index.html` und muss dort mitgezogen werden; `DEBREV` nur erhöhen, wenn sich das Paket ohne neue Plugin-Version ändert. Ein neues Plugin in `scripts/plugins.txt` eintragen.
3. Committen und pushen. Der Bau auf GitHub (`.github/workflows/plugins.yml`) baut Linux (CLAP, VST3, DEB-Paket), Windows (VST3, CLAP, Installer; geprüft auf einem Windows-Rechner von GitHub) und Mac (Audio Unit, VST3, Installerpaket), prüft und veröffentlicht alles als Release `laney-<V>_b100ii-<V>_…`; ältere Releases werden gelöscht (`scripts/release.sh`).
4. Die Webseite veröffentlicht `.github/workflows/pages.yml` nach dem Release von selbst (GitHub Pages).

Installation beim Nutzer: Linux `update-tonestack.sh --nur plugins` (CLAP nach `~/.clap`, VST3 nach `~/.vst3`) oder `sudo apt install ./tonestack-plugins_<V>_amd64.deb` (nach `/usr/lib/clap`, `/usr/lib/vst3`); Windows `tonestack-Plugins-Windows.exe` (VST3 nach `C:\Program Files\Common Files\VST3`, CLAP nach `…\Common Files\CLAP`).

## Bau auf GitHub

`.github/workflows/plugins.yml` läuft bei jedem Push auf `main` mit Änderungen an `engine/`, `mac/`, `reaper-clap/` oder den Paket-Skripten (oder von Hand: Actions → Plugins → Run workflow). Linux: Ubuntu 24.04, `VST3=1 bash engine/build.sh`, kurze Prüfung, Windows-Fassungen mit `engine/win/build.sh`, `scripts/pack-plugins.sh`, `pack-deb.sh`, `pack-windows.sh`. Windows (windows-2025): Steinberg-Validator aus dem VST3-SDK bauen und auf alle VST3 anwenden, Klang der DLLs gegen Linux-Referenzen (`plugtest.exe`), Oberflächen mit `wingui.exe` (Prüfbilder `win-*.png`, Mausrad, Anzeigeskalierung), VST3-Oberflächen im `editorhost` des SDK (so lädt auch Cubase). Der Quelltext von Cairo und pixman (statisch in den Windows-Plugins) liegt als Ubuntu-Quellpaket im Release und auf der Seite unter `download/`. Mac: Plugins bauen, mit `auval` prüfen, ad hoc signieren, Installerpaket bauen und probehalber installieren. Die Bau-Jobs haben nur Leserechte; der Veröffentlichungsschritt prüft die SHA-256-Summen und lädt alles ins Release (ZIPs, `.deb`, `.exe`, `.pkg`, Prüfbilder `mac-*.png` und `win-*.png`, Summen). Das Mac-Paket ist nicht bei Apple beglaubigt; die Freigabe beschreibt `mac/LIESMICH-Mac.txt`. Der Windows-Installer ist nicht signiert (SmartScreen: „Weitere Informationen“ → „Trotzdem ausführen“).

Signaturschlüssel und andere Zugangsdaten gehören **nicht** in dieses Repository (siehe `.gitignore`).

## Webseite (GitHub Pages)

`.github/workflows/pages.yml` veröffentlicht die Seite auf https://tonestack.fluxstatus.de/: nach jedem erfolgreichen Plugin-Bau auf `main`, bei Pushes mit Änderungen unter `web/` und von Hand (Actions → Pages → Run workflow). Er baut die Seite mit `SITE=_site bash scripts/pack-web.sh` (Seiten plus Release-Dateien in `download/`, gegen die Prüfsummen des Releases geprüft). Voraussetzung: Settings → Pages → Source „GitHub Actions“, eigene Domain dort eingetragen, DNS-Eintrag CNAME `tonestack` → `colonade.github.io`.

Eine Fassung für einen eigenen Apache baut `bash scripts/pack-web.sh` weiterhin als `web/tonestack-apache.zip` (einspielen mit `scripts/update-tonestack.sh --nur web`, erstmalig `scripts/install-tonestack.sh`); `.htaccess` und `LIESMICH.txt` gehören nur dazu.
