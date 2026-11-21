#!/bin/bash
# Baut pixman und Cairo als statische Bibliotheken für Windows x86-64 (MinGW-w64), für die Oberflächen der Windows-Plugins.
#   bash engine/win/build-cairo.sh ZIELORDNER
# Quellen: Ubuntu-Quellpakete (apt-get source pixman cairo; braucht „deb-src“ in den Paketquellen), in ZIELORDNER/src.
# Ergebnis: ZIELORDNER/prefix/{include/cairo,lib/libcairo.a,lib/libpixman-1.a}. Braucht g++-mingw-w64-x86-64-posix, meson, ninja, pkg-config.
# Cairo nur mit dem, was die Oberflächen brauchen: Bild-Oberfläche, Schrift über DirectWrite (win32), ohne PNG, FreeType, Fontconfig, X11, zlib.
# Exporte: pixman und Cairo markieren ihre Funktionen sonst mit __declspec(dllexport) – sie stünden dann in der Exporttabelle jeder Plugin-DLL.
set -euo pipefail
Z="$(realpath -m "${1:?Zielordner angeben}")"; H="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$Z/src" "$Z/prefix"
cd "$Z/src"
[ -d pixman-0.42.2 ] || apt-get source -q pixman=0.42.2-1build1 >/dev/null
[ -d cairo-1.18.0 ] || apt-get source -q cairo=1.18.0-3build1 >/dev/null
[ -d pixman-0.42.2 ] && [ -d cairo-1.18.0 ] || { echo "FEHLER: Quellen pixman 0.42.2 / cairo 1.18.0 fehlen" >&2; exit 1; }
sed "s|@PFX@|$Z/prefix|" > "$Z/mingw64.ini" <<'EOF'
[binaries]
c = 'x86_64-w64-mingw32-gcc-posix'
cpp = 'x86_64-w64-mingw32-g++-posix'
ar = 'x86_64-w64-mingw32-ar'
strip = 'x86_64-w64-mingw32-strip'
windres = 'x86_64-w64-mingw32-windres'
pkg-config = 'pkg-config'

[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
EOF
export PKG_CONFIG_LIBDIR="$Z/prefix/lib/pkgconfig" PKG_CONFIG_PATH=
# pixman: dllexport abschalten (meson.build setzt es fest, sobald der Übersetzer es kennt)
sed -i "s/^if cc.has_function_attribute('dllexport')$/if false  # tonestack: statisch, keine Exporte/" pixman-0.42.2/pixman/meson.build
grep -q "tonestack: statisch" pixman-0.42.2/pixman/meson.build || { echo "FEHLER: pixman/meson.build nicht wie erwartet" >&2; exit 1; }
rm -rf "$Z/b-pixman" "$Z/b-cairo"
meson setup "$Z/b-pixman" pixman-0.42.2 --cross-file "$Z/mingw64.ini" --prefix "$Z/prefix" --default-library static --buildtype release \
  -Dtests=disabled -Dgtk=disabled -Dlibpng=disabled -Dopenmp=disabled >/dev/null
ninja -C "$Z/b-pixman" install >/dev/null
meson setup "$Z/b-cairo" cairo-1.18.0 --cross-file "$Z/mingw64.ini" --prefix "$Z/prefix" --default-library static --buildtype release \
  -Dcpp_args=-DCAIRO_WIN32_STATIC_BUILD -Ddwrite=enabled -Dfontconfig=disabled -Dfreetype=disabled -Dpng=disabled -Dquartz=disabled -Dtee=disabled \
  -Dxcb=disabled -Dxlib=disabled -Dxlib-xcb=disabled -Dzlib=disabled -Dtests=disabled -Dglib=disabled -Dspectre=disabled -Dsymbol-lookup=disabled -Dgtk2-utils=disabled >/dev/null
ninja -C "$Z/b-cairo" install >/dev/null
for l in libcairo.a libpixman-1.a; do [ -f "$Z/prefix/lib/$l" ] || { echo "FEHLER: $l fehlt" >&2; exit 1; }; done
# keine dllexport-Markierungen mehr in den Bibliotheken
if x86_64-w64-mingw32-objdump -s -j .drectve "$Z/prefix/lib/libcairo.a" "$Z/prefix/lib/libpixman-1.a" 2>/dev/null | grep -q "export"; then echo "FEHLER: Exporte in Cairo/pixman" >&2; exit 1; fi
echo "Cairo 1.18.0 / pixman 0.42.2 für Windows in $Z/prefix"
