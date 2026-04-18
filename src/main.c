#include "emulator.h"
#include "graphics.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <unistd.h>

void loop() {
  SDL_Event event;
  while (1) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return;
      }
    }
    instruction *instruction_ptr = (instruction *)emulator_fetch();

    emulator_decode(*instruction_ptr);

    graphics_sleep(1);
  }
}

int main() {
  graphics_init();
  char *rom = get_string_file("roms/br8kout.ch8");
  emulator_init(rom);
  loop();
  graphics_exit();
}
