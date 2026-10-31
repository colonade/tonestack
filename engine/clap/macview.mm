// NSView für die Plugin-Oberfläche: gespiegelte Koordinaten (y nach unten, wie unter X11), Zeichnen über CoreGraphics,
// Maus und Mausrad an die Oberfläche weiterreichen, eigener Takt (30/s) für Anzeigen und Neuzeichnen.
// Der Klassenname wird je Plugin gesetzt (TS_VIEW_CLASS), damit Laney und Yamaha im selben Host nicht kollidieren.
#import <Cocoa/Cocoa.h>
#include "macview.h"
#ifndef TS_VIEW_CLASS
#define TS_VIEW_CLASS TSPluginView
#endif

@interface TS_VIEW_CLASS : NSView { @public tsmac::Client *client; NSTimer *timer; NSTrackingArea *track; double wheelAcc; }
@end
@implementation TS_VIEW_CLASS
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)e { return YES; }
- (BOOL)isOpaque { return YES; }
- (void)drawRect:(NSRect)r {
  if (!client) return; CGContextRef cg = [[NSGraphicsContext currentContext] CGContext]; if (!cg) return;
  CGContextSaveGState(cg); client->drawCG(cg, self.bounds.size.width, self.bounds.size.height); CGContextRestoreGState(cg); }
- (NSPoint)pt:(NSEvent *)e { return [self convertPoint:[e locationInWindow] fromView:nil]; }
- (void)send:(int)kind ev:(NSEvent *)e button:(int)b { if (!client) return; NSPoint p = [self pt:e]; if (client->mouse(kind, p.x, p.y, b, [e timestamp])) [self setNeedsDisplay:YES]; }
- (void)mouseDown:(NSEvent *)e { [self send:0 ev:e button:1]; }
- (void)mouseDragged:(NSEvent *)e { [self send:1 ev:e button:1]; }
- (void)mouseMoved:(NSEvent *)e { [self send:1 ev:e button:0]; }
- (void)mouseUp:(NSEvent *)e { [self send:2 ev:e button:1]; }
- (void)mouseExited:(NSEvent *)e { [self send:3 ev:e button:0]; }
- (void)scrollWheel:(NSEvent *)e {   // Mausrad und Trackpad: in Rasten wie unter X11 (Taste 4 = hoch, 5 = runter)
  double d = [e hasPreciseScrollingDeltas] ? [e scrollingDeltaY] / 12.0 : [e scrollingDeltaY]; wheelAcc += d;
  while (wheelAcc >= 1) { wheelAcc -= 1; [self send:0 ev:e button:4]; } while (wheelAcc <= -1) { wheelAcc += 1; [self send:0 ev:e button:5]; } }
- (void)updateTrackingAreas { [super updateTrackingAreas]; if (track) [self removeTrackingArea:track];
  track = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect) owner:self userInfo:nil];
  [self addTrackingArea:track]; }
- (void)tick:(NSTimer *)t { if (client && client->tick()) [self setNeedsDisplay:YES]; }
- (void)viewDidMoveToWindow { [super viewDidMoveToWindow];
  if ([self window] && !timer) { timer = [NSTimer timerWithTimeInterval:1.0 / 30 target:self selector:@selector(tick:) userInfo:nil repeats:YES]; [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes]; }
  else if (![self window] && timer) { [timer invalidate]; timer = nil; } }
@end

namespace tsmac {
void *create(void *parent, int w, int h, Client *c) {
  TS_VIEW_CLASS *v = [[TS_VIEW_CLASS alloc] initWithFrame:NSMakeRect(0, 0, w, h)]; v->client = c; v->wheelAcc = 0;
  if (parent) [(NSView *)parent addSubview:v];
  return (__bridge_retained void *)v; }
void setSize(void *view, int w, int h) { if (!view) return; TS_VIEW_CLASS *v = (__bridge TS_VIEW_CLASS *)view; [v setFrameSize:NSMakeSize(w, h)]; [v setNeedsDisplay:YES]; }
void show(void *view) { if (view) { TS_VIEW_CLASS *v = (__bridge TS_VIEW_CLASS *)view; [v setHidden:NO]; [v setNeedsDisplay:YES]; } }
void hide(void *view) { if (view) [(__bridge TS_VIEW_CLASS *)view setHidden:YES]; }
void redraw(void *view) { if (view) [(__bridge TS_VIEW_CLASS *)view setNeedsDisplay:YES]; }
void destroy(void *view) { if (!view) return; TS_VIEW_CLASS *v = (__bridge_transfer TS_VIEW_CLASS *)view; v->client = nullptr;
  if (v->timer) { [v->timer invalidate]; v->timer = nil; } [v removeFromSuperview]; }
}
