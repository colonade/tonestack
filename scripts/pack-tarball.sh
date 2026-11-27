#!/bin/bash
# Archiv mit allen Linux-Plugins für Nutzer ohne Debian-Paket (andere Distributionen, Flatpak-Hosts; ab 1.17242-1):
#   bash scripts/pack-tarball.sh   → dist/tonestack-plugins-<DEBVER>-linux-<arch>.tar.gz   (arch: x86_64, aarch64; Bau-Ordner BUILD=…, Architektur DEBARCH=…)
# Inhalt (Ordner tonestack-plugins-<DEBVER>-linux-<arch>/): clap/<Name>.clap, vst3/<Name>.vst3, install.sh (nach ~/.clap und ~/.vst3, --uninstall),
# README-Linux.txt, README-<Name>.txt, LICENSE.txt, THIRD-PARTY-LICENSES.txt. Reproduzierbar: sortiert, feste Zeitstempel und Besitzer, gzip ohne Name/Zeit.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"; bash scripts/check-plugins.sh
ARCH="${DEBARCH:-$(dpkg --print-architecture)}"
case "$ARCH" in amd64) MACH=x86_64;; arm64) MACH=aarch64;; *) echo "Architektur $ARCH nicht vorgesehen" >&2; exit 1;; esac
B="${BUILD:-engine/build}"; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
EPOCH=1767225600   # wie das Debian-Paket (scripts/pack-deb.sh)
N="tonestack-plugins-$DEBVER-linux-$MACH"; P="$T/$N"; mkdir -p "$P/clap" "$P/vst3"; NAMEN=""
while IFS='|' read -r NAME VV README SHORT DISP; do case "$NAME" in ''|'#'*) continue;; esac
  for f in "$B/$NAME.clap" "$B/vst3/$NAME.vst3/Contents/$MACH-linux/$NAME.so" "$B/vst3/$NAME.vst3/Contents/Resources/moduleinfo.json"; do [ -e "$f" ] || { echo "fehlt: $f" >&2; exit 1; }; done
  cp "$B/$NAME.clap" "$P/clap/"; cp -r "$B/vst3/$NAME.vst3" "$P/vst3/"; cp "reaper-clap/$README" "$P/README-$NAME.txt"; NAMEN="$NAMEN $NAME"
done < scripts/plugins.txt
sed "s/@DEBVER@/$DEBVER/; s/@MACH@/$MACH/; s/@NAMEN@/${NAMEN# }/" packaging/linux/install.sh > "$P/install.sh"
if grep -q "@[A-Z]*@" "$P/install.sh"; then echo "install.sh: Platzhalter übrig" >&2; exit 1; fi
sh -n "$P/install.sh"
cp packaging/linux/README-Linux.txt reaper-clap/THIRD-PARTY-LICENSES.txt "$P/"; cp LICENSE "$P/LICENSE.txt"
find "$P" -type d -exec chmod 755 {} +; find "$P" -type f -exec chmod 644 {} +; chmod 755 "$P/install.sh"
OUT="$D/$N.tar.gz"; rm -f "$D"/tonestack-plugins-*-linux-"$MACH".tar.gz
tar -C "$T" --sort=name --mtime="@$EPOCH" --owner=0 --group=0 --numeric-owner --format=gnu -cf - "$N" | gzip -n -9 > "$OUT"
echo "$OUT"
