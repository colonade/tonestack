#!/bin/bash
# Mac-Installerpaket (.pkg) aus den signierten Bundles (nach mac/package.sh): alle Plugins einzeln wählbar, je Audio Unit + VST3,
# Ziel /Library/Audio/Plug-Ins/{Components,VST3} (für alle Benutzer). Vom Installer abgelegte Dateien tragen keine
# Quarantäne-Markierung; nur das Paket selbst muss beim ersten Öffnen freigegeben werden (nicht bei Apple beglaubigt).
#   mac/make_pkg.sh <build-Ordner> <Ausgabe-Ordner>
set -euo pipefail
B="$1"; OUT="$2"; HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/.."
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
eval "$(bash "$ROOT/scripts/versions.sh")"; LV="$LANEY"; YV="$YAMAHA"; BV="$B15"; GV="$GK"   # Versionen wie im Release
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
OLD=""; for n in $NAMES; do [ "$n" = Ampeg_B15NF ] && OLD=1; done
# Ampeg_B15NF (1.0) heißt seit 1.1 AmpFett_B15NF: alte systemweite Fassung entfernen, sonst stünden beide in Logic
if [ -n "$OLD" ]; then rm -rf "/Library/Audio/Plug-Ins/Components/Ampeg_B15NF.component" "/Library/Audio/Plug-Ins/VST3/Ampeg_B15NF.vst3"; fi
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
post laney Laney_AOR50; post yamaha Yamaha_B100II; post b15nf AmpFett_B15NF Ampeg_B15NF; post gk800rb GK_800RB
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
one Yamaha_B100II yamaha "$YV" component; one Yamaha_B100II yamaha "$YV" vst3
one AmpFett_B15NF b15nf "$BV" component; one AmpFett_B15NF b15nf "$BV" vst3
one GK_800RB gk800rb "$GV" component; one GK_800RB gk800rb "$GV" vst3
sed -e "s/@LV@/$LV/g" -e "s/@YV@/$YV/g" -e "s/@BV@/$BV/g" -e "s/@GV@/$GV/g" "$HERE/welcome.html" > "$W/res/welcome.html"
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
  </choices-outline>
  <choice id="laney" title="Laney AOR 50 Preamp $LV" description="Röhren-Vorverstärker mit Endstufe und Lautsprecher. Audio Unit (Logic, GarageBand, MainStage) und VST3.">
    <pkg-ref id="de.tonestack.laney.component"/><pkg-ref id="de.tonestack.laney.vst3"/>
  </choice>
  <choice id="yamaha" title="Yamaha B100-II Bass Amp $YV" description="Transistor-Bassverstärker mit 5-Band-EQ, Endstufe und Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.yamaha.component"/><pkg-ref id="de.tonestack.yamaha.vst3"/>
  </choice>
  <choice id="b15nf" title="AmpFett B-15NF Bass Amp $BV" description="Röhren-Bassverstärker nach dem Werksschaltplan (6SL7, 6L6GC, 5AR4) mit Netzteil, Gegenkopplung und Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.b15nf.component"/><pkg-ref id="de.tonestack.b15nf.vst3"/>
  </choice>
  <choice id="gk800rb" title="GK 800RB Bass Amp $GV" description="Transistor-Bassverstärker nach dem Service-Manual 1991: Vorstufe als Schaltung (4-Band-EQ, Filter, Boost mit J113), Bi-Amp-Weiche, zwei Endstufen, Lautsprecher. Audio Unit und VST3.">
    <pkg-ref id="de.tonestack.gk800rb.component"/><pkg-ref id="de.tonestack.gk800rb.vst3"/>
  </choice>
  <pkg-ref id="de.tonestack.laney.component" version="$LV">laney-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.laney.vst3" version="$LV">laney-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.component" version="$YV">yamaha-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.vst3" version="$YV">yamaha-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.component" version="$BV">b15nf-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.vst3" version="$BV">b15nf-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.gk800rb.component" version="$GV">gk800rb-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.gk800rb.vst3" version="$GV">gk800rb-vst3.pkg</pkg-ref>
</installer-gui-script>
X
P="$OUT/$PKG"
productbuild --distribution "$W/dist.xml" --resources "$W/res" --package-path "$W/pkgs" "$P"
echo "fertig: $P"
