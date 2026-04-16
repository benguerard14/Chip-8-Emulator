#include "emulator.h"
#include "graphics.h"
#include <unistd.h>

void loop() {
  int i = 0;
  while (i < 100) {
    instruction *instruction_ptr = (instruction *)emulator_fetch();

    emulator_decode(*instruction_ptr);
    i++;
    graphics_sleep(100);
  }
}

int main() {
  graphics_init();
  char *rom = get_string_file("roms/3-corax+.ch8");
  emulator_init(rom);
  loop();
  graphics_sleep(4000);
  graphics_exit();
}
