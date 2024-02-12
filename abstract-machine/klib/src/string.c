#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t size = 0;
  while (*s++) {
    size++;
  }
  return size;
  // panic("Not implemented");
}

//
char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++))
    ;
  return ret;
  // panic("Not implemented");
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *ret = dst;
  for (; *src && n; n--) {
    *dst++ = *src++;
  }
  while (n--) {
    *dst++ = 0;
  }
  return ret;
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
  while (n && *s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
  // panic("Not implemented");
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
  if (dst == src||(!dst&&!src))//同时为空才返回NULL,否则让系统报错去(经测试行为)
    return dst;
  void *ret = dst;

  if ((uint8_t *)dst > (uint8_t *)src           //如果dst<src直接从低位开始忘高位复制即可，不会覆盖src
      && (uint8_t *)dst < (uint8_t *)src + n) { //
    uint8_t *dst = (uint8_t *)dst + n - 1;
    uint8_t *src = (uint8_t *)src + n - 1;
    while (n--)
      *dst-- = *src--;
  } else {
    uint8_t *dst = (uint8_t *)dst;
    uint8_t *src = (uint8_t *)src;
    while (n--)
      *dst++ = *src++;
  }
  return ret;
}

//
void *memcpy(void *out, const void *in, size_t n) {
  uint8_t *_out = (uint8_t *)out;
  uint8_t *_in = (uint8_t *)in;
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
