# apt-Paketquelle für tonestack-plugins

Ab Paket 1.17242-1 kann der Bau auf GitHub die Debian-Pakete (amd64, arm64) zusätzlich als signierte apt-Paketquelle veröffentlichen. Dann
bekommen Nutzer neue Versionen mit `sudo apt update && sudo apt upgrade`, statt jedes Paket einzeln zu laden. Die Mechanik steht in
`scripts/apt-repo.sh` und im Schritt „apt-Paketquelle signieren“ von `.github/workflows/plugins.yml`; sie läuft nur, wenn das Geheimnis
`APT_SIGNING_KEY` eingerichtet ist. Ohne Geheimnis bleibt alles wie bisher (Pakete zum Herunterladen), und die Webseite zeigt den Abschnitt
zur Paketquelle nicht (`scripts/pack-web.sh`).

## Aufbau

Eine „flache“ Paketquelle neben den übrigen Downloads, also unter `https://tonestack.fluxstatus.de/download/`:

| Datei | Inhalt |
|---|---|
| `Packages`, `Packages.gz` | die Pakete dieser Version (alle Architekturen) |
| `Release` | Prüfsummen von `Packages*`, Herkunft `tonestack`, Architekturen |
| `InRelease`, `Release.gpg` | dasselbe signiert (eingebettet bzw. getrennt) |
| `tonestack-archive-keyring.gpg`, `.asc` | öffentlicher Schlüssel (für `Signed-By`) |
| `tonestack.sources` | fertiger Eintrag für `/etc/apt/sources.list.d/` |
| `SHA256SUMS-apt.txt` | Prüfsummen dieser Dateien (für die Webseite) |

Gleiche Pakete ergeben dieselben Bytes: Datum und Signaturzeit sind fest (nicht vor dem Anlegen des Schlüssels), Ed25519 signiert
deterministisch. Darum besteht auch ein erneuter Bau dieselbe Prüfung „gleicher Name, gleicher Inhalt“ in `scripts/release.sh`.

## Schlüssel einrichten (einmalig, durch den Inhaber des Repositorys)

Auf dem eigenen Rechner, nicht auf GitHub und nicht im Repository:

```sh
export GNUPGHOME="$(mktemp -d)"
gpg --batch --passphrase '' --quick-gen-key "tonestack apt <colonade@users.noreply.github.com>" ed25519 sign never
FP="$(gpg --with-colons --list-secret-keys | awk -F: '$1 == "fpr" { print $10; exit }')"
gpg --armor --export-secret-keys "$FP" > tonestack-apt-geheim.asc     # sicher aufbewahren (z. B. im Passwortspeicher), nicht weitergeben
gh secret set APT_SIGNING_KEY -R colonade/tonestack < tonestack-apt-geheim.asc
gpg --armor --export "$FP"                                               # öffentlicher Teil: darf überall stehen
```

Der geheime Schlüssel hat keine Passphrase, weil der Bau ihn unbeaufsichtigt benutzt; geschützt ist er als GitHub-Geheimnis (nur der
Veröffentlichungs-Job auf `main` bekommt es, die Bau-Jobs mit fremden Werkzeugen nicht). Geht er verloren, einen neuen anlegen und das
Geheimnis ersetzen; Nutzer müssen dann den neuen öffentlichen Schlüssel holen (Schritt 1 unten). Wird er bekannt, ebenso – und den alten aus
`/usr/share/keyrings` der Nutzer entfernen lassen.

## Paketquelle benutzen (Nutzer)

```sh
sudo curl -fsSLo /usr/share/keyrings/tonestack-archive-keyring.gpg https://tonestack.fluxstatus.de/download/tonestack-archive-keyring.gpg
sudo curl -fsSLo /etc/apt/sources.list.d/tonestack.sources https://tonestack.fluxstatus.de/download/tonestack.sources
sudo apt update && sudo apt install tonestack-plugins
```

Den Fingerabdruck des Schlüssels mit dem auf der Webseite vergleichen: `gpg --show-keys /usr/share/keyrings/tonestack-archive-keyring.gpg`.

Geprüft (2026-11-27) mit einem Wegwerf-Schlüssel: zweimal erzeugt → bytegleich; `apt-get update` über einen lokalen Webserver liest
`InRelease` und findet `tonestack-plugins 1.17242-1`; mit einem fremden Schlüssel in `Signed-By` verweigert apt die Quelle (`NO_PUBKEY`).
