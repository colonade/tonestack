// Plattform-Hilfen für die Plugins: Denormal-Schutz (x86: MXCSR FTZ/DAZ, ARM64: FPCR FZ) und Schalter für die Oberfläche.
// Oberfläche: Linux X11 + Cairo, macOS NSView + CoreGraphics (cairo_cg.hpp, macview.mm); TS_NO_GUI schaltet sie ab.
#pragma once
#if defined(__SSE__) || defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>
struct DenormGuard { unsigned csr; DenormGuard() : csr(_mm_getcsr()) { _mm_setcsr(csr | 0x8040); } ~DenormGuard() { _mm_setcsr(csr); } };
#elif defined(__aarch64__)
#include <cstdint>
struct DenormGuard { uint64_t fpcr; DenormGuard() { __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr)); uint64_t n = fpcr | (1ull << 24); __asm__ volatile("msr fpcr, %0" : : "r"(n)); }
  ~DenormGuard() { __asm__ volatile("msr fpcr, %0" : : "r"(fpcr)); } };
#else
struct DenormGuard {};
#endif
#if defined(TS_NO_GUI) || defined(__APPLE__)
typedef struct _XDisplay Display;   // nur als Zeigertyp in der Plugin-Struktur
#endif
