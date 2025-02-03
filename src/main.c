#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <jansson.h>
#include <SDL2/SDL.h>

#include "m65xx.h"


typedef struct {
  uint16_t pc_;
  uint8_t a_, x_, y_, s_, p_;
} tomharte_t;

void load_tomharte(m65xx_t* const m, tomharte_t* const t, const char *file) {
  json_error_t error;
  json_t *root = json_load_file(file, 0, &error);
  if (!root) {
    fprintf(stderr, "Error: %s has not been parsed\n", file);
    return;
  }
    size_t index;
    json_t *test_case;
    json_array_foreach(root, index, test_case) {
        const char *name = json_string_value(json_object_get(test_case, "name"));
        printf("Processing test case: %s\n", name);

        // Extract 'initial' and 'final' states
        json_t *initial = json_object_get(test_case, "initial");
        json_t *final = json_object_get(test_case, "final");

        if (!initial || !final) {
            fprintf(stderr, "Error: 'initial' or 'final' state is missing in test case %zu\n", index);
            continue;
        }
  // Load initial state
  m->pc = json_integer_value(json_object_get(initial, "pc"));
  m->s  = json_integer_value(json_object_get(initial, "s"));
  m->a  = json_integer_value(json_object_get(initial, "a"));
  m->x  = json_integer_value(json_object_get(initial, "x"));
  m->y  = json_integer_value(json_object_get(initial, "y"));
  m->p  = json_integer_value(json_object_get(initial, "p"));

  // Load final state for comparison
  t->pc_ = json_integer_value(json_object_get(final, "pc"));
  t->s_  = json_integer_value(json_object_get(final, "s"));
  t->a_  = json_integer_value(json_object_get(final, "a"));
  t->x_  = json_integer_value(json_object_get(final, "x"));
  t->y_  = json_integer_value(json_object_get(final, "y"));
  t->p_  = json_integer_value(json_object_get(final, "p"));

  // Load RAM data
  json_t *ram = json_object_get(initial, "ram");
  if (json_is_array(ram)) {
    size_t i;
    for (i = 0; i < json_array_size(ram); i++) {
      json_t *entry = json_array_get(ram, i);
      if (json_is_array(entry) && json_array_size(entry) == 2) {
        uint16_t addr = json_integer_value(json_array_get(entry, 0));
        uint8_t data  = json_integer_value(json_array_get(entry, 1));
        m->ram[addr] = data;
      }
      // json_array_get() does not increase reference count, no need to json_decref(entry)
    }
  }

  m65xx_run(m);

  if(m->pins & SYNC) {
  // Compare CPU states
  if (m->pc != t->pc_) {
    printf("PC mismatch: expected %04X, got %04X\n", t->pc_, m->pc);
  }
  if (m->a != t->a_) {
    printf("A mismatch: expected %02X, got %02X\n", t->a_, m->a);
  }
  if (m->x != t->x_) {
    printf("X mismatch: expected %02X, got %02X\n", t->x_, m->x);
  }
  if (m->y != t->y_) { 
    printf("Y mismatch: expected %02X, got %02X\n", t->y_, m->y);
  }
  if (m->s != t->s_) {
    printf("S mismatch: expected %02X, got %02X\n", t->s_, m->s);
  }
  if (m->p != t->p_) {
    printf("P mismatch: expected %02X, got %02X\n", t->p_, m->p);
  }
  printf("\n\n");
    }
  }
  // Free the root object (and all children)
  json_decref(root);
}


int main(int argc, char **argv) {

  m65xx_t m;
  tomharte_t t;
  m65xx_init(&m);
  load_tomharte(&m, &t, "tests/6502/v1/a9.json");

  return 0;
}
