#!/bin/bash
# Prüft das Debian-Paket durch echtes Installieren (braucht root, ändert das System: legt drei Testbenutzer an und entfernt sie wieder).
# Läuft im Bau auf GitHub nach dem Packen; von Hand: sudo bash scripts/test-deb.sh dist/tonestack-plugins_<DEBVER>_<arch>.deb
# Geprüft (ab 1.17242-1): alle Plugins landen in /usr/lib/clap und /usr/lib/vst3 (samt moduleinfo.json), Rechte 644; tonestack-reaper-setup trägt
# /usr/lib/vst3 für jeden Benutzer, der REAPER eingerichtet hat, in vstpath= ein – auch ohne sudo-Umgebung (PackageKit), mit dessen Rechten
# (Besitzer und Rechte der Datei bleiben, Sicherung und Merkdatei gehören ihm), zweites Installieren trägt nichts doppelt ein; bei laufendem REAPER
# bleibt die Datei unberührt (Hinweis auf tonestack-reaper-setup); Entfernen nimmt nur eigene Einträge heraus, einen selbst gesetzten nicht, und
# keinen, solange andere Plugins in /usr/lib/vst3 liegen; Übernahme der Einträge von 1.17227-3; das Werkzeug lehnt root und Verknüpfungen ab.
set -euo pipefail
DEB="${1:?Paket angeben}"; [ "$(id -u)" = 0 ] || { echo "als root ausführen" >&2; exit 1; }
! dpkg -s tonestack-plugins >/dev/null 2>&1 || { echo "tonestack-plugins ist schon installiert – Test bräche das vorhandene Paket" >&2; exit 1; }
U1=tstest1; U2=tstest2; U3=tstest3; bad=0
fehler() { echo "FEHLER: $*" >&2; bad=1; }
for u in $U1 $U2 $U3; do id "$u" >/dev/null 2>&1 && { echo "Benutzer $u gibt es schon" >&2; exit 1; }; useradd -m "$u"; done
aufraeumen() { pkill -u $U1 -x reaper 2>/dev/null || true; dpkg -P tonestack-plugins >/dev/null 2>&1 || true; for u in $U1 $U2 $U3; do userdel -r "$u" 2>/dev/null || true; done; rm -rf "$T" /usr/lib/vst3/Fremd.vst3; }
T="$(mktemp -d)"; trap aufraeumen EXIT
ini() { echo "/home/$1/.config/REAPER/reaper.ini"; }
mk() { local u="$1"; shift; mkdir -p "/home/$u/.config/REAPER"; rm -f "$(ini "$u")".*; printf '%s\n' "$@" > "$(ini "$u")"; chown -R "$u:$u" "/home/$u/.config"; chmod 600 "$(ini "$u")"; }
zeile() { grep '^vstpath=' "$(ini "$1")" || true; }
soll() { [ "$(zeile "$1")" = "$2" ] || fehler "$3: $1 hat „$(zeile "$1")“, erwartet „$2“"; }
rechte() { local f; for f in "$(ini "$1")" "$(ini "$1")".vor-tonestack "$(ini "$1")".tonestack; do [ -e "$f" ] || continue
  [ "$(stat -c '%U %G' "$f")" = "$1 $1" ] || fehler "$2: $f gehört $(stat -c '%U:%G' "$f")"; done
  [ "$(stat -c '%a' "$(ini "$1")")" = 600 ] || fehler "$2: Rechte von $(ini "$1") geändert ($(stat -c '%a' "$(ini "$1")"))"; }
als() { local u="$1"; shift; runuser -u "$u" -- env -i HOME="/home/$u" PATH=/usr/sbin:/usr/bin:/sbin:/bin LANG=C.UTF-8 "$@"; }   # leere Umgebung (im Bau auf GitHub ist XDG_CONFIG_HOME gesetzt)
V1='vstpath=~/.vst;~/.vst3'; V2='vstpath=~/.vst;~/.vst3;/usr/lib/vst3'

echo "== Installieren ohne sudo-Umgebung (wie PackageKit), drei Benutzer"
mk $U1 '[REAPER]' 'a=1' "$V1" 'b=2'; mk $U2 '[REAPER]' 'a=1' '[andere]' 'vstpath=x'; mk $U3 '[REAPER]' "$V2"
env -u SUDO_USER -u PKEXEC_UID dpkg -i "$DEB" | tee "$T/aus1"
dpkg-deb -c "$DEB" > "$T/inhalt"   # erst in eine Datei: grep -q in der Leitung beendete dpkg-deb vorzeitig (SIGPIPE, mit pipefail ein Fehler)
N="$(grep -c '\.clap$' "$T/inhalt" || true)"; A="$(dpkg-deb -f "$DEB" Architecture)"
case "$A" in amd64) TR=x86_64-linux;; arm64) TR=aarch64-linux;; *) fehler "Architektur $A"; TR=?;; esac
[ "$(ls /usr/lib/clap/*.clap | wc -l)" = "$N" ] || fehler "nicht alle CLAP in /usr/lib/clap"
[ "$(ls -d /usr/lib/vst3/*.vst3 | wc -l)" = "$N" ] || fehler "nicht alle VST3 in /usr/lib/vst3"
[ "$(ls /usr/lib/vst3/*.vst3/Contents/$TR/*.so | wc -l)" = "$N" ] || fehler "VST3 nicht in Contents/$TR"
[ "$(ls /usr/lib/vst3/*.vst3/Contents/Resources/moduleinfo.json | wc -l)" = "$N" ] || fehler "moduleinfo.json fehlt"
for f in /usr/lib/clap/*.clap /usr/lib/vst3/*.vst3/Contents/$TR/*.so; do [ "$(stat -c %a "$f")" = 644 ] || fehler "$f hat Rechte $(stat -c %a "$f")"; done
[ -x /usr/bin/tonestack-reaper-setup ] || fehler "tonestack-reaper-setup fehlt"
# Handbuch und Anleitung im Paket (auf dem Rechner fehlen sie, wo dpkg /usr/share/man und /usr/share/doc ausnimmt, etwa in schlanken Containern)
grep -q ' ./usr/share/man/man1/tonestack-reaper-setup.1.gz$' "$T/inhalt" || fehler "Handbuchseite fehlt im Paket"
grep -q ' ./usr/share/doc/tonestack-plugins/README-Linux.txt$' "$T/inhalt" || fehler "README-Linux.txt fehlt im Paket"
grep -q "Installation erfolgreich" "$T/aus1" || fehler "keine Erfolgsmeldung"
soll $U1 "$V2" "Eintrag an vorhandene Zeile"
[ "$(sed -n 2p "$(ini $U2)")" = "$V2" ] || fehler "neue Zeile nicht direkt nach [REAPER] ($(sed -n 2p "$(ini $U2)"))"
[ "$(grep -c '^vstpath=' "$(ini $U2)")" = 2 ] && grep -qx 'vstpath=x' "$(ini $U2)" || fehler "Zeile im Abschnitt [andere] verändert"
soll $U3 "$V2" "schon eingetragen"
for u in $U1 $U2; do [ -f "$(ini $u).vor-tonestack" ] || fehler "keine Sicherung bei $u"; [ -f "$(ini $u).tonestack" ] || fehler "keine Merkdatei bei $u"; rechte $u Installieren; done
[ ! -e "$(ini $U3).tonestack" ] && [ ! -e "$(ini $U3).vor-tonestack" ] || fehler "$U3: Merkdatei/Sicherung, obwohl schon eingetragen"
cmp -s "$(ini $U1).vor-tonestack" <(printf '%s\n' '[REAPER]' 'a=1' "$V1" 'b=2') || fehler "Sicherung von $U1 ist nicht der alte Stand"
echo "== zweites Installieren"
dpkg -i "$DEB" > /dev/null
soll $U1 "$V2" "zweites Installieren"; [ "$(grep -c '^vstpath=' "$(ini $U2)")" = 2 ] || fehler "zweites Installieren: Zeile doppelt"
echo "== Entfernen"
dpkg -r tonestack-plugins
soll $U1 "$V1" "Entfernen"; [ "$(sed -n 2p "$(ini $U2)")" = "$V1" ] || fehler "Entfernen bei $U2"
soll $U3 "$V2" "selbst gesetzter Eintrag muss bleiben"; rechte $U1 Entfernen
for u in $U1 $U2; do [ ! -e "$(ini $u).tonestack" ] || fehler "Merkdatei bei $u bleibt"; done
[ ! -e /usr/bin/tonestack-reaper-setup ] || fehler "tonestack-reaper-setup bleibt nach dem Entfernen"
echo "== REAPER läuft bei $U1"
mk $U1 '[REAPER]' "$V1"; cp "$(command -v sleep)" "$T/reaper"; chmod 755 "$T"
runuser -u $U1 -- "$T/reaper" 120 & sleep 1
pgrep -u $U1 -x reaper >/dev/null || fehler "Test-REAPER läuft nicht"
dpkg -i "$DEB" | tee "$T/aus2"
soll $U1 "$V1" "bei laufendem REAPER unberührt"
grep -q "tonestack-reaper-setup" "$T/aus2" || fehler "kein Hinweis bei laufendem REAPER"
als $U1 tonestack-reaper-setup && fehler "Werkzeug bei laufendem REAPER ohne Fehlerstatus" || [ $? = 2 ] || fehler "Werkzeug bei laufendem REAPER: Status nicht 2"
pkill -u $U1 -x reaper; wait 2>/dev/null || true
als $U1 tonestack-reaper-setup | tee "$T/aus3"
soll $U1 "$V2" "von Hand nach dem Beenden"; rechte $U1 "von Hand"
grep -q "eingetragen" "$T/aus3" || fehler "keine Meldung des Werkzeugs"
echo "== Werkzeug: root, Verknüpfung, --undo, --help"
tonestack-reaper-setup --ini "$(ini $U1)" 2>/dev/null && fehler "Werkzeug läuft als root" || true
als $U1 tonestack-reaper-setup --help | grep -q -- --undo || fehler "--help"
als $U1 tonestack-reaper-setup --undo > /dev/null; soll $U1 "$V1" "--undo"
mkdir -p /home/$U2/x; mv "$(ini $U2)" /home/$U2/x/r.ini; ln -s /home/$U2/x/r.ini "$(ini $U2)"; chown -h $U2:$U2 "$(ini $U2)"
als $U2 tonestack-reaper-setup 2>/dev/null && fehler "Verknüpfung bearbeitet" || true
rm "$(ini $U2)"; mk $U2 '[REAPER]' 'a=1'
echo "== Übernahme der Einträge von 1.17227-3, Entfernen bei fremden Plugins in /usr/lib/vst3"
dpkg -r tonestack-plugins > /dev/null
mk $U1 '[REAPER]' "$V2"; mkdir -p /var/lib/tonestack-plugins; echo "$(ini $U1)" > /var/lib/tonestack-plugins/reaper-vstpath
dpkg -i "$DEB" > /dev/null
[ -f "$(ini $U1).tonestack" ] || fehler "Eintrag von 1.17227-3 nicht übernommen"; [ ! -e /var/lib/tonestack-plugins ] || fehler "/var/lib/tonestack-plugins bleibt"
mkdir -p /usr/lib/vst3/Fremd.vst3; dpkg -P tonestack-plugins | tee "$T/aus4"
soll $U1 "$V2" "fremde Plugins in /usr/lib/vst3"; grep -q "andere Plugins" "$T/aus4" || fehler "kein Hinweis auf andere Plugins"
rm -rf /usr/lib/vst3/Fremd.vst3; rmdir /usr/lib/vst3 2>/dev/null || true
dpkg -i "$DEB" > /dev/null; dpkg -P tonestack-plugins > /dev/null; soll $U1 "$V1" "übernommener Eintrag beim Entfernen"
[ "$bad" = 0 ] && echo "Debian-Paket: alle Prüfungen bestanden" || { echo "Debian-Paket: FEHLER" >&2; exit 1; }
