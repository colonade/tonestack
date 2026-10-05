#!/bin/bash
# Spielt ein Update der Klangregler-Simulatoren ein – alles, was im Ordner liegt:
#   1. Webserver:     tonestack-apache.zip      (über install-tonestack.sh, nur wenn neuer)
#   2. Handy-Apps:    B100-II_Klangregler.apk, AOR50_Klangregler.apk   (per adb, Einstellungen bleiben)
#   3. REAPER-Plugin: Laney_AOR50_REAPER.zip    (nach ~/.config/REAPER/Effects)
#
#   bash update-tonestack.sh [Optionen]          (als normaler Benutzer; für den Webserver wird sudo abgefragt)
#
# Optionen:
#   --nur web|app|reaper   nur diesen Teil (mehrfach möglich)
#   --ordner PFAD          wo die Dateien liegen        (Standard: Ordner dieses Skripts)
#   --ziel PFAD            Installationsordner Web      (Standard: /var/www/html/tonestack)
#   --url PFAD             URL-Pfad Web                 (Standard: /tonestack)
#   --host NAME            Server-Name für die Prüfung  (Standard: localhost)
#   --reaper PFAD          REAPER-Ressourcenordner      (Standard: ~/.config/REAPER)
#   --erzwingen            Web auch bei gleicher Version neu installieren
#   --hilfe                diese Hilfe

set -uo pipefail

ORDNER="$(cd "$(dirname "$0")" && pwd)"
ZIEL="/var/www/html/tonestack"
URLPFAD="/tonestack"
HOST="localhost"
REAPER="${HOME}/.config/REAPER"
ERZWINGEN=0
NUR=""

rot()  { printf '\033[31m%s\033[0m\n' "$*"; }
gelb() { printf '\033[33m%s\033[0m\n' "$*"; }
gruen(){ printf '\033[32m%s\033[0m\n' "$*"; }
info() { printf '  %s\n' "$*"; }

while [ $# -gt 0 ]; do
  case "$1" in
    --nur)      NUR="$NUR $2"; shift 2 ;;
    --ordner)   ORDNER="$(cd "$2" && pwd)"; shift 2 ;;
    --ziel)     ZIEL="${2%/}"; shift 2 ;;
    --url)      URLPFAD="$2"; shift 2 ;;
    --host)     HOST="$2"; shift 2 ;;
    --reaper)   REAPER="${2%/}"; shift 2 ;;
    --erzwingen) ERZWINGEN=1; shift ;;
    --hilfe|-h) sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) rot "Unbekannte Option: $1 (--hilfe zeigt alle)"; exit 1 ;;
  esac
done
teil() { [ -z "$NUR" ] || [[ " $NUR " == *" $1 "* ]]; }

if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
  gelb "Hinweis: bitte ohne sudo starten – sonst landen App-Schlüssel und REAPER-Plugin beim Benutzer root."
  gelb "Das Skript fragt für den Webserver selbst nach sudo."
fi

ERGEBNIS=()
merke() { ERGEBNIS+=("$1"); }
echo "Dateien aus: $ORDNER"

# ---------- 1. Webserver ----------
if teil web; then
  echo; echo "== Webserver"
  ZIP="$ORDNER/tonestack-apache.zip"; INST="$ORDNER/install-tonestack.sh"
  if [ ! -f "$ZIP" ]; then info "tonestack-apache.zip nicht gefunden – übersprungen"; merke "Webserver: übersprungen (kein Paket)"
  elif [ ! -f "$INST" ]; then rot "  install-tonestack.sh fehlt im selben Ordner – übersprungen"; merke "Webserver: übersprungen (Installationsskript fehlt)"
  elif ! command -v apache2ctl >/dev/null; then info "kein Apache auf diesem Rechner – übersprungen"; merke "Webserver: übersprungen (kein Apache)"
  else
    NEU="$(unzip -p "$ZIP" tonestack/sw.js 2>/dev/null | grep -o 'VERSION = "[^"]*"' | cut -d'"' -f2 || true)"
    [ -z "$NEU" ] && NEU="$(python3 -c "import zipfile,sys,re; print(re.search(r'VERSION = \"([^\"]*)\"', zipfile.ZipFile(sys.argv[1]).read('tonestack/sw.js').decode()).group(1))" "$ZIP" 2>/dev/null || true)"
    ALT="$(grep -o 'VERSION = "[^"]*"' "$ZIEL/sw.js" 2>/dev/null | cut -d'"' -f2 || true)"
    info "installiert: ${ALT:-(nichts)}  ·  im Paket: ${NEU:-?}"
    if [ -n "$ALT" ] && [ "$ALT" = "$NEU" ] && [ "$ERZWINGEN" -eq 0 ]; then
      info "bereits aktuell (--erzwingen installiert trotzdem neu) – prüfe die installierten Seiten:"
      SUDO=""; [ "$(id -u)" -ne 0 ] && SUDO="sudo"
      if $SUDO bash "$INST" --nur-pruefen --ziel "$ZIEL" --url "$URLPFAD" --host "$HOST"; then merke "Webserver: bereits aktuell ($ALT), Prüfung bestanden"
      else merke "Webserver: bereits aktuell ($ALT), Prüfung FEHLER"; fi
    else
      SUDO=""; [ "$(id -u)" -ne 0 ] && SUDO="sudo"
      if $SUDO bash "$INST" "$ZIP" --ziel "$ZIEL" --url "$URLPFAD" --host "$HOST"; then merke "Webserver: aktualisiert ${ALT:-–} → $NEU"
      else rot "  Webserver-Update fehlgeschlagen (Details oben)"; merke "Webserver: FEHLER"; fi
    fi
  fi
fi

# ---------- 2. Handy-Apps ----------
if teil app; then
  echo; echo "== Handy-Apps (adb)"
  APKS=(); for a in B100-II_Klangregler.apk AOR50_Klangregler.apk; do [ -f "$ORDNER/$a" ] && APKS+=("$a"); done
  if [ ${#APKS[@]} -eq 0 ]; then info "keine APK-Dateien gefunden – übersprungen"; merke "Apps: übersprungen (keine APK)"
  elif ! command -v adb >/dev/null; then
    info "adb nicht installiert – übersprungen (Debian/Ubuntu: sudo apt install adb)"
    info "Alternativ die APK-Dateien aufs Handy kopieren und dort antippen."
    merke "Apps: übersprungen (kein adb)"
  else
    adb start-server >/dev/null 2>&1
    GERAETE="$(adb devices | awk 'NR>1 && $2=="device" {print $1}')"
    UNBERECHTIGT="$(adb devices | awk 'NR>1 && $2=="unauthorized" {print $1}')"
    if [ -n "$UNBERECHTIGT" ]; then rot "  Gerät $UNBERECHTIGT wartet auf Freigabe: auf dem Handy 'USB-Debugging zulassen' bestätigen und neu starten"; fi
    ANZAHL="$(printf '%s\n' "$GERAETE" | grep -c . || true)"
    if [ "$ANZAHL" -eq 0 ]; then
      info "kein Handy verbunden – übersprungen"
      info "Handy per USB anschließen, Entwickleroptionen → USB-Debugging einschalten, Freigabe bestätigen."
      merke "Apps: übersprungen (kein Gerät)"
    elif [ "$ANZAHL" -gt 1 ]; then
      rot "  mehrere Geräte verbunden ($GERAETE) – bitte nur eines anschließen"; merke "Apps: übersprungen (mehrere Geräte)"
    else
      info "Gerät: $(adb shell getprop ro.product.model 2>/dev/null | tr -d '\r') ($GERAETE)"
      for a in "${APKS[@]}"; do
        case "$a" in B100*) PKG=de.tonestack.yamahab100 ;; *) PKG=de.tonestack.laneyaor50 ;; esac
        VOR="$(adb shell dumpsys package "$PKG" 2>/dev/null | tr -d '\r' | sed -n 's/.*versionName=\(.*\)/\1/p' | head -1)"
        AUS="$(adb install -r "$ORDNER/$a" 2>&1 | tr -d '\r' | tail -1)"
        NACH="$(adb shell dumpsys package "$PKG" 2>/dev/null | tr -d '\r' | sed -n 's/.*versionName=\(.*\)/\1/p' | head -1)"
        if [ "$AUS" = "Success" ]; then info "$a: ${VOR:-nicht installiert} → $NACH"; merke "App $a: ${VOR:-neu} → $NACH"
        else
          rot "  $a: $AUS"
          case "$AUS" in
            *INSTALL_FAILED_UPDATE_INCOMPATIBLE*) info "Andere Signatur installiert: App auf dem Handy erst deinstallieren (Einstellungen gehen verloren).";;
            *INSTALL_FAILED_VERSION_DOWNGRADE*)   info "Auf dem Handy ist bereits eine neuere Version.";;
            *INSTALL_FAILED_USER_RESTRICTED*)     info "Auf dem Handy die Installation über USB erlauben (Entwickleroptionen).";;
          esac
          merke "App $a: FEHLER"
        fi
      done
    fi
  fi
fi

# ---------- 3. REAPER-Plugin ----------
if teil reaper; then
  echo; echo "== REAPER-Plugin"
  RZ="$ORDNER/Laney_AOR50_REAPER.zip"; JS="Laney AOR 50 Preamp.jsfx"
  if [ ! -f "$RZ" ] && [ ! -f "$ORDNER/$JS" ]; then info "kein Plugin-Paket gefunden – übersprungen"; merke "REAPER: übersprungen (kein Paket)"
  elif [ ! -d "$REAPER" ]; then info "REAPER-Ordner $REAPER nicht gefunden – übersprungen (--reaper PFAD angeben)"; merke "REAPER: übersprungen (Ordner fehlt)"
  else
    T="$(mktemp -d)"
    if [ -f "$RZ" ]; then (cd "$T" && unzip -q -o "$RZ" "$JS" 2>/dev/null) || python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extract(sys.argv[2], sys.argv[3])" "$RZ" "$JS" "$T"
    else cp "$ORDNER/$JS" "$T/"; fi
    if [ ! -f "$T/$JS" ]; then rot "  Plugin nicht im Paket gefunden"; merke "REAPER: FEHLER"
    else
      mkdir -p "$REAPER/Effects"
      ZIELJS="$REAPER/Effects/$JS"
      if [ -f "$ZIELJS" ] && cmp -s "$T/$JS" "$ZIELJS"; then info "bereits aktuell"; merke "REAPER: bereits aktuell"
      else
        # Sicherung außerhalb von Effects: REAPER würde jede Datei dort als weiteren Effekt anzeigen
        if [ -f "$ZIELJS" ]; then SICH="$REAPER/tonestack-sicherungen"; mkdir -p "$SICH"
          cp -p "$ZIELJS" "$SICH/$JS.$(date +%Y%m%d-%H%M%S)"; info "alte Fassung gesichert in $SICH"; fi
        cp "$T/$JS" "$ZIELJS"; info "kopiert nach $ZIELJS"
        info "In REAPER: FX-Browser → F5; geöffnete Projekte nutzen die neue Fassung nach dem Neuladen."
        merke "REAPER: aktualisiert"
      fi
    fi
    rm -rf "$T"
  fi
fi

echo; echo "== Zusammenfassung"
FEHLER=0
for z in "${ERGEBNIS[@]}"; do
  case "$z" in *FEHLER*) rot "  $z"; FEHLER=1 ;; *übersprungen*) info "$z" ;; *) gruen "  $z" ;; esac
done
exit $FEHLER
