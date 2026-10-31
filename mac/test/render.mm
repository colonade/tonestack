// Prüfbild der Mac-Oberfläche: zeichnet die Frontplatte über cairo_cg.hpp in ein Bitmap (wie die NSView) und speichert ein PNG.
// Bauen: clang++ -std=c++17 -fobjc-arc -DTS_GUI_HDR='"gui.hpp"' -DTS_NS=aorgui -DTS_FW=FW ... (siehe mac/test/run.sh)
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#include <cstdio>
#include TS_GUI_HDR
using namespace TS_NS;
int main(int argc, char **argv) {
  const char *out = argc > 1 ? argv[1] : "render.png"; double vals[64]; for (double &v : vals) v = 5;
  for (int i = 2; i < argc; i++) { int id; double v; if (std::sscanf(argv[i], "%d=%lf", &id, &v) == 2 && id >= 0 && id < 64) vals[id] = v; }
  Gui g; g.getP = [&](int i) { return vals[i]; }; g.setP = [&](int i, double v) { vals[i] = v; std::printf("setP %d = %g\n", i, v); };
  g.diag = []() { return std::string("CPU 3 % of one core · mac render test"); };
  g.computeCurves = [](std::vector<double> &cur, std::vector<double> &ref) { cur.assign(NF, 0); ref.assign(NF, 0); for (int i = 0; i < NF; i++) { double f = freqAt(i); cur[i] = 6 * std::sin(std::log(f) * 1.3); ref[i] = 3 * std::cos(std::log(f)); } };
  int W = 1200, H = (int)std::lround(W * TOTALH / TS_FW); g.W = W; g.H = H; g.loadImg();
  // Maus: Doppelklick auf einen Knopf setzt ihn auf Mitte, Mausrad dreht (Prüfung der Ereignisse)
  g.onMouse(1, W * 0.5, 40, 0, 0);
  int sc = 2; CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGContextRef cg = CGBitmapContextCreate(nullptr, W * sc, H * sc, 8, 0, cs, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  CGContextTranslateCTM(cg, 0, H * sc); CGContextScaleCTM(cg, sc, -sc);   // wie eine gespiegelte NSView auf einem Retina-Bildschirm
  g.drawCG(cg, W, H);
  CGImageRef im = CGBitmapContextCreateImage(cg);
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, (const UInt8 *)out, std::strlen(out), false);
  CGImageDestinationRef d = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr); CGImageDestinationAddImage(d, im, nullptr);
  bool ok = CGImageDestinationFinalize(d); std::printf("%s %dx%d %s\n", out, W * sc, H * sc, ok ? "ok" : "FEHLER");
  CFRelease(d); CFRelease(url); CGImageRelease(im); CGContextRelease(cg); CGColorSpaceRelease(cs); return ok ? 0 : 1;
}
