#!/bin/bash
# Debian/Ubuntu-Paket mit allen Linux-Plugins (CLAP und VST3) aus engine/build (vorher: VST3=1 bash engine/build.sh):
#   bash scripts/pack-deb.sh   → dist/tonestack-plugins_<DEBVER>_amd64.deb   (DEBVER = SUMVER-DEBREV aus scripts/versions.sh; Liste: scripts/plugins.txt)
# Ziele: /usr/lib/clap/<Name>.clap und /usr/lib/vst3/<Name>.vst3 (dort suchen CLAP- und VST3-Hosts unter Linux von selbst),
# Anleitungen in /usr/share/doc/tonestack-plugins/. postinst meldet das Ergebnis (alle Dateien da, Liste der Plugins, Handkopien in ~/.clap und ~/.vst3) und trägt /usr/lib/vst3 in die REAPER-Einstellungen ein (postrm nimmt es wieder heraus). Braucht glibc ≥ 2.38 (Debian 13, Ubuntu 24.04 und neuer), libcairo2, libx11-6, procps. Prüfung: scripts/test-deb.sh.
# Reproduzierbar: feste Zeitstempel, sortiert, root als Besitzer – gleicher Inhalt ergibt dieselben Bytes.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"; bash scripts/check-plugins.sh   # Namen in plugins.txt nur mit harmlosen Zeichen
B=engine/build; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
P="$T/pkg"; DOC="$P/usr/share/doc/tonestack-plugins"; mkdir -p "$P/DEBIAN" "$P/usr/lib/clap" "$P/usr/lib/vst3" "$DOC"
LISTE=""; NAMEN=""; N=0
add() {   # $1 Name, $2 Version, $3 README
  for f in "$B/$1.clap" "$B/vst3/$1.vst3"; do [ -e "$f" ] || { echo "fehlt: $f (erst VST3=1 bash engine/build.sh)" >&2; exit 1; }; done
  cp "$B/$1.clap" "$P/usr/lib/clap/"; cp -r "$B/vst3/$1.vst3" "$P/usr/lib/vst3/"; cp "reaper-clap/$3" "$DOC/README-$1.txt"
  LISTE="$LISTE $1 $2,"; NAMEN="$NAMEN $1"; N=$((N + 1)); }
while IFS='|' read -r NAME VV README SHORT DISP; do case "$NAME" in ''|'#'*) continue;; esac
  add "$NAME" "${!VV}" "$README"; done < scripts/plugins.txt
# glibc-Anspruch aus den Dateien selbst
G="$(for f in "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/x86_64-linux/*.so; do objdump -T "$f" | grep -o 'GLIBC_[0-9.]*'; done | sed 's/GLIBC_//' | sort -uV | tail -n1)"
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
cp reaper-clap/THIRD-PARTY-LICENSES.txt "$DOC/"
# Änderungsliste (Debian verlangt sie; Datum fest, damit das Paket reproduzierbar bleibt)
{ echo "tonestack-plugins ($DEBVER) unstable; urgency=medium"; echo; echo "  * Plugins:${LISTE%,}." | fold -s -w 76 | sed 's/ *$//; 2,$s/^/    /'; echo
  echo " -- colonade <colonade@users.noreply.github.com>  Thu, 01 Jan 2026 00:00:00 +0000"; } | gzip -n -9 > "$DOC/changelog.Debian.gz"
INHALT="$(echo "Enthalten:${LISTE%,}." | fold -s -w 76 | sed 's/^/ /; s/ *$//')"
SIZE="$(du -sk --exclude=DEBIAN "$P" | cut -f1)"
cat > "$P/DEBIAN/control" <<EOF
Package: tonestack-plugins
Version: $DEBVER
Architecture: amd64
Maintainer: colonade <colonade@users.noreply.github.com>
Installed-Size: $SIZE
Depends: libc6 (>= $G), libcairo2, libx11-6, procps
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
 eingerichtet hat, in die VST-Pfade ein (Sicherung reaper.ini.vor-tonestack);
 beim Entfernen wird der Eintrag wieder herausgenommen.
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
  [ -s "/usr/lib/vst3/$n.vst3/Contents/x86_64-linux/$n.so" ] || fehlt="$fehlt /usr/lib/vst3/$n.vst3"
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
# REAPER (Linux) sucht VST von sich aus nur in ~/.vst;~/.vst3, CLAP auch in /usr/lib/clap. Ab 1.17227-3 trägt das Paket /usr/lib/vst3 in die Zeile
# vstpath= der reaper.ini jedes Benutzers ein, der REAPER eingerichtet hat (UID_MIN … UID_MAX, ~/.config/REAPER/reaper.ini) – unabhängig davon, wie
# installiert wird (sudo, pkexec, PackageKit). Nur diese Zeile, Sicherung reaper.ini.vor-tonestack, gemerkt in /var/lib/tonestack-plugins (postrm
# nimmt den Eintrag wieder heraus, wenn /usr/lib/vst3 danach leer ist). Läuft REAPER, schriebe es die Datei beim Beenden zurück: dann nur Hinweis.
umin="$(awk '$1 == "UID_MIN" { print $2 }' /etc/login.defs 2>/dev/null)"; umax="$(awk '$1 == "UID_MAX" { print $2 }' /etc/login.defs 2>/dev/null)"
getent passwd | awk -F: -v a="${umin:-1000}" -v b="${umax:-60000}" '$3 >= a && $3 <= b { print $1 ":" $6 }' | while IFS=: read -r wer home; do
  ini="$home/.config/REAPER/reaper.ini"
  [ -f "$ini" ] || continue
  if pgrep -u "$wer" -x reaper >/dev/null 2>&1; then
    if ! awk -F= '$1 == "vstpath" { n = split(substr($0, 9), a, ";"); for (i = 1; i <= n; i++) { p = a[i]; sub(/\/+$/, "", p); if (p == "/usr/lib/vst3") f = 1 } } END { exit !f }' "$ini"; then
      echo "Hinweis REAPER ($wer): REAPER läuft gerade, darum ist /usr/lib/vst3 (die VST3-Fassungen) noch nicht in seine VST-Pfade eingetragen."
      echo "  REAPER schließen und dann ausführen: sudo dpkg-reconfigure tonestack-plugins"
    fi
    continue
  fi
  neu="$(mktemp)"
  awk -v ziel=/usr/lib/vst3 '
    $0 ~ /^vstpath=/ && !gesehen { gesehen = 1; w = substr($0, 9); n = split(w, a, ";"); for (i = 1; i <= n; i++) { p = a[i]; sub(/\/+$/, "", p); if (p == ziel) da = 1 }
      if (!da) { $0 = "vstpath=" (w == "" ? ziel : w ";" ziel); st = "eingetragen" } else st = "schon da" }
    { z[++m] = $0; if (!kopf && tolower($0) == "[reaper]") kopf = m }
    END { if (!gesehen) { if (!kopf) { print "ohne Abschnitt [REAPER]" > "/dev/stderr"; exit 3 } st = "eingetragen" }
      for (i = 1; i <= m; i++) { print z[i]; if (!gesehen && i == kopf) print "vstpath=~/.vst;~/.vst3;" ziel }
      print st > "/dev/stderr" }' "$ini" > "$neu" 2> "$neu.st" && ok=1 || ok=0
  st="$(cat "$neu.st" 2>/dev/null || true)"
  if [ "$ok" = 1 ] && [ "$st" = eingetragen ]; then
    # Inhalt ersetzen (cat statt mv): Besitzer und Rechte der Datei bleiben; ohne Sicherung keine Änderung
    if { [ -e "$ini.vor-tonestack" ] || cp -p -- "$ini" "$ini.vor-tonestack"; } && cat "$neu" > "$ini"; then
      mkdir -p /var/lib/tonestack-plugins; grep -qxF -- "$ini" /var/lib/tonestack-plugins/reaper-vstpath 2>/dev/null || echo "$ini" >> /var/lib/tonestack-plugins/reaper-vstpath
      echo "REAPER ($wer): /usr/lib/vst3 in die VST-Pfade eingetragen (Sicherung $ini.vor-tonestack) – beim nächsten Start findet REAPER die VST3-Fassungen."
    else echo "Hinweis REAPER ($wer): $ini ließ sich nicht schreiben. Bitte in REAPER unter Options > Preferences > Plug-ins > VST bei „VST plug-in paths“ ;/usr/lib/vst3 anhängen."; fi
  elif [ "$ok" = 0 ]; then
    echo "Hinweis REAPER ($wer): $ini ließ sich nicht anpassen ($st). Bitte in REAPER unter Options > Preferences > Plug-ins > VST bei „VST plug-in paths“ ;/usr/lib/vst3 anhängen."
  fi
  rm -f -- "$neu" "$neu.st"
done || true   # die REAPER-Einträge dürfen die Installation nie scheitern lassen
exit 0
EOF
ZEILEN="$(echo "${LISTE# }" | sed 's/,$//' | fold -s -w 74 | sed 's/ *$//; s/^/echo "  /; s/$/"/' | sed ':a;N;$!ba;s/\n/\\n/g')"
sed -i "s|@NAMEN@|${NAMEN# }|g; s|@N@|$N|g; s|@DEBVER@|$DEBVER|g; s|^@LISTE@\$|$ZEILEN|" "$P/DEBIAN/postinst"
if grep -q @ "$P/DEBIAN/postinst"; then echo "postinst: Platzhalter übrig" >&2; exit 1; fi
sh -n "$P/DEBIAN/postinst"
# postrm (ab 1.17227-3): den von postinst eingetragenen REAPER-Pfad /usr/lib/vst3 wieder herausnehmen – nur wenn /usr/lib/vst3 danach leer ist
# (sonst gehört er auch anderen Plugins) und REAPER beim Besitzer der Datei nicht läuft
cat > "$P/DEBIAN/postrm" <<'EOF'
#!/bin/sh
set -e
liste=/var/lib/tonestack-plugins/reaper-vstpath
if [ "$1" = remove ] || [ "$1" = purge ]; then
  if [ "$1" = remove ]; then echo "tonestack: Plugins entfernt (/usr/lib/clap, /usr/lib/vst3)."; fi
  if [ -f "$liste" ]; then
    if [ -n "$(ls -A /usr/lib/vst3 2>/dev/null)" ]; then
      echo "REAPER: /usr/lib/vst3 bleibt in den VST-Pfaden eingetragen (dort liegen noch andere Plugins)."
    else
      while IFS= read -r ini; do
        [ -f "$ini" ] || continue
        wer="$(stat -c %U -- "$ini" 2>/dev/null || true)"
        if [ -n "$wer" ] && pgrep -u "$wer" -x reaper >/dev/null 2>&1; then
          echo "REAPER läuft – /usr/lib/vst3 bleibt in $ini eingetragen (schadet nicht; von Hand unter Options > Preferences > Plug-ins > VST entfernen)."; continue
        fi
        neu="$(mktemp)"
        if awk '$0 ~ /^vstpath=/ && !g { g = 1; n = split(substr($0, 9), a, ";"); w = ""; for (i = 1; i <= n; i++) { p = a[i]; sub(/\/+$/, "", p); if (p != "/usr/lib/vst3") w = w (w == "" ? "" : ";") a[i] } $0 = "vstpath=" w } { print }' "$ini" > "$neu"; then
          if cat "$neu" > "$ini"; then echo "REAPER ($wer): /usr/lib/vst3 aus den VST-Pfaden entfernt."; fi
        fi
        rm -f -- "$neu"
      done < "$liste" || true
    fi
    rm -f -- "$liste"; rmdir /var/lib/tonestack-plugins 2>/dev/null || true
  fi
fi
exit 0
EOF
sh -n "$P/DEBIAN/postrm"
( cd "$P" && find usr -type f -print0 | LC_ALL=C sort -z | xargs -0 md5sum ) > "$P/DEBIAN/md5sums"
find "$P" -type d -exec chmod 755 {} +; find "$P" -type f -exec chmod 644 {} +; chmod 755 "$P"/DEBIAN/postinst "$P"/DEBIAN/postrm "$P"/usr/lib/clap/*.clap "$P"/usr/lib/vst3/*/Contents/x86_64-linux/*.so
find "$P" -exec touch -h -d "2026-01-01 00:00:00 UTC" {} +
OUT="$D/tonestack-plugins_${DEBVER}_amd64.deb"; rm -f "$D"/tonestack-plugins_*_amd64.deb
SOURCE_DATE_EPOCH=1767225600 dpkg-deb --root-owner-group -Zxz --build "$P" "$OUT" >/dev/null
dpkg-deb --info "$OUT" | sed -n 's/^ *\(Package\|Version\|Depends\|Installed-Size\):/  &/p'
echo "$OUT"
