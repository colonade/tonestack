#!/bin/bash
# Spielt ein Update der Klangregler-Simulatoren ein – alles, was im Ordner liegt:
#   1. Webserver:     tonestack-apache.zip      (über install-tonestack.sh, nur wenn neuer)
#   2. Handy-Apps:    B100-II_Klangregler.apk, AOR50_Klangregler.apk   (per adb, Einstellungen bleiben)
#   3. REAPER-Plugin: Laney_AOR50_REAPER.zip    (JSFX, nach ~/.config/REAPER/Effects)
#   4. DAW-Plugins:   <Name>_CLAP-*.zip, <Name>_VST3-*.zip (oder <Name>_Plugin-*.zip mit beiden)   (CLAP nach ~/.clap, VST3 nach ~/.vst3; danach REAPER neu starten)
#
#   bash update-tonestack.sh [Optionen]          (als normaler Benutzer; für den Webserver wird sudo abgefragt)
#
# Optionen:
#   --nur web|app|reaper|plugins   nur diesen Teil (mehrfach möglich; clap = plugins)
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
    --nur)      case "${2:-}" in web|app|reaper|plugins) NUR="$NUR $2";; clap|vst3) NUR="$NUR plugins";; *) rot "Unbekannter Teil: ${2:-} (web, app, reaper oder plugins)"; exit 1;; esac; shift 2 ;;
    --ordner)   [ -d "${2:-}" ] || { rot "Ordner nicht gefunden: ${2:-}"; exit 1; }; ORDNER="$(cd "$2" && pwd)"; shift 2 ;;
    --ziel)     ZIEL="${2%/}"; shift 2 ;;
    --url)      URLPFAD="$2"; shift 2 ;;
    --host)     HOST="$2"; shift 2 ;;
    --reaper)   REAPER="${2%/}"; shift 2 ;;
    --erzwingen) ERZWINGEN=1; shift ;;
    --hilfe|-h) sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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
    T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
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

# ---------- 4. DAW-Plugins (CLAP und VST3) ----------
# Ein Paket je Verstärker: <Name>.clap (Datei) und <Name>.vst3 (Ordner). Alte Fassungen werden gesichert, gleiche übersprungen.
if teil plugins; then
  echo; echo "== DAW-Plugins"
  CLAPDIR="${HOME}/.clap"; VST3DIR="${HOME}/.vst3"; GEF=0
  # Je Verstärker und Format nur das Paket mit der höchsten Versionsnummer (3.12 > 3.9): ältere ZIPs im Download-Ordner
  # dürfen eine neue Fassung nicht wieder überschreiben.
  neuestes() {   # $1 Verstärker, $2 Format (CLAP|VST3) → Pfad des neuesten Pakets, das dieses Format enthalten kann
    local f v
    for f in "$ORDNER/$1"_Plugin-*.zip "$ORDNER/$1_$2"-*.zip; do
      [ -f "$f" ] || continue
      v="$(basename "$f" .zip | sed -E 's/^.*_(Plugin|CLAP|VST3)-([0-9][0-9.]*).*$/\2/')"
      printf '%s\t%s\n' "$v" "$f"
    done | sort -V -k1,1 | tail -n1 | cut -f2-
  }
  for AMP in Laney_AOR50 Yamaha_B100II; do
   for FMT in CLAP VST3; do
    PZ="$(neuestes "$AMP" "$FMT")"; [ -n "$PZ" ] || continue
    GEF=1; NAME="$(basename "$PZ" .zip)"
    ALTE="$(ls "$ORDNER/$AMP"_Plugin-*.zip "$ORDNER/${AMP}_$FMT"-*.zip 2>/dev/null | grep -vxF "$PZ" | wc -l)"
    [ "$ALTE" -gt 0 ] && info "$AMP $FMT: neuestes Paket $NAME ($ALTE ältere im Ordner ignoriert)"
    T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
    if command -v unzip >/dev/null; then unzip -q -o "$PZ" -d "$T" 2>/dev/null
    else python3 -c "import zipfile,sys; z=zipfile.ZipFile(sys.argv[1]); z.extractall(sys.argv[2], filter='data') if hasattr(zipfile,'data_filter') else z.extractall(sys.argv[2])" "$PZ" "$T"; fi
    STAMP="$(date +%Y%m%d-%H%M%S)"
    CL=""; V3=""
    [ "$FMT" = CLAP ] && CL="$(find "$T" -maxdepth 1 -type f -name '*.clap' | head -n1)"
    [ "$FMT" = VST3 ] && V3="$(find "$T" -maxdepth 1 -type d -name '*.vst3' | head -n1)"
    if [ -n "$CL" ]; then
      mkdir -p "$CLAPDIR"; ZIELCL="$CLAPDIR/$(basename "$CL")"
      if [ -f "$ZIELCL" ] && cmp -s "$CL" "$ZIELCL"; then info "$NAME: CLAP bereits aktuell"; merke "CLAP $NAME: bereits aktuell"
      else
        if [ -f "$ZIELCL" ]; then SICH="$CLAPDIR/tonestack-sicherungen"; mkdir -p "$SICH"; cp -p "$ZIELCL" "$SICH/$(basename "$CL").$STAMP"; fi
        cp "$CL" "$ZIELCL"; chmod 755 "$ZIELCL"; info "$NAME: CLAP nach $ZIELCL"; merke "CLAP $NAME: aktualisiert"
      fi
    fi
    if [ -n "$V3" ]; then
      SO="$(find "$V3/Contents/x86_64-linux" -maxdepth 1 -name '*.so' 2>/dev/null | head -n1)"
      if [ -z "$SO" ]; then rot "  $NAME: VST3-Ordner ohne x86_64-linux/*.so"; merke "VST3 $NAME: FEHLER"
      else
        mkdir -p "$VST3DIR"; ZIELV3="$VST3DIR/$(basename "$V3")"
        if [ -f "$ZIELV3/Contents/x86_64-linux/$(basename "$SO")" ] && cmp -s "$SO" "$ZIELV3/Contents/x86_64-linux/$(basename "$SO")"; then info "$NAME: VST3 bereits aktuell"; merke "VST3 $NAME: bereits aktuell"
        else
          # Sicherung außerhalb von ~/.vst3: ein zweiter Ordner dort würde als weiteres Plugin gefunden
          if [ -d "$ZIELV3" ]; then SICH="${HOME}/.vst3-tonestack-sicherungen"; mkdir -p "$SICH"; mv "$ZIELV3" "$SICH/$(basename "$V3").$STAMP"; fi
          cp -r "$V3" "$ZIELV3"; chmod -R u+rwX,go+rX "$ZIELV3"; info "$NAME: VST3 nach $ZIELV3"; merke "VST3 $NAME: aktualisiert"
        fi
      fi
    fi
    [ -n "$CL" ] || [ -n "$V3" ] || { rot "  $NAME: kein $FMT im Paket"; merke "$FMT $NAME: FEHLER"; }
    rm -rf "$T"
   done
  done
  [ $GEF -eq 1 ] && info "In REAPER: Options > Preferences > Plug-ins > CLAP bzw. VST > Re-scan (REAPER hält geladene Plugins bis zum Neustart)."
  [ $GEF -eq 1 ] || { info "kein Plugin-Paket gefunden – übersprungen"; merke "Plugins: übersprungen (kein Paket)"; }
fi

echo; echo "== Zusammenfassung"
FEHLER=0
for z in "${ERGEBNIS[@]:-}"; do
  case "$z" in *FEHLER*) rot "  $z"; FEHLER=1 ;; *übersprungen*) info "$z" ;; *) gruen "  $z" ;; esac
done
exit $FEHLER
