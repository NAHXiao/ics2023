#include <am.h>
#include <stdio.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// int printf(const char *fmt, ...) {
//     // snprintf(); // sprintf
// }
//
// int vsprintf(char *out, const char *fmt, va_list ap) {
// }
//
// int sprintf(char *out, const char *fmt, ...) {
//     vsprintf();
// }
//
// int snprintf(char *out, size_t n, const char *fmt, ...) {
//     vsnprintf();
// }
//
// int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
//     vsprintf();
// }

#endif
