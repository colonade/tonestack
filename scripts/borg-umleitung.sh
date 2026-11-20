#!/bin/bash
# Leitet die alte Apache-Installation der Klangregler-Simulatoren auf die neue Adresse um (ab 2026-11-20: GitHub Pages).
#
#   sudo bash borg-umleitung.sh [Optionen]
#
# Optionen:
#   --ziel PFAD     alter Installationsordner   (Standard: /var/www/html/tonestack)
#   --url PFAD      alter URL-Pfad              (Standard: /tonestack)
#   --neu URL       neue Adresse                (Standard: https://tonestack.fluxstatus.de)
#   --basis URL     alte Adresse für die Prüfung (Standard: http://localhost; z. B. https://borg.fluxstatus.de)
#   --trotzdem      auch umleiten, wenn die neue Adresse (noch) nicht antwortet
#   --nur-pruefen   nichts ändern, nur die Umleitung prüfen
#   --hilfe         diese Hilfe
#
# Was das Skript tut:
#   1. prüft, ob die neue Adresse antwortet (sonst Abbruch, außer mit --trotzdem)
#   2. sichert den alten Ordner nach /var/backups/tonestack/ (tar.gz) und leert ihn bis auf
#      sw.js: ein Service Worker, der den alten Offline-Speicher im Browser löscht, sich abmeldet
#      und offene Seiten auf die neue Adresse lädt (ein umgeleiteter sw.js würde vom Browser
#      nicht angenommen, alte Offline-Kopien blieben sonst stehen)
#   3. legt /etc/apache2/conf-available/tonestack-umleitung.conf an: dauerhafte Umleitung (301)
#      jeder Seite <url>/… auf <neu>/… mit demselben Pfad, außer <url>/sw.js
#   4. prüft die Apache-Konfiguration (bei Fehler zurück), lädt Apache neu und prüft per curl
# Rückgängig: sudo a2disconf tonestack-umleitung && sudo systemctl reload apache2, Sicherung zurückspielen.
set -euo pipefail
ZIEL="/var/www/html/tonestack"; URLPFAD="/tonestack"; NEU="https://tonestack.fluxstatus.de"; BASIS="http://localhost"; TROTZDEM=0; NUR=0
fehler() { echo "FEHLER: $*" >&2; exit 1; }
while [ $# -gt 0 ]; do case "$1" in
  --ziel) ZIEL="${2%/}"; shift 2;; --url) URLPFAD="/${2#/}"; URLPFAD="${URLPFAD%/}"; shift 2;; --neu) NEU="${2%/}"; shift 2;;
  --basis) BASIS="${2%/}"; shift 2;; --trotzdem) TROTZDEM=1; shift;; --nur-pruefen) NUR=1; shift;;
  --hilfe|-h) sed -n '2,24p' "$0" | sed 's/^# \{0,1\}//'; exit 0;; *) fehler "unbekannte Option $1 (--hilfe)";; esac; done
# Eingaben landen in der Apache-Konfiguration: nur einfache Zeichen
ZIEL="$(realpath -m -- "$ZIEL")"
[[ "$ZIEL" =~ ^/[A-Za-z0-9._/-]+$ ]] || fehler "ungültiger Ordner '$ZIEL'"
case "$ZIEL" in /|/var|/var/www|/var/www/html|/srv|/home|/root|/tmp|/opt|/etc|/etc/*|/usr|/usr/*|/bin|/bin/*|/sbin|/sbin/*|/lib|/lib/*|/boot|/boot/*|/proc|/proc/*|/sys|/sys/*|/dev|/dev/*) fehler "Ordner '$ZIEL' ist gesperrt";; esac
[[ "$URLPFAD" =~ ^/[A-Za-z0-9._~-]+(/[A-Za-z0-9._~-]+)*$ ]] || fehler "ungültiger URL-Pfad '$URLPFAD'"
[[ "$NEU" =~ ^https://[A-Za-z0-9.-]+(/[A-Za-z0-9._~-]+)*$ ]] || fehler "ungültige neue Adresse '$NEU' (nur https://host[/pfad])"
[[ "$BASIS" =~ ^https?://[A-Za-z0-9.:-]+$ ]] || fehler "ungültige Basis '$BASIS' (http(s)://host)"
CONF="/etc/apache2/conf-available/tonestack-umleitung.conf"
RX="$(printf '%s' "$URLPFAD" | sed 's/[.]/\\./g')"   # Punkte im Pfad als Zeichen

pruefen() {   # Umleitungen (Weiterleitungen folgen, z. B. erst http → https auf dem alten Host) und sw.js per curl
  local b="$BASIS$URLPFAD" p code loc want typ ok=0
  for p in "" "/" "/laney-aor50/" "/plugins/" "/download/SHA256SUMS.txt"; do
    read -r code loc < <(curl -s -L --max-redirs 5 --max-time 20 -o /dev/null -w '%{http_code} %{url_effective}\n' "$b$p" || true)
    case "$p" in "") want="$NEU/";; *) want="$NEU$p";; esac
    if [ "$loc" = "$want" ] && { [ "$code" = 200 ] || [ "$TROTZDEM" = 1 ]; }; then echo "  ok     $URLPFAD$p → $loc ($code)"
    else echo "  FALSCH $URLPFAD$p → ${loc:-?} ($code), erwartet $want (200)"; ok=1; fi; done
  read -r code typ loc < <(curl -s -L --max-redirs 5 --max-time 20 -o /dev/null -w '%{http_code} %{content_type} %{url_effective}\n' "$b/sw.js" || true)
  if [ "$code" = 200 ] && [[ "$typ" == *javascript* ]] && [[ "$loc" == *"$URLPFAD/sw.js" ]] && [[ "$loc" != "$NEU"* ]] && curl -s -L --max-redirs 5 "$b/sw.js" | grep -q "umgezogen"; then
    echo "  ok     $URLPFAD/sw.js: Aufräum-Service-Worker ($loc, $typ)"
  else echo "  FALSCH $URLPFAD/sw.js: $code $typ $loc (erwartet 200, JavaScript, nicht umgeleitet)"; ok=1; fi
  return $ok; }

if [ "$NUR" = 1 ]; then echo "Prüfung von $BASIS$URLPFAD"; if pruefen; then echo "Umleitung in Ordnung"; exit 0; else echo "Umleitung fehlerhaft"; exit 1; fi; fi
[ "$(id -u)" = 0 ] || fehler "bitte mit sudo starten"
command -v apache2ctl >/dev/null || fehler "apache2ctl fehlt (kein Apache?)"
command -v curl >/dev/null || fehler "curl fehlt"
code="$(curl -s -o /dev/null -w '%{http_code}' --max-time 20 "$NEU/" || true)"; code="${code:-000}"
if [ "$code" = 200 ]; then echo "Neue Adresse $NEU/ antwortet (200)"
elif [ "$TROTZDEM" = 1 ]; then echo "Warnung: $NEU/ antwortet mit $code – leite trotzdem um"
else fehler "$NEU/ antwortet mit $code; erst die neue Seite einrichten (oder --trotzdem)"; fi
[ -d "$ZIEL" ] || mkdir -p "$ZIEL"
a2enmod -q alias >/dev/null; a2enmod -q headers >/dev/null
# 2. Sicherung und Aufräum-Service-Worker
mkdir -p /var/backups/tonestack; chmod 700 /var/backups/tonestack
SICH="/var/backups/tonestack/vor-umleitung-$(date +%Y%m%d-%H%M%S).tar.gz"
tar czf "$SICH" -C "$(dirname "$ZIEL")" "$(basename "$ZIEL")"; echo "Sicherung: $SICH"
find "$ZIEL" -mindepth 1 -delete
cat > "$ZIEL/sw.js" <<EOF
// tonestack ist umgezogen nach $NEU/ – dieser Service Worker löscht den alten Offline-Speicher dieser Adresse,
// meldet sich ab und lädt offene Seiten unter der neuen Adresse.
self.addEventListener("install", () => self.skipWaiting());
self.addEventListener("activate", e => e.waitUntil((async () => {
  const scope = self.registration.scope;
  for (const k of await caches.keys()) if ((k.startsWith("tonestack-") && k.endsWith("@" + scope)) || /^\\d{4}-\\d\\d-\\d\\d(-\\d+)?\$/.test(k)) await caches.delete(k);
  await self.registration.unregister();
  for (const c of await self.clients.matchAll({ type: "window" })) {
    const p = new URL(c.url).pathname.slice(new URL(scope).pathname.length);
    c.navigate("$NEU/" + p).catch(() => {}); }
})()));
EOF
printf '%s\n' '<IfModule mod_headers.c>' '  <Files "sw.js">' '    Header set Cache-Control "no-cache"' '  </Files>' '</IfModule>' > "$ZIEL/.htaccess"
chmod 644 "$ZIEL/sw.js" "$ZIEL/.htaccess"
# 3. Umleitung
ALT=""; [ -f "$CONF" ] && ALT="$(cat "$CONF")"
cat > "$CONF" <<EOF
# Klangregler-Simulatoren: umgezogen nach $NEU/ (angelegt von borg-umleitung.sh)
# Jede Seite unter $URLPFAD/ dauerhaft (301) auf dieselbe Seite unter $NEU/, außer sw.js (räumt den alten Offline-Speicher auf).
RedirectMatch 301 "^$RX\$" "$NEU/"
RedirectMatch 301 "^$RX/(?!sw\\.js\$)(.*)\$" "$NEU/\$1"
EOF
a2enconf -q tonestack-umleitung >/dev/null
if ! apache2ctl configtest >/dev/null 2>&1; then
  apache2ctl configtest || true
  if [ -n "$ALT" ]; then printf '%s\n' "$ALT" > "$CONF"; else a2disconf -q tonestack-umleitung >/dev/null; rm -f "$CONF"; fi
  fehler "Apache-Konfiguration fehlerhaft – Umleitung nicht aktiviert (Ordner ist geleert, Sicherung: $SICH)"; fi
if command -v systemctl >/dev/null && systemctl is-active --quiet apache2 2>/dev/null; then systemctl reload apache2; else apache2ctl graceful 2>/dev/null || apache2ctl start; fi
echo "Prüfung von $BASIS$URLPFAD"
if pruefen; then echo "Umleitung aktiv: $URLPFAD/… → $NEU/…"; else echo "Umleitung fehlerhaft (Konfiguration: $CONF)"; exit 1; fi
