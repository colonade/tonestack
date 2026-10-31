// Plattform-Hilfen für die Plugins: Denormal-Schutz (x86: MXCSR FTZ/DAZ, ARM64: FPCR FZ) und Schalter für die Oberfläche.
// Auf macOS gibt es (noch) keine eigene Oberfläche: TS_NO_GUI, der Host zeigt seine Reglerliste.
#pragma once
#if defined(__APPLE__) && !defined(TS_NO_GUI)
#define TS_NO_GUI 1
#endif
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
#ifdef TS_NO_GUI
typedef struct _XDisplay Display;   // nur als Zeigertyp in der Plugin-Struktur
#endif
