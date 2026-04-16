#ifndef EMULATOR_H
#define EMULATOR_H

#include "stack.h"

#define PROGRAM_SIZE 3584

typedef struct {
  unsigned char RAM[4096];
  unsigned char display[256];
  short PC;
  short I;
  stack stk;
  unsigned char delay_timer;
  unsigned char sound_timer;
  unsigned char V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF;
} __attribute__((packed)) CHIP_8;

typedef struct {
  unsigned char N : 4;
  unsigned char Y : 4;
  unsigned char X : 4;
  unsigned char F : 4;
} __attribute__((packed)) instruction;

void emulator_init(char *program);

short *emulator_fetch();

void display();

void emulator_decode(instruction instruction_t);

char *get_string_file(char *file);

#endif
