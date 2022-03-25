#include <stdio.h>
#include <riffa.h>
#include <cassert>
#include <string.h>
#include <iostream>
#include <cmath>
#include "softmc.h"
#include <fstream>
#include <unistd.h>
#include <sstream>

using namespace std;

#define tRCD_MAX 8
#define TESTED_CHIP "SamsungLong"
#define TEMP "40"
#define RETENTION_STEP 20
#define RETENTION_STEP_COUNT 7

// set CLK Speed with three parameters
void setCLKspeed(fpga_t *fpga, uint clkfbout_mult, uint divclk_divide,
                 uint clkout_divide)
{
  InstructionSequence *iseq = new InstructionSequence;

  // Tip: Only adjust first parameter -> clkfbout_mult * 100MHz = DDR_speed
  //      with divclk_divide = 1, clkout_divide = 4
  iseq->insert(genCLKSPEED_CONFIG(clkfbout_mult, divclk_divide, clkout_divide));

  iseq->insert(genEND());

  iseq->execute(fpga);

  return;
}

void flushReadFIFO(fpga_t *fpga)
{
  printf("Start Read FIFO flushing\n");
  unsigned int rbuf[16];
  for (int i = 0; i < 200;
       i++)
  {
    fpga_recv(fpga, 0, (void *)rbuf, 16, 5);
  }
}

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

void sendRowColumnReadCommand(fpga_t *fpga, uint row, uint bank, uint column,
                              InstructionSequence *&iseq, uint tRCD)
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
  iseq->insert(genWAIT(tRCD));

  iseq->insert(genRD(bank, column));

  // We need to wait for tCL and 4 cycles burst (double data-rate)
  iseq->insert(genWAIT(6 + 4));

  // Wait some more in any case
  iseq->insert(genWAIT(3));

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);
}

void receiveRowData(fpga_t *fpga, uint64_t *data)
{
  // Receive the data
  uint rbuf[16];
  for (int i = 0; i < NUM_COLS; i += 8)
  { // we receive 64 bytes (8x64bit)
    fpga_recv(fpga, 0, (void *)rbuf, 16, 0);

    uint64_t *rbuf64 = (uint64_t *)rbuf;

    for (int j = 0; j < 8; j++)
    {
      data[i + j] = rbuf64[j];
    }
  }
  return;
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

void readAndCompare(fpga_t *fpga, uint64_t rowOffset, uint8_t pattern, uint tRCD, uint64_t *errors, float *errorProzent, uint row_count)
{
  InstructionSequence *iseq = nullptr; // we temporarily store (before sending
                                       // them to the FPGA) the generated
                                       // instructions here

  uint64_t results[NUM_COLS];

  // 64 bit pattern
  uint64_t pattern_64 = 0;
  for (int i = 0; i < 7; i++)
  {
    pattern_64 |= (uint64_t)pattern;
    pattern_64 = (pattern_64 << 8);
  }
  pattern_64 |= (uint64_t)pattern;

  for (uint row = 0; row < row_count; row++)
  {
    // Read the entire row
    for (int i = 0; i < NUM_COLS; i += 8)
    { // we use 8x burst mode
      sendRowColumnReadCommand(fpga, row + rowOffset, 0, i, iseq, tRCD);
    }
    receiveRowData(fpga, results);

    for (uint i = 0; i < NUM_COLS; i++)
    {
      if (results[i] != pattern_64)
      {
        uint64_t testMask = 0x00000001;
        for (uint j = 0; j < 64; j++)
        {
          uint64_t testResult = results[i] & testMask;
          uint64_t testPattern = pattern_64 & testMask;

          if (testResult != testPattern)
          {
            *errors = *errors + 1;
          }
          testMask = testMask << 1;
        }
      }
    }
  }
  uint64_t numWords = row_count * NUM_COLS;
  *errorProzent = (float)*errors / ((float)numWords * 64) * 100;
  return;
}

void systematicTest(fpga_t *fpga, uint8_t pattern)
{
  ostringstream filePath;
  filePath << "results/" << TESTED_CHIP << TEMP << "°C/";
  ostringstream fileName;
  fileName << TESTED_CHIP << TEMP << "x" << hex << +pattern << dec << ".csv";
  ofstream resultFile(filePath.str() + fileName.str());
  InstructionSequence *iseq = nullptr; // we temporarily store (before sending
                                       // them to the FPGA) the generated
                                       // instructions here

  resultFile << "tRCD in ns,retention,FehlerAbs,FehlerProzent" << endl;

  uint row_count = 100; // NUM_ROWS / (tRCD_MAX * RETENTION_STEP_COUNT);
  printf("row_count: %d\n", row_count);
  for (size_t i = 6; i <= 10; i++)
  {
    size_t clkSpeed = (200 * i / 4);
    printf("Set Clock to: %ld\n", clkSpeed);
    setCLKspeed(fpga, i, 1, 4);
    sleep(1);

    // write Rows
    turnBus(fpga, BUSDIR::WRITE, iseq);
    for (uint row = 0; row < row_count * tRCD_MAX * RETENTION_STEP_COUNT; row++)
    {
      writeRow(fpga, row, 0, pattern, iseq);
      usleep(1);
    }
    usleep(20);
    // Read Rows
    turnBus(fpga, BUSDIR::READ, iseq);
    uint64_t rowOffset = 0;
    for (uint retention = 0; retention < RETENTION_STEP_COUNT; retention++)
    {
      uint64_t errors = 0;
      float errorProzent;
      for (uint tRCD = 1; tRCD <= tRCD_MAX; tRCD++)
      {
        printf("tRCD: %d\n", tRCD);
        readAndCompare(fpga, rowOffset, pattern, tRCD, &errors, &errorProzent, row_count);
        rowOffset += row_count;
        float tRCDns = (float)(tRCD * 1000) / (float)clkSpeed;
        resultFile << tRCDns << "," << retention * RETENTION_STEP << "," << errors << "," << errorProzent << endl;
        errors = 0;
      }
      sleep(RETENTION_STEP);
    }
  }

  resultFile.close();
  return;
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

  // Open an FPGA device, so we can read/write from/to it
  fpga = fpga_open(fid);

  if (!fpga)
  {
    printf("Problem on opening the fpga \n");
    return -1;
  }
  printf("The FPGA has been opened successfully! \n");

  // send a reset signal to the FPGA
  fpga_reset(fpga); // keep this, recovers FPGA from some unwanted state
  usleep(100);
  setRefreshConfig(fpga, 0, 104);
  usleep(100);
  flushReadFIFO(fpga);

  // uint8_t pattern = 0xff;

  // systematicTest(fpga, pattern);

  uint8_t pattern = 0x00;

  systematicTest(fpga, pattern);

  printf("The test has been completed! \n");
  fpga_close(fpga);

  return 0;
}
