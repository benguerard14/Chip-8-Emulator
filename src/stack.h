#ifndef STACK_H
#define STACK_H

#define STACK_SIZE 64

typedef struct {
  int stack_ptr;
  short stack_mem[STACK_SIZE];
} __attribute__((packed)) stack;

void stack_init(stack *stk);

void push(stack *stk, short element);

short pop(stack *stk);

#endif
