// NSView für die Plugin-Oberfläche: gespiegelte Koordinaten (y nach unten, wie unter X11), Zeichnen über CoreGraphics,
// Maus und Mausrad an die Oberfläche weiterreichen, eigener Takt (30/s) für Anzeigen und Neuzeichnen.
// Ab Laney 3.33 / Yamaha 1.14 / AmpFett 1.14 wird die Klasse zur Laufzeit angelegt, mit einem Namen, der je geladener Binärdatei
// eindeutig ist (Adresse eines eigenen Symbols): vorher hieß sie in CLAP, VST3 und Audio Unit desselben Plugins gleich (und in
// alter und neuer Fassung). Lädt ein Host zwei davon (REAPER: VST3 und AU; alte Handinstallation neben der neuen), nimmt das
// Objective-C-Laufzeitsystem nur eine der gleichnamigen Klassen – deren Code läge dann in der anderen Datei, und wird die entladen,
// stürzt jede offene Oberfläche ab. TS_VIEW_CLASS bleibt Namensvorsilbe (lesbar in Absturzberichten).
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <cstdio>
#include <memory>
#include "macview.h"
#ifndef TS_VIEW_CLASS
#define TS_VIEW_CLASS TSPluginView
#endif
#define TS_STR2(x) #x
#define TS_STR(x) TS_STR2(x)
@interface NSObject (TSViewTick) - (void)tsTick:(NSTimer *)t; @end   // Selektor des Takts bekannt machen (nur Deklaration)

namespace {
// Zustand je Ansicht. Besitz: ein der Ansicht zugeordneter Block hält einen shared_ptr darauf; gibt AppKit die Ansicht frei,
// werden ihre zugeordneten Objekte losgelassen und der Zustand gelöscht (kein eigener Klassenname nötig). Zugriff über einen zweiten Eintrag (Zeiger).
struct ViewState { tsmac::Client *client = nullptr; NSTimer *timer = nil; NSTrackingArea *track = nil; double wheelAcc = 0; };
const char kStateKey = 0, kOwnerKey = 0;
}

namespace {
ViewState *stateOf(id self) { NSValue *v = objc_getAssociatedObject(self, &kStateKey); return v ? (ViewState *)[v pointerValue] : nullptr; }
void callSuper(id self, SEL sel) { struct objc_super sup = {self, [NSView class]}; ((void (*)(struct objc_super *, SEL))objc_msgSendSuper)(&sup, sel); }
BOOL yes(id, SEL) { return YES; }
BOOL yes1(id, SEL, id) { return YES; }
void drawRect(id self, SEL, NSRect) { ViewState *s = stateOf(self); if (!s || !s->client) return;
  CGContextRef cg = [[NSGraphicsContext currentContext] CGContext]; if (!cg) return;
  NSRect b = [(NSView *)self bounds]; CGContextSaveGState(cg); s->client->drawCG(cg, b.size.width, b.size.height); CGContextRestoreGState(cg); }
void send(id self, int kind, NSEvent *e, int button) { ViewState *s = stateOf(self); if (!s || !s->client) return;
  NSPoint p = [(NSView *)self convertPoint:[e locationInWindow] fromView:nil]; if (s->client->mouse(kind, p.x, p.y, button, [e timestamp])) [(NSView *)self setNeedsDisplay:YES]; }
void mouseDown(id self, SEL, NSEvent *e) { send(self, 0, e, 1); }
void mouseDragged(id self, SEL, NSEvent *e) { send(self, 1, e, 1); }
void mouseMoved(id self, SEL, NSEvent *e) { send(self, 1, e, 0); }
void mouseUp(id self, SEL, NSEvent *e) { send(self, 2, e, 1); }
void mouseExited(id self, SEL, NSEvent *e) { send(self, 3, e, 0); }
void scrollWheel(id self, SEL, NSEvent *e) {   // Mausrad und Trackpad: in Rasten wie unter X11 (Taste 4 = hoch, 5 = runter)
  ViewState *s = stateOf(self); if (!s) return; double d = [e hasPreciseScrollingDeltas] ? [e scrollingDeltaY] / 12.0 : [e scrollingDeltaY]; s->wheelAcc += d;
  while (s->wheelAcc >= 1) { s->wheelAcc -= 1; send(self, 0, e, 4); } while (s->wheelAcc <= -1) { s->wheelAcc += 1; send(self, 0, e, 5); } }
void updateTrackingAreas(id self, SEL sel) { callSuper(self, sel); ViewState *s = stateOf(self); if (!s) return; NSView *v = self;
  if (s->track) [v removeTrackingArea:s->track];
  s->track = [[NSTrackingArea alloc] initWithRect:[v bounds] options:(NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect) owner:v userInfo:nil];
  [v addTrackingArea:s->track]; }
void tick(id self, SEL, NSTimer *) { ViewState *s = stateOf(self); NSView *v = self; if (!s || !s->client || [v isHiddenOrHasHiddenAncestor]) return; if (s->client->tick()) [v setNeedsDisplay:YES]; }
void viewDidMoveToWindow(id self, SEL sel) { callSuper(self, sel); ViewState *s = stateOf(self); if (!s) return; NSView *v = self;
  if ([v window] && !s->timer) { s->timer = [NSTimer timerWithTimeInterval:1.0 / 30 target:v selector:@selector(tsTick:) userInfo:nil repeats:YES]; [[NSRunLoop currentRunLoop] addTimer:s->timer forMode:NSRunLoopCommonModes]; }
  else if (![v window] && s->timer) { [s->timer invalidate]; s->timer = nil; } }
// Klasse einmal je Binärdatei anlegen; der Name enthält die Adresse von kStateKey (je geladenem Bild verschieden)
const char *enc(SEL sel) { Method m = class_getInstanceMethod([NSView class], sel); return m ? method_getTypeEncoding(m) : "v@:@"; }   // Typangabe wie bei NSView
Class viewClass() {
  static Class cls = nil; if (cls) return cls;
  char name[160]; std::snprintf(name, sizeof name, "%s_%llx", TS_STR(TS_VIEW_CLASS), (unsigned long long)(uintptr_t)&kStateKey);
  if ((cls = objc_getClass(name))) return cls;   // (nur falls dieselbe Datei zweimal initialisiert würde)
  Class c = objc_allocateClassPair([NSView class], name, 0); if (!c) return nil;
  class_addMethod(c, @selector(isFlipped), (IMP)yes, enc(@selector(isFlipped)));
  class_addMethod(c, @selector(isOpaque), (IMP)yes, enc(@selector(isOpaque)));
  class_addMethod(c, @selector(acceptsFirstMouse:), (IMP)yes1, enc(@selector(acceptsFirstMouse:)));
  class_addMethod(c, @selector(drawRect:), (IMP)drawRect, enc(@selector(drawRect:)));
  class_addMethod(c, @selector(mouseDown:), (IMP)mouseDown, enc(@selector(mouseDown:)));
  class_addMethod(c, @selector(mouseDragged:), (IMP)mouseDragged, enc(@selector(mouseDragged:)));
  class_addMethod(c, @selector(mouseMoved:), (IMP)mouseMoved, enc(@selector(mouseMoved:)));
  class_addMethod(c, @selector(mouseUp:), (IMP)mouseUp, enc(@selector(mouseUp:)));
  class_addMethod(c, @selector(mouseExited:), (IMP)mouseExited, enc(@selector(mouseExited:)));
  class_addMethod(c, @selector(scrollWheel:), (IMP)scrollWheel, enc(@selector(scrollWheel:)));
  class_addMethod(c, @selector(updateTrackingAreas), (IMP)updateTrackingAreas, enc(@selector(updateTrackingAreas)));
  class_addMethod(c, @selector(viewDidMoveToWindow), (IMP)viewDidMoveToWindow, enc(@selector(viewDidMoveToWindow)));
  class_addMethod(c, @selector(tsTick:), (IMP)tick, "v@:@");
  objc_registerClassPair(c); cls = c; return cls; }
}

namespace tsmac {
void *create(void *parent, int w, int h, Client *c) {
  Class cls = viewClass(); if (!cls) return nullptr;
  NSView *v = [[cls alloc] initWithFrame:NSMakeRect(0, 0, w, h)]; if (!v) return nullptr;
  std::shared_ptr<ViewState> sp = std::make_shared<ViewState>(); sp->client = c; ViewState *s = sp.get();
  objc_setAssociatedObject(v, &kStateKey, [NSValue valueWithPointer:s], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  objc_setAssociatedObject(v, &kOwnerKey, [^{ (void)sp; } copy], OBJC_ASSOCIATION_RETAIN_NONATOMIC);   // Block besitzt den Zustand
  if (parent) [(__bridge NSView *)parent addSubview:v];
  return (__bridge_retained void *)v; }
void setSize(void *view, int w, int h) { if (!view) return; NSView *v = (__bridge NSView *)view; [v setFrameSize:NSMakeSize(w, h)]; [v setNeedsDisplay:YES]; }
void show(void *view) { if (view) { NSView *v = (__bridge NSView *)view; [v setHidden:NO]; [v setNeedsDisplay:YES]; } }
void hide(void *view) { if (view) [(__bridge NSView *)view setHidden:YES]; }
void redraw(void *view) { if (view) [(__bridge NSView *)view setNeedsDisplay:YES]; }
void destroy(void *view) { if (!view) return; NSView *v = (__bridge_transfer NSView *)view; ViewState *s = stateOf(v);
  if (s) { s->client = nullptr; if (s->timer) { [s->timer invalidate]; s->timer = nil; } if (s->track) { [v removeTrackingArea:s->track]; s->track = nil; } }
  [v removeFromSuperview]; }
}
