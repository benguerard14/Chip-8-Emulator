#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

void stack_init(stack *stk) { stk->stack_ptr = (STACK_SIZE - 1); }

void push(stack *stk, short element) {
  if (stk->stack_ptr < -1 * (STACK_SIZE - 1)) {
    printf("ERROR: Stack Overflow\n");
    exit(-1);
  }
  stk->stack_mem[stk->stack_ptr] = element;
  stk->stack_ptr--;
}

short pop(stack *stk) {
  if (stk->stack_ptr >= 0) {
    printf("ERROR: Nothing to pop\n");
    exit(-1);
  }
  stk->stack_ptr++;
  short element = stk->stack_mem[stk->stack_ptr];
  return element;
}
