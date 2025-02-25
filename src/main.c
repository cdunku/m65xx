#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <jansson.h>
#include <SDL2/SDL.h>

#include "m65xx.h"

void m65xx_init(m65xx_t* const m);

typedef struct {
  uint16_t pc_;
  uint8_t a_, x_, y_, s_, p_;
} tomharte_t;

void load_tomharte(m65xx_t* const m, tomharte_t* const t, const char *file) {
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error: Failed to parse %s (%s at line %d)\n", file, error.text, error.line);
        return;
    }

    size_t num_tests = json_array_size(root);
    if (num_tests == 0) {
        fprintf(stderr, "Error: No test cases found in %s\n", file);
        json_decref(root);
        return;
    }

    printf("Starting 6502 test...\n");

    size_t passed = 0, failed = 0;

    for (size_t i = 0; i < num_tests; i++) {
        json_t *test_case = json_array_get(root, i);
        if (!test_case) continue;

        const char *name = json_string_value(json_object_get(test_case, "name"));
        printf("Running test: %s\n", name ? name : "(unnamed)");

        json_t *initial = json_object_get(test_case, "initial");
        json_t *final = json_object_get(test_case, "final");
        if (!initial || !final) {
            fprintf(stderr, "Error: Missing 'initial' or 'final' state in test %s\n", name);
            failed++;
            continue;
        }

        m65xx_init(m);

        // Load initial CPU state
        m->pc = json_integer_value(json_object_get(initial, "pc"));
        m->s  = json_integer_value(json_object_get(initial, "s"));
        m->a  = json_integer_value(json_object_get(initial, "a"));
        m->x  = json_integer_value(json_object_get(initial, "x"));
        m->y  = json_integer_value(json_object_get(initial, "y"));
        m->p  = json_integer_value(json_object_get(initial, "p"));

        // Expected final state
        t->pc_ = json_integer_value(json_object_get(final, "pc"));
        t->s_  = json_integer_value(json_object_get(final, "s"));
        t->a_  = json_integer_value(json_object_get(final, "a"));
        t->x_  = json_integer_value(json_object_get(final, "x"));
        t->y_  = json_integer_value(json_object_get(final, "y"));
        t->p_  = json_integer_value(json_object_get(final, "p"));

        // Load RAM
        json_t *ram = json_object_get(initial, "ram");
        if (json_is_array(ram)) {
            size_t j;
            for (j = 0; j < json_array_size(ram); j++) {
                json_t *entry = json_array_get(ram, j);
                if (json_is_array(entry) && json_array_size(entry) == 2) {
                    uint16_t addr = json_integer_value(json_array_get(entry, 0));
                    uint8_t data  = json_integer_value(json_array_get(entry, 1));
                    m->ram[addr] = data;
                }
            }
        }

        // Execute one CPU cycle
        do { m65xx_run(m); } while (!(m->pins & SYNC));

        // Compare final CPU state
        int match = 1;
        if (m->pc != t->pc_) { printf("  PC mismatch: expected %04X, got %04X\n", t->pc_, m->pc); match = 0; }
        if (m->a  != t->a_)  { printf("  A mismatch: expected %02X, got %02X\n", t->a_, m->a); match = 0; }
        if (m->x  != t->x_)  { printf("  X mismatch: expected %02X, got %02X\n", t->x_, m->x); match = 0; }
        if (m->y  != t->y_)  { printf("  Y mismatch: expected %02X, got %02X\n", t->y_, m->y); match = 0; }
        if (m->s  != t->s_)  { printf("  S mismatch: expected %02X, got %02X\n", t->s_, m->s); match = 0; }
        if (m->p  != t->p_)  { printf("  P mismatch: expected %02X, got %02X\n", t->p_, m->p); match = 0; }

        // Compare final RAM state
        json_t *final_ram = json_object_get(final, "ram");
        if (json_is_array(final_ram)) {
            size_t j;
            for (j = 0; j < json_array_size(final_ram); j++) {
                json_t *entry = json_array_get(final_ram, j);
                if (json_is_array(entry) && json_array_size(entry) == 2) {
                    uint16_t addr = json_integer_value(json_array_get(entry, 0));
                    uint8_t expected_value = json_integer_value(json_array_get(entry, 1));
                    uint8_t actual_value = m->ram[addr];

                    if (actual_value != expected_value) {
                        printf("  RAM mismatch at %04X: expected %02X, got %02X\n", 
                               addr, expected_value, actual_value);
                        match = 0;
                    }
                }
            }
        }

        if (match) {
            passed++;
        } else {
            printf("Test failed: %s\n", name);
            failed++;
        }
    }

    // Print final results
    printf("\nTest completed! Passed: %zu, Failed: %zu\n", passed, failed);
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

static inline uint8_t rb(m65xx_t* const m, uint16_t addr) {
  return m->ram[addr];
}
static inline void wb(m65xx_t* const m, uint16_t addr, uint8_t data) {
  m->ram[addr] = data;
}

void m65xx_init(m65xx_t* const m) {
//  memset(m->ram, 0, 0x10000);
  m->pins = 0;
  m->pins |= (SYNC | RW);
  m->a = m->x = m->y = m->s = m->p = m->tcu = 0;
  m->ir = 0xcb;
  m->p |= 0x20;
  m->ad = 0;
  m->pcl = rb(m, 0xFFFC);
  m->pch = rb(m, 0xFFFD);
  m->bra = 0;

  m->halt = 0;
}



static int allsuiteasm(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);

  load_file(m, "tests/AllSuiteA.bin", 0x4000);

  m65xx_init(m);

  while(true) {
    do { m65xx_run(m); } while (!(m->pins & SYNC));

    if(m->pc == 0x45C0) {
      if(rb(m, 0x210) == 0xFF) {
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

int main(void) {
    m65xx_t m;
    tomharte_t t;

    printf("Starting 6502 test...\n");
    // load_tomharte(&m, &t, "tests/6502/v1/cb.json");

    allsuiteasm(&m);
    printf("Test completed!\n");
    return 0;
}

