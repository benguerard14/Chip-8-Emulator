#include "emulator.h"
#include "graphics.h"
#include "stack.h"
#include "timer.h"
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static CHIP_8 emulator;
static short current_instruction;

char shift_old = 0;

int keymap[16] = {
    SDL_SCANCODE_X, // 0
    SDL_SCANCODE_1, // 1
    SDL_SCANCODE_2, // 2
    SDL_SCANCODE_3, // 3
    SDL_SCANCODE_Q, // 4
    SDL_SCANCODE_W, // 5
    SDL_SCANCODE_E, // 6
    SDL_SCANCODE_A, // 7
    SDL_SCANCODE_S, // 8
    SDL_SCANCODE_D, // 9
    SDL_SCANCODE_Z, // A
    SDL_SCANCODE_C, // B
    SDL_SCANCODE_4, // C
    SDL_SCANCODE_R, // D
    SDL_SCANCODE_F, // E
    SDL_SCANCODE_V  // F
};
unsigned char font[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
                          0x20, 0x60, 0x20, 0x20, 0x70,  // 1
                          0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
                          0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
                          0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
                          0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
                          0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
                          0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
                          0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
                          0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
                          0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
                          0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
                          0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
                          0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
                          0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
                          0xF0, 0x80, 0xF0, 0x80, 0x80}; // F

char *get_string_file(char *file) {
  static char buff[PROGRAM_SIZE];
  FILE *f = fopen(file, "rb"); // "rb" not "r"
  if (!f) {
    printf("Error opening ROM\n");
    exit(-1);
  }
  size_t n = fread(buff, 1, PROGRAM_SIZE, f);
  fclose(f);
  if (n == 0) {
    printf("Nothing in file!\n");
    exit(-1);
  }
  return buff;
}

void set_pixel(int x, int y) {
  int i = ((x / 8)) + y * 8;
  emulator.display[i] |= 1 << (x % 8);
}
void clear_pixel(int x, int y) {
  int i = ((x / 8)) + y * 8;
  emulator.display[i] &= ~(1 << (x % 8));
}

// DXYN
// X->ptr to register with sprite X position
// Y->ptr to register with sprite Y position
// N->Number of rows
void display_instruction(instruction ins) {
  char x_pos = (*(&emulator.V0 + ins.X) % 64);
  char y_pos = (*(&emulator.V0 + ins.Y) % 32);

  emulator.VF = 0;

  for (int y = 0; y < ins.N; y++) {
    char sprite = emulator.RAM[emulator.I + y];

    for (int x = 0; x < 8; x++) {
      char pixel = (sprite >> (7 - x)) & 1;

      int total_x = x_pos + x;
      int total_y = y_pos + y;

      if (total_x >= 64 || total_y >= 32)
        continue;

      int byte_index = total_y * 8 + (total_x / 8);
      char mask = 0x80 >> (total_x % 8);

      unsigned char *screen_byte = &emulator.display[byte_index];

      // collision check (1 → 0)
      if ((*screen_byte & mask) && pixel)
        emulator.VF = 1;

      // XOR draw
      if (pixel)
        *screen_byte ^= mask;
    }
  }
  display();
}

void arithmetic_instruction(unsigned char flag, unsigned char *X,
                            unsigned char *Y) {
  switch (flag) {
  // 0 -> X = Y
  case (0x0):
    *X = *Y;
    break;
  // 1 -> X \= Y
  case (0x1):
    *X |= *Y;
    break;
  // 1 -> X &= Y
  case (0x2):
    *X &= *Y;
    break;
  // 1 -> X ^= Y
  case (0x3):
    *X ^= *Y;
    break;
  // 1 -> X += Y
  case (0x4):
    *X += *Y;
    break;
  // 1 -> X -= Y
  case (0x5):
    *X -= *Y;
    emulator.VF = (*X > 0) ? 1 : 0;
    break;
  // Shift to the right. If shift_old, VX = VY
  case (0x6):
    if (shift_old) {
      *X = *Y;
    }
    emulator.VF = (*X & 0x1) ? 1 : 0;
    *X = *X >> 1;
    break;
  // 1 -> X = Y -X
  case (0x7):
    *X = (*Y - *X);
    emulator.VF = (*X > 0) ? 1 : 0;
    break;
  // Shift to the right. If shift_old, VX = VY
  case (0xE):
    if (shift_old) {
      *X = *Y;
    }
    emulator.VF = (*X & 0b10000000) ? 1 : 0;
    *X = *X << 1;
    break;
  default:
    printf("ERROR: Unknown Arithmetical Instruction %d\n", flag);
    exit(-1);
  }
}

void check_input(instruction instruction_t) {
  unsigned char *reg_ptr;
  reg_ptr = &emulator.V0 + instruction_t.X;
  unsigned char key = *reg_ptr;
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  switch (instruction_t.N) {
  case 0x1:
    if (!state[keymap[key]]) {
      emulator.PC += 2;
    }
    break;
  case 0xE:
    if (state[keymap[key]]) {
      emulator.PC += 2;
    }
    break;
  default:
    printf("ERROR: Invalid N instruction, %d\n", instruction_t.N);
    exit(-1);
  }
}

void F_instruction(instruction instruction_t) {
  unsigned char *reg_ptr;
  reg_ptr = &emulator.V0 + instruction_t.X;
  short flag = (instruction_t.Y << 4) | instruction_t.N;
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  char n;
  switch (flag) {
  case 0x07:
    *reg_ptr = emulator.delay_timer;
    break;
  case 0x15:
    emulator.delay_timer = *reg_ptr;
    break;
  case 0x18:
    emulator.sound_timer = *reg_ptr;
    break;
  case 0x1E:
    emulator.I += *reg_ptr;
    break;
  case 0x0A:
    for (int i = 0; i < 16; i++) {
      if (state[keymap[i]]) {
        *reg_ptr = i; // store key in V[x]
        return;       // done, move on
      }
    }
    emulator.PC -= 2;
    break;
  case 0x29:
    emulator.I = FONT_ADDRESS + (*reg_ptr * 5);
    break;
  case 0x33:
    n = *reg_ptr;
    for (int i = 2; i >= 0; i--) {
      emulator.RAM[emulator.I + i] = n % 10;
      n /= 10;
    }
    break;
  case 0x55:
    for (int i = 0; i <= instruction_t.X; i++) {
      reg_ptr = &emulator.V0 + i;
      emulator.RAM[(emulator.I + i)] = *reg_ptr;
    }
    break;
  case 0x65:
    for (int i = 0; i <= instruction_t.X; i++) {
      reg_ptr = &emulator.V0 + i;
      *reg_ptr = emulator.RAM[(emulator.I + i)];
    }
    break;
  default:
    printf("ERROR: Invalid F instruction, %02x\n", flag);
    exit(-1);
  }
}

void set_pc(instruction instruction_t, unsigned char extra) {
  emulator.PC =
      ((instruction_t.X << 8) | (instruction_t.Y << 4) | instruction_t.N) +
      extra;
}

void emulator_init(char *program) {
  memcpy(&emulator.RAM[4096 - PROGRAM_SIZE], program, PROGRAM_SIZE);
  memcpy(&emulator.RAM[FONT_ADDRESS], font, 80);
  timer_init();
  srand(time(NULL));
  emulator.PC = (4096 - PROGRAM_SIZE);
}

short *emulator_fetch() {
  current_instruction = 0;
  current_instruction = (emulator.RAM[emulator.PC] << 8); // high byte
  current_instruction |= (unsigned char)emulator.RAM[emulator.PC + 1];
  emulator.PC += 2;
  return &current_instruction;
}

void emulator_decode(instruction instruction_t) {
  printf("%04X\n", *((unsigned short *)&instruction_t));
  unsigned char *reg_ptr;
  unsigned char *reg2_ptr;
  if (tick_passed()) {
    emulator.delay_timer =
        (emulator.delay_timer > 0) ? (emulator.delay_timer - 1) : 0;
    emulator.sound_timer =
        (emulator.sound_timer > 0) ? (emulator.sound_timer - 1) : 0;
  }
  switch (instruction_t.F) {
  // 00E0 -> CLEAR SCREEN
  case 0x0:
    if (instruction_t.N == 0xE) {
      emulator.PC = pop(&emulator.stk);
      break;
    }
    memset(emulator.display, 0, sizeof(emulator.display));
    display();
    break;
  // 0x1NNN -> PC = NNNN;
  case 0x1:
    set_pc(instruction_t, 0);
    break;
  // 0x2NNN -> PC = NNN; + push to stack
  case 0x2:
    push(&emulator.stk, emulator.PC);
    set_pc(instruction_t, 0);
    break;
  // 0x3XNN -> skip 1 instruction if *X = NN
  case 0x3:
    reg_ptr = &emulator.V0 + instruction_t.X;
    if (*reg_ptr == ((instruction_t.Y << 4) | instruction_t.N)) {
      emulator.PC += 2;
    }
    break;
  // 0x3XNN -> skip 1 instruction if *X != NN
  case 0x4:
    reg_ptr = &emulator.V0 + instruction_t.X;
    if (*reg_ptr != ((instruction_t.Y << 4) | instruction_t.N)) {
      emulator.PC += 2;
    }
    break;
  // 5XY0 -> SKIP 1 if *X = *Y
  case 0x5:
    reg_ptr = &emulator.V0 + instruction_t.X;
    reg2_ptr = &emulator.V0 + instruction_t.Y;

    if (*reg_ptr == *reg2_ptr) {
      emulator.PC += 2;
    }
    break;
  // 6XNN -> register VX, set to NN
  case 0x6:
    reg_ptr = &emulator.V0 + instruction_t.X;
    *reg_ptr = (instruction_t.Y << 4) | instruction_t.N;
    break;
  // 7XNN -> register VX += NN
  case 0x7:
    reg_ptr = &emulator.V0 + instruction_t.X;
    *reg_ptr += (instruction_t.Y << 4) | instruction_t.N;
    break;
  // 9XY0 -> SKIP 1 if *X != *Y
  case 0x8:
    reg_ptr = &emulator.V0 + instruction_t.X;
    reg2_ptr = &emulator.V0 + instruction_t.Y;
    arithmetic_instruction(instruction_t.N, reg_ptr, reg2_ptr);
    break;
  case 0x9:
    reg_ptr = &emulator.V0 + instruction_t.X;
    reg2_ptr = &emulator.V0 + instruction_t.Y;

    if (*reg_ptr != *reg2_ptr) {
      emulator.PC += 2;
    }
    break;
  // ANNN -> set I to NNN
  case 0xA:
    emulator.I =
        (instruction_t.X << 8) | (instruction_t.Y << 4) | instruction_t.N;
    break;
  // BNNN -> JUMP to V0 + NNN
  case 0xB:
    set_pc(instruction_t, emulator.V0);
    break;
  // CXNN -> Returns NN & Random in VX;
  case 0xC:
    reg_ptr = &emulator.V0 + instruction_t.X;
    *reg_ptr = ((instruction_t.Y << 4 | instruction_t.N) & (rand() % 256));
    break;
  // DXYN -> NO idea
  case 0xD:
    display_instruction(instruction_t);
    break;
  // E -> handle input
  case 0xE:
    check_input(instruction_t);
    break;
  case 0xF:
    F_instruction(instruction_t);
    break;
  default:
    printf("Unknown Instruction: %d\n", instruction_t.F);
    exit(-1);
  }
}

void display() {
  set_screen((char *)emulator.display);
  graphics_update();
}
