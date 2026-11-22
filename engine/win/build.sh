#!/bin/bash
# Windows-Fassungen der Plugins (x86-64, VST3 und CLAP, mit Frontplatte), unter Linux mit MinGW-w64 übersetzt.
#   bash engine/win/build.sh [CAIRO_ORDNER]
# CAIRO_ORDNER (Standard engine/build/win-cairo): statisches Cairo/pixman, wird mit engine/win/build-cairo.sh angelegt, falls es fehlt.
# Ergebnis in engine/build/win: VST3/<Name>.vst3 (Ordner-Bundle), CLAP/<Name>.clap, Prüfprogramme plugtest.exe und wingui.exe.
# Braucht: g++-mingw-w64-x86-64-posix, cmake, ninja, meson, pkg-config (für Cairo zusätzlich die Ubuntu-Quellpakete, siehe build-cairo.sh).
set -euo pipefail
E="$(cd "$(dirname "$0")/.." && pwd)"; C="$(realpath -m "${1:-$E/build/win-cairo}")"; B="$E/build/win"
[ -f "$C/prefix/lib/libcairo.a" ] || bash "$E/win/build-cairo.sh" "$C"
cmake -S "$E/vst3" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$E/win/mingw64.cmake" -DTS_CAIRO_PREFIX="$C/prefix" >/dev/null
cmake --build "$B"
CXX=x86_64-w64-mingw32-g++-posix
$CXX -O2 -std=gnu++17 -I"$E/third_party" "$E/tests/plugtest.cpp" -o "$B/plugtest.exe" -static -Wl,--no-insert-timestamp
$CXX -O2 -std=gnu++17 -I"$E/third_party" "$E/tests/wingui.cpp" -o "$B/wingui.exe" -static -lgdi32 -luser32 -Wl,--no-insert-timestamp
# Prüfung: jede Plugin-DLL braucht nur Bibliotheken, die zu Windows gehören, und exportiert nur ihre Einstiege
ok=1; n=0
for f in "$B"/CLAP/*.clap "$B"/VST3/*.vst3/Contents/x86_64-win/*.vst3; do n=$((n + 1))
  imp="$(x86_64-w64-mingw32-objdump -p "$f" | sed -n 's/^\s*DLL Name: //p' | tr 'A-Z' 'a-z' | sort -u | tr '\n' ' ')"
  for d in $imp; do case "$d" in kernel32.dll|user32.dll|gdi32.dll|msvcrt.dll|ole32.dll|shell32.dll|advapi32.dll|msimg32.dll|dwrite.dll|d2d1.dll|windowscodecs.dll) ;; *) echo "FEHLER: $f braucht $d" >&2; ok=0;; esac; done
  exp="$(x86_64-w64-mingw32-objdump -p "$f" | sed -n '/\[Ordinal\/Name Pointer\] Table/,/^$/p' | sed -n 's/^\s*\[ *[0-9]*\] //p' | sort | tr '\n' ' ')"
  case "$f" in *.clap) want="clap_entry ";; *) want="ExitDll GetPluginFactory InitDll clap_entry ";; esac
  [ "$exp" = "$want" ] || { echo "FEHLER: $f exportiert: $exp" >&2; ok=0; }
done
[ "$n" = 30 ] || { echo "FEHLER: $n statt 30 Plugin-Dateien" >&2; ok=0; }
[ "$ok" = 1 ] || exit 1
echo "Windows: $n Plugin-Dateien in $B (nur Windows-Bibliotheken, nur Einstiege exportiert)"
