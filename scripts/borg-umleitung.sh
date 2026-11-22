#!/bin/bash
# Leitet die alte Apache-Installation der Klangregler-Simulatoren auf die neue Adresse um (ab 2026-11-20: GitHub Pages).
#
#   sudo bash borg-umleitung.sh [Optionen]
#
# Optionen:
#   --ziel PFAD      alter Installationsordner, unter /var/www/ oder /srv/ (Standard: /var/www/html/tonestack)
#   --url PFAD       alter URL-Pfad                 (Standard: /tonestack)
#   --neu URL        neue Adresse                   (Standard: https://tonestack.fluxstatus.de)
#   --basis URL      alte Adresse für die Prüfung   (Standard: https://borg.fluxstatus.de; https nehmen, wenn http auf https umleitet)
#   --trotzdem       auch umleiten, wenn die neue Adresse (noch) nicht antwortet
#   --nur-pruefen    nichts ändern, nur die Umleitung prüfen (ohne root)
#   --rueckgaengig   Umleitung entfernen und die gesicherte alte Seite zurückholen
#   --hilfe          diese Hilfe
#
# Was das Skript tut:
#   1. prüft die neue Adresse (Übersicht und eine Unterseite) und die Apache-Konfiguration vor jeder Änderung
#   2. legt /etc/apache2/conf-available/tonestack-umleitung.conf an: dauerhafte Umleitung (301) jeder Adresse
#      <url>/… auf <neu>/… mit demselben Pfad und Abfrageteil, außer <url>/sw.js; prüft die Konfiguration erneut
#   3. verschiebt den alten Ordner nach /var/backups/tonestack/vor-umleitung-<zeit>/ (nichts wird gelöscht) und legt
#      an seiner Stelle nur sw.js an: ein Service Worker, der in Browsern mit offenem alten Tab den alten
#      Offline-Speicher löscht, sich abmeldet und den Tab unter der neuen Adresse lädt
#   4. lädt Apache neu und prüft per curl (Status 301, genaues Ziel, sw.js nicht umgeleitet)
#   Bei einem Fehler in 2.–4. wird alles zurückgenommen (Ordner, Konfiguration, Neuladen).
#
# Grenzen:
#   - Die Konfiguration gilt serverweit: in jedem VirtualHost wird <url>/… umgeleitet. Eigene RewriteRule/Redirect
#     in einem VirtualHost haben Vorrang.
#   - Browser, die die alte Seite offline gespeichert haben, laden den Aufräum-Service-Worker nur, wenn ein alter Tab
#     offen ist (Chromium prüft nach einer Umleitung auf eine fremde Adresse nicht auf Updates). Online führt die
#     Umleitung trotzdem richtig weiter; die alte Offline-Kopie bleibt nur als toter Speicher stehen.
#   - Gespeicherte Voreinstellungen gehören zur alten Adresse und sind auf der neuen nicht sichtbar: vorher auf der
#     alten Seite unter „Voreinstellungen“ exportieren und auf der neuen importieren. Als App installierte Seiten neu installieren.
set -euo pipefail
ZIEL="/var/www/html/tonestack"; URLPFAD="/tonestack"; NEU="https://tonestack.fluxstatus.de"; BASIS="https://borg.fluxstatus.de"
TROTZDEM=0; AKTION="umleiten"
fehler() { echo "FEHLER: $*" >&2; exit 1; }
wert() { [ -n "${2:-}" ] || fehler "Option $1 braucht einen Wert (--hilfe)"; }
while [ $# -gt 0 ]; do case "$1" in
  --ziel) wert "$1" "${2:-}"; ZIEL="${2%/}"; shift 2;;
  --url) wert "$1" "${2:-}"; URLPFAD="/${2#/}"; URLPFAD="${URLPFAD%/}"; shift 2;;
  --neu) wert "$1" "${2:-}"; NEU="${2%/}"; shift 2;;
  --basis) wert "$1" "${2:-}"; BASIS="${2%/}"; shift 2;;
  --trotzdem) TROTZDEM=1; shift;;
  --nur-pruefen) AKTION="pruefen"; shift;;
  --rueckgaengig) AKTION="zurueck"; shift;;
  --hilfe|-h) sed -n '2,32p' "$0" | sed 's/^# \{0,1\}//'; exit 0;;
  *) fehler "unbekannte Option $1 (--hilfe)";; esac; done

# Eingaben prüfen: sie landen in der Apache-Konfiguration und im JavaScript; der Ordner wird verschoben
LABEL='[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?'
[[ "$ZIEL" == /* ]] || fehler "--ziel muss ein absoluter Pfad sein"
[[ "$ZIEL" =~ ^/(var/www|srv)(/[A-Za-z0-9._-]+)+$ ]] || fehler "Ordner '$ZIEL' nicht erlaubt (nur unter /var/www/ oder /srv/, einfache Zeichen)"
[ "$ZIEL" != /var/www/html ] || fehler "Ordner '$ZIEL' nicht erlaubt"
[ "$(realpath -m -- "$ZIEL")" = "$ZIEL" ] || fehler "Ordner '$ZIEL' enthält einen symbolischen Verweis, „.“ oder „..“"
[[ "$URLPFAD" =~ ^/[A-Za-z0-9._~-]+(/[A-Za-z0-9._~-]+)*$ ]] || fehler "ungültiger URL-Pfad '$URLPFAD'"
[[ "$NEU" =~ ^https://($LABEL\.)+[A-Za-z]{2,}(/[A-Za-z0-9._~-]+)*$ ]] || fehler "ungültige neue Adresse '$NEU' (nur https://name.tld[/pfad])"
[[ "$BASIS" =~ ^https?://$LABEL(\.$LABEL)*(:[0-9]{1,5})?$ ]] || fehler "ungültige Basis '$BASIS' (http(s)://host[:port])"
host_von() { local h="${1#*://}"; h="${h%%/*}"; h="${h%%:*}"; printf '%s' "${h,,}"; }
NEUHOST="$(host_von "$NEU")"
[ "$NEUHOST" != "$(host_von "$BASIS")" ] || fehler "neue Adresse liegt auf dem alten Host – das wäre eine Umleitungsschleife"
HIER="$(hostname -f 2>/dev/null || true)"; [ "$NEUHOST" != "${HIER,,}" ] || fehler "neue Adresse ist dieser Rechner – das wäre eine Umleitungsschleife"
CONF="/etc/apache2/conf-available/tonestack-umleitung.conf"; CONFAN="/etc/apache2/conf-enabled/tonestack-umleitung.conf"
SICHERUNGEN="/var/backups/tonestack"
RX="$(printf '%s' "$URLPFAD" | sed 's/[.]/\\./g')"   # Punkte im Pfad als Zeichen
MARKE="tonestack ist umgezogen"

art_von_ziel() {   # alt | umgestellt | fehlt | fremd
  if [ ! -e "$ZIEL" ]; then echo fehlt; return; fi
  if [ -L "$ZIEL" ] || [ ! -d "$ZIEL" ]; then echo fremd; return; fi
  if [ -f "$ZIEL/sw.js" ] && grep -q "$MARKE" "$ZIEL/sw.js" && [ -z "$(find "$ZIEL" -mindepth 1 ! -name sw.js ! -name .htaccess -print -quit)" ]; then echo umgestellt; return; fi
  if [ -f "$ZIEL/sw.js" ] && grep -q '"tonestack-"' "$ZIEL/sw.js" && [ -d "$ZIEL/laney-aor50" ] && [ -f "$ZIEL/index.html" ]; then echo alt; return; fi
  echo fremd; }

apache_neu_laden() {
  if command -v systemctl >/dev/null && systemctl is-active --quiet apache2 2>/dev/null; then systemctl reload apache2
  else apache2ctl graceful 2>/dev/null || apache2ctl start; fi; }

# Seite laden (ohne Weiterleitungen zu folgen), bis zu drei Versuche bei Verbindungsfehlern; Ergebnis in HCODE, HTYP, HKOERPER.
# Ohne Pipe zu grep: unter pipefail scheitert „curl | grep -q“, wenn grep beim ersten Treffer aufhört und curl danach noch schreibt
# (Fehler 23) – je nach Ankunft der Daten mal ja, mal nein (2026-11-22 einmal beim Nutzer gesehen).
holen() { local t; t="$(mktemp)"; HCODE=000; HTYP=""
  for _ in 1 2 3; do read -r HCODE HTYP < <(curl -s -o "$t" --max-time 20 -w '%{http_code} %{content_type}\n' "$1" || true); HCODE="${HCODE:-000}"
    [ "$HCODE" = 000 ] || break; sleep 2; done
  HKOERPER="$(cat "$t")"; rm -f "$t"; }

pruefen() {   # ohne Weiterleitungen zu folgen: jede alte Adresse muss selbst mit 301 auf genau das neue Ziel zeigen
  local b="$BASIS$URLPFAD" p code loc want typ ok=0
  for p in "" "/" "/laney-aor50/" "/laney-aor50/?x=1&y=2" "/plugins/" "/download/SHA256SUMS.txt"; do
    read -r code loc < <(curl -s -o /dev/null --max-time 20 -w '%{http_code} %{redirect_url}\n' "$b$p" || true)
    case "$p" in "") want="$NEU/";; *) want="$NEU$p";; esac
    if [ "${code:-}" = 301 ] && [ "${loc:-}" = "$want" ]; then echo "  ok     $URLPFAD$p → 301 $loc"
    else echo "  FALSCH $URLPFAD$p → ${code:-000} ${loc:-ohne Ziel} (erwartet 301 $want)"; ok=1; fi; done
  holen "$b/sw.js"; code="$HCODE"; typ="$HTYP"
  if [ "$code" = 200 ] && [[ "$typ" == *javascript* ]] && [[ "$HKOERPER" == *"$MARKE"* ]]; then
    echo "  ok     $URLPFAD/sw.js → 200 Aufräum-Service-Worker ($typ)"
  else echo "  FALSCH $URLPFAD/sw.js → ${code:-000} ${typ:-} (erwartet 200, JavaScript, Aufräum-Service-Worker, nicht umgeleitet)"; ok=1; fi
  return $ok; }

if [ "$AKTION" = pruefen ]; then
  echo "Prüfung von $BASIS$URLPFAD"; if pruefen; then echo "Umleitung in Ordnung"; exit 0; else echo "Umleitung fehlerhaft"; exit 1; fi; fi

[ "$(id -u)" = 0 ] || fehler "bitte mit sudo starten"
for t in apache2ctl a2enconf a2disconf a2enmod curl; do command -v "$t" >/dev/null || fehler "$t fehlt (Apache 2.4 auf Debian/Ubuntu nötig)"; done
apache2ctl configtest >/dev/null 2>&1 || { apache2ctl configtest || true; fehler "Apache-Konfiguration ist schon vor jeder Änderung fehlerhaft – erst beheben"; }
ART="$(art_von_ziel)"

if [ "$AKTION" = zurueck ]; then
  [ "$ART" = umgestellt ] || fehler "$ZIEL ist nicht umgestellt (Zustand: $ART) – nichts zurückzunehmen"
  SICH="$(find "$SICHERUNGEN" -maxdepth 1 -type d -name 'vor-umleitung-*' 2>/dev/null | sort | tail -n 1)"
  if [ -z "$SICH" ] || [ ! -f "$SICH/sw.js" ] || [ ! -d "$SICH/laney-aor50" ]; then fehler "keine Sicherung der alten Seite in $SICHERUNGEN/vor-umleitung-*/ gefunden"; fi
  [ -e "$CONFAN" ] && a2disconf -q tonestack-umleitung >/dev/null; rm -f "$CONF"
  rm -f "$ZIEL/sw.js" "$ZIEL/.htaccess"; rmdir "$ZIEL"; mv "$SICH" "$ZIEL"
  apache2ctl configtest >/dev/null 2>&1 || { apache2ctl configtest || true; fehler "Apache-Konfiguration fehlerhaft nach dem Zurücknehmen"; }
  apache_neu_laden; echo "Umleitung entfernt, alte Seite aus $SICH zurück in $ZIEL"; exit 0; fi

[ "$ART" != fremd ] || fehler "$ZIEL sieht weder wie die alte Seite noch wie eine Umleitung aus – nichts geändert"

# 1. neue Adresse
holen "$NEU/laney-aor50/"; code="$HCODE"; holen "$NEU/"; code0="$HCODE"; inhalt=nein; [[ "$HKOERPER" == *laney-aor50/* ]] && inhalt=ja
befund="Unterseite $code, Übersicht $code0, Verweis auf laney-aor50/ in der Übersicht: $inhalt"
if [ "$code" = 200 ] && [ "$code0" = 200 ] && [ "$inhalt" = ja ]; then echo "Neue Adresse $NEU/ antwortet (Übersicht und Unterseite)"
elif [ "$TROTZDEM" = 1 ]; then echo "Warnung: $NEU/ antwortet nicht richtig ($befund) – leite trotzdem um"
else fehler "$NEU/ antwortet nicht richtig ($befund); erst die neue Seite einrichten (oder --trotzdem)"; fi

# Rücknahme bei Fehlern ab hier
ALTCONF=""; [ -f "$CONF" ] && ALTCONF="$(cat "$CONF")"
WARAN=0; [ -e "$CONFAN" ] && WARAN=1
NEUDIR=""; SICH=""; VERSCHOBEN=0
zuruecknehmen() {
  trap - ERR; set +e
  echo "Fehler – nehme die Änderungen zurück" >&2
  if [ "$VERSCHOBEN" = 1 ]; then rm -f "$ZIEL/sw.js" "$ZIEL/.htaccess"; rmdir "$ZIEL" && mv "$SICH" "$ZIEL" && echo "  alte Seite zurück in $ZIEL" >&2
  elif [ "$VERSCHOBEN" = 2 ]; then rm -f "$ZIEL/sw.js" "$ZIEL/.htaccess"; rmdir "$ZIEL"; fi
  [ -n "$NEUDIR" ] && rm -rf -- "$NEUDIR"
  if [ -n "$ALTCONF" ]; then printf '%s\n' "$ALTCONF" > "$CONF"; else rm -f "$CONF"; fi
  if [ "$WARAN" = 1 ]; then a2enconf -q tonestack-umleitung >/dev/null 2>&1; else a2disconf -q tonestack-umleitung >/dev/null 2>&1; fi
  if apache2ctl configtest >/dev/null 2>&1; then apache_neu_laden >/dev/null 2>&1; echo "  Apache-Konfiguration wie vorher" >&2
  else echo "  ACHTUNG: Apache-Konfiguration fehlerhaft, bitte prüfen: apache2ctl configtest" >&2; fi
  exit 1; }
trap zuruecknehmen ERR; set -E

a2enmod -q alias >/dev/null; a2enmod -q headers >/dev/null
# 2. Umleitung
cat > "$CONF" <<EOF
# Klangregler-Simulatoren: umgezogen nach $NEU/ (angelegt von borg-umleitung.sh)
# Jede Adresse unter $URLPFAD/ dauerhaft (301) auf dieselbe unter $NEU/ (Abfrageteil bleibt), außer sw.js (räumt den alten Offline-Speicher auf).
RedirectMatch 301 "^$RX\$" "$NEU/"
RedirectMatch 301 "^$RX/(?!sw\\.js\$)(.*)\$" "$NEU/\$1"
EOF
a2enconf -q tonestack-umleitung >/dev/null
apache2ctl configtest >/dev/null 2>&1 || { apache2ctl configtest || true; false; }

# 3. Aufräum-Service-Worker vorbereiten, alten Ordner verschieben
NEUDIR="$(mktemp -d "$(dirname "$ZIEL")/.tonestack-umleitung.XXXXXX")"
cat > "$NEUDIR/sw.js" <<EOF
// $MARKE nach $NEU/ – dieser Service Worker löscht den alten Offline-Speicher dieser Adresse,
// lädt offene Seiten unter der neuen Adresse (gleicher Pfad, Abfrageteil und Anker) und meldet sich ab.
self.addEventListener("install", () => self.skipWaiting());
self.addEventListener("activate", e => e.waitUntil((async () => {
  const scope = self.registration.scope, basis = new URL(scope).pathname;
  for (const k of await caches.keys()) if ((k.startsWith("tonestack-") && k.endsWith("@" + scope)) || /^\\d{4}-\\d\\d-\\d\\d(-\\d+)?\$/.test(k)) await caches.delete(k);
  const wege = [];
  for (const c of await self.clients.matchAll({ type: "window", includeUncontrolled: true })) {
    const u = new URL(c.url); if (!u.pathname.startsWith(basis)) continue;   // nur Seiten der alten Installation
    const p = u.pathname.slice(basis.length);
    wege.push(c.navigate("$NEU/" + p + u.search + u.hash).catch(() => {})); }
  await Promise.all(wege);
  await self.registration.unregister();
})()));
EOF
printf '%s\n' '<IfModule mod_headers.c>' '  <Files "sw.js">' '    Header set Cache-Control "no-cache"' '  </Files>' '</IfModule>' > "$NEUDIR/.htaccess"
chmod 755 "$NEUDIR"; chmod 644 "$NEUDIR/sw.js" "$NEUDIR/.htaccess"
case "$ART" in
  alt) mkdir -p "$SICHERUNGEN"; chmod 700 "$SICHERUNGEN"; SICH="$SICHERUNGEN/vor-umleitung-$(date +%Y%m%d-%H%M%S)"
       mv "$ZIEL" "$SICH"; VERSCHOBEN=1; mv "$NEUDIR" "$ZIEL"; NEUDIR=""; echo "Alte Seite gesichert: $SICH";;
  umgestellt) install -m 644 "$NEUDIR/sw.js" "$ZIEL/sw.js"; install -m 644 "$NEUDIR/.htaccess" "$ZIEL/.htaccess"; rm -rf -- "$NEUDIR"; NEUDIR="";;
  fehlt) mkdir -p "$(dirname "$ZIEL")"; mv "$NEUDIR" "$ZIEL"; NEUDIR=""; VERSCHOBEN=2;;
esac
# 4. Neu laden und prüfen
apache_neu_laden
trap - ERR
echo "Prüfung von $BASIS$URLPFAD"
if pruefen; then echo "Umleitung aktiv: $URLPFAD/… → $NEU/…  (rückgängig: sudo bash $0 --rueckgaengig)"
else echo "Prüfung fehlgeschlagen. Ist $BASIS die richtige Adresse dieses Servers (--basis)? Rückgängig: sudo bash $0 --rueckgaengig"; exit 1; fi
