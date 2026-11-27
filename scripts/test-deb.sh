#!/bin/bash
# Prüft das Debian-Paket durch echtes Installieren (braucht root, ändert das System: legt zwei Testbenutzer an und entfernt sie wieder).
# Läuft im Bau auf GitHub nach dem Packen; von Hand: sudo bash scripts/test-deb.sh dist/tonestack-plugins_<DEBVER>_amd64.deb
# Geprüft: alle Plugins landen in /usr/lib/clap und /usr/lib/vst3; /usr/lib/vst3 wird in die REAPER-Einstellungen (vstpath=) jedes Benutzers
# eingetragen, der REAPER eingerichtet hat – auch ohne sudo-Umgebung (PackageKit), Besitzer und Rechte der Datei bleiben, Sicherung angelegt,
# zweites Installieren trägt nichts doppelt ein; bei laufendem REAPER bleibt die Datei unberührt; Entfernen nimmt den Eintrag wieder heraus.
set -euo pipefail
DEB="${1:?Paket angeben}"; [ "$(id -u)" = 0 ] || { echo "als root ausführen" >&2; exit 1; }
! dpkg -s tonestack-plugins >/dev/null 2>&1 || { echo "tonestack-plugins ist schon installiert – Test bräche das vorhandene Paket" >&2; exit 1; }
U1=tstest1; U2=tstest2; bad=0
fehler() { echo "FEHLER: $*" >&2; bad=1; }
for u in $U1 $U2; do id "$u" >/dev/null 2>&1 && { echo "Benutzer $u gibt es schon" >&2; exit 1; }; useradd -m "$u"; done
aufraeumen() { pkill -u $U1 -x reaper 2>/dev/null || true; dpkg -P tonestack-plugins >/dev/null 2>&1 || true; userdel -r $U1 2>/dev/null || true; userdel -r $U2 2>/dev/null || true; rm -rf "$T"; }
T="$(mktemp -d)"; trap aufraeumen EXIT
ini() { echo "/home/$1/.config/REAPER/reaper.ini"; }
mk() { local u="$1"; shift; mkdir -p "/home/$u/.config/REAPER"; printf '%s\n' "$@" > "$(ini "$u")"; chown -R "$u:$u" "/home/$u/.config"; chmod 600 "$(ini "$u")"; }
zeile() { grep '^vstpath=' "$(ini "$1")" || true; }
soll() { [ "$(zeile "$1")" = "$2" ] || fehler "$3: $1 hat „$(zeile "$1")“, erwartet „$2“"; }
rechte() { [ "$(stat -c '%U %a' "$(ini "$1")")" = "$1 600" ] || fehler "$2: Besitzer/Rechte von $(ini "$1") geändert ($(stat -c '%U %a' "$(ini "$1")"))"; }

echo "== Installieren ohne sudo-Umgebung (wie PackageKit), zwei Benutzer"
mk $U1 '[REAPER]' 'a=1' 'vstpath=~/.vst;~/.vst3' 'b=2'; mk $U2 '[REAPER]' 'a=1'
env -u SUDO_USER -u PKEXEC_UID dpkg -i "$DEB" | tee "$T/aus1"
N="$(dpkg-deb -c "$DEB" | grep -c '\.clap$' || true)"
[ "$(ls /usr/lib/clap/*.clap | wc -l)" = "$N" ] || fehler "nicht alle CLAP in /usr/lib/clap"
[ "$(ls -d /usr/lib/vst3/*.vst3 | wc -l)" = "$N" ] || fehler "nicht alle VST3 in /usr/lib/vst3"
grep -q "Installation erfolgreich" "$T/aus1" || fehler "keine Erfolgsmeldung"
soll $U1 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' "Eintrag an vorhandene Zeile"
soll $U2 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' "Zeile ergänzt"
rechte $U1 Installieren; rechte $U2 Installieren
for u in $U1 $U2; do [ -f "$(ini $u).vor-tonestack" ] || fehler "keine Sicherung bei $u"; done
[ "$(sed -n 2p "$(ini $U2)")" = 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' ] || fehler "neue Zeile nicht im Abschnitt [REAPER]"
echo "== zweites Installieren"
dpkg -i "$DEB" > /dev/null
soll $U1 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' "zweites Installieren"
echo "== Entfernen"
dpkg -r tonestack-plugins
soll $U1 'vstpath=~/.vst;~/.vst3' "Entfernen"; soll $U2 'vstpath=~/.vst;~/.vst3' "Entfernen"; rechte $U1 Entfernen
[ ! -e /var/lib/tonestack-plugins ] || fehler "/var/lib/tonestack-plugins bleibt nach dem Entfernen"
echo "== REAPER läuft bei $U1"
mk $U1 '[REAPER]' 'vstpath=~/.vst;~/.vst3'; cp "$(command -v sleep)" "$T/reaper"; chmod 755 "$T"
runuser -u $U1 -- "$T/reaper" 120 & sleep 1
pgrep -u $U1 -x reaper >/dev/null || fehler "Test-REAPER läuft nicht"
dpkg -i "$DEB" | tee "$T/aus2"
soll $U1 'vstpath=~/.vst;~/.vst3' "bei laufendem REAPER unberührt"
grep -q "dpkg-reconfigure tonestack-plugins" "$T/aus2" || fehler "kein Hinweis bei laufendem REAPER"
pkill -u $U1 -x reaper; wait 2>/dev/null || true
dpkg-reconfigure tonestack-plugins > /dev/null
soll $U1 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' "nach dpkg-reconfigure"
echo "== Entfernen, wenn in /usr/lib/vst3 noch anderes liegt"
mkdir -p /usr/lib/vst3/Fremd.vst3; dpkg -P tonestack-plugins
soll $U1 'vstpath=~/.vst;~/.vst3;/usr/lib/vst3' "fremde Plugins in /usr/lib/vst3"
rm -rf /usr/lib/vst3/Fremd.vst3; rmdir /usr/lib/vst3 2>/dev/null || true
[ "$bad" = 0 ] && echo "Debian-Paket: alle Prüfungen bestanden" || { echo "Debian-Paket: FEHLER" >&2; exit 1; }
