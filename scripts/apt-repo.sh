#!/bin/bash
# Signierte apt-Paketquelle aus den Debian-Paketen eines Release-Ordners (ab 2026-11-27; Einrichtung und Schlüssel: docs/APT.md):
#   APT_SIGNING_KEY="$(gpg --export-secret-keys --armor <Fingerabdruck>)" bash scripts/apt-repo.sh <Ordner>
# Schreibt in den Ordner eine „flache“ Paketquelle (alle Dateien nebeneinander, wie die Downloads der Webseite): Packages, Packages.gz, Release,
# InRelease (Release mit eingebetteter Signatur), Release.gpg, den öffentlichen Schlüssel tonestack-archive-keyring.gpg (für Signed-By) und .asc,
# tonestack.sources (Eintrag für /etc/apt/sources.list.d/) und SHA256SUMS-apt.txt. Der geheime Schlüssel kommt nur aus der Umgebung (im Bau auf GitHub:
# das Geheimnis APT_SIGNING_KEY) und landet in einem Wegwerf-Schlüsselbund; nichts davon ins Repository.
# Reproduzierbar: Datum und Signaturzeit fest wie die Zeitstempel der Pakete (gleiche Pakete → dieselben Bytes, auch beim erneuten Lauf eines späteren
# Commits; Ed25519 und RSA signieren deterministisch). apt prüft das Datum nur gegen die Zukunft (Acquire::Check-Date), ohne Valid-Until läuft nichts ab.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; DIR="$(cd "${1:?Ordner angeben}" && pwd)"
[ -n "${APT_SIGNING_KEY:-}" ] || { echo "APT_SIGNING_KEY fehlt" >&2; exit 1; }
cd "$R"; eval "$(bash scripts/versions.sh)"
SITE="${APT_URL:-https://tonestack.fluxstatus.de/download/}"
EPOCH=1767225600   # wie scripts/pack-deb.sh
G="$(mktemp -d)"; trap 'gpgconf --homedir "$G" --kill all >/dev/null 2>&1 || true; rm -rf "$G"' EXIT; chmod 700 "$G"
printf '%s\n' "$APT_SIGNING_KEY" | gpg --homedir "$G" --batch --quiet --import
FP="$(gpg --homedir "$G" --batch --with-colons --list-secret-keys | awk -F: '$1 == "fpr" { print $10; exit }')"
[ "$(gpg --homedir "$G" --batch --with-colons --list-secret-keys | grep -c '^sec:')" = 1 ] || { echo "APT_SIGNING_KEY muss genau einen geheimen Schlüssel enthalten" >&2; exit 1; }
cd "$DIR"
ls tonestack-plugins_"$DEBVER"_*.deb >/dev/null || { echo "keine Debian-Pakete $DEBVER in $DIR" >&2; exit 1; }
rm -f Packages Packages.gz Release InRelease Release.gpg tonestack-archive-keyring.gpg tonestack-archive-keyring.asc tonestack.sources SHA256SUMS-apt.txt
# nur die Pakete dieser Version, sortiert; Filename relativ zur Quelle („./…“)
apt-ftparchive -q=2 packages --no-md5 . > Packages.alle
python3 - Packages.alle "$DEBVER" > Packages <<'PY'
import sys
st = [s for s in open(sys.argv[1], encoding='utf-8').read().split('\n\n') if s.strip()]
keep = sorted((s for s in st if ('\nVersion: ' + sys.argv[2] + '\n') in ('\n' + s + '\n')), key=lambda s: s)
print('\n\n'.join(k.strip('\n') for k in keep) + '\n', end='')
PY
rm -f Packages.alle; grep -q '^Package: tonestack-plugins$' Packages || { echo "Packages leer" >&2; exit 1; }
gzip -n -9 < Packages > Packages.gz
ARCHS="$(sed -n 's/^Architecture: //p' Packages | sort -u | tr '\n' ' ' | sed 's/ $//')"
apt-ftparchive -q=2 -o APT::FTPArchive::Release::Date="$(LC_ALL=C date -u -d "@$EPOCH" '+%a, %d %b %Y %H:%M:%S UTC')" \
  -o APT::FTPArchive::Release::Origin=tonestack -o APT::FTPArchive::Release::Label=tonestack -o APT::FTPArchive::Release::Suite=stable \
  -o APT::FTPArchive::Release::Codename=tonestack -o APT::FTPArchive::Release::Architectures="$ARCHS" \
  -o APT::FTPArchive::Release::Description="tonestack plugins (CLAP, VST3)" -o APT::FTPArchive::Release::Signed-By="$FP" \
  -o APT::FTPArchive::Release::Default-Patterns=false -o APT::FTPArchive::Release::Patterns::=Packages -o APT::FTPArchive::Release::Patterns::=Packages.gz \
  -o APT::FTPArchive::MD5=false -o APT::FTPArchive::SHA1=false -o APT::FTPArchive::SHA512=false release . > Release
grep -q "^Date: " Release || { echo "Release ohne Date" >&2; exit 1; }
# Signaturzeit: fest, aber nicht vor dem Anlegen des Schlüssels (sonst verweigert gpg ihn) – also dessen Erstellungszeit, wenn sie später liegt
KT="$(gpg --homedir "$G" --batch --with-colons --list-secret-keys | awk -F: '$1 == "sec" { print $6; exit }')"; ST=$EPOCH; [ -n "$KT" ] && [ "$KT" -gt "$ST" ] && ST=$KT
T="$(date -u -d "@$ST" +%Y%m%dT%H%M%S)!"
gpg --homedir "$G" --batch --quiet --faked-system-time "$T" --local-user "$FP" --digest-algo SHA512 --clearsign -o InRelease Release
gpg --homedir "$G" --batch --quiet --faked-system-time "$T" --local-user "$FP" --digest-algo SHA512 --armor --detach-sign -o Release.gpg Release
gpg --homedir "$G" --batch --export "$FP" > tonestack-archive-keyring.gpg
gpg --homedir "$G" --batch --armor --export "$FP" > tonestack-archive-keyring.asc
gpg --homedir "$G" --batch --quiet --verify InRelease 2>/dev/null && gpg --homedir "$G" --batch --quiet --verify Release.gpg Release 2>/dev/null || { echo "Signatur lässt sich nicht prüfen" >&2; exit 1; }
cat > tonestack.sources <<EOF
Types: deb
URIs: $SITE
Suites: ./
Signed-By: /usr/share/keyrings/tonestack-archive-keyring.gpg
EOF
sha256sum Packages Packages.gz Release InRelease Release.gpg tonestack-archive-keyring.gpg tonestack-archive-keyring.asc tonestack.sources > SHA256SUMS-apt.txt
echo "apt-Paketquelle in $DIR (Schlüssel $FP, Architekturen $ARCHS)"
