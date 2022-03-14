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

void readAndCompareRow(fpga_t *fpga, const uint row, const uint bank,
                       const uint8_t pattern, InstructionSequence *&iseq)
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

//! Runs a test to check the DRAM cells against the given retention time.
/*!
  \param \e fpga is a pointer to the RIFFA FPGA device.
  \param \e retention is the retention time in milliseconds to test.
*/
void test(fpga_t *fpga)
{

  uint8_t pattern = 0x66; // the data pattern that we write to the DRAM

  InstructionSequence *iseq = nullptr; // we temporarily store (before sending
                                       // them to the FPGA) the generated
                                       // instructions here

  uint cur_row = 0;
  uint cur_bank = 0;
  printf("Start Read FIFO flushing\n");
  unsigned int rbuf[16];
  for (int i = 0; i < 200;
       i++)
  {
    fpga_recv(fpga, 0, (void *)rbuf, 16, 5);
  }

  // printf("Read FIFo flushed\n");
  turnBus(fpga, BUSDIR::WRITE, iseq);

  writeRow(fpga, cur_row, cur_bank, pattern, iseq);

  // Switch the memory bus to read mode
  turnBus(fpga, BUSDIR::READ, iseq);

  sleep(1);

  // Read the data back and compare

  readAndCompareRow(fpga, cur_row, cur_bank, pattern, iseq);

  printf("\n");

  delete iseq;
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
  fpga_t *fpga;
  fpga_info_list info;
  int fid = 0; // fpga id
  int ch = 0;  // channel id

  // Get the list of FPGA's attached to the system
  if (fpga_list(&info) != 0)
  {
    printf("Error populating fpga_info_list\n");
    return -1;
  }
  printf("Number of devices: %d\n", info.num_fpgas);
  for (int i = 0; i < info.num_fpgas; i++)
  {
    printf("%d: id:%d\n", i, info.id[i]);
    printf("%d: num_chnls:%d\n", i, info.num_chnls[i]);
    printf("%d: name:%s\n", i, info.name[i]);
    printf("%d: vendor id:%04X\n", i, info.vendor_id[i]);
    printf("%d: device id:%04X\n", i, info.device_id[i]);
  }

  // // Open an FPGA device, so we can read/write from/to it
  // fpga = fpga_open(fid);

  // if (!fpga)
  // {
  //   printf("Problem on opening the fpga \n");
  //   return -1;
  // }
  // printf("The FPGA has been opened successfully! \n");

  // // send a reset signal to the FPGA
  // fpga_reset(fpga); // keep this, recovers FPGA from some unwanted state

  // // uint trefi = 7800/200; //7.8us (divide by 200ns as the HW counts with that
  // // period)
  // // uint trfc = 104; //default trfc for 4Gb device
  // // printf("Activating AutoRefresh. tREFI: %d, tRFC: %d \n", trefi, trfc);
  // // setRefreshConfig(fpga, trefi, trfc);

  // test(fpga);

  // fpga_close(fpga);

  //64 bit pattern

  uint8_t pattern = 0xff;

  uint64_t pattern_64 = 0;

  for (int i = 0; i < 7; i++)
  {
    pattern_64 |= (uint64_t)pattern;
    pattern_64 = (pattern_64 << 8);
  }
  pattern_64 |= (uint64_t)pattern;

  // Receive the data
  uint rbuf[16];
  for (int i = 0; i < NUM_COLS;
       i += 8)
  { // we receive a single burst(8x8 bytes = 64 bytes)
    for (int i = 0; i < 16; i++)
    {
      rbuf[i] = 0xffffffff;
    }

    // compare with the pattern
    uint64_t *rbuf8 = (uint64_t *)rbuf;

    for (int j = 0; j < 8; j++)
    {
      if (rbuf8[j] != pattern_64)
        printf("DATA: %lx \n", rbuf8[j]);
    }
  }

  return 0;
}
