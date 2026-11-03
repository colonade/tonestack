#!/bin/bash
# Prüfbilder der Mac-Oberflächen (Laney, Yamaha, AmpFett B-15NF, GK 800RB) nach mac/test/out/*.png
set -euo pipefail
cd "$(dirname "$0")/../.."; E=engine; O=mac/test/out; mkdir -p "$O"
FL=(-std=c++17 -O3 -arch arm64 -arch x86_64 -fobjc-arc -I$E/clap -I$E/src -I$E/gen -I$E/third_party -framework Foundation -framework CoreGraphics -framework CoreText -framework ImageIO -framework Cocoa)
clang++ "${FL[@]}" -DTS_GUI_HDR='"gui.hpp"' -DTS_NS=aorgui -DTS_FW=FW mac/test/render.mm $E/clap/macview.mm -o /tmp/render_laney
clang++ "${FL[@]}" -DTS_GUI_HDR='"ygui.hpp"' -DTS_NS=ygui -DTS_FW=YFW mac/test/render.mm $E/clap/macview.mm -o /tmp/render_yamaha
clang++ "${FL[@]}" -DTS_GUI_HDR='"b15gui.hpp"' -DTS_NS=bgui -DTS_FW=FW mac/test/render.mm $E/clap/macview.mm -o /tmp/render_b15nf
clang++ "${FL[@]}" -DTS_GUI_HDR='"gkgui.hpp"' -DTS_NS=gkgui -DTS_FW=FW mac/test/render.mm $E/clap/macview.mm -o /tmp/render_gk
/tmp/render_laney "$O/laney.png" 0=0 1=0 2=2 3=2 4=2 9=1 10=1 11=1 12=1 13=1 14=0 15=0 16=1 17=100 18=1 20=3 21=3 22=3 23=0 24=1 25=0
/tmp/render_yamaha "$O/yamaha.png" 0=0 5=0 6=2 7=-1 8=0 9=1 10=0 11=1 12=100 13=1 14=0
/tmp/render_b15nf "$O/b15nf.png" 0=1 1=7 2=3.5 3=6 4=5 5=5 6=5 7=0 8=1 9=1 10=100 11=1 12=0
/tmp/render_gk "$O/gk.png" 0=0 1=5 2=0 3=1 4=0 5=6 6=5 7=4 8=7 9=5 10=1 11=0 12=5 13=5 14=5 15=1 16=1 17=100 18=1 19=0 20=0
# Zeichenzeit unter Rosetta (x86_64-Teil derselben Programme), Bilder nur zum Vergleich
if arch -x86_64 /usr/bin/true 2>/dev/null; then
  arch -x86_64 /tmp/render_laney /tmp/rosetta_laney.png 0=0 1=0 2=2 3=2 4=2 9=1 10=1 11=1 12=1 13=1 14=0 15=0 16=1 17=100 18=1 20=3 21=3 22=3 23=0 24=1 25=0 | grep RESULT | sed 's/kind=draw/kind=draw arch=x86_64/'
  arch -x86_64 /tmp/render_yamaha /tmp/rosetta_yamaha.png 0=0 5=0 6=2 7=-1 8=0 9=1 10=0 11=1 12=100 13=1 14=0 | grep RESULT | sed 's/kind=draw/kind=draw arch=x86_64/'
  arch -x86_64 /tmp/render_b15nf /tmp/rosetta_b15nf.png 0=1 1=7 2=3.5 3=6 4=5 5=5 6=5 7=0 8=1 9=1 10=100 11=1 12=0 | grep RESULT | sed 's/kind=draw/kind=draw arch=x86_64/'
  arch -x86_64 /tmp/render_gk /tmp/rosetta_gk.png 0=0 1=5 2=0 3=1 4=0 5=6 6=5 7=4 8=7 9=5 10=1 11=0 12=5 13=5 14=5 15=1 16=1 17=100 18=1 19=0 20=0 | grep RESULT | sed 's/kind=draw/kind=draw arch=x86_64/'
else echo "RESULT kind=draw arch=x86_64 rosetta=missing"; fi
