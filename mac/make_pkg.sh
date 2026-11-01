#!/bin/bash
# Mac-Installerpaket (.pkg) aus den signierten Bundles (nach mac/package.sh): alle Plugins einzeln wählbar, je Audio Unit + VST3,
# Ziel /Library/Audio/Plug-Ins/{Components,VST3} (für alle Benutzer). Vom Installer abgelegte Dateien tragen keine
# Quarantäne-Markierung; nur das Paket selbst muss beim ersten Öffnen freigegeben werden (nicht bei Apple beglaubigt).
#   mac/make_pkg.sh <build-Ordner> <Ausgabe-Ordner>
set -euo pipefail
B="$1"; OUT="$2"; HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/.."
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
eval "$(bash "$ROOT/scripts/versions.sh")"; LV="$LANEY"; YV="$YAMAHA"; BV="$B15"   # Versionen wie im Release
W="$(mktemp -d)"; mkdir -p "$W/pkgs" "$W/scripts" "$W/res"
cat > "$W/scripts/postinstall" <<'S'
#!/bin/bash
# Audio-Unit-Verzeichnis neu einlesen lassen, damit Logic die neue Fassung sofort findet
# Ampeg_B15NF (1.0) heißt seit 1.1 AmpFett_B15NF: alte Fassung entfernen, sonst stünden beide in Logic (feste Pfade)
rm -rf "/Library/Audio/Plug-Ins/Components/Ampeg_B15NF.component" "/Library/Audio/Plug-Ins/VST3/Ampeg_B15NF.vst3"
killall -9 AudioComponentRegistrar 2>/dev/null || true
exit 0
S
chmod +x "$W/scripts/postinstall"
one() {   # one <Name> <id> <Version> <component|vst3>
  local NAME="$1" ID="$2" V="$3" KIND="$4" DEST; [ "$KIND" = component ] && DEST=Components || DEST=VST3
  local SRC; SRC="$(find "$B" -maxdepth 3 -name "$NAME.$KIND" -type d | head -1)"; [ -n "$SRC" ] || { echo "FEHLER: $NAME.$KIND fehlt"; exit 1; }
  local R="$W/root-$ID-$KIND"; mkdir -p "$R/Library/Audio/Plug-Ins/$DEST"; cp -R "$SRC" "$R/Library/Audio/Plug-Ins/$DEST/"
  pkgbuild --analyze --root "$R" "$W/$ID-$KIND.plist" >/dev/null
  local n; n=$(plutil -extract . raw -o - "$W/$ID-$KIND.plist" 2>/dev/null || echo 1)
  for ((i = 0; i < n; i++)); do plutil -replace "$i.BundleIsRelocatable" -bool NO "$W/$ID-$KIND.plist"; done   # immer an den festen Ort
  local SC=(); [ "$KIND" = component ] && SC=(--scripts "$W/scripts")
  pkgbuild --root "$R" --component-plist "$W/$ID-$KIND.plist" --identifier "de.tonestack.$ID.$KIND" --version "$V" --install-location / ${SC[@]+"${SC[@]}"} "$W/pkgs/$ID-$KIND.pkg"
}
one Laney_AOR50 laney "$LV" component; one Laney_AOR50 laney "$LV" vst3
one Yamaha_B100II yamaha "$YV" component; one Yamaha_B100II yamaha "$YV" vst3
one AmpFett_B15NF b15nf "$BV" component; one AmpFett_B15NF b15nf "$BV" vst3
sed -e "s/@LV@/$LV/g" -e "s/@YV@/$YV/g" -e "s/@BV@/$BV/g" "$HERE/welcome.html" > "$W/res/welcome.html"
cat > "$W/dist.xml" <<X
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>tonestack Verstärker-Plugins</title>
  <welcome file="welcome.html" mime-type="text/html"/>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
  <os-version min="11.0"/>
  <choices-outline>
    <line choice="laney"/>
    <line choice="yamaha"/>
    <line choice="b15nf"/>
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
  <pkg-ref id="de.tonestack.laney.component" version="$LV">laney-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.laney.vst3" version="$LV">laney-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.component" version="$YV">yamaha-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.yamaha.vst3" version="$YV">yamaha-vst3.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.component" version="$BV">b15nf-component.pkg</pkg-ref>
  <pkg-ref id="de.tonestack.b15nf.vst3" version="$BV">b15nf-vst3.pkg</pkg-ref>
</installer-gui-script>
X
P="$OUT/$PKG"
productbuild --distribution "$W/dist.xml" --resources "$W/res" --package-path "$W/pkgs" "$P"
echo "fertig: $P"
