#!/bin/bash
# Spielt ein Update der Klangregler-Simulatoren ein – alles, was im Ordner liegt:
#   1. Webserver:     tonestack-apache.zip      (über install-tonestack.sh, nur wenn neuer)
#   2. DAW-Plugins:   <Name>_CLAP-*.zip, <Name>_VST3-*.zip   (CLAP nach ~/.clap, VST3 nach ~/.vst3; danach REAPER neu starten)
#
#   bash update-tonestack.sh [Optionen]          (als normaler Benutzer; für den Webserver wird sudo abgefragt)
#
# Optionen:
#   --nur web|plugins      nur diesen Teil (mehrfach möglich; clap, vst3 = plugins)
#   --ordner PFAD          wo die Dateien liegen        (Standard: Ordner dieses Skripts)
#   --ziel PFAD            Installationsordner Web      (Standard: /var/www/html/tonestack)
#   --url PFAD             URL-Pfad Web                 (Standard: /tonestack)
#   --host NAME            Server-Name für die Prüfung  (Standard: localhost)
#   --erzwingen            Web auch bei gleicher Version neu installieren
#   --hilfe                diese Hilfe

set -uo pipefail

ORDNER="$(cd "$(dirname "$0")" && pwd)"
ZIEL="/var/www/html/tonestack"
URLPFAD="/tonestack"
HOST="localhost"
ERZWINGEN=0
NUR=""

rot()  { printf '\033[31m%s\033[0m\n' "$*"; }
gelb() { printf '\033[33m%s\033[0m\n' "$*"; }
gruen(){ printf '\033[32m%s\033[0m\n' "$*"; }
info() { printf '  %s\n' "$*"; }

while [ $# -gt 0 ]; do
  case "$1" in
    --nur)      case "${2:-}" in web|plugins) NUR="$NUR $2";; clap|vst3) NUR="$NUR plugins";; *) rot "Unbekannter Teil: ${2:-} (web oder plugins)"; exit 1;; esac; shift 2 ;;
    --ordner)   [ -d "${2:-}" ] || { rot "Ordner nicht gefunden: ${2:-}"; exit 1; }; ORDNER="$(cd "$2" && pwd)"; shift 2 ;;
    --ziel)     ZIEL="${2%/}"; shift 2 ;;
    --url)      URLPFAD="$2"; shift 2 ;;
    --host)     HOST="$2"; shift 2 ;;
    --erzwingen) ERZWINGEN=1; shift ;;
    --hilfe|-h) sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) rot "Unbekannte Option: $1 (--hilfe zeigt alle)"; exit 1 ;;
  esac
done
teil() { [ -z "$NUR" ] || [[ " $NUR " == *" $1 "* ]]; }

if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
  gelb "Hinweis: bitte ohne sudo starten – sonst landen die Plugins beim Benutzer root."
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

# ---------- 2. DAW-Plugins (CLAP und VST3) ----------
# Ein Paket je Verstärker: <Name>.clap (Datei) und <Name>.vst3 (Ordner). Alte Fassungen werden gesichert, gleiche übersprungen.
if teil plugins; then
  echo; echo "== DAW-Plugins"
  CLAPDIR="${HOME}/.clap"; VST3DIR="${HOME}/.vst3"; GEF=0
  # Läuft REAPER (oder eine andere DAW) noch, hat sie die alten Plugins geladen: eine Datei, die dabei überschrieben wird, bringt
  # den laufenden Prozess zum Absturz. Deshalb Warnung, und neue Fassungen werden nie überschrieben, sondern daneben geschrieben
  # und umbenannt (neue Datei; ein laufender Prozess behält die alte, bis er neu startet).
  if pgrep -x -i reaper >/dev/null 2>&1; then rot "REAPER läuft noch. Bitte REAPER schließen und das Skript erneut starten (sonst kann REAPER abstürzen)."; exit 1; fi
  # Je Verstärker und Format nur das Paket mit der höchsten Versionsnummer (3.12 > 3.9): ältere ZIPs im Download-Ordner
  # dürfen eine neue Fassung nicht wieder überschreiben.
  neuestes() {   # $1 Verstärker, $2 Format (CLAP|VST3) → Pfad des neuesten Pakets dieses Formats
    local f v
    for f in "$ORDNER/$1_$2"-*.zip; do
      [ -f "$f" ] || continue
      v="$(basename "$f" .zip | sed -E 's/^.*_(CLAP|VST3)-([0-9][0-9.]*).*$/\2/')"
      printf '%s\t%s\n' "$v" "$f"
    done | sort -V -k1,1 | tail -n1 | cut -f2-
  }
  for AMP in Laney_AOR50 Yamaha_B100II AmpFett_B15NF GK_800RB; do
   for FMT in CLAP VST3; do
    PZ="$(neuestes "$AMP" "$FMT")"; [ -n "$PZ" ] || continue
    GEF=1; NAME="$(basename "$PZ" .zip)"
    ALTE="$(ls "$ORDNER/${AMP}_$FMT"-*.zip 2>/dev/null | grep -vxF "$PZ" | wc -l)"
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
        if [ -f "$ZIELCL" ]; then SICH="$CLAPDIR/tonestack-sicherungen"; mkdir -p "$SICH"; SZ="$SICH/$(basename "$CL").$STAMP"; n=1; while [ -e "$SZ" ]; do SZ="$SICH/$(basename "$CL").$STAMP-$n"; n=$((n+1)); done; cp -p "$ZIELCL" "$SZ"; fi
        cp "$CL" "$ZIELCL.neu.$$" && chmod 755 "$ZIELCL.neu.$$" && mv -f "$ZIELCL.neu.$$" "$ZIELCL"; info "$NAME: CLAP nach $ZIELCL"; merke "CLAP $NAME: aktualisiert"
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
          # eindeutiger Sicherungsname (zwei Läufe in derselben Sekunde); mv muss gelingen, sonst landete cp -r im alten Ordner
          OKV3=1
          if [ -d "$ZIELV3" ]; then SICH="${HOME}/.vst3-tonestack-sicherungen"; mkdir -p "$SICH"
            SZ="$SICH/$(basename "$V3").$STAMP"; n=1; while [ -e "$SZ" ]; do SZ="$SICH/$(basename "$V3").$STAMP-$n"; n=$((n+1)); done
            mv -T "$ZIELV3" "$SZ" || OKV3=0; fi
          if [ $OKV3 -eq 1 ] && [ ! -e "$ZIELV3" ]; then cp -r "$V3" "$ZIELV3"; chmod -R u+rwX,go+rX "$ZIELV3"; info "$NAME: VST3 nach $ZIELV3"; merke "VST3 $NAME: aktualisiert"
          else rot "  $NAME: alter VST3-Ordner $ZIELV3 ließ sich nicht wegsichern – nichts kopiert"; merke "VST3 $NAME: FEHLER"; fi
        fi
      fi
    fi
    [ -n "$CL" ] || [ -n "$V3" ] || { rot "  $NAME: kein $FMT im Paket"; merke "$FMT $NAME: FEHLER"; }
    rm -rf "$T"
   done
  done
  # umbenannt: Ampeg_B15NF (1.0) heißt seit 1.1 AmpFett_B15NF. Die alte Fassung erst wegräumen, wenn die neue im selben Format da ist
  for PAAR in "$CLAPDIR/Ampeg_B15NF.clap:$CLAPDIR/AmpFett_B15NF.clap" "$VST3DIR/Ampeg_B15NF.vst3:$VST3DIR/AmpFett_B15NF.vst3"; do
    VORHER="${PAAR%%:*}"; NEUES="${PAAR#*:}"
    [ -e "$VORHER" ] && [ -e "$NEUES" ] || continue
    SICH="$(dirname "$VORHER")/../.tonestack-sicherungen"; mkdir -p "$SICH"; SICH="$(cd "$SICH" && pwd)"
    if mv "$VORHER" "$SICH/$(basename "$VORHER").$(date +%Y%m%d-%H%M%S)"; then info "alte Fassung $(basename "$VORHER") nach $SICH verschoben (heißt jetzt AmpFett_B15NF)"; merke "$(basename "$VORHER"): weggeräumt (umbenannt)"
    else rot "  $(basename "$VORHER") ließ sich nicht verschieben – bitte von Hand löschen"; merke "$(basename "$VORHER"): FEHLER beim Wegräumen"; fi
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
