# m65xx — A cycle-ticked 6502 Emulator

**m65xx** is a simple, clean, and cycle-ticked emulator for the [MOS Technology 6502](https://en.wikipedia.org/wiki/MOS_Technology_6502) CPU.

Please note that the projects has been compiled using `std=c2x`, this version alongside `std=c11` are the recommended versions.

Successfully compiled under MacOS Sonoma and Fedora 42. If you are using Windows, please use WSL.

This project is licensed under the **GNU GPLv3**, please abide by the license.

## Little about the emulator

**Cycle-ticked emulation** — Accurate per-clock-cycle simulation of instruction timing  

**MOS 6502 core** — Fully implements the original 6502 instruction set (both documented and undocumented instructions)

**Planned 65C02 support** — Adding both documented and undocumented 65c02 instructions in the future

**The 6502 emulator has been tested by the following test suites:** 
- [Klaus Dormann](https://github.com/Klaus2m5/6502_65C02_functional_tests) (6502): functional, decimal and interrupts tests
- [AllSuiteASM](https://github.com/MusicOfMusiX/6502emu/blob/master/AllSuiteA.asm)
- [Timing Test](https://github.com/BigEd/6502timing)
- [TomHarte 6502](https://github.com/SingleStepTests/ProcessorTests/tree/main/6502) test suite


## Build and run

To build and run:

```bash
git clone https://github.com/cdunku/m65xx.git
cd m65xx
make
```

## Running the tests yourself

If you want to run a specific test file you can edit the main function:
```c
int main(void) {
  ...
  
  // Runs all tests at once for TomHarte:
  
  printf("Starting 6502 test...\n");
  // for(int i = 0; i < 0x100; i++) {
  //  char file[50];
  //  snprintf(file, sizeof(file), "tests/6502/v1/%02x.json", i);
  //  pass += m65xx_harte_tests(&m, file);
  // }
  // printf("Tests passed = %d\n", pass);
  
  // For running a test for a specific opcode:
  // Pass: 1 (All tests pass), Pass: 0 (A test has failed)
  /*
  pass = m65xx_harte_tests(&m, "tests/6502/v1/00.json");
  printf("Pass: %d, opcode: 0x%02X\n", pass, m.ir);
  */

  // AllSuiteA test
  // allsuiteasm(&m);

  // Timing test for legal opcodes
  // m6502_timing_test(&m);

  // Klaus Dormann test
  // m6502_decimal_test(&m);
  // m6502_functional_test(&m);
  // m6502_interrupt_test(&m);

  ...
}
```

Additionally, adding the `if(m->tcu == 0) { m6502_print(m); }` statement right after the `m65xx_tick(m)` function in the test functions will print out information in the following format:
```bash
PC:0400 A:00 X:00 Y:00 S:FD [--1-----]| cyc=2 d8---- CLD
PC:0401 A:00 X:00 Y:00 S:FD [--1---Z-]| cyc=4 a900-- LDA #$00
PC:0403 A:00 X:00 Y:00 S:FD [--1---Z-]| cyc=8 8d0302 STA $0203
PC:0406 A:00 X:FF Y:00 S:FD [N-1-----]| cyc=10 a2ff-- LDX #$FF
PC:0408 A:00 X:FF Y:00 S:FF [N-1-----]| cyc=12 9a---- TXS
PC:0409 A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=16 adfcbf LDA $BFFC
PC:040C A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=18 297e-- AND #$7E
PC:040E A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=22 8dfcbf STA $BFFC
PC:0411 A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=26 adfcbf LDA $BFFC
PC:0414 A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=28 297d-- AND #$7D
PC:0416 A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=32 8dfcbf STA $BFFC
PC:0419 A:02 X:FF Y:00 S:FF [--1-----]| cyc=34 a902-- LDA #$02
PC:041B A:02 X:FF Y:00 S:FF [--1-----]| cyc=38 8d0302 STA $0203
PC:041E A:00 X:FF Y:00 S:FF [--1---Z-]| cyc=40 a900-- LDA #$00
...
```
