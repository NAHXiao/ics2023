#include <am.h>
#include <klib.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t kc = inl(KBD_ADDR);
  if ((kc | KEYDOWN_MASK) == kc) {
    kbd->keydown = 1;
  } else {
    kbd->keydown = 0;
  }
  kbd->keycode = kc & ~KEYDOWN_MASK;//md按位与给忘了...
  // if(kc){
  // printf("kc   : 0x%x\n", kc);
  // printf("kcode: 0x%x\n", kbd->keycode);
  // }
}
// kc: 0x802b
// kc: 0x2b
