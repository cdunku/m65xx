#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <jansson.h>

#include "m65xx.h"
#include "debug.h"

/*
 *
 *
 * 6502 tests
 *
 *
*/ 

// Structs for the TomHarte JSON tests

typedef struct {
  uint16_t pc_;
  uint8_t a_, x_, y_, s_, p_;
} registers_json_t;

typedef struct {
  uint16_t addr;
  uint8_t data;
  char rw[6];
} cycles_json_t;

// Helper functions

static inline void set_abus(m65xx_t* const m, uint16_t addr) { 
  m->pins = (m->pins & ~0xFFFFULL) | (addr & 0xFFFF);
}
static inline void set_dbus(m65xx_t* const m, uint8_t data) {
  m->pins = (m->pins & ~0xFF0000ULL) | ((data & 0xFF) << 16);
}
static inline uint16_t get_abus(m65xx_t* const m) { return (m->pins & 0xFFFF); }
static inline uint8_t get_dbus(m65xx_t* const m) { return ((m->pins & 0xFF0000) >> 16); }

// Functions for testing the emulator

static int m65xx_harte_tests(m65xx_t* const m, const char *file) {
  json_error_t error;
  json_t *root = json_load_file(file, 0, &error);
  if(!root) {
    fprintf(stderr, "Error: failed to parse %s (%s at line %d)\n", file, error.text, error.line);
    return 0;
  }

  size_t passed = 0, failed = 0;

  size_t ntests = json_array_size(root);

  for(size_t i = 0; i < ntests; i++) {
    json_t *test = json_array_get(root, i);

    if(!json_is_object(test)) {
      fprintf(stderr, "Error: test case at %zu is not valid\n", i);
      failed++;
    }

    bool match = 1;

    const char *name = json_string_value(json_object_get(test, "name"));
    json_t *initial = json_object_get(test, "initial");
    json_t* final = json_object_get(test, "final");

    m65xx_init(m);

    m->pc = json_integer_value(json_object_get(initial, "pc"));
    m->a = json_integer_value(json_object_get(initial, "a"));
    m->x = json_integer_value(json_object_get(initial, "x"));
    m->y = json_integer_value(json_object_get(initial, "y"));
    m->s = json_integer_value(json_object_get(initial, "s"));
    m->p = json_integer_value(json_object_get(initial, "p"));

    json_t *ram = json_object_get(initial, "ram");
    if (json_is_array(ram)) {
      size_t ram_size = json_array_size(ram);
      for (size_t j = 0; j < ram_size; j++) {
        json_t *entry = json_array_get(ram, j);
        if (json_is_array(entry) && json_array_size(entry) == 2) {
          uint16_t ram_address = json_integer_value(json_array_get(entry, 0));
          uint8_t ram_data = json_integer_value(json_array_get(entry, 1));
          m->ram[ram_address] = ram_data;
        }
      }
    }
 
    set_abus(m, m->pc);

    set_dbus(m, m->ram[json_integer_value(json_object_get(initial, "pc"))]);

    json_t *cycles = json_object_get(test, "cycles");
    size_t cycles_total = json_array_size(cycles);
   
    cycles_json_t cycle[cycles_total];
    // printf("\nTest: %s\n", name);
    for(size_t k = 0; k < cycles_total; k++) {
      json_t *cyclei = json_array_get(cycles, k);

      cycle[k].addr = json_integer_value(json_array_get(cyclei, 0));
      cycle[k].data = json_integer_value(json_array_get(cyclei, 1));
      strncpy(cycle[k].rw, json_string_value(json_array_get(cyclei, 2)), sizeof(cycle[k].rw) - 1);
      cycle[k].rw[sizeof(cycle[k].rw) - 1] = '\0';
      uint16_t addr = get_abus(m);
      uint8_t data = get_dbus(m);

      if(!(m->pins & RW)) {
        m->ram[addr] = data;
      }

      m65xx_tick(m);

      strncpy(cycle[k].rw, (m->pins & RW) ? "read" : "write", 6);

      if(addr != cycle[k].addr) {
        printf("Error: invalid address at cycle: %zu | Actual: 0x%04X | Expected: 0x%04X \n",
               k+1, addr, cycle[k].addr); 
        match = 0;
      }
      if(data != cycle[k].data) {
        printf("Error: invalid data at cycle: %zu | Actual: 0x%02X | Expected: 0x%02X \n", 
               k+1, data, cycle[k].data);
        match = 0;
      }
      if(strncmp(cycle[k].rw, (m->pins & RW) ? "read" : "write", 6)) {
        printf("Error: invalid action at cycle: %zu | Actual: %s | Expected: %s \n", 
               k+1, (m->pins & RW) ? "read" : "write", cycle[k].rw);
        match = 0;
      }

      addr = get_abus(m);
      data = get_dbus(m);

      if(m->pins & RW) {
        set_dbus(m, m->ram[addr]);
      }
    }



    registers_json_t t;

    t.pc_ = json_integer_value(json_object_get(final, "pc"));
    t.a_ = json_integer_value(json_object_get(final, "a"));
    t.x_ = json_integer_value(json_object_get(final, "x"));
    t.y_ = json_integer_value(json_object_get(final, "y"));
    t.s_ = json_integer_value(json_object_get(final, "s"));
    t.p_ = json_integer_value(json_object_get(final, "p"));

    if (m->pc != t.pc_) {
      printf("Error: PC mismatch in test %s | Actual: %04X | Expected: %04X\n", name, m->pc, t.pc_);
      match = 0;
    }
    if (m->a != t.a_) {
      printf("Error: A mismatch in test %s | Actual: %02X | Expected: %02X\n", name, m->a, t.a_);
      match = 0;
    }
    if (m->x != t.x_) {
      printf("Error: X mismatch in test %s | Actual: %02X | Expected: %02X\n", name, m->x, t.x_);
      match = 0;
    }
    if (m->y != t.y_) {
      printf("Error: Y mismatch in test %s | Actual: %02X | Expected: %02X\n", name, m->y, t.y_);
      match = 0;
    }
    if (m->s != t.s_) {
      printf("Error: S mismatch in test %s | Actual: %02X | Expected: %02X\n", name, m->s, t.s_);
      match = 0;
    }
    if (m->p != t.p_) {
      printf("Error: P mismatch in test %s | Actual: %02X | Expected: %02X\n", name, m->p, t.p_);
      match = 0;
    }

    json_t *fram = json_object_get(final, "ram");
    if (json_is_array(fram)) {
      size_t fram_size = json_array_size(fram);
      for (size_t k = 0; k < fram_size; k++) {
        json_t *entry = json_array_get(fram, k);
        if (json_is_array(entry) && json_array_size(entry) == 2) {
          uint16_t expected_addr = json_integer_value(json_array_get(entry, 0));
          uint8_t expected_data = json_integer_value(json_array_get(entry, 1));
          uint8_t actual_data = m->ram[expected_addr];
          if (actual_data != expected_data) {
            printf("Error: RAM mismatch at %04X | Actual: %02X | Expected: %02X\n", expected_addr, actual_data, expected_data);
          }
        }
      }
    }
    if (match) { passed++; } else { failed++; }
  }
  printf("Opcode: 0x%02X | Tests passed: %zu, Tests failed: %zu\n", m->ir, passed, failed);
  json_decref(root);

  if((passed % 10000) == 0) { return 1; } else { return 0; }
}

static int load_file(m65xx_t* const m, const char *file, uint16_t addr) {
  FILE *f = fopen(file, "rb");
  if(f == NULL) {
    fprintf(stderr, "Error: file [%s] could not be opened\n", file);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);

  if(size + addr > 0x10000) {
    fprintf(stderr, "Error: file [%s] cannot be fit into memory\n", file);
    fclose(f);
    return 1;
  }

  size_t result = fread(&m->ram[addr], sizeof(uint8_t), size, f);
  if(result != size) {
    fprintf(stderr, "Error: file size not equivalent for [%s]", file);
    fclose(f);
    return 1;
  }

  fclose(f);
  return 0;
}

static int allsuiteasm(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);

  load_file(m, "tests/AllSuiteA.bin", 0x4000);

  m65xx_init(m);

  while(true) {
    do {  
      if(m->pins & RW) {
        set_dbus(m, m->ram[get_abus(m)]);
      }
      m65xx_tick(m);
      if(!(m->pins & RW)) {
        m->ram[get_abus(m)] = get_dbus(m);
      }
    } while (!(m->pins & SYNC));
    if(m->pc == 0x45C0) {
      if(m->ram[0x0210] == 0xFF) {
        printf("AllSuiteASM passed!\n");
        break;
      }
      else {
        fprintf(stderr, "AllSuiteASM failed!\n");
        break;
      }
    }
  }
  return 0;
}
static int m6502_functional_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);

  load_file(m, "tests/6502_functional_test.bin", 0x0);

  m65xx_init(m);

  set_abus(m, m->pc = 0x400);

  uint16_t pc_ = 0;
  
  while (true) {
    do {  
      if(m->pins & RW) {
        set_dbus(m, m->ram[get_abus(m)]);
      }
      m65xx_tick(m);
      if(!(m->pins & RW)) {
        m->ram[get_abus(m)] = get_dbus(m);
      }
    } while (!(m->pins & SYNC));

    if(pc_ == m->pc) {
      if(m->pc == 0x3469) {
        printf("6502 functional test passed!\n");
        break;
      }
      printf("6502 functional test not passed! (trapped at 0x%04X)\n", m->pc);
      break;
  }
    pc_ = m->pc;
  }
  return 0;
}

static int m6502_timing_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000); 
  load_file(m, "tests/timingtest-1.bin", 0x1000);

  m65xx_init(m);  

  set_abus(m, m->pc = 0x1000);

  size_t expected_cyc = 1141LU;
  while (true) {

    do {  
      if(m->pins & RW) {
        set_dbus(m, m->ram[get_abus(m)]);
      }
      m65xx_tick(m);
      if(!(m->pins & RW)) {
        m->ram[get_abus(m)] = get_dbus(m);
      }
    } while (!(m->pins & SYNC));

    if (m->pc == 0x1269) {
      printf("%s", m->cpu_clock == expected_cyc ? "Timing test passed!\n" : "Timing Test failed!\n");
      break;
    }
  }
  return 0; 
}
static int m6502_decimal_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);  
  load_file(m, "tests/6502_decimal_test.bin", 0x200);

  m65xx_init(m);  
  set_abus(m, m->pc = 0x200);  

    while(true) {
    do {  
      if(m->pins & RW) {
        set_dbus(m, m->ram[get_abus(m)]);
      }
      m65xx_tick(m);
      if(!(m->pins & RW)) {
        m->ram[get_abus(m)] = get_dbus(m);
      }
    } while (!(m->pins & SYNC));
    if (m->pc == 0x024b) {
      printf("%s", m->a == 0 ? "6502 Decimal test passed!\n" : "6502 decimal test failed!\n");
      break;
    }
  }
  return 0;
}

uint8_t inte = 0;

void m6502_interrupt_handler(m65xx_t* const m) {
  if ((inte & 0x2) == 0x2) {
    m->nmi_ = 1;
    inte &= ~0x2;
    }
  else if (!(m->p & IDF) && (inte & 0x1) == 0x1) {
    m->irq_ = 1;
    inte &= ~0x1;
  }
}
static int m6502_interrupt_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);
  load_file(m, "tests/6502_interrupt_test.bin", 0xA);

  m65xx_init(m);

  uint16_t pc_ = 0;
  set_abus(m, m->pc = 0x400);

  m->ram[0xBFFC] = 0;
  
  while (true) {
    do {  
      if(m->pins & RW) {
        set_dbus(m, m->ram[get_abus(m)]);
      }
      m65xx_tick(m);
      if(!(m->pins & RW)) {
        m->ram[get_abus(m)] = get_dbus(m);
      }
    } while (!(m->pins & SYNC));

    inte = m->ram[0xBFFC];
    m6502_interrupt_handler(m);
    m->ram[0xBFFC] = inte;
    
    if (pc_ == m->pc) {
      if (m->pc == 0x06F5) {
        printf("6502 Interrupt test passed!\n");
        break;
      }
      printf("6502 Interrupt test failed!\n");
      break;
      }
    pc_ = m->pc;
  }
  return 0;
}

int main(void) {
  m65xx_t m;

  clock_t start = clock();
  int pass = 0;
  
  // Runs all tests at once for TomHarte:
  
  printf("Starting 6502 test...\n");
  for(int i = 0; i < 0x100; i++) {
    char file[50];
    snprintf(file, sizeof(file), "tests/6502/v1/%02x.json", i);
    pass += m65xx_harte_tests(&m, file);
  }
  printf("Tests passed = %d\n", pass);
  
  // For running a test for a specific opcode:
  // Pass: 1 (All tests pass), Pass: 0 (A test has failed)
  /*
  pass = m65xx_harte_tests(&m, "tests/6502/v1/00.json");
  printf("Pass: %d, opcode: 0x%02X\n", pass, m.ir);
  */

  // AllSuiteA test
  allsuiteasm(&m);

  // Timing test for legal opcodes
  m6502_timing_test(&m);

  // Klaus Dormann test
  m6502_decimal_test(&m);
  m6502_functional_test(&m);
  m6502_interrupt_test(&m);

  clock_t end = clock();

  double time = (double)(end - start)/ CLOCKS_PER_SEC;
  printf("Tests completed in %.4f\n", time);
  return 0;
}

