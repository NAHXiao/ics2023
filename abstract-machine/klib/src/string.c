#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) { panic("Not implemented"); }

//
char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++))
    ;
  return ret;
  // panic("Not implemented");
}

char *strncpy(char *dst, const char *src, size_t n) {
  panic("Not implemented");
}

//
char *strcat(char *dst, const char *src) {
  char *ret = dst;
  while (*dst)
    dst++;
  while ((*dst++ = *src++))
    ;
  return ret;
  // panic("Not implemented");
}
//>0: l>r
int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
  // panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

//
void *memset(void *s, int c, size_t n) {
  char *dst = (char *)s;
  for (size_t i = 0; i < n; i++)
    dst[i] = c & 0xff;
  return s;
  // panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

//
void *memcpy(void *out, const void *in, size_t n) {
  char *_out = (char *)out;
  char *_in = (char *)in;
  while (n--)
    *_out++ = *_in++;
  return out;
  // panic("Not implemented");
}

//
int memcmp(const void *s1, const void *s2, size_t n) {
  char *out = (char *)s1;
  char *in = (char *)s2;
  for (size_t i = 0; i < n; i++) {
    if (out[i] != in[i]) {
      return out[i] - in[i];
    }
  }
  return 0;
  // panic("Not implemented");
}
#endif
