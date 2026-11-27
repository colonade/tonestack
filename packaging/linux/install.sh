#!/bin/sh
# tonestack-Plugins ohne Paketverwaltung installieren (Archiv tonestack-plugins-<Version>-linux-<arch>.tar.gz, ab 1.17242-1):
# kopiert die CLAP-Fassungen nach ~/.clap und die VST3-Fassungen nach ~/.vst3 – dort suchen alle Hosts, auch REAPER ohne Einstellung
# und Flatpak-Hosts, die das Home-Verzeichnis sehen. Für alle Benutzer des Rechners ist das Debian-Paket der bessere Weg.
#   ./install.sh              installieren (vorhandene tonestack-Plugins gleichen Namens werden ersetzt)
#   ./install.sh --uninstall  genau diese Plugins wieder entfernen
#   ./install.sh --help
# Andere Ziele: CLAP_DIR=… VST3_DIR=… ./install.sh
set -eu
VERSION=@DEBVER@; ARCH=@MACH@; NAMEN="@NAMEN@"
HIER="$(cd "$(dirname "$0")" && pwd)"
CLAP_DIR="${CLAP_DIR:-$HOME/.clap}"; VST3_DIR="${VST3_DIR:-$HOME/.vst3}"
case "${1:-}" in
  -h|--help) sed -n '2,8p' "$0" | sed 's/^# \{0,1\}//'; exit 0;;
  --uninstall)
    for n in $NAMEN; do rm -f -- "$CLAP_DIR/$n.clap"; rm -rf -- "$VST3_DIR/$n.vst3"; done
    echo "tonestack: Plugins aus $CLAP_DIR und $VST3_DIR entfernt."; exit 0;;
  "") ;;
  *) echo "Unbekannte Angabe: $1 (Hilfe: ./install.sh --help)" >&2; exit 1;;
esac
[ "$(id -u)" != 0 ] || echo "Hinweis: als root ausgeführt – die Plugins landen in den Ordnern von root ($CLAP_DIR, $VST3_DIR). Für alle Benutzer besser das Debian-Paket." >&2
m="$(uname -m)"; [ "$m" = "$ARCH" ] || { echo "Dieses Archiv ist für $ARCH, der Rechner ist $m – bitte das passende Archiv laden." >&2; exit 1; }
g="$(getconf GNU_LIBC_VERSION 2>/dev/null | awk '{ print $2 }')"
if [ -n "$g" ] && [ "$(printf '%s\n%s\n' "$g" 2.38 | sort -t. -k1,1n -k2,2n | head -n1)" != 2.38 ]; then
  echo "Die Plugins brauchen glibc 2.38 oder neuer, hier ist $g (z. B. Debian 12, Ubuntu 22.04) – sie ließen sich nicht laden." >&2; exit 1; fi
for lib in libcairo.so.2 libX11.so.6; do
  if command -v ldconfig >/dev/null 2>&1 && ! ldconfig -p 2>/dev/null | grep -q "$lib"; then echo "Hinweis: $lib nicht gefunden (Pakete cairo und libX11) – ohne sie lädt der Host die Plugins nicht." >&2; fi
done
mkdir -p -- "$CLAP_DIR" "$VST3_DIR"
for n in $NAMEN; do
  cp -f -- "$HIER/clap/$n.clap" "$CLAP_DIR/$n.clap.neu" && mv -f -- "$CLAP_DIR/$n.clap.neu" "$CLAP_DIR/$n.clap"
  rm -rf -- "$VST3_DIR/$n.vst3.neu"; cp -R -- "$HIER/vst3/$n.vst3" "$VST3_DIR/$n.vst3.neu"; rm -rf -- "$VST3_DIR/$n.vst3"; mv -- "$VST3_DIR/$n.vst3.neu" "$VST3_DIR/$n.vst3"
done
echo "tonestack $VERSION: $(echo $NAMEN | wc -w) Plugins als CLAP in $CLAP_DIR und als VST3 in $VST3_DIR installiert."
doppelt=""; for n in $NAMEN; do [ -e "/usr/lib/clap/$n.clap" ] || [ -e "/usr/lib/vst3/$n.vst3" ] && doppelt=1; done
[ -z "$doppelt" ] || echo "Hinweis: das Debian-Paket tonestack-plugins ist auch installiert – der Host fände jedes Plugin zweimal. Eines von beiden entfernen."
echo "Den Host neu starten oder die Plugins neu suchen lassen."
