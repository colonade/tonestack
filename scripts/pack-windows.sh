#!/bin/bash
# Packt die Windows-Plugins aus engine/build/win (vorher: bash engine/win/build.sh) nach dist/:
#   <Name>_Windows-<Version>.zip   (Ordner <Name>.vst3, <Name>.clap, README.txt, INSTALL-Windows.txt, THIRD-PARTY-LICENSES*.txt)
#   tonestack-Plugins-Windows.exe  Installer (NSIS, makensis): Auswahl je Plugin, VST3 nach %CommonProgramFiles%\VST3, CLAP nach
#                                  %CommonProgramFiles%\CLAP, Anleitungen nach %ProgramFiles%\tonestack, Deinstallation unter „Apps“
# Reproduzierbar: feste Zeitstempel und Reihenfolge.
set -euo pipefail
R="$(cd "$(dirname "$0")/.." && pwd)"; cd "$R"; eval "$(bash scripts/versions.sh)"
W=engine/build/win; D=dist; mkdir -p "$D"; T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT
command -v makensis >/dev/null || { echo "makensis fehlt (Paket nsis)" >&2; exit 1; }
STAMP="2026-01-01 00:00:00 UTC"; mkdir -p "$T/inst/docs"; SECS=""; UNS=""; N=0
pack() {   # $1 Name, $2 Version, $3 README, $4 Anzeigename
  for f in "$W/CLAP/$1.clap" "$W/VST3/$1.vst3"; do [ -e "$f" ] || { echo "fehlt: $f (erst bash engine/win/build.sh)" >&2; exit 1; }; done
  rm -f "$D/$1"_Windows-*.zip; local z="$T/$1"; mkdir -p "$z"
  cp "$W/CLAP/$1.clap" "$z/"; cp -r "$W/VST3/$1.vst3" "$z/"; cp "reaper-clap/$3" "$z/README.txt"; cp reaper-clap/INSTALL-Windows.txt reaper-clap/THIRD-PARTY-LICENSES.txt reaper-clap/THIRD-PARTY-LICENSES-Windows.txt "$z/"
  find "$z" -exec touch -h -d "$STAMP" {} +
  (cd "$z" && find . -mindepth 1 | LC_ALL=C sort | TZ=UTC zip -q -X -9 "$R/$D/$1_Windows-$2.zip" -@)
  mkdir -p "$T/inst/$1"; cp "$W/CLAP/$1.clap" "$T/inst/$1/"; cp -r "$W/VST3/$1.vst3" "$T/inst/$1/"; cp "reaper-clap/$3" "$T/inst/docs/README-$1.txt"
  N=$((N + 1))
  SECS="$SECS
Section \"$4 $2\" sec$N
  SetOutPath \"\$COMMONFILES64\\VST3\"
  RMDir /r \"\$COMMONFILES64\\VST3\\$1.vst3\"
  File /r \"$1\\$1.vst3\"
  SetOutPath \"\$COMMONFILES64\\CLAP\"
  File \"$1\\$1.clap\"
  SetOutPath \"\$INSTDIR\"
  File \"docs\\README-$1.txt\"
SectionEnd"
  UNS="$UNS
  RMDir /r \"\$COMMONFILES64\\VST3\\$1.vst3\"
  Delete \"\$COMMONFILES64\\CLAP\\$1.clap\"
  Delete \"\$INSTDIR\\README-$1.txt\""
  echo "$D/$1_Windows-$2.zip"; }
pack Laney_AOR50 "$LANEY" README.txt "Laney AOR 50 Preamp"
pack Lametta_B100II "$YAMAHA" README-B100II.txt "Lametta B100-II Bass Amp"
pack AmpFett_B15NF "$B15" README-B15NF.txt "AmpFett B-15NF Bass Amp"
pack GK_800RB "$GK" README-GK.txt "GK 800RB Bass Amp"
pack Lametta_G100 "$G100" README-G100.txt "Lametta G100-212II Guitar Amp"
pack Lametta_B100III "$B100III" README-B100III.txt "Lametta B100-III Bass Amp"
pack AmpFett_SVTCL "$SVTCL" README-SVTCL.txt "AmpFett SVT-CL Bass Amp"
pack Wien_EQ "$WIENEQ" README-WienEQ.txt "Wien EQ"
pack AmpFett_76 "$AMPFETT76" README-AmpFett76.txt "AmpFett 76 Limiting Amplifier"
pack Loldano_SLO100 "$LOLDANO" README-Loldano.txt "Loldano SLO-100 Guitar Amp"
pack Kaiserin_Reverb "$KAISERIN" README-Kaiserin.txt "Kaiserin Reverb"
pack Blech_140 "$BLECH" README-Blech140.txt "Blech 140 Plate Reverb"
pack Wisent_Echoreck "$WISENT" README-Wisent.txt "Wisent Echoreck Drum Echo"
pack Viereck_34 "$VIERECK" README-Viereck34.txt "Viereck 34 Tilt EQ"
cp reaper-clap/INSTALL-Windows.txt reaper-clap/THIRD-PARTY-LICENSES.txt reaper-clap/THIRD-PARTY-LICENSES-Windows.txt "$T/inst/docs/"
cat > "$T/inst/tonestack.nsi" <<EOF
; tonestack-Plugins für Windows (erzeugt von scripts/pack-windows.sh)
Unicode true
!include "x64.nsh"
!include "MUI2.nsh"
Name "tonestack plugins"
OutFile "out.exe"
InstallDir "\$PROGRAMFILES64\\tonestack"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
!define MUI_WELCOMEPAGE_TEXT "Amp and effect simulators after the original schematics, as VST3 and CLAP plugins with front panels.\$\\r\$\\n\$\\r\$\\nVST3: \$COMMONFILES64\\VST3\$\\r\$\\nCLAP: \$COMMONFILES64\\CLAP\$\\r\$\\n\$\\r\$\\nPlease close your DAW (Cubase, REAPER, ...) before continuing: replacing a plugin that a running host has loaded can make it crash."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"
Function .onInit
  \${IfNot} \${RunningX64}
    MessageBox MB_ICONSTOP "These plugins need 64-bit Windows (x86-64)."
    Abort
  \${EndIf}
  SetRegView 64
FunctionEnd
Function un.onInit
  SetRegView 64
FunctionEnd
Section "-common"
  SetOutPath "\$INSTDIR"
  File "docs\\INSTALL-Windows.txt"
  File "docs\\THIRD-PARTY-LICENSES.txt"
  File "docs\\THIRD-PARTY-LICENSES-Windows.txt"
  WriteUninstaller "\$INSTDIR\\uninstall.exe"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "DisplayName" "tonestack plugins"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "Publisher" "tonestack"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "URLInfoAbout" "https://tonestack.fluxstatus.de/"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "UninstallString" '"\$INSTDIR\\uninstall.exe"'
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "InstallLocation" "\$INSTDIR"
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "NoModify" 1
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "NoRepair" 1
SectionEnd
$SECS
Section "Uninstall"
$UNS
  Delete "\$INSTDIR\\INSTALL-Windows.txt"
  Delete "\$INSTDIR\\THIRD-PARTY-LICENSES.txt"
  Delete "\$INSTDIR\\THIRD-PARTY-LICENSES-Windows.txt"
  Delete "\$INSTDIR\\uninstall.exe"
  RMDir "\$INSTDIR"
  RMDir "\$COMMONFILES64\\CLAP"
  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack"
SectionEnd
EOF
find "$T/inst" -exec touch -h -d "$STAMP" {} +
(cd "$T/inst" && SOURCE_DATE_EPOCH=1767225600 makensis -V2 -INPUTCHARSET UTF8 tonestack.nsi >/dev/null)
mv "$T/inst/out.exe" "$D/$WINEXE"; echo "$D/$WINEXE ($N Plugins)"
