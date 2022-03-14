#include <stdio.h>
#include <riffa.h>
#include <cassert>
#include <string.h>
#include <iostream>
#include <cmath>
#include "softmc.h"
#include <fstream>

using namespace std;

// set CLK Speed with three parameters
void setCLKspeed(fpga_t *fpga, uint clkfbout_mult, uint divclk_divide,
                 uint clkout_divide) {
  InstructionSequence *iseq = new InstructionSequence;

  // Tip: Only adjust first parameter -> clkfbout_mult * 100MHz = DDR_speed
  //      with divclk_divide = 1, clkout_divide = 4
  iseq->insert(genCLKSPEED_CONFIG(clkfbout_mult, divclk_divide, clkout_divide));

  iseq->insert(genEND());

  iseq->execute(fpga);

  return;
}

// Note that capacity of the instruction buffer is 8192 instructions
void writeRow(fpga_t *fpga, uint row, uint bank, uint8_t pattern,
              InstructionSequence *&iseq) {

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
  for (int i = 0; i < NUM_COLS; i += 8) { // we use 8x burst mode
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

void sendRowReadCommand(fpga_t *fpga, uint row, uint bank,
                        InstructionSequence *&iseq, uint tRCD) {
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

  // Read the entire row
  for (int i = 0; i < NUM_COLS; i += 8) { // we use 8x burst mode
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
}

void receiveRowData(fpga_t *fpga, uint64_t *data) {
  // Receive the data
  uint rbuf[16];
  for (int i = 0; i < NUM_COLS; i += 8) { // we receive 64 bytes (8x64bit)
    fpga_recv(fpga, 0, (void *)rbuf, 16, 0);

    // compare with the pattern
    uint64_t *rbuf64 = (uint64_t *)rbuf;

    for (int j = 0; j < 8; j++) {
      data[i + j] = rbuf64[j];
    }
  }
  return;
}

void readAndCompareRow(fpga_t *fpga, const uint row, const uint bank,
                       const uint8_t pattern, InstructionSequence *&iseq) {

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
  for (int i = 0; i < NUM_COLS; i += 8) { // we use 8x burst mode
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
       i += 8) { // we receive a single burst at two times (32 bytes each)
    fpga_recv(fpga, 0, (void *)rbuf, 16, 0);

    // compare with the pattern
    uint8_t *rbuf8 = (uint8_t *)rbuf;

    for (int j = 0; j < 64; j++) {
      if (rbuf8[j] != pattern)
        fprintf(stderr, "Error at Col: %d, Row: %u, Bank: %u, DATA: %x \n", i,
                row, bank, rbuf8[j]);
    }
  }
}

void turnBus(fpga_t *fpga, BUSDIR b, InstructionSequence *iseq = nullptr) {

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

void testAccessTimings(fpga_t *fpga) {

  uint8_t pattern = 0xff; // the data pattern that we write to the DRAM

  InstructionSequence *iseq = nullptr; // we temporarily store (before sending
                                       // them to the FPGA) the generated
                                       // instructions here

  uint64_t results[10][NUM_COLS];

  uint64_t *test;

  // Iterate through different clk speeds an retention times
  for (size_t i = 1; i < 11; i++) {
    turnBus(fpga, BUSDIR::WRITE, iseq);
    writeRow(fpga, 0, 0, pattern, iseq);
    turnBus(fpga, BUSDIR::READ, iseq);
    sendRowReadCommand(fpga, 0, 0, iseq, i);
    receiveRowData(fpga, results[i]);
  }

  // write results into file
  ofstream resultFile("result.csv");
  resultFile << "Col, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10" << endl;
  for (size_t i = 0; i < NUM_COLS; i++) {
    resultFile << i;
    for (size_t j = 0; j < 10; j++) {
      resultFile << "," << results[j][i];
    }
    resultFile << endl;
  }
  resultFile.close();
}

// provide trefi = 0 to disable auto-refresh
// auto-refresh is disabled by default (disabled after FPGA boots, disables on
// pushing reset button)
void setRefreshConfig(fpga_t *fpga, uint trefi, uint trfc) {
  InstructionSequence *iseq = new InstructionSequence;

  iseq->insert(genREF_CONFIG(trfc, REGISTER::TRFC));
  iseq->insert(genREF_CONFIG(trefi, REGISTER::TREFI));

  // START Transaction
  iseq->insert(genEND());

  iseq->execute(fpga);

  delete iseq;
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
  fpga_t *fpga;
  fpga_info_list info;
  int fid = 0; // fpga id
  int ch = 0;  // channel id

  if (argc != 2 || strcmp(argv[1], "--help") == 0) {
    printHelp(argv);
    return -2;
  }

  string s_ref(argv[1]);
  int refresh_interval = 0;

  try {
    refresh_interval = stoi(s_ref);
  } catch (...) {
    printHelp(argv);
    return -3;
  }

  if (refresh_interval <= 0) {
    printHelp(argv);
    return -4;
  }

  // Get the list of FPGA's attached to the system
  if (fpga_list(&info) != 0) {
    printf("Error populating fpga_info_list\n");
    return -1;
  }
  printf("Number of devices: %d\n", info.num_fpgas);
  for (int i = 0; i < info.num_fpgas; i++) {
    printf("%d: id:%d\n", i, info.id[i]);
    printf("%d: num_chnls:%d\n", i, info.num_chnls[i]);
    printf("%d: name:%s\n", i, info.name[i]);
    printf("%d: vendor id:%04X\n", i, info.vendor_id[i]);
    printf("%d: device id:%04X\n", i, info.device_id[i]);
  }

  // Open an FPGA device, so we can read/write from/to it
  fpga = fpga_open(fid);

  if (!fpga) {
    printf("Problem on opening the fpga \n");
    return -1;
  }
  printf("The FPGA has been opened successfully! \n");

  // send a reset signal to the FPGA
  fpga_reset(fpga); // keep this, recovers FPGA from some unwanted state

  // uint trefi = 7800/200; //7.8us (divide by 200ns as the HW counts with that
  // period)
  // uint trfc = 104; //default trfc for 4Gb device
  // printf("Activating AutoRefresh. tREFI: %d, tRFC: %d \n", trefi, trfc);
  // setRefreshConfig(fpga, trefi, trfc);

  testAccessTimings(fpga);

  printf("The test has been completed! \n");
  fpga_close(fpga);

  return 0;
}
