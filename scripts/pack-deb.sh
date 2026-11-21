#!/bin/bash
# Debian/Ubuntu-Paket mit allen Linux-Plugins (CLAP und VST3) aus engine/build (vorher: VST3=1 bash engine/build.sh):
#   bash scripts/pack-deb.sh   → dist/tonestack-plugins_<DEBVER>_amd64.deb   (DEBVER aus scripts/versions.sh)
# Ziele: /usr/lib/clap/<Name>.clap und /usr/lib/vst3/<Name>.vst3 (dort suchen CLAP- und VST3-Hosts unter Linux von selbst),
# Anleitungen in /usr/share/doc/tonestack-plugins/. Braucht glibc ≥ 2.38 (Debian 13, Ubuntu 24.04 und neuer), libcairo2, libx11-6.
# Reproduzierbar: feste Zeitstempel, sortiert, root als Besitzer – gleicher Inhalt ergibt dieselben Bytes.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
B=engine/build; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
P="$T/pkg"; DOC="$P/usr/share/doc/tonestack-plugins"; mkdir -p "$P/DEBIAN" "$P/usr/lib/clap" "$P/usr/lib/vst3" "$DOC"
LISTE=""
add() {   # $1 Name, $2 Version, $3 README
  for f in "$B/$1.clap" "$B/vst3/$1.vst3"; do [ -e "$f" ] || { echo "fehlt: $f (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }; done
  cp "$B/$1.clap" "$P/usr/lib/clap/"; cp -r "$B/vst3/$1.vst3" "$P/usr/lib/vst3/"; cp "reaper-clap/$3" "$DOC/README-$1.txt"
  LISTE="$LISTE $1 $2,"; }
add Laney_AOR50 "$LANEY" README.txt
add Lametta_B100II "$YAMAHA" README-B100II.txt
add AmpFett_B15NF "$B15" README-B15NF.txt
add GK_800RB "$GK" README-GK.txt
add Lametta_G100 "$G100" README-G100.txt
add Lametta_B100III "$B100III" README-B100III.txt
add AmpFett_SVTCL "$SVTCL" README-SVTCL.txt
add Wien_EQ "$WIENEQ" README-WienEQ.txt
add AmpFett_76 "$AMPFETT76" README-AmpFett76.txt
add Loldano_SLO100 "$LOLDANO" README-Loldano.txt
add Kaiserin_Reverb "$KAISERIN" README-Kaiserin.txt
add Blech_140 "$BLECH" README-Blech140.txt
add Wisent_Echoreck "$WISENT" README-Wisent.txt
add Viereck_34 "$VIERECK" README-Viereck34.txt
# glibc-Anspruch aus den Dateien selbst
G="$(for f in "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/x86_64-linux/*.so; do objdump -T "$f" | grep -o 'GLIBC_[0-9.]*'; done | sed 's/GLIBC_//' | sort -uV | tail -n1)"
[ -n "$G" ] || { echo "glibc-Version nicht ermittelt" >&2; exit 1; }
{ echo "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/"
  echo "Upstream-Name: tonestack"; echo "Source: https://github.com/colonade/tonestack"; echo
  echo "Files: *"; echo "License: GPL-3.0-or-later"; echo " Plugin-Code von tonestack. Volltext: /usr/share/common-licenses/GPL-3"; echo
  echo "Files: usr/lib/vst3/* usr/lib/clap/*"; echo "License: MIT"; echo " Enthaltene Bibliotheken Dritter (CLAP, clap-wrapper, VST3-SDK, stb_image u. a.):"
  sed 's/^$/./; s/^/ /' reaper-clap/THIRD-PARTY-LICENSES.txt; } > "$DOC/copyright"
cp reaper-clap/THIRD-PARTY-LICENSES.txt "$DOC/"
SIZE="$(du -sk --exclude=DEBIAN "$P" | cut -f1)"
cat > "$P/DEBIAN/control" <<EOF
Package: tonestack-plugins
Version: $DEBVER
Architecture: amd64
Maintainer: colonade <colonade@users.noreply.github.com>
Installed-Size: $SIZE
Depends: libc6 (>= $G), libcairo2, libx11-6
Section: sound
Priority: optional
Homepage: https://tonestack.fluxstatus.de/
Description: tonestack: Verstärker- und Effekt-Simulatoren als CLAP- und VST3-Plugins
 Schaltungsnachbildungen nach Original-Schaltplänen, je als CLAP (/usr/lib/clap)
 und VST3 (/usr/lib/vst3), mit Frontplatte. Enthalten:${LISTE%,}.
 .
 Wer die Plugins vorher von Hand nach ~/.clap bzw. ~/.vst3 kopiert hat, sollte
 diese Kopien entfernen, sonst findet der Host jedes Plugin zweimal.
EOF
( cd "$P" && find usr -type f -print0 | LC_ALL=C sort -z | xargs -0 md5sum ) > "$P/DEBIAN/md5sums"
find "$P" -type d -exec chmod 755 {} +; find "$P" -type f -exec chmod 644 {} +; chmod 755 "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/x86_64-linux/*.so
find "$P" -exec touch -h -d "2026-01-01 00:00:00 UTC" {} +
OUT="$D/tonestack-plugins_${DEBVER}_amd64.deb"; rm -f "$D"/tonestack-plugins_*_amd64.deb
SOURCE_DATE_EPOCH=1767225600 dpkg-deb --root-owner-group -Zxz --build "$P" "$OUT" >/dev/null
dpkg-deb --info "$OUT" | sed -n 's/^ *\(Package\|Version\|Depends\|Installed-Size\):/  &/p'
echo "$OUT"
