#include <am.h>
#include <SDL2/SDL.h>

//#define MODE_800x600
#ifdef MODE_800x600
# define W    800
# define H    600
#else
# define W    400
# define H    300
#endif

#define FPS   60

#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#define AMASK 0x00000000
//1
/*
static SDL_Window *window = NULL;
static SDL_Surface *surface = NULL;

static Uint32 texture_sync(Uint32 interval, void *param) {
  SDL_BlitScaled(surface, NULL, SDL_GetWindowSurface(window), NULL);
  SDL_UpdateWindowSurface(window);
  return interval;
}

void __am_gpu_init() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  window = SDL_CreateWindow("Native Application",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
#ifdef MODE_800x600
      W, H,
#else
      W * 2, H * 2,
#endif
      SDL_WINDOW_OPENGL);
  surface = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, 32,
      RMASK, GMASK, BMASK, AMASK);
  SDL_AddTimer(1000 / FPS, texture_sync, NULL);
}



void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (w == 0 || h == 0) return;
  feclearexcept(-1);
  SDL_Surface *s = SDL_CreateRGBSurfaceFrom(ctl->pixels, w, h, 32, w * sizeof(uint32_t),
      RMASK, GMASK, BMASK, AMASK);
  SDL_Rect rect = { .x = x, .y = y };
  SDL_BlitSurface(s, NULL, surface, &rect);
  SDL_FreeSurface(s);
}
*/
void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = W, .height = H,
    .vmemsz = 0
  };
}
void __am_gpu_status(AM_GPU_STATUS_T *stat) {
  stat->ready = true;
}
//2
//
static SDL_Window *window = NULL;
static uint32_t *vmem = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static void sync();
void __am_gpu_init() {
  vmem = (uint32_t *)malloc(W * H * sizeof(uint32_t));
  SDL_CreateWindowAndRenderer(W, H, 0, &window, &renderer);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STATIC, W, H);

  SDL_RenderPresent(renderer);
}
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *pixel = (uint32_t *)(ctl->pixels);
  for (int _w = 0; _w < ctl->w; _w++) {
    for (int _h = 0; _h < ctl->h; _h++) {
      vmem[((ctl->y + _h) * W + ctl->x + _w)] = pixel[_h * (ctl->w) + _h];
    }
  }
  if (ctl->sync) {
  sync();
  }
}
static void sync() {
  SDL_UpdateTexture(texture, NULL, vmem, W * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}
