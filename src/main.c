#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <jansson.h>
#include <SDL2/SDL.h>

#include "m65xx.h"
#include "debug.h"

void m65xx_init(m65xx_t* const m);

typedef struct {
  uint16_t pc_;
  uint8_t a_, x_, y_, s_, p_;
  uint64_t cyc_;
} json_tests_t;

static inline uint8_t rb(m65xx_t* const m, uint16_t addr) { return m->ram[addr]; }
static inline void wb(m65xx_t* const m, uint16_t addr, uint8_t data) { m->ram[addr] = data; }
static inline void set_abus(m65xx_t* const m, uint16_t addr) { 
    m->pins = (m->pins & ~0xFFFFULL) | (addr & 0xFFFF);
}

void json_tests(m65xx_t* const m, json_tests_t* const t, const char *file) {
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error: Failed to parse %s (%s at line %d)\n", file, error.text, error.line);
        return;
    }

    size_t passed = 0, failed = 0;
    bool match = 1;

    size_t num_tests = json_array_size(root);

    for (size_t i = 0; i < num_tests; i++) {
        match = 1; // Reset match for each test
        json_t *test = json_array_get(root, i);

        if (!json_is_object(test)) {
            fprintf(stderr, "Error: Test case at index %zu is not an object.\n", i);
            failed++;
            continue;
        }
    const char *name = json_string_value(json_object_get(test, "name"));
        json_t *initial = json_object_get(test, "initial");
        json_t *final = json_object_get(test, "final");

    m65xx_init(m);

        // Initialize registers, RAM, and Cycles
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

        // Get final values
        t->pc_ = json_integer_value(json_object_get(final, "pc"));
        t->a_ = json_integer_value(json_object_get(final, "a"));
        t->x_ = json_integer_value(json_object_get(final, "x"));
        t->y_ = json_integer_value(json_object_get(final, "y"));
        t->s_ = json_integer_value(json_object_get(final, "s"));
        t->p_ = json_integer_value(json_object_get(final, "p"));

        do {
            m65xx_run(m);
        } while (!(m->pins & SYNC));
    // printf("Test: %s\n", name);
        // Check for final values
        if (m->pc != t->pc_) {
            printf("Error: Actual PC: %04X | Final PC: %04X\n", m->pc, t->pc_);
            match = 0;
        }
        if (m->a != t->a_) {
            printf("Error: Actual A: %02X | Final A: %02X\n", m->a, t->a_);
            match = 0;
        }
        if (m->x != t->x_) {
            printf("Error: Actual X: %02X | Final X: %02X\n", m->x, t->x_);
            match = 0;
        }
        if (m->y != t->y_) {
            printf("Error: Actual Y: %02X | Final Y: %02X\n", m->y, t->y_);
            match = 0;
        }
        if (m->s != t->s_) {
            printf("Error: Actual S: %02X | Final S: %02X\n", m->s, t->s_);
            match = 0;
        }
        if (m->p != t->p_) {
            printf("Error: Actual P: %02X | Final P: %02X\n", m->p, t->p_);
            match = 0;
        }

        json_t *fram = json_object_get(final, "ram");
        if (json_is_array(fram)) {
            size_t fram_size = json_array_size(fram);
            for (size_t k = 0; k < fram_size; k++) {
                json_t *entry = json_array_get(fram, k);
                if (json_is_array(entry) && json_array_size(entry) == 2) {
                    uint16_t expected_address = json_integer_value(json_array_get(entry, 0));
                    uint8_t expected_data = json_integer_value(json_array_get(entry, 1));
                    uint8_t actual_data = m->ram[expected_address];

                    if (actual_data != expected_data) {
                        printf("Error: RAM mismatch at %04X. Actual: %02X, Expected: %02X\n", expected_address, actual_data, expected_data);
                        match = 0;
                    }
                }
            }
        }
        if (match) {
            passed++;
        } else {
            failed++;
        }
    }
    printf("Tests for %02X (%s) -> passed: %zu, failed %zu\n", m->ir, m6502_opcodes[m->ir].fmt, passed, failed);
    json_decref(root);
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
    do { m65xx_run(m); } while (!(m->pins & SYNC));

    if(m->pc == 0x45C0) {
      if(rb(m, 0x0210) == 0xFF) {
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
    do { m65xx_run(m); } while (!(m->pins & SYNC));

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
}

static int m6502_timing_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000); 
  load_file(m, "tests/timingtest-1.bin", 0x1000);

  m65xx_init(m);  

  set_abus(m, m->pc = 0x1000);

  size_t expected_cyc = 1141LU;
  while (true) {
    do { m65xx_run(m); } while (!(m->pins & SYNC));  

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

  while (true) {
    do { m65xx_run(m); } while (!(m->pins & SYNC));  
    if (m->pc == 0x024b) {
      printf("%s", m->a == 0 ? "6502 Decimal test passed!\n" : "6502 decimal test failed!\n");
      break;
    }
  }
  return 0;
}

void m6502_interrupt_handler(m65xx_t* const m) {
  if ((m->inte & 0x2) == 0x2) {
    m->nmi_ = 1;
    m->inte &= ~0x2;
    }
  else if (!(m->p & IDF) && (m->inte & 0x1) == 0x1) {
    m->irq_ = 1;
    m->inte &= ~0x1;
  }
}

static int m6502_interrupt_test(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);
  load_file(m, "tests/6502_interrupt_test.bin", 0xA);

  m65xx_init(m);

  uint16_t pc_ = 0;
  set_abus(m, m->pc = 0x400);

  wb(m, 0xBFFC, 0);
  
  while (true) {
    do { m65xx_run(m); } while (!(m->pins & SYNC));

    m->inte = rb(m, 0xBFFC);
    m6502_interrupt_handler(m);
    wb(m, 0xBFFC, m->inte);
    
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
  json_tests_t t;

  clock_t start = clock();

  printf("Starting 6502 test...\n");

  json_tests(&m, &t, "tests/6502/v1/00.json");
  // allsuiteasm(&m);
  // m6502_functional_test(&m);
  // m6502_timing_test(&m);
  // m6502_decimal_test(&m);
  // m6502_interrupt_test(&m);

  clock_t end = clock();

  double time = (double)(end - start)/ CLOCKS_PER_SEC;
  printf("Tests completed in %.4f\n", time);
  return 0;
}

