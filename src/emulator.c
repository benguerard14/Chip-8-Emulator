#include "emulator.h"
#include "graphics.h"
#include "stack.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CHIP_8 emulator;
static short current_instruction;

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
  printf("Read %zu bytes\n", n);
  printf("First 16 bytes: ");
  for (int i = 0; i < 16; i++)
    printf("%02X ", (unsigned char)buff[i]);
  printf("\n");
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

void set_pc(instruction instruction_t) {
  emulator.PC =
      (instruction_t.X << 8) | (instruction_t.Y << 4) | instruction_t.N;
}

void emulator_init(char *program) {
  memcpy(&emulator.RAM[4096 - PROGRAM_SIZE], program, PROGRAM_SIZE);

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
  printf("%d\n", *((short *)&instruction_t));
  unsigned char *reg_ptr;
  unsigned char *reg2_ptr;
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
    set_pc(instruction_t);
    break;
  // 0x2NNN -> PC = NNN; + push to stack
  case 0x2:
    set_pc(instruction_t);
    push(&emulator.stk, emulator.PC);
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
  // DXYN -> NO idea
  case 0xD:
    display_instruction(instruction_t);
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
