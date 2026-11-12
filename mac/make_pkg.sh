#!/bin/bash
# Mac-Installerpaket (.pkg) aus den signierten Bundles (nach mac/package.sh): alle Plugins einzeln wählbar, je Audio Unit + VST3,
# Ziel /Library/Audio/Plug-Ins/{Components,VST3} (für alle Benutzer). Vom Installer abgelegte Dateien tragen keine
# Quarantäne-Markierung; nur das Paket selbst muss beim ersten Öffnen freigegeben werden (nicht bei Apple beglaubigt).
#   mac/make_pkg.sh <build-Ordner> <Ausgabe-Ordner>
set -euo pipefail
B="$1"; OUT="$2"; HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/.."
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
eval "$(bash "$ROOT/scripts/versions.sh")"; LV="$LANEY"; YV="$YAMAHA"; BV="$B15"; GV="$GK"; G1V="$G100"; B3V="$B100III"; SVV="$SVTCL"; WQV="$WIENEQ"; U76V="$AMPFETT76"   # Versionen wie im Release
W="$(mktemp -d)"; mkdir -p "$W/pkgs" "$W/res"
# Nachlauf je Plugin (ab 2026-11-03): fasst nur die Dateien des gewählten Plugins an. Vorher lief für jedes Teilpaket dasselbe Skript und
# verschob von Hand installierte Fassungen aller drei Plugins in den Papierkorb, auch wenn nur eines gewählt war.
post() {   # post <id> <Name> [weitere Namen]
  local id="$1"; shift; mkdir -p "$W/scripts-$id"; local names="$*"
  { cat <<'S'
#!/bin/bash
# Läuft als root nach dem Audio-Unit-Teilpaket dieses Plugins. Fasst nur feste Pfade dieses Plugins an.
S
    echo "NAMES=\"$names\""
    cat <<'S'
# Frühere Namen (Ampeg_B15NF heißt seit 1.1 AmpFett_B15NF; Yamaha_B100II, Yamaha_G100, Yamaha_B100III heißen seit 2026-11-12 Lametta_…):
# alte systemweite Fassung entfernen, sonst stünden beide in Logic
for n in $NAMES; do case "$n" in Ampeg_*|Yamaha_*) rm -rf "/Library/Audio/Plug-Ins/Components/$n.component" "/Library/Audio/Plug-Ins/VST3/$n.vst3";; esac; done
# Früher von Hand (ZIP) im Benutzerordner installierte Fassungen: dieselbe Audio-Unit-Kennung zweimal, und Logic nimmt womöglich die alte.
# In den Papierkorb des angemeldeten Benutzers verschieben (wiederherstellbar), nicht löschen; geht das nicht (Papierkorb gesperrt),
# in ~/Library/Audio/Plug-Ins/tonestack-alte-Fassungen (dort sucht kein Host).
U="$(stat -f%Su /dev/console 2>/dev/null || true)"
if [ -n "$U" ] && [ "$U" != root ]; then
  H="$(dscl . -read "/Users/$U" NFSHomeDirectory 2>/dev/null | awk '{print $2}')"
  if [ -n "$H" ] && [ -d "$H" ]; then
    T="$(date +%Y%m%d-%H%M%S)"
    for n in $NAMES; do for f in "Components/$n.component" "VST3/$n.vst3"; do
      p="$H/Library/Audio/Plug-Ins/$f"; [ -e "$p" ] || continue
      if [ ! -d "$H/.Trash" ]; then mkdir -p "$H/.Trash" && chown "$U" "$H/.Trash" && chmod 700 "$H/.Trash"; fi
      if ! mv "$p" "$H/.Trash/$(basename "$p")-alt-$T" 2>/dev/null; then
        A="$H/Library/Audio/Plug-Ins/tonestack-alte-Fassungen"; mkdir -p "$A" && chown "$U" "$A"; mv "$p" "$A/$(basename "$p")-alt-$T" 2>/dev/null || true
      fi
    done; done
  fi
fi
# Audio-Unit-Verzeichnis neu einlesen lassen (startet bei Bedarf von selbst neu), damit Logic die neue Fassung sofort findet
killall AudioComponentRegistrar 2>/dev/null || true
exit 0
S
  } > "$W/scripts-$id/postinstall"; chmod +x "$W/scripts-$id/postinstall"; }
post laney Laney_AOR50; post yamaha Lametta_B100II Yamaha_B100II; post b15nf AmpFett_B15NF Ampeg_B15NF; post gk800rb GK_800RB; post g100 Lametta_G100 Yamaha_G100; post b100iii Lametta_B100III Yamaha_B100III; post svtcl AmpFett_SVTCL; post wieneq Wien_EQ; post ampfett76 AmpFett_76
one() {   # one <Name> <id> <Version> <component|vst3>
  local NAME="$1" ID="$2" V="$3" KIND="$4" DEST; [ "$KIND" = component ] && DEST=Components || DEST=VST3
  local SRC; SRC="$(find "$B" -maxdepth 3 -name "$NAME.$KIND" -type d | head -1)"; [ -n "$SRC" ] || { echo "FEHLER: $NAME.$KIND fehlt"; exit 1; }
  local R="$W/root-$ID-$KIND"; mkdir -p "$R/Library/Audio/Plug-Ins/$DEST"; cp -R "$SRC" "$R/Library/Audio/Plug-Ins/$DEST/"
  pkgbuild --analyze --root "$R" "$W/$ID-$KIND.plist" >/dev/null
  local n; n=$(plutil -extract . raw -o - "$W/$ID-$KIND.plist" 2>/dev/null || echo 1)
  for ((i = 0; i < n; i++)); do plutil -replace "$i.BundleIsRelocatable" -bool NO "$W/$ID-$KIND.plist"; done   # immer an den festen Ort
  local SC=(); [ "$KIND" = component ] && SC=(--scripts "$W/scripts-$ID")
  pkgbuild --root "$R" --component-plist "$W/$ID-$KIND.plist" --identifier "de.tonestack.$ID.$KIND" --version "$V" --install-location / ${SC[@]+"${SC[@]}"} "$W/pkgs/$ID-$KIND.pkg"
}
one Laney_AOR50 laney "$LV" component; one Laney_AOR50 laney "$LV" vst3
one Lametta_B100II yamaha "$YV" component; one Lametta_B100II yamaha "$YV" vst3
one AmpFett_B15NF b15nf "$BV" component; one AmpFett_B15NF b15nf "$BV" vst3
one GK_800RB gk800rb "$GV" component; one GK_800RB gk800rb "$GV" vst3
one Lametta_G100 g100 "$G1V" component; one Lametta_G100 g100 "$G1V" vst3
one Lametta_B100III b100iii "$B3V" component; one Lametta_B100III b100iii "$B3V" vst3
one AmpFett_SVTCL svtcl "$SVV" component; one AmpFett_SVTCL svtcl "$SVV" vst3
one Wien_EQ wieneq "$WQV" component; one Wien_EQ wieneq "$WQV" vst3
one AmpFett_76 ampfett76 "$U76V" component; one AmpFett_76 ampfett76 "$U76V" vst3
sed -e "s/@LV@/$LV/g" -e "s/@YV@/$YV/g" -e "s/@BV@/$BV/g" -e "s/@GV@/$GV/g" -e "s/@G1V@/$G1V/g" -e "s/@B3V@/$B3V/g" -e "s/@SVV@/$SVV/g" -e "s/@WQV@/$WQV/g" -e "s/@U76V@/$U76V/g" "$HERE/welcome.html" > "$W/res/welcome.html"
cat > "$W/dist.xml" <<X
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>tonestack Verstärker-Plugins</title>
  <welcome file="welcome.html" mime-type="text/html"/>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
  <os-version min="11.0"/>
  <installation-check script="laeuftNichts()"/>
  <script><![CDATA[
  // Warnung, wenn eine Musik-Software läuft: sie hat die alten Plugins evtl. geladen, und deren Austausch kann sie abstürzen lassen.
  function laeuftNichts() {
    var apps = [['com.apple.logic10', 'Logic Pro'], ['com.apple.mainstage3', 'MainStage'], ['com.apple.garageband10', 'GarageBand'],
                ['com.cockos.reaper', 'REAPER'], ['com.ableton.live', 'Ableton Live'], ['com.presonus.studioone6', 'Studio One']];
    var offen = [];
    for (var i = 0; i < apps.length; i++) { var r = null; try { r = system.applications.fromIdentifier(apps[i][0]); } catch (e) { r = null; } if (r && r.length > 0) offen.push(apps[i][1]); }
    if (offen.length == 0) return true;
    my.result.type = 'Warning'; my.result.title = 'Bitte zuerst schließen: ' + offen.join(', ');
    my.result.message = 'Eine Musik-Software läuft noch und hat die Plugins vielleicht geladen. Bitte vor dem Installieren beenden, sonst kann sie beim Austausch der Plugins abstürzen.';
    return false;
  }
  ]]></script>
  <choices-outline>
    <line choice="laney"/>
    <line choice="yamaha"/>
    <line choice="b15nf"/>
    <line choice="gk800rb"/>
    <line choice="g100"/>
    <line choice="b100iii"/>
    <line choice="svtcl"/>
    <line choice="wieneq"/>
    <line choice="ampfett76"/>
  </choices-outline>
  <choice id="laney" title="Laney AOR 50 Preamp $LV" description="Röhren-Vorverstärker mit Federhall, Endstufe und Lautsprecher. Audio Unit (Logic, GarageBand, MainStage) und VST3.">
    <pkg-ref id="de.tonestack.laney.component"/><pkg-ref id="de.tonestack.laney.vst3"/>
  </choice>
  <choice id="yamaha" title="Lametta B100-II Bass Amp $YV" description="Transistor-Bassverstärker mit 5-Band-EQ, Endstufe und Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.yamaha.component"/><pkg-ref id="de.tonestack.yamaha.vst3"/>
  </choice>
  <choice id="b15nf" title="AmpFett B-15NF Bass Amp $BV" description="Röhren-Bassverstärker nach dem Werksschaltplan (6SL7, 6L6GC, 5AR4) mit Netzteil, Gegenkopplung und Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.b15nf.component"/><pkg-ref id="de.tonestack.b15nf.vst3"/>
  </choice>
  <choice id="gk800rb" title="GK 800RB Bass Amp $GV" description="Transistor-Bassverstärker nach dem Service-Manual 1991: Vorstufe als Schaltung (4-Band-EQ, Filter, Boost mit J113), Bi-Amp-Weiche, zwei Endstufen, Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.gk800rb.component"/><pkg-ref id="de.tonestack.gk800rb.vst3"/>
  </choice>
  <choice id="g100" title="Lametta G100-212II Guitar Amp $G1V" description="Transistor-Gitarrenverstärker nach dem Service Manual 1979: zwei Kanäle (A sauber, B mit Diodenbegrenzer und FET-Stufe), Zugschalter FAT/BRIGHT, parametrischer EQ, Federhall, 100-W-Endstufe; ohne Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.g100.component"/><pkg-ref id="de.tonestack.g100.vst3"/>
  </choice>
  <choice id="b100iii" title="Lametta B100-III Bass Amp $B3V" description="Transistor-Bassverstärker nach dem Service Manual 1984: aktive Klangregelung, zwei parametrische Entzerrer mit PRESET LEVEL (ein/aus wie mit dem Fußschalter), 100-W-Endstufe; ohne Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.b100iii.component"/><pkg-ref id="de.tonestack.b100iii.vst3"/>
  </choice>
  <choice id="svtcl" title="AmpFett SVT-CL Bass Amp $SVV" description="Röhren-Bassverstärker (300 W) nach dem Service Manual des Ampeg SVT-CL: Vorstufe mit 12AX7, Klangregelung mit fünfstufiger Mittenfrequenz, ULTRA HI/LO, Phasenumkehr, Treiber mit 12AU7, Endstufe mit sechs 6550 und Ausgangsübertrager (4 oder 2 Ω); ohne Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.svtcl.component"/><pkg-ref id="de.tonestack.svtcl.vst3"/>
  </choice>
  <choice id="wieneq" title="Wien EQ $WQV" description="Fünfband-Stereo-Equalizer mit Wien-Brücken nach den veröffentlichten Angaben zum adt-audio TM205: LOW und HIGH als Kuhschwanzfilter mit je zwei Tasten, drei vollparametrische Mittenbänder, EQ ON, Regelbereich ±6/±12/±20 dB. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.wieneq.component"/><pkg-ref id="de.tonestack.wieneq.vst3"/>
  </choice>
  <choice id="ampfett76" title="AmpFett 76 Limiting Amplifier $U76V" description="FET-Kompressor/Limiter nach dem Schaltplan des UREI 1176LN (Op-Amp-Eingang, Klasse-AB-Ausgang): INPUT, OUTPUT, ATTACK, RELEASE, vier RATIO-Tasten auch gemeinsam, VU-Anzeige. Dazu nach dem Vorbild des Origin Effects Cali76: stufenlose Ratio, DRY, Hochpass in der Seitenkette, zweite Stufe. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.ampfett76.component"/><pkg-ref id="de.tonestack.ampfett76.vst3"/>
  </choice>
  <pkg-ref id="de.tonestack.laney.component" version="$LV">laney-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.laney.vst3" version="$LV">laney-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.component" version="$YV">yamaha-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.vst3" version="$YV">yamaha-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.component" version="$BV">b15nf-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.vst3" version="$BV">b15nf-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.gk800rb.component" version="$GV">gk800rb-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.gk800rb.vst3" version="$GV">gk800rb-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.g100.component" version="$G1V">g100-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.g100.vst3" version="$G1V">g100-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b100iii.component" version="$B3V">b100iii-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b100iii.vst3" version="$B3V">b100iii-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.svtcl.component" version="$SVV">svtcl-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.svtcl.vst3" version="$SVV">svtcl-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.wieneq.component" version="$WQV">wieneq-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.wieneq.vst3" version="$WQV">wieneq-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.ampfett76.component" version="$U76V">ampfett76-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.ampfett76.vst3" version="$U76V">ampfett76-vst3.pkg</pkg-ref>
</installer-gui-script>
X
P="$OUT/$PKG"
productbuild --distribution "$W/dist.xml" --resources "$W/res" --package-path "$W/pkgs" "$P"
echo "fertig: $P"
