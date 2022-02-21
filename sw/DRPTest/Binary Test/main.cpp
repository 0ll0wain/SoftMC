#include <stdio.h>
#include <cassert>

void showbits(unsigned int x) {
  int i = 0;
  for (i = (sizeof(int) * 8) - 1; i >= 0; i--) {
    putchar(x & (1u << i) ? '1' : '0');
  }
  printf("\n");
}

int main(int argc, char *argv[]) {

  unsigned int val = 2;

  assert(val <
         0x10000000); // Checks if val fits into registers, else abort program

  // unsigned int instr = 0x58000000;
  unsigned int clkfbout_mult = 3;
  unsigned int divclk_divide = 1;
  unsigned int clkout_divide = 2;

  unsigned int instr = 0x50;
  instr <<= 8;
  instr |= clkout_divide;
  instr <<= 8;
  instr |= divclk_divide;
  instr <<= 8;
  instr |= clkfbout_mult;

  // instr <<= 28; //left shift 28 bits

  // instr |= val;

  printf("instr = %d\n", instr);

  showbits(instr);

  return 0;
}