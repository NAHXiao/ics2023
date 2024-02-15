#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>
#include <stdio.h>
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
#include <stdint.h>
#include <string.h>
#include <unistd.h>
static int vformat(const char *fmt, char *buf, uint32_t size, va_list args) {
  char *bufbak = buf;
  while (*fmt && size > 0) {
        switch (*fmt) {
    case '%':
      fmt++;
      switch (*fmt++) {
      case 'u': {
        unsigned int u = va_arg(args, unsigned int);
        unsigned int tmp = 1;
        while (u / tmp >= 10) {
          tmp *= 10;
        }
                while (tmp) {
          *buf++ = '0' + u / tmp;
          size--;
          u %= tmp;
          tmp /= 10;
        }
        break;
      }
      case 'd': {
        int d = va_arg(args, int);
        if (d < 0) {
          *buf++ = '-';
          size--;
          d = -d;
        }
        unsigned int tmp = 1;
        while (d / tmp >= 10) {
          tmp *= 10;
        }
                while (tmp) {
          *buf++ = '0' + d / tmp;
          size--;
          d %= tmp;
          tmp /= 10;
        }
        break;
      }
      case 's': {
        char *s = va_arg(args, char *);
        int len = strlen(s);
        strncpy(buf, s, len);
        buf += len;
        size -= len;
        break;
      }
      case 'x': {
        int x = va_arg(args, int);
        unsigned int tmp = 1;
        while (x / tmp >= 16) {
          tmp *= 16;
        }
                while (tmp) {
          *buf++ = x / tmp >= 10 ? 'a' + x / tmp - 10 : '0' + x / tmp;
          size--;
          x %= tmp;
          tmp /= 16;
        }
        break;
      }
      case 'c': {
        char c = va_arg(args, int);
        *buf++ = c;
        size--;
        break;
      }
      case '%': {
        *buf++ = '%';
        size--;
        break;
      }
      }
      break;
    default:
      *buf++ = *fmt++;
      size--;
      break;
    }
  }
  buf[0] = '\0';
  return buf - bufbak;
}

int printf(const char *fmt, ...) {
    char buf[1024];
  va_list args;
  va_start(args, fmt);
  int ret =vformat(fmt, buf, 1024, args);
  va_end(args);
  puts(buf);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vformat(fmt, out, -1, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    return vformat(fmt, out, n, ap);
}
#endif
