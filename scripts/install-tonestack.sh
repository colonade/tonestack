#!/bin/bash
# Installiert oder aktualisiert die Klangregler-Simulatoren (tonestack-apache.zip) auf Apache 2.4 (Debian/Ubuntu).
#
#   sudo ./install-tonestack.sh [ZIP-Datei] [Optionen]
#
# Optionen:
#   --ziel PFAD        Installationsordner        (Standard: /var/www/html/tonestack)
#   --url PFAD         URL-Pfad für die Prüfung   (Standard: /tonestack)
#   --host NAME        Host für die Prüfung       (Standard: localhost)
#   --deinstallieren   Ordner und Apache-Einstellung entfernen (Sicherungen bleiben)
#   --nur-pruefen      nichts ändern, nur die installierten Seiten prüfen
#   --hilfe            Diese Hilfe
#
# Was das Skript tut:
#   1. prüft Apache, das ZIP und dessen Inhalt
#   2. aktiviert die Module headers, expires, deflate, mime
#   3. erlaubt die mitgelieferte .htaccess nur für den Installationsordner
#      (/etc/apache2/conf-available/tonestack-<ordner>.conf)
#   4. sichert eine vorhandene Installation (Ordner.bak-DATUM) und installiert neu
#   5. prüft die Apache-Konfiguration; bei Fehlern wird der alte Stand wiederhergestellt
#   6. lädt Apache neu und prüft die Seiten per curl

set -euo pipefail

ZIP=""
ZIEL="/var/www/html/tonestack"
URLPFAD="/tonestack"
HOST="localhost"
AKTION="install"

rot()  { printf '\033[31m%s\033[0m\n' "$*"; }
gruen(){ printf '\033[32m%s\033[0m\n' "$*"; }
info() { printf '  %s\n' "$*"; }
fehler(){ rot "FEHLER: $*"; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --ziel)  ZIEL="${2%/}"; shift 2 ;;
    --url)   URLPFAD="/${2#/}"; URLPFAD="${URLPFAD%/}"; shift 2 ;;
    --host)  HOST="$2"; shift 2 ;;
    --deinstallieren) AKTION="remove"; shift ;;
    --nur-pruefen) AKTION="check"; shift ;;
    --hilfe|-h) sed -n '2,22p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*) fehler "Unbekannte Option: $1 (--hilfe zeigt alle)" ;;
    *)  ZIP="$1"; shift ;;
  esac
done

[ "$(id -u)" -eq 0 ] || fehler "Bitte mit sudo ausführen."
# eigene Apache-Einstellung je Installationsordner, damit sich mehrere Installationen nicht stören
CONF_NAME="tonestack$(printf '%s' "$ZIEL" | tr -c 'A-Za-z0-9' '-' | sed 's/--*/-/g; s/-$//')"
CONF="/etc/apache2/conf-available/${CONF_NAME}.conf"
command -v apache2ctl >/dev/null || fehler "Apache (apache2) ist nicht installiert: sudo apt install apache2"

apache_neu_laden() {
  if command -v systemctl >/dev/null && systemctl is-active --quiet apache2 2>/dev/null; then
    systemctl reload apache2
  else
    apache2ctl graceful 2>/dev/null || apache2ctl start
  fi
}

# ---------- Prüfung (folgt Weiterleitungen, prüft auch den Inhalt) ----------
seiten_pruefen() {
  command -v curl >/dev/null || { rot "curl fehlt – Prüfung übersprungen (sudo apt install curl)"; PROBLEME=0; return; }
  local BASIS="http://$HOST$URLPFAD" ziel code typ kopf
  # -L: Weiterleitungen folgen (z. B. http → https); -k: Zertifikat bei localhost nicht prüfen – es geht nur um die eigenen Dateien
  CURL=(curl -sSLk --max-time 15)
  ziel="$("${CURL[@]}" -o /dev/null -w '%{url_effective}' "$BASIS/" 2>/dev/null || true)"
  if [ -n "$ziel" ] && [ "$ziel" != "$BASIS/" ]; then info "Weiterleitung: $BASIS/ → $ziel"; fi
  PROBLEME=0
  pruefe() {   # Pfad, erwarteter Text im Inhalt, Beschreibung
    local inhalt code
    inhalt="$("${CURL[@]}" -w '\n%{http_code}' "$BASIS$1" 2>/dev/null || true)"; code="${inhalt##*$'\n'}"
    # Inhalt ohne Pipe prüfen: "printf | grep -q" bricht bei großen Seiten mit pipefail fälschlich ab
    if [ "$code" = "200" ] && [[ "$inhalt" == *"$2"* ]]; then info "ok   $3"
    elif [ "$code" = "200" ]; then rot "  FALSCH $3: Seite kommt an, aber mit fremdem Inhalt (Umleitung auf Anmeldung oder andere Seite?) – $BASIS$1"; PROBLEME=$((PROBLEME+1))
    else rot "  FEHLT $3: Status $code – $BASIS$1"; PROBLEME=$((PROBLEME+1)); fi
  }
  pruefe "/" "laney-aor50/" "Übersicht"
  pruefe "/yamaha-b100ii/" "B100-II" "Yamaha B100-II"
  pruefe "/laney-aor50/" "AOR 50" "Laney AOR 50"
  pruefe "/yamaha-b100ii/b100ii-worklet.js" "registerProcessor" "Yamaha-Rechenkern"
  pruefe "/laney-aor50/laney-worklet.js" "registerProcessor" "Laney-Rechenkern"
  code="$("${CURL[@]}" -o /dev/null -w '%{http_code}' "$BASIS/.htaccess" 2>/dev/null || true)"
  if [ "$code" = "403" ] || [ "$code" = "404" ]; then info "ok   .htaccess nicht abrufbar ($code)"; else rot "  .htaccess ist öffentlich abrufbar (Status $code)"; PROBLEME=$((PROBLEME+1)); fi
  typ="$("${CURL[@]}" -o /dev/null -w '%{content_type}' "$BASIS/yamaha-b100ii/b100ii-worklet.js" 2>/dev/null || true)"
  case "$typ" in text/javascript*) info "ok   Rechenkern als JavaScript ausgeliefert" ;;
    *) rot "  Rechenkern hat Dateityp '${typ:-?}' statt text/javascript – Audio-Thread würde nicht starten"; PROBLEME=$((PROBLEME+1)) ;; esac
  kopf="$("${CURL[@]}" -D - -o /dev/null "$BASIS/" 2>/dev/null || true)"
  if grep -qi '^content-security-policy:' <<< "$kopf"; then info "ok   .htaccess wird gelesen (Sicherheits-Kopfzeilen vorhanden)"
  else rot "  .htaccess wird nicht gelesen – AllowOverride greift nicht (andere <Directory>-Regel oder anderer DocumentRoot im https-VirtualHost?)"; PROBLEME=$((PROBLEME+1)); fi
}

# ---------- Nur prüfen ----------
if [ "$AKTION" = "check" ]; then
  echo "Prüfung von http://$HOST$URLPFAD/"
  seiten_pruefen
  if [ "$PROBLEME" -eq 0 ]; then gruen "Alles in Ordnung."; exit 0; else rot "$PROBLEME Prüfung(en) fehlgeschlagen."; exit 2; fi
fi

# ---------- Deinstallieren ----------
if [ "$AKTION" = "remove" ]; then
  echo "Deinstallation von $ZIEL"
  [ -e "$CONF" ] && { a2disconf -q "$CONF_NAME" >/dev/null 2>&1 || true; rm -f "$CONF"; info "Apache-Einstellung entfernt"; }
  [ -d "$ZIEL" ] && { rm -rf "$ZIEL"; info "Ordner entfernt"; }
  apache2ctl configtest >/dev/null 2>&1 && apache_neu_laden
  gruen "Fertig. Sicherungen ($ZIEL.bak-*) wurden nicht gelöscht."
  exit 0
fi

# ---------- 1. Prüfen ----------
echo "1/6 Prüfen"
if [ -z "$ZIP" ]; then
  for k in ./tonestack-apache.zip "$(dirname "$0")/tonestack-apache.zip"; do [ -f "$k" ] && { ZIP="$k"; break; }; done
fi
[ -n "$ZIP" ] && [ -f "$ZIP" ] || fehler "ZIP nicht gefunden. Aufruf: sudo $0 /pfad/tonestack-apache.zip"
info "Paket:      $ZIP ($(sha256sum "$ZIP" | cut -c1-16)…)"
info "Ziel:       $ZIEL"
info "Adresse:    http://$HOST$URLPFAD/"
info "Apache:     $(apache2ctl -v 2>/dev/null | sed -n 's/^Server version: //p')"

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
if command -v unzip >/dev/null; then unzip -q "$ZIP" -d "$TMP"
else python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$ZIP" "$TMP"; fi
NEU="$TMP/tonestack"
for f in index.html sw.js .htaccess yamaha-b100ii/index.html yamaha-b100ii/b100ii-worklet.js laney-aor50/index.html laney-aor50/laney-worklet.js assets/fonts/archivo-latin.woff2; do
  [ -f "$NEU/$f" ] || fehler "Im ZIP fehlt tonestack/$f – ist es das richtige Paket?"
done
info "Paketinhalt vollständig (Version $(grep -o 'VERSION = "[^"]*"' "$NEU/sw.js" | cut -d'"' -f2))"

# ---------- 2. Module ----------
echo "2/6 Apache-Module"
MODULE="$(apache2ctl -M 2>/dev/null || true)"
for m in headers expires deflate mime; do
  if grep -q "^ ${m}_module" <<< "$MODULE"; then info "$m: bereits aktiv"
  else a2enmod -q "$m" >/dev/null; info "$m: aktiviert"; fi
done

# ---------- 3. .htaccess erlauben (nur für diesen Ordner) ----------
echo "3/6 .htaccess für $ZIEL erlauben"
CONF_ALT=""; [ -f "$CONF" ] && CONF_ALT="$(cat "$CONF")"
cat > "$CONF" <<EOF
# Klangregler-Simulatoren: mitgelieferte .htaccess für diesen Ordner zulassen (angelegt von install-tonestack.sh)
<Directory "$ZIEL">
    AllowOverride FileInfo Indexes Options
    Require all granted
</Directory>
EOF
a2enconf -q "$CONF_NAME" >/dev/null
info "$CONF aktiv"

# ---------- 4. Sichern und installieren ----------
echo "4/6 Installieren"
SICHERUNG=""
if [ -e "$ZIEL" ]; then
  SICHERUNG="$ZIEL.bak-$(date +%Y%m%d-%H%M%S)"
  mv "$ZIEL" "$SICHERUNG"; info "bisherige Installation gesichert: $SICHERUNG"
fi
mkdir -p "$(dirname "$ZIEL")"
cp -a "$NEU" "$ZIEL"
chown -R root:root "$ZIEL"
find "$ZIEL" -type d -exec chmod 755 {} +
find "$ZIEL" -type f -exec chmod 644 {} +
info "Dateien kopiert, Rechte 755/644"

# ---------- 5. Konfiguration prüfen (bei Fehler zurück) ----------
echo "5/6 Apache-Konfiguration prüfen"
if ! TEST="$(apache2ctl configtest 2>&1)"; then
  rot "$TEST"
  rm -rf "$ZIEL"; [ -n "$SICHERUNG" ] && mv "$SICHERUNG" "$ZIEL"
  if [ -n "$CONF_ALT" ]; then printf '%s\n' "$CONF_ALT" > "$CONF"; else a2disconf -q "$CONF_NAME" >/dev/null 2>&1 || true; rm -f "$CONF"; fi
  fehler "Apache-Konfiguration fehlerhaft – alter Stand wiederhergestellt, Apache nicht neu geladen."
fi
info "Syntax OK"
apache_neu_laden
info "Apache neu geladen"

# ---------- 6. Seiten prüfen ----------
echo "6/6 Seiten prüfen"
sleep 1
seiten_pruefen
BASIS="http://$HOST$URLPFAD"
echo
if [ "$PROBLEME" -eq 0 ]; then
  gruen "Installation erfolgreich: $BASIS/"
  [ -n "$SICHERUNG" ] && info "Sicherung des alten Stands: $SICHERUNG (kann gelöscht werden, wenn alles läuft)"
  info "Hinweis: Audio-Thread, Offline-Betrieb und 'Zum Startbildschirm' nur über https (z. B. certbot --apache)."
else
  rot "$PROBLEME Prüfung(en) fehlgeschlagen – Details oben."
  [ -n "$SICHERUNG" ] && info "Alter Stand liegt unter $SICHERUNG (zurück: sudo rm -rf $ZIEL && sudo mv $SICHERUNG $ZIEL)"
  info "Liegt die Seite unter einem anderen Namen? Prüfen mit: sudo bash $0 --nur-pruefen --host deine.domain"
  exit 2
fi
