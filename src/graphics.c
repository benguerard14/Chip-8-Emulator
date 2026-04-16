#include "graphics.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static char screen_bmp[(SCREEN_WIDTH * SCREEN_HEIGHT) / 8];

static SDL_Window *window;
static SDL_Surface *new_surface;

int graphics_init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Video Initialization Error");
    return -1;
  }
  window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * PIXEL_SIZE,
                            SCREEN_HEIGHT * PIXEL_SIZE, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window Creation Error");
    return -1;
  }
  return 0;
}

int graphics_update() {
  SDL_Surface *window_surface;
  window_surface = SDL_GetWindowSurface(window);

  new_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH * PIXEL_SIZE,
                                     SCREEN_HEIGHT * PIXEL_SIZE, 32, 0x00FF0000,
                                     0x0000FF00, 0x000000FF, 0xFF000000);

  SDL_FillRect(new_surface, NULL, 0x00000000);
  SDL_FillRect(window_surface, NULL, 0x00000000);
  uint8_t *pixel_ptr = new_surface->pixels;

  int pitch = new_surface->pitch;
  for (int i = 0; i < (SCREEN_HEIGHT * SCREEN_WIDTH); i++) {
    int chip_x = i % SCREEN_WIDTH;
    int chip_y = i / SCREEN_WIDTH;

    int byte_index = (chip_x / 8) + chip_y * (SCREEN_WIDTH / 8);
    int bit_index = chip_x % 8;

    uint8_t pixel_on = (screen_bmp[byte_index] >> (7 - bit_index)) & 1;

    uint32_t target = pixel_on ? SCREEN_COLOR : 0x00000000;
    uint32_t *sample = (uint32_t *)(pixel_ptr + (chip_y * PIXEL_SIZE) * pitch +
                                    (chip_x * PIXEL_SIZE) * 4);
    if (*sample == target)
      continue;

    for (int y = 0; y < PIXEL_SIZE; y++) {
      for (int x = 0; x < PIXEL_SIZE; x++) {

        int screen_x = chip_x * PIXEL_SIZE + x;
        int screen_y = chip_y * PIXEL_SIZE + y;

        uint32_t *pixel =
            (uint32_t *)(pixel_ptr + screen_y * pitch + screen_x * 4);

        *pixel = pixel_on ? SCREEN_COLOR : 0x00000000;
      }
    }
  }
  SDL_BlitSurface(new_surface, NULL, window_surface, NULL);
  SDL_UpdateWindowSurface(window);
  return 0;
}

int graphics_exit() {
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void graphics_sleep(int ms) { SDL_Delay(ms); }

void set_screen(char *screen) { memcpy(&screen_bmp, screen, 256); }
