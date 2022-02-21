#include <stdio.h>
#include <riffa.h>
#include <cassert>
#include <string.h>
#include <iostream>
#include <cmath>
#include "softmc.h"
<<<<<<< HEAD
#include<unistd.h>

using namespace std;

#define s  1000000
#define ms  1000
#define us  1

// Note that capacity of the instruction buffer is 8192 instructions
void writeRow(fpga_t *fpga, uint row, uint bank, uint8_t pattern,
              InstructionSequence *&iseq)
{
=======

using namespace std;

// Note that capacity of the instruction buffer is 8192 instructions
void writeRow(fpga_t *fpga, uint row, uint bank, uint8_t pattern,
              InstructionSequence *&iseq) {
>>>>>>> Software

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
<<<<<<< HEAD
  for (int i = 0; i < NUM_COLS; i += 8)
  { // we use 8x burst mode
=======
  for (int i = 0; i < NUM_COLS; i += 8) { // we use 8x burst mode
>>>>>>> Software
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
<<<<<<< HEAD
                       const uint8_t pattern, InstructionSequence *&iseq)
{
=======
                       const uint8_t pattern, InstructionSequence *&iseq) {
>>>>>>> Software

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
<<<<<<< HEAD
  for (int i = 0; i < NUM_COLS; i += 8)
  { // we use 8x burst mode
=======
  for (int i = 0; i < NUM_COLS; i += 8) { // we use 8x burst mode
>>>>>>> Software
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
  uint rbuf[16];
  for (int i = 0; i < NUM_COLS;
<<<<<<< HEAD
       i += 8)
  { // we receive a single burst at two times (32 bytes)
=======
       i += 8) { // we receive a single burst at two times (32 bytes)
>>>>>>> Software
    fpga_recv(fpga, 0, (void *)rbuf, 16, 0);

    // compare with the pattern
    uint8_t *rbuf8 = (uint8_t *)rbuf;

<<<<<<< HEAD
    for (int j = 0; j < 64; j++)
    {
      if (rbuf8[j] != pattern)
        printf("Error at Col: %d, Row: %u, Bank: %u, DATA: %x \n", i + j, row,
               bank, rbuf8[j]);
      else
        printf("Testet successfully at Col: %d, Row: %u, Bank: %u, DATA: %x \n", i +(j/4), row,
               bank, rbuf8[j]);
=======
    for (int j = 0; j < 64; j++) {
      if (rbuf8[j] != pattern)
        printf("Error at Col: %d, Row: %u, Bank: %u, DATA: %x \n", i + j, row,
               bank, rbuf8[j]);
      printf("i: %d, j: %d\n", i, j);
>>>>>>> Software
    }
  }
}

<<<<<<< HEAD
void turnBus(fpga_t *fpga, BUSDIR b, InstructionSequence *iseq = nullptr)
{
=======
void turnBus(fpga_t *fpga, BUSDIR b, InstructionSequence *iseq = nullptr) {
>>>>>>> Software

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
<<<<<<< HEAD
void testRetention(fpga_t *fpga, const int retention)
{
=======
void testRetention(fpga_t *fpga, const int retention) {
>>>>>>> Software

  uint8_t pattern = 0xff; // the data pattern that we write to the DRAM

  bool dimm_done = false;

  InstructionSequence *iseq = nullptr; // we temporarily store (before sending
                                       // them to the FPGA) the generated
                                       // instructions here

  GET_TIME_INIT(2);

  // writing the entire row takes approximately 5 ms
  uint group_size = ceil(
      retention / 5.0f); // number of rows to be written in a single iteration
  uint cur_row_write = 0;
  uint cur_bank_write = 0;
  uint cur_row_read = 0;
  uint cur_bank_read = 0;

  printf("\n");

<<<<<<< HEAD
  while (!dimm_done)
  { // continue until we cover the entire DRAM
=======
  while (!dimm_done) { // continue until we cover the entire DRAM
>>>>>>> Software
    // print the number of the row that we are about to test
    printf("%c[2K\r", 27);
    printf("Current Bank: %d, Row: %d", cur_bank_write, cur_row_write);
    fflush(stdout);

    // Switch the memory bus to write mode
    turnBus(fpga, BUSDIR::WRITE, iseq);

    GET_TIME_VAL(0);

    // We write to a chunk of rows (group_size) successively
<<<<<<< HEAD
    for (int i = 0; i < group_size; i++)
    {
=======
    for (int i = 0; i < group_size; i++) {
>>>>>>> Software
      // write the data pattern to the entire row
      writeRow(fpga, cur_row_write, cur_bank_write, pattern, iseq);
      cur_row_write++;

      // when we complete testing all of the rows in a bank, we move to the next
      // bank
<<<<<<< HEAD
      if (cur_row_write == NUM_ROWS)
      {
=======
      if (cur_row_write == NUM_ROWS) {
>>>>>>> Software
        cur_row_write = 0;
        cur_bank_write++;
      }

      // the entire DIMM is covered when we complete testing all of the bank
<<<<<<< HEAD
      if (cur_bank_write == NUM_BANKS)
      {
=======
      if (cur_bank_write == NUM_BANKS) {
>>>>>>> Software
        // we are done with the entire DIMM
        dimm_done = true;
        break;
      }
    }

    // Switch the memory bus to read mode
    turnBus(fpga, BUSDIR::READ, iseq);

    // wait for the specified retention time (retention)
<<<<<<< HEAD
    do
    {
=======
    do {
>>>>>>> Software
      GET_TIME_VAL(1);
    } while ((TIME_VAL_TO_MS(1) - TIME_VAL_TO_MS(0)) < retention);

    // Read the data back and compare
<<<<<<< HEAD
    for (int i = 0; i < group_size; i++)
    {
=======
    for (int i = 0; i < group_size; i++) {
>>>>>>> Software
      readAndCompareRow(fpga, cur_row_read, cur_bank_read, pattern, iseq);

      cur_row_read++;

<<<<<<< HEAD
      if (cur_row_read == NUM_ROWS)
      { // NUM_ROWS
=======
      if (cur_row_read == NUM_ROWS) { // NUM_ROWS
>>>>>>> Software
        cur_row_read = 0;
        cur_bank_read++;
      }

<<<<<<< HEAD
      if (cur_bank_read == NUM_BANKS)
      { // NUM_BANKS
=======
      if (cur_bank_read == NUM_BANKS) { // NUM_BANKS
>>>>>>> Software
        // we are done with the entire DIMM
        break;
      }
    }
  }

  printf("\n");

  delete iseq;
}

<<<<<<< HEAD
void readWriteTest(fpga_t *fpga)
{
  uint8_t pattern = 0xff; // the data pattern that we write to the DRAM
  InstructionSequence *iseq = nullptr;

  printf("Start readWriteTest\n");
  // Switch the memory bus to write mode
  turnBus(fpga, BUSDIR::WRITE, iseq);
  printf("Turned Bus direction\n");
  writeRow(fpga, 0, 0, pattern, iseq);
  printf("Write success\n");

  // Switch the memory bus to read mode
  turnBus(fpga, BUSDIR::READ, iseq);
  printf("Turned Bus direction\n");
  readAndCompareRow(fpga, 0, 0, pattern, iseq);
  printf("Read success\n");
=======
void readWriteTest(fpga_t *fpga) {
  uint8_t pattern = 0xff; // the data pattern that we write to the DRAM
  InstructionSequence *iseq = nullptr;

  // Switch the memory bus to write mode
  turnBus(fpga, BUSDIR::WRITE, iseq);

  writeRow(fpga, 0, 0, pattern, iseq);

  // Switch the memory bus to read mode
  turnBus(fpga, BUSDIR::READ, iseq);

  readAndCompareRow(fpga, 0, 0, pattern, iseq);
>>>>>>> Software
}

// provide trefi = 0 to disable auto-refresh
// auto-refresh is disabled by default (disabled after FPGA boots, disables on
// pushing reset button)
<<<<<<< HEAD
void setRefreshConfig(fpga_t *fpga, uint trefi, uint trfc)
{
=======
void setRefreshConfig(fpga_t *fpga, uint trefi, uint trfc) {
>>>>>>> Software
  InstructionSequence *iseq = new InstructionSequence;

  iseq->insert(genREF_CONFIG(trfc, REGISTER::TRFC));
  iseq->insert(genREF_CONFIG(trefi, REGISTER::TREFI));

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);

  delete iseq;
}

// set CLK Speed with three parameters
<<<<<<< HEAD
void setCLKspeed(fpga_t *fpga, uint clkfbout_mult, uint divclk_divide,
                 uint clkout_divide)
{
=======
int setCLKspeed(fpga_t *fpga, uint clkfbout_mult, uint divclk_divide,
                uint clkout_divide) {
>>>>>>> Software
  InstructionSequence *iseq = new InstructionSequence;

  // Tip: Only adjust first parameter -> clkfbout_mult * 100MHz = DDR_speed
  //      with divclk_divide = 1, clkout_divide = 4
  iseq->insert(genCLKSPEED_CONFIG(clkfbout_mult, divclk_divide, clkout_divide));

  iseq->insert(genEND());

  iseq->execute(fpga);

<<<<<<< HEAD
  delete iseq;
}

void printHelp(char *argv[])
{
  cout << "A sample application that tests clk reprogramming of DRAM and MemoryController Clocks using "
          "SoftMC"
       << endl;
  cout << "Usage:" << argv[0] << " [MULTIPLIER]" << endl;
  cout << "The Multiplier should be a positive integer > 5, indicating the "
          "target frequency. Frequency = MULT * 100MHz"
       << endl;
}

int main(int argc, char *argv[])
{
=======
  // Receive the data
  uint rbuf[8];
  int read_result = fpga_recv(fpga, 0, (void *)rbuf, 16, 0);

  printf("Read Result: %d", read_result);

  if (read_result < 0) {
    return -1;
  }

  // compare with the pattern
  uint8_t *rbuf8 = (uint8_t *)rbuf;

  uint8_t clkfbout_mult = rbuf[0];
  uint8_t divclk_divide = rbuf[1];
  uint8_t clkout_divide = rbuf[2];

  uint8_t base_clk = 400;

  int DDR_clk = (base_clk * clkfbout_mult) / (divclk_divide * clkout_divide);
  int fabric_clk = DDR_clk / 2;

  delete iseq;
  return fabric_clk;
}

void printHelp(char *argv[]) {
  cout << "A sample application that tests retention time of DRAM cells using "
          "SoftMC"
       << endl;
  cout << "Usage:" << argv[0] << " [REFRESH INTERVAL]" << endl;
  cout << "The Refresh Interval should be a positive integer, indicating the "
          "target retention time in milliseconds."
       << endl;
}

int main(int argc, char *argv[]) {
>>>>>>> Software
  fpga_t *fpga;
  fpga_info_list info;
  int fid = 0; // fpga id
  int ch = 0;  // channel id

<<<<<<< HEAD
  if (argc != 2 || strcmp(argv[1], "--help") == 0) {
    printHelp(argv);
    return -2;
  }

  string s_freq(argv[1]);
  int freq_mult = 0;

  try {
    freq_mult = stoi(s_freq);
  } catch (...) {
    printHelp(argv);
    return -3;
  }

  if (freq_mult < 0) {
    printHelp(argv);
    return -4;
  }

  // Get the list of FPGA's attached to the system
  if (fpga_list(&info) != 0)
  {
=======
  // if (argc != 2 || strcmp(argv[1], "--help") == 0) {
  //   printHelp(argv);
  //   return -2;
  // }

  // string s_ref(argv[1]);
  // int refresh_interval = 0;

  // try {
  //   refresh_interval = stoi(s_ref);
  // } catch (...) {
  //   printHelp(argv);
  //   return -3;
  // }

  // if (refresh_interval <= 0) {
  //   printHelp(argv);
  //   return -4;
  // }

  // Get the list of FPGA's attached to the system
  if (fpga_list(&info) != 0) {
>>>>>>> Software
    printf("Error populating fpga_info_list\n");
    return -1;
  }
  printf("Number of devices: %d\n", info.num_fpgas);
<<<<<<< HEAD
  for (int i = 0; i < info.num_fpgas; i++)
  {
=======
  for (int i = 0; i < info.num_fpgas; i++) {
>>>>>>> Software
    printf("%d: id:%d\n", i, info.id[i]);
    printf("%d: num_chnls:%d\n", i, info.num_chnls[i]);
    printf("%d: name:%s\n", i, info.name[i]);
    printf("%d: vendor id:%04X\n", i, info.vendor_id[i]);
    printf("%d: device id:%04X\n", i, info.device_id[i]);
  }

  // Open an FPGA device, so we can read/write from/to it
  fpga = fpga_open(fid);

<<<<<<< HEAD
  if (!fpga)
  {
=======
  if (!fpga) {
>>>>>>> Software
    printf("Problem on opening the fpga \n");
    return -1;
  }
  printf("The FPGA has been opened successfully! \n");

  // send a reset signal to the FPGA
  fpga_reset(fpga); // keep this, recovers FPGA from some unwanted state

  uint trefi =
      7800 / 200;  // 7.8us (divide by 200ns as the HW counts with that period)
  uint trfc = 104; // default trfc for 4Gb device
  printf("Activating AutoRefresh. tREFI: %d, tRFC: %d \n", trefi, trfc);
  setRefreshConfig(fpga, trefi, trfc);

  // printf("Starting Retention Time Test @ %d ms! \n", refresh_interval);

  // testRetention(fpga, refresh_interval);

  printf("Setting clock Speed to:\n");

<<<<<<< HEAD
  // set speed to x MHz
  // Default is (6,1,3), corresponds to 800 Mhz DDR Clk and 400 MHz Fabric Clk
  // Tip: Only adjust first parameter -> clkfbout_mult * 100MHz = DDR_speed
  //      with divclk_divide = 1, clkout_divide = 4

  setCLKspeed(fpga, freq_mult, 1, 4);
  usleep(100 * ms);//sleeps for 100 milli seconds
=======
  // set speed to 900 MHz
  // Tip: Only adjust first parameter -> clkfbout_mult * 100MHz = DDR_speed
  //      with divclk_divide = 1, clkout_divide = 4

  int new_clk_speed = setCLKspeed(fpga, 9, 1, 4);
  if (new_clk_speed < 0) {
    printf("Error: Something went wrong during Clk speed change");
    return;
  } else {
    printf("%dMHz\nClk Change Successful!\n", new_clk_speed);
  }
>>>>>>> Software

  printf("Starting Basic Read/Write Test! \n");

  readWriteTest(fpga);

  printf("The test has been completed! \n");
  fpga_close(fpga);

  return 0;
}
