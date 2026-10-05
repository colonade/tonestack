# Klangregler-Simulatoren (Yamaha B100-II, Laney AOR 50)

Stand: 2026-10-05. Dieses Repository enthält alles, was aus den ausgelieferten Dateien wiederherstellbar war.

## Aufbau

| Ordner | Inhalt | bearbeitbar als |
|---|---|---|
| `web/tonestack/` | Apache-Fassung (Übersicht, Yamaha, Laney), so wie auf borg.fluxstatus.de | HTML/JS direkt; Rechenkerne als `*-worklet.js`, WebAssembly-Kerne als Base64 in den `index.html` eingebettet |
| `web/*_tonestack.html` | Einzeldatei-Fassungen (für claude.ai) | HTML/JS direkt |
| `app/laney/`, `app/yamaha/` | Android-Apps, mit apktool zerlegt (Smali-Code + `assets/index.html`) | Smali und HTML; Bau siehe unten |
| `reaper-jsfx/` | JSFX-Plugin (Quelltext = die `.jsfx`-Datei) | Text |
| `reaper-clap/` | CLAP-Plugin, **nur das fertige Binärprogramm** | – |
| `scripts/` | `install-tonestack.sh`, `update-tonestack.sh` | Shell |
| `docs/` | Bass-Umbau (Kondensatoren), PDF | – |

## Nicht wiederherstellbar (beim Zurücksetzen der Arbeitsumgebung verloren)

- C++-Quelltexte des CLAP-Plugins und der WebAssembly-Kerne (`core.hpp`, `yamaha_core.hpp`, Plugin, Oberfläche).
  Die WebAssembly-Module selbst stecken weiterhin eingebettet in den Seiten; das CLAP-Plugin läuft unverändert.
- Erzeugungsskripte (Netzlisten- und JSFX-Generatoren, Build- und Testskripte).

## Apps bauen

    java -jar apktool.jar b app/laney  -o laney_unsigned.apk
    java -jar apktool.jar b app/yamaha -o yamaha_unsigned.apk
    java -jar uber-apk-signer.jar -a laney_unsigned.apk -a yamaha_unsigned.apk \
         --ks tonestack-release.jks --ksAlias tonestack --ksPass PASSWORT --ksKeyPass PASSWORT -o signed

Vor jedem Bau `versionCode` und `versionName` in `app/*/apktool.yml` erhöhen.
Der Signaturschlüssel gehört **nicht** in dieses Repository (siehe `.gitignore`); er liegt separat in `App-Signaturschluessel.zip`.

## Web einspielen

`tonestack/` als ZIP packen (`zip -qr9 tonestack-apache.zip tonestack` im Ordner `web/`) und mit `scripts/update-tonestack.sh` einspielen.
