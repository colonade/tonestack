# Flatpak-Erweiterung org.freedesktop.LinuxAudio.Plugins.Tonestack

DAWs aus Flathub (Ardour, Bitwig, REAPER, Studio Pro …) laufen in einer Sandbox und sehen `/usr/lib/clap` und `/usr/lib/vst3` nicht. Flathub
verteilt Plugins darum als Erweiterungen von `org.freedesktop.LinuxAudio.BaseExtension`: jede liegt unter
`/app/extensions/Plugins/<Name>/` mit den Ordnern `clap/` und `vst3/`, und die Hosts finden sie dort.

Hier liegen die Vorlagen für eine solche Erweiterung (Stand 2026-11-27):

| Datei | Zweck |
|---|---|
| `org.freedesktop.LinuxAudio.Plugins.Tonestack.json.in` | Manifest: baut alle Plugins aus den Quellen eines Commits (ONLY_PLUGINS=1 VST3=1 `engine/build.sh`), ohne Netz – clap-wrapper, CLAP 1.2.10 und VST3-SDK 3.8.0 als feste Git-Quellen |
| `org.freedesktop.LinuxAudio.Plugins.Tonestack.metainfo.xml.in` | AppStream-Daten (`type="addon"`, erweitert die BaseExtension) |

`bash scripts/flatpak-manifest.sh <Ordner>` füllt Commit, Version und Datum ein und legt `flathub.json` dazu. Den Bau prüft
`.github/workflows/flatpak.yml` mit flatpak-builder so, wie Flathub ihn ausführt (Laufzeit 25.08).

## Einreichen (Schritte für den Inhaber; noch nicht geschehen)

1. Mit dem eigenen GitHub-Konto das Repository `flathub/flathub` forken und einen Zweig vom Zweig `new-pr` anlegen.
2. Die drei erzeugten Dateien hineinlegen und einen Pull Request gegen `new-pr` stellen (Titel: „Add org.freedesktop.LinuxAudio.Plugins.Tonestack“).
3. Fragen der Prüfer beantworten; nach der Aufnahme entsteht `flathub/org.freedesktop.LinuxAudio.Plugins.Tonestack`, in dem künftige Versionen
   nur noch Commit, Version und Datum ändern (wieder mit `scripts/flatpak-manifest.sh`).

Nutzer installieren die Erweiterung dann mit `flatpak install flathub org.freedesktop.LinuxAudio.Plugins.Tonestack`.
Bis dahin: das Archiv `tonestack-plugins-<Version>-linux-<arch>.tar.gz` mit `./install.sh` (nach `~/.clap` und `~/.vst3`) – das reicht für
Flatpak-Hosts, die das Home-Verzeichnis sehen.
