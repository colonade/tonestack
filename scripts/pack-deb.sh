#!/bin/bash
# Debian/Ubuntu-Paket mit allen Linux-Plugins (CLAP und VST3) aus engine/build (vorher: VST3=1 bash engine/build.sh):
#   bash scripts/pack-deb.sh   → dist/tonestack-plugins_<DEBVER>_<arch>.deb   (DEBVER = SUMVER-DEBREV aus scripts/versions.sh; Liste: scripts/plugins.txt)
#   Architektur: die des Bau-Rechners (amd64, arm64; ab 1.17242-1) oder DEBARCH=…; anderer Bau-Ordner: BUILD=… (Kreuzbau, engine/arm64/build.sh)
# Ziele: /usr/lib/clap/<Name>.clap und /usr/lib/vst3/<Name>.vst3 (dort suchen CLAP- und VST3-Hosts unter Linux von selbst), Anleitungen in
# /usr/share/doc/tonestack-plugins/ (README-Linux.txt: Hosts, Distributionen, Grenzen), /usr/bin/tonestack-reaper-setup samt Handbuchseite.
# postinst meldet das Ergebnis (alle Dateien da, Liste der Plugins, Handkopien in ~/.clap und ~/.vst3) und ruft für jeden Benutzer, der REAPER
# eingerichtet hat, tonestack-reaper-setup mit dessen Rechten auf (trägt /usr/lib/vst3 in seine VST-Pfade ein); prerm nimmt beim Entfernen nur
# diese eigenen Einträge wieder heraus. Abhängigkeiten aus dpkg-shlibdeps. Prüfung: scripts/test-deb.sh (Installieren), lintian (im Bau auf GitHub).
# Reproduzierbar: feste Zeitstempel (SOURCE_DATE_EPOCH = Datum des Changelog-Eintrags, wie bei dpkg-buildpackage), sortiert, root als Besitzer,
# xz Stufe 6 in einem Faden – gleicher Inhalt ergibt dieselben Bytes.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"; bash scripts/check-plugins.sh   # Namen in plugins.txt nur mit harmlosen Zeichen
ARCH="${DEBARCH:-$(dpkg --print-architecture)}"
case "$ARCH" in amd64) TRIPLE=x86_64-linux; GNU=x86_64-linux-gnu;; arm64) TRIPLE=aarch64-linux; GNU=aarch64-linux-gnu;; *) echo "Architektur $ARCH nicht vorgesehen" >&2; exit 1;; esac
B="${BUILD:-engine/build}"; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
EPOCH=1767225600; DATUM="Thu, 01 Jan 2026 00:00:00 +0000"   # fest: gleiche Plugin-Dateien ergeben dasselbe Paket, auch in einem späteren Bau
P="$T/pkg"; DOC="$P/usr/share/doc/tonestack-plugins"; mkdir -p "$P/DEBIAN" "$P/usr/lib/clap" "$P/usr/lib/vst3" "$DOC" "$P/usr/bin" "$P/usr/share/man/man1" "$P/usr/share/lintian/overrides"
LISTE=""; NAMEN=""; N=0
add() {   # $1 Name, $2 Version, $3 README
  for f in "$B/$1.clap" "$B/vst3/$1.vst3/Contents/$TRIPLE/$1.so" "$B/vst3/$1.vst3/Contents/Resources/moduleinfo.json"; do [ -e "$f" ] || { echo "fehlt: $f (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }; done
  [ "$(LC_ALL=C ls "$B/vst3/$1.vst3/Contents")" = "$(printf 'Resources\n%s' "$TRIPLE")" ] || { echo "$B/vst3/$1.vst3: unerwarteter Inhalt in Contents/ (nur $TRIPLE und Resources)" >&2; exit 1; }
  cp "$B/$1.clap" "$P/usr/lib/clap/"; cp -r "$B/vst3/$1.vst3" "$P/usr/lib/vst3/"; cp "reaper-clap/$3" "$DOC/README-$1.txt"
  LISTE="$LISTE $1 $2,"; NAMEN="$NAMEN $1"; N=$((N + 1)); }
while IFS='|' read -r NAME VV README SHORT DISP; do case "$NAME" in ''|'#'*) continue;; esac
  add "$NAME" "${!VV}" "$README"; done < scripts/plugins.txt
# glibc-Anspruch aus den Dateien selbst (Gegenprobe zu dpkg-shlibdeps unten)
G="$(for f in "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/$TRIPLE/*.so; do objdump -T "$f" | grep -o 'GLIBC_[0-9.]*'; done | sed 's/GLIBC_//' | sort -uV | tail -n1)"
[ -n "$G" ] || { echo "glibc-Version nicht ermittelt" >&2; exit 1; }
cat > "$DOC/copyright" <<'EOF'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: tonestack
Upstream-Contact: https://github.com/colonade/tonestack/issues
Source: https://github.com/colonade/tonestack

Files: *
Copyright: tonestack (colonade)
License: GPL-3
Comment: The plugin files in /usr/lib/clap and /usr/lib/vst3 are built from the
 tonestack source (GPL-3.0, file LICENSE in the repository) and contain
 third-party code under its own GPL-compatible licences: CLAP SDK,
 clap-wrapper, VST3 SDK 3.8 and {fmt} (MIT), stb_image (MIT / public domain),
 PreSonus Plug-In Extensions (public domain). Full texts:
 /usr/share/doc/tonestack-plugins/THIRD-PARTY-LICENSES.txt

License: GPL-3
 This program is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.
 .
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 .
 On Debian systems, the complete text of the GNU General Public License
 version 3 can be found in "/usr/share/common-licenses/GPL-3".
EOF
cp reaper-clap/THIRD-PARTY-LICENSES.txt packaging/linux/README-Linux.txt "$DOC/"
# Änderungsliste (Debian verlangt sie; Datum fest, damit das Paket reproduzierbar bleibt)
{ echo "tonestack-plugins ($DEBVER) unstable; urgency=medium"; echo; echo "  * Plugins:${LISTE%,}." | fold -s -w 76 | sed 's/ *$//; 2,$s/^/    /'; echo
  echo " -- colonade <colonade@users.noreply.github.com>  $DATUM"; } | gzip -n -9 > "$DOC/changelog.Debian.gz"
# REAPER-Werkzeug (ab 1.17242-1; vorher bearbeitete der postinst die reaper.ini selbst, als root) und seine Handbuchseite
sed "s/@DEBVER@/$DEBVER/" packaging/linux/tonestack-reaper-setup > "$P/usr/bin/tonestack-reaper-setup"
if grep -q "@[A-Z]*@" "$P/usr/bin/tonestack-reaper-setup"; then echo "tonestack-reaper-setup: Platzhalter übrig" >&2; exit 1; fi
sh -n "$P/usr/bin/tonestack-reaper-setup"
gzip -n -9 < packaging/linux/tonestack-reaper-setup.1 > "$P/usr/share/man/man1/tonestack-reaper-setup.1.gz"
# Lintian-Hinweise, die nicht zutreffen (Begründung in der Datei)
cp packaging/linux/lintian-overrides "$P/usr/share/lintian/overrides/tonestack-plugins"
# Abhängigkeiten aus den Dateien (dpkg-shlibdeps, ab 1.17242-1; vorher von Hand): libc6 mit Mindestversion, libcairo2, libx11-6 – libstdc++ und
# libgcc sind statisch eingebunden. awk (Paketskripte, tonestack-reaper-setup) gilt als faktisch essenziell (Lintian: needlessly-depends-on-awk).
mkdir -p "$T/sd/debian"; printf 'Source: tonestack\n\nPackage: tonestack-plugins\nArchitecture: any\n' > "$T/sd/debian/control"
EA=(); for f in "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/"$TRIPLE"/*.so; do EA+=("-e$f"); done
SHL="$(cd "$T/sd" && dpkg-shlibdeps -O ${SHLIBDEPS_ARGS:-} "${EA[@]}" 2>"$T/sd/err")" || { cat "$T/sd/err" >&2; exit 1; }
DEPS="$(printf '%s' "$SHL" | sed -n 's/^shlibs:Depends=//p')"
[ -n "$DEPS" ] || { echo "dpkg-shlibdeps ohne Ergebnis: $SHL" >&2; cat "$T/sd/err" >&2; exit 1; }
# bekannte, harmlose Warnung unter Ubuntu 24.04: usrmerge lenkt ld-linux um (diversion by libc6 … .usr-is-merged); jede andere Meldung bricht ab
if grep -v -e 'warning: diversions involved - output may be incorrect' -e '^ diversion by libc6 \(from\|to\): /lib64/ld-linux-[a-z0-9_-]*\.so\.[0-9]*\(\.usr-is-merged\)\?$' "$T/sd/err" | grep -q .; then echo "dpkg-shlibdeps meldet:" >&2; cat "$T/sd/err" >&2; exit 1; fi
case ", $DEPS," in *", libc6 (>= $G),"*) ;; *) echo "dpkg-shlibdeps ($DEPS) passt nicht zur glibc-Version der Dateien ($G)" >&2; exit 1;; esac
case "$DEPS" in *libstdc++*|*libgcc*) echo "libstdc++/libgcc sollten statisch eingebunden sein: $DEPS" >&2; exit 1;; esac
INHALT="$(echo "Enthalten:${LISTE%,}." | fold -s -w 76 | sed 's/^/ /; s/ *$//')"
SIZE="$(du -sk --exclude=DEBIAN "$P" | cut -f1)"
cat > "$P/DEBIAN/control" <<EOF
Package: tonestack-plugins
Version: $DEBVER
Architecture: $ARCH
Maintainer: colonade <colonade@users.noreply.github.com>
Installed-Size: $SIZE
Depends: $DEPS
Provides: clap-plugin, vst3-plugin
Section: sound
Priority: optional
Homepage: https://tonestack.fluxstatus.de/
Description: tonestack: Verstärker- und Effekt-Simulatoren als CLAP- und VST3-Plugins
 Schaltungsnachbildungen nach Original-Schaltplänen, je als CLAP
 (/usr/lib/clap) und VST3 (/usr/lib/vst3), mit Frontplatte.
$INHALT
 .
 Wer die Plugins vorher von Hand nach ~/.clap bzw. ~/.vst3 kopiert hat, sollte
 diese Kopien entfernen, sonst findet der Host jedes Plugin zweimal.
 .
 REAPER durchsucht /usr/lib/clap von selbst, /usr/lib/vst3 aber nicht: die
 Installation trägt /usr/lib/vst3 darum bei jedem Benutzer, der REAPER
 eingerichtet hat, mit tonestack-reaper-setup in die VST-Pfade ein (Sicherung
 reaper.ini.vor-tonestack); beim Entfernen wird der Eintrag wieder
 herausgenommen. Hosts und Grenzen:
 /usr/share/doc/tonestack-plugins/README-Linux.txt
EOF
# Rückmeldung nach dem Einrichten (dpkg, apt und gdebi zeigen sie an): sind alle Dateien da? Dazu Hinweis auf Handkopien im Home dessen,
# der installiert (sudo: SUDO_USER, grafische Installer über pkexec: PKEXEC_UID) – nur melden, nichts löschen.
cat > "$P/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
[ "$1" = configure ] || exit 0
fehlt=""
for n in @NAMEN@; do
  [ -s "/usr/lib/clap/$n.clap" ] || fehlt="$fehlt /usr/lib/clap/$n.clap"
  [ -s "/usr/lib/vst3/$n.vst3/Contents/@TRIPLE@/$n.so" ] || fehlt="$fehlt /usr/lib/vst3/$n.vst3"
done
# fehlende Dateien nur melden (ab 2026-11-24): mit exit 1 bliebe das Paket halb eingerichtet und apt bei jedem Aufruf hängen
if [ -n "$fehlt" ]; then echo "tonestack: WARNUNG – Installation unvollständig, es fehlt:$fehlt (Paket neu installieren: sudo apt install --reinstall tonestack-plugins)" >&2; exit 0; fi
echo "tonestack: Installation erfolgreich – @N@ Plugins (Paket @DEBVER@) als CLAP in /usr/lib/clap und als VST3 in /usr/lib/vst3:"
@LISTE@
echo "Den Host neu starten oder die Plugins neu suchen lassen, dann stehen sie in seiner Liste."
home=""
if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then home="$(getent passwd -- "$SUDO_USER" | cut -d: -f6)" || home=""
elif [ -n "${PKEXEC_UID:-}" ] && [ "$PKEXEC_UID" != 0 ]; then home="$(getent passwd -- "$PKEXEC_UID" | cut -d: -f6)" || home=""; fi
if [ -n "$home" ] && [ -d "$home" ]; then
  kopf=""
  for n in @NAMEN@; do for f in "$home/.clap/$n.clap" "$home/.vst3/$n.vst3"; do
    if [ -e "$f" ] || [ -L "$f" ]; then   # Verknüpfungen (auch ins Leere) nur benennen, nicht verfolgen
      if [ -z "$kopf" ]; then echo "Hinweis: von Hand kopierte Plugins gefunden – der Host fände sie doppelt. Bitte entfernen:"; kopf=1; fi
      if [ -L "$f" ]; then echo "  $f (Verknüpfung)"; else echo "  $f"; fi
    fi
  done; done
fi
# REAPER (Linux) sucht VST von sich aus nur in ~/.vst;~/.vst3, CLAP auch in /usr/lib/clap. Für jeden Benutzer (UID_MIN … UID_MAX), der REAPER
# eingerichtet hat (~/.config/REAPER/reaper.ini), trägt tonestack-reaper-setup /usr/lib/vst3 ein – mit den Rechten dieses Benutzers (setpriv aus
# util-linux), damit root nie in fremde Home-Ordner schreibt; nur bei beendetem REAPER, mit Sicherung, Merkdatei reaper.ini.tonestack.
# Übergang von 1.17227-3 (dort schrieb der postinst als root und merkte sich die Dateien in /var/lib/tonestack-plugins): diese Einträge werden
# als eigene übernommen, die Liste gelöscht.
mit() {   # $1 Benutzer, Rest: Befehl – mit dessen Rechten und Umgebung
  wer="$1"; shift
  setpriv --reuid="$wer" --regid="$(id -g -- "$wer")" --init-groups --reset-env -- "$@"
}
alt=/var/lib/tonestack-plugins/reaper-vstpath
if [ -f "$alt" ]; then
  while IFS= read -r ini; do
    [ -f "$ini" ] && [ ! -L "$ini" ] || continue
    wer="$(stat -c %U -- "$ini" 2>/dev/null)" || continue
    [ "$wer" != root ] && [ "$wer" != UNKNOWN ] || continue
    mit "$wer" /usr/bin/tonestack-reaper-setup --auto --adopt --ini "$ini" || true
  done < "$alt" || true
  rm -f -- "$alt"; rmdir /var/lib/tonestack-plugins 2>/dev/null || true
fi
umin="$(awk '$1 == "UID_MIN" { print $2 }' /etc/login.defs 2>/dev/null)"; umax="$(awk '$1 == "UID_MAX" { print $2 }' /etc/login.defs 2>/dev/null)"
getent passwd | awk -F: -v a="${umin:-1000}" -v b="${umax:-60000}" '$3 >= a && $3 <= b { print $1 ":" $6 }' | while IFS=: read -r wer home; do
  ini="$home/.config/REAPER/reaper.ini"
  [ -f "$ini" ] && [ ! -L "$ini" ] || continue
  mit "$wer" /usr/bin/tonestack-reaper-setup --auto --ini "$ini" || echo "Hinweis REAPER ($wer): /usr/lib/vst3 ließ sich nicht eintragen – als $wer ausführen: tonestack-reaper-setup"
done || true   # die REAPER-Einträge dürfen die Installation nie scheitern lassen
exit 0
EOF
ZEILEN="$(echo "${LISTE# }" | sed 's/,$//' | fold -s -w 74 | sed 's/ *$//; s/^/echo "  /; s/$/"/' | sed ':a;N;$!ba;s/\n/\\n/g')"
sed -i "s|@NAMEN@|${NAMEN# }|g; s|@N@|$N|g; s|@DEBVER@|$DEBVER|g; s|@TRIPLE@|$TRIPLE|g; s|^@LISTE@\$|$ZEILEN|" "$P/DEBIAN/postinst"
if grep -q "@[A-Z]*@" "$P/DEBIAN/postinst"; then echo "postinst: Platzhalter übrig" >&2; exit 1; fi
sh -n "$P/DEBIAN/postinst"
# prerm (ab 1.17242-1): beim Entfernen (nicht beim Aktualisieren) die von tonestack-reaper-setup eingetragenen REAPER-Pfade wieder herausnehmen –
# nur eigene (Merkdatei), nur wenn in /usr/lib/vst3 außer diesen Plugins nichts liegt (sonst gehört der Pfad auch anderen) und nur bei beendetem
# REAPER; im prerm, weil tonestack-reaper-setup im postrm schon fehlt
cat > "$P/DEBIAN/prerm" <<'EOF'
#!/bin/sh
set -e
[ "$1" = remove ] || exit 0
fremd=""
for f in /usr/lib/vst3/* /usr/lib/vst3/.[!.]*; do
  [ -e "$f" ] || [ -L "$f" ] || continue
  case " @NAMEN@ " in *" $(basename -- "$f" .vst3) "*) [ "${f%.vst3}" != "$f" ] && continue;; esac
  fremd=1
done
if [ -n "$fremd" ]; then echo "REAPER: /usr/lib/vst3 bleibt in den VST-Pfaden eingetragen (dort liegen noch andere Plugins)."; exit 0; fi
umin="$(awk '$1 == "UID_MIN" { print $2 }' /etc/login.defs 2>/dev/null)"; umax="$(awk '$1 == "UID_MAX" { print $2 }' /etc/login.defs 2>/dev/null)"
getent passwd | awk -F: -v a="${umin:-1000}" -v b="${umax:-60000}" '$3 >= a && $3 <= b { print $1 ":" $6 }' | while IFS=: read -r wer home; do
  ini="$home/.config/REAPER/reaper.ini"
  [ -f "$ini" ] && [ ! -L "$ini" ] && [ -f "$ini.tonestack" ] || continue
  setpriv --reuid="$wer" --regid="$(id -g -- "$wer")" --init-groups --reset-env -- /usr/bin/tonestack-reaper-setup --auto --undo --ini "$ini" || true
done || true
exit 0
EOF
sed -i "s|@NAMEN@|${NAMEN# }|g" "$P/DEBIAN/prerm"
if grep -q "@[A-Z]*@" "$P/DEBIAN/prerm"; then echo "prerm: Platzhalter übrig" >&2; exit 1; fi
sh -n "$P/DEBIAN/prerm"
cat > "$P/DEBIAN/postrm" <<'EOF'
#!/bin/sh
set -e
case "$1" in
  remove) echo "tonestack: Plugins entfernt (/usr/lib/clap, /usr/lib/vst3).";;
  purge) rm -f /var/lib/tonestack-plugins/reaper-vstpath; rmdir /var/lib/tonestack-plugins 2>/dev/null || true;;   # Liste von 1.17227-3, falls übrig
esac
exit 0
EOF
sh -n "$P/DEBIAN/postrm"
( cd "$P" && find usr -type f -print0 | LC_ALL=C sort -z | xargs -0 md5sum ) > "$P/DEBIAN/md5sums"
find "$P" -type d -exec chmod 755 {} +; find "$P" -type f -exec chmod 644 {} +
chmod 755 "$P"/DEBIAN/postinst "$P"/DEBIAN/prerm "$P"/DEBIAN/postrm "$P"/usr/bin/tonestack-reaper-setup
find "$P" -exec touch -h -d "@$EPOCH" {} +
OUT="$D/tonestack-plugins_${DEBVER}_${ARCH}.deb"; rm -f "$D"/tonestack-plugins_*_"$ARCH".deb
SOURCE_DATE_EPOCH=$EPOCH DPKG_DEB_THREADS_MAX=1 dpkg-deb --root-owner-group -Zxz -z6 --build "$P" "$OUT" >/dev/null
dpkg-deb --info "$OUT" | sed -n 's/^ *\(Package\|Version\|Architecture\|Depends\|Provides\|Installed-Size\):/  &/p'
echo "$OUT"
