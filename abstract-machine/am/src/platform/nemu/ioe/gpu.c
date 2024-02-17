#include <am.h>
#include <klib.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
static int W, H;
void __am_gpu_init() {
  uint32_t data = inl(VGACTL_ADDR);
  W = data >> 16, H = data & 0xffff;
}

// nemu假设W H不变...不代表am不变
// uint32_t data = inl(VGACTL_ADDR);
// uint32_t w = data >> 16, h = data & 0xffff;
void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t data = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T){.present = true,
                           .has_accel = false,
                           .width = data >> 16,
                           .height = data & 0xffff,
                           .vmemsz = 0};
}
/*
static uint32_t count = 0;
请务必关闭Trace,否则几十秒一帧....()
count++;
if (ctl->w == 0 || ctl->h == 0) return;
reflash信号有的会只reflash不给w,h所以不能直接返回
例如am-kernels/kernels/demo/include/io.h
..RTFSC(native
gpu.c有提示)(或者熟悉SDL
TEXTURE pixels也行)
*/
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  for (int _w = 0; _w < ctl->w; _w++) {
    for (int _h = 0; _h < ctl->h; _h++) {
      outl(FB_ADDR +4* ((ctl->y + _h) * W + ctl->x + _w),
           *(uint32_t*)(ctl->pixels+4*(_h * (ctl->w) + _w)));
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}
void __am_gpu_status(AM_GPU_STATUS_T *status) { status->ready = true; }
