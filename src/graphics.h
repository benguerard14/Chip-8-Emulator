#ifndef GRAPHICS_H
#define GRAPHICS_H

#define PIXEL_SIZE 15
#define SCREEN_COLOR 0xFFFFFFFF
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

int graphics_init(void);

int graphics_update(void);

int graphics_exit(void);

void graphics_sleep(int ms);

void set_screen(char *screen);
#endif
