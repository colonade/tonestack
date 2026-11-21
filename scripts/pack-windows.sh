#!/bin/bash
# Packt die Windows-Plugins aus engine/build/win (vorher: bash engine/win/build.sh) nach dist/:
#   <Name>_Windows-<Version>.zip   (Ordner <Name>.vst3, <Name>.clap, README.txt, INSTALL-Windows.txt, LICENSE.txt (GPL-3.0), THIRD-PARTY-LICENSES*.txt)
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
  cp "$W/CLAP/$1.clap" "$z/"; cp -r "$W/VST3/$1.vst3" "$z/"; cp "reaper-clap/$3" "$z/README.txt"; cp reaper-clap/INSTALL-Windows.txt reaper-clap/THIRD-PARTY-LICENSES.txt reaper-clap/THIRD-PARTY-LICENSES-Windows.txt "$z/"; cp LICENSE "$z/LICENSE.txt"
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
while IFS='|' read -r NAME VV README SHORT DISP; do case "$NAME" in ''|'#'*) continue;; esac
  pack "$NAME" "${!VV}" "$README" "$DISP"; done < scripts/plugins.txt
cp reaper-clap/INSTALL-Windows.txt reaper-clap/THIRD-PARTY-LICENSES.txt reaper-clap/THIRD-PARTY-LICENSES-Windows.txt "$T/inst/docs/"; cp LICENSE "$T/inst/docs/LICENSE.txt"
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
!define MUI_WELCOMEPAGE_TEXT "\$(welcome)"
!insertmacro MUI_PAGE_WELCOME
!define MUI_LICENSEPAGE_BUTTON "\$(^NextBtn)"
!define MUI_LICENSEPAGE_TEXT_BOTTOM "\$(licbottom)"
!insertmacro MUI_PAGE_LICENSE "docs\\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"
LangString welcome \${LANG_ENGLISH} "Amp and effect simulators after the original schematics, as VST3 and CLAP plugins with front panels.\$\\r\$\\n\$\\r\$\\nVST3: \$COMMONFILES64\\VST3\$\\r\$\\nCLAP: \$COMMONFILES64\\CLAP\$\\r\$\\n\$\\r\$\\nPlease close your DAW (Cubase, REAPER, ...) before continuing: replacing a plugin that a running host has loaded can make it crash."
LangString welcome \${LANG_GERMAN} "Verstärker- und Effekt-Simulatoren nach den Original-Schaltplänen, als VST3- und CLAP-Plugins mit Frontplatte.\$\\r\$\\n\$\\r\$\\nVST3: \$COMMONFILES64\\VST3\$\\r\$\\nCLAP: \$COMMONFILES64\\CLAP\$\\r\$\\n\$\\r\$\\nBitte vorher die DAW (Cubase, REAPER, ...) schließen: tauscht man ein Plugin aus, das ein laufender Host geladen hat, kann er abstürzen."
LangString licbottom \${LANG_ENGLISH} "The plugins are free software under the GNU GPL version 3. You do not have to accept this licence to use them; it grants you the rights to copy, change and pass them on."
LangString licbottom \${LANG_GERMAN} "Die Plugins sind freie Software unter der GNU GPL Version 3. Zum Benutzen müssen Sie die Lizenz nicht annehmen; sie gibt Ihnen das Recht, sie zu kopieren, zu ändern und weiterzugeben."
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
  File "docs\\LICENSE.txt"
  WriteUninstaller "\$INSTDIR\\uninstall.exe"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "DisplayName" "tonestack plugins"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "Publisher" "tonestack"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "URLInfoAbout" "https://tonestack.fluxstatus.de/"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "UninstallString" '"\$INSTDIR\\uninstall.exe"'
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "InstallLocation" "\$INSTDIR"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "DisplayVersion" "$SUMVER"
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "NoModify" 1
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack" "NoRepair" 1
SectionEnd
$SECS
Section "Uninstall"
$UNS
  Delete "\$INSTDIR\\INSTALL-Windows.txt"
  Delete "\$INSTDIR\\THIRD-PARTY-LICENSES.txt"
  Delete "\$INSTDIR\\THIRD-PARTY-LICENSES-Windows.txt"
  Delete "\$INSTDIR\\LICENSE.txt"
  Delete "\$INSTDIR\\uninstall.exe"
  RMDir "\$INSTDIR"
  RMDir "\$COMMONFILES64\\CLAP"
  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\tonestack"
SectionEnd
EOF
find "$T/inst" -exec touch -h -d "$STAMP" {} +
(cd "$T/inst" && SOURCE_DATE_EPOCH=1767225600 makensis -V2 -INPUTCHARSET UTF8 tonestack.nsi >/dev/null)
mv "$T/inst/out.exe" "$D/$WINEXE"; echo "$D/$WINEXE ($N Plugins)"
