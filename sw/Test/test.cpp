#include <stdio.h>
#include <riffa.h>
#include <cassert>
#include <string.h>
#include <iostream>
#include <cmath>
#include "softmc.h"
#include <unistd.h>

using namespace std;

// Note that capacity of the instruction buffer is 8192 instructions
void writeRow(fpga_t *fpga, uint row, uint bank, uint8_t pattern,
              InstructionSequence *&iseq)
{

  if (iseq == nullptr)
    iseq = new InstructionSequence();
  else
    iseq->size = 0; // reuse the provided InstructionSequence to avoid dynamic
                    // allocation on each call

  // Precharge target bank (just in case if its left activated)
  iseq->insert(genPRE(bank, PRE_TYPE::SINGLE));

  // Wait for tRP
  iseq->insert(genWAIT(5)); // 2.5ns have already been passed as we issue in
                            // next cycle. So, 5 means 6 cycles latency, 15 ns

  // Activate target row
  iseq->insert(genACT(bank, row));

  // Wait for tRCD
  iseq->insert(genWAIT(5));

  // Write to the entire row
  for (int i = 0; i < NUM_COLS; i += 8)
  { // we use 8x burst mode
    iseq->insert(genWR(bank, i, pattern));

    // We need to wait for tCL and 4 cycles burst (double data-rate)
    iseq->insert(genWAIT(6 + 4));
  }

  // Wait some more in any case
  iseq->insert(genWAIT(3));

  // Precharge target bank
  iseq->insert(genPRE(bank, PRE_TYPE::SINGLE));

  // Wait for tRP
  iseq->insert(genWAIT(5)); // we have already 2.5ns passed as we issue in next
                            // cycle. So, 5 means 6 cycles latency, 15 ns

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);
}

void readRow(fpga_t *fpga, const uint row, const uint bank,
             InstructionSequence *&iseq)
{

  if (iseq == nullptr)
    iseq = new InstructionSequence();
  else
    iseq->size = 0; // reuse the provided InstructionSequence to avoid dynamic
                    // allocation for each call

  // Precharge target bank (just in case if its left activated)
  iseq->insert(genPRE(bank, PRE_TYPE::SINGLE));

  // Wait for tRP
  iseq->insert(genWAIT(5)); // 2.5ns have already been passed as we issue in
                            // next cycle. So, 5 means 6 cycles latency, 15 ns

  // Activate target row
  iseq->insert(genACT(bank, row));

  // Wait for tRCD
  iseq->insert(genWAIT(5));

  // Read the entire row
  for (int i = 0; i < NUM_COLS; i += 8)
  { // we use 8x burst mode
    iseq->insert(genRD(bank, i));

    // We need to wait for tCL and 4 cycles burst (double data-rate)
    iseq->insert(genWAIT(6 + 4));
  }

  // Wait some more in any case
  iseq->insert(genWAIT(3));

  // Precharge target bank
  iseq->insert(genPRE(
      bank, PRE_TYPE::SINGLE)); // pre 1 -> precharge all, pre 0 precharge bank

  // Wait for tRP
  iseq->insert(genWAIT(5)); // we have already 2.5ns passed as we issue in next
                            // cycle. So, 5 means 6 cycles latency, 15 ns

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);

  // Receive the data
  unsigned int rbuf[16];
  for (int i = 0; i < NUM_COLS;
       i += 8)
  {
    fpga_recv(fpga, 0, (void *)rbuf, 16, 0);
    for (size_t j = 0; j < 16; j++)
    {
      printf("%x\n", rbuf[j]);
    }
  }
  // for (int i = 0; i < NUM_COLS;
  //      i += 8)
  // { // we receive a single burst(8x8 bytes = 64 bytes)

  //   uint rbuf[16];
  //   fpga_recv(fpga, 0, (void *)rbuf, 16, 0);
  //   // compare with the pattern
  //   uint8_t *rbuf8 = (uint8_t *)rbuf;

  //   for (int j = 0; j < 64; j++)
  //   {
  //     printf("%x\n", rbuf8[j]);
  //   }
  // }
}

void turnBus(fpga_t *fpga, BUSDIR b, InstructionSequence *iseq = nullptr)
{

  if (iseq == nullptr)
    iseq = new InstructionSequence();
  else
    iseq->size = 0; // reuse the provided InstructionSequence to avoid dynamic
                    // allocation for each call

  iseq->insert(genBUSDIR(b));

  // WAIT
  iseq->insert(genWAIT(5));

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);
}

// provide trefi = 0 to disable auto-refresh
// auto-refresh is disabled by default (disabled after FPGA boots, disables on
// pushing reset button)
void setRefreshConfig(fpga_t *fpga, uint trefi, uint trfc)
{
  InstructionSequence *iseq = new InstructionSequence;

  iseq->insert(genREF_CONFIG(trfc, REGISTER::TRFC));
  iseq->insert(genREF_CONFIG(trefi, REGISTER::TREFI));

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);

  delete iseq;
}

int main(int argc, char *argv[])
{
  uint8_t pattern = 0xff;
  // 64 bit pattern
  uint64_t pattern_64 = 0;
  for (int i = 0; i < 7; i++)
  {
    pattern_64 |= (uint64_t)pattern;
    pattern_64 = (pattern_64 << 8);
  }
  pattern_64 |= (uint64_t)pattern;

  uint64_t match = 0;
  uint64_t errors = 0;

  uint64_t rbuf64[8] = {0x0fffffffffffffff, 0x6fffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff};
  uint64_t data[8] = {0xffffffffffffffff, 0x6fffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff};
  for (int j = 0; j < 8; j++)
  {
    if (rbuf64[j] != pattern_64)
    {
      uint column = j;
      uint64_t testMask = 0x00000001;
      for (uint bit = 0; bit < 64; bit++)
      {
        uint64_t testResult = rbuf64[j] & testMask;
        uint64_t testPattern = pattern_64 & testMask;
        uint64_t testData = data[column] & testMask;

        if (testResult != testPattern)
        {
          errors++;
          if (testData != testPattern)
          {
            match++;
          }
        }
        testMask = testMask << 1;
      }
    }
    else
      printf("No Error\n");
  }
  printf("Matches: %ld, Errors: %ld\n", match, errors);
  printf("Result: %f\n", (float)match / (float)errors * 100);

  return 0;
}
