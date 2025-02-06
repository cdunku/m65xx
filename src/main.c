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
/*
void load_tomharte(m65xx_t* const m, tomharte_t* const t, const char *file) {
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error: Failed to parse %s (%s at line %d)\n", file, error.text, error.line);
        return;
    }

    json_t *test_case = json_array_get(root, 0); // Load first test case
    if (!test_case) {
        fprintf(stderr, "Error: No test case found in %s\n", file);
        json_decref(root);
        return;
    }

    const char *name = json_string_value(json_object_get(test_case, "name"));
    printf("Running test: %s\n", name);

    json_t *initial = json_object_get(test_case, "initial");
    json_t *final = json_object_get(test_case, "final");
    if (!initial || !final) {
        fprintf(stderr, "Error: Missing 'initial' or 'final' state\n");
        json_decref(root);
        return;
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
        size_t i;
        for (i = 0; i < json_array_size(ram); i++) {
            json_t *entry = json_array_get(ram, i);
            if (json_is_array(entry) && json_array_size(entry) == 2) {
                uint16_t addr = json_integer_value(json_array_get(entry, 0));
                uint8_t data  = json_integer_value(json_array_get(entry, 1));
                m->ram[addr] = data;
            }
        }
    }

    // Execute one CPU cycle
    do { m65xx_run(m); }while(!(m->pins & SYNC));

    // Compare final state
    if (m->pc != t->pc_ || m->a != t->a_ || m->x != t->x_ || m->y != t->y_ || m->s != t->s_ || m->p != t->p_) {
        printf("Test failed: %s\n", name);
        if (m->pc != t->pc_) printf("  PC mismatch: expected %04X, got %04X\n", t->pc_, m->pc);
        if (m->a  != t->a_)  printf("  A mismatch: expected %02X, got %02X\n", t->a_, m->a);
        if (m->x  != t->x_)  printf("  X mismatch: expected %02X, got %02X\n", t->x_, m->x);
        if (m->y  != t->y_)  printf("  Y mismatch: expected %02X, got %02X\n", t->y_, m->y);
        if (m->s  != t->s_)  printf("  S mismatch: expected %02X, got %02X\n", t->s_, m->s);
        if (m->p  != t->p_)  printf("  P mismatch: expected %02X, got %02X\n", t->p_, m->p);
    } else {
        printf("Test passed: %s\n", name);
    }

    json_decref(root);
}
*/ 
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
        printf("Running test: %s\n", name);

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

        // Compare final state
        if (m->pc != t->pc_ || m->a != t->a_ || m->x != t->x_ || m->y != t->y_ || m->s != t->s_ || m->p != t->p_) {
            printf("Test failed: %s\n", name);
            if (m->pc != t->pc_) printf("  PC mismatch: expected %04X, got %04X\n", t->pc_, m->pc);
            if (m->a  != t->a_)  printf("  A mismatch: expected %02X, got %02X\n", t->a_, m->a);
            if (m->x  != t->x_)  printf("  X mismatch: expected %02X, got %02X\n", t->x_, m->x);
            if (m->y  != t->y_)  printf("  Y mismatch: expected %02X, got %02X\n", t->y_, m->y);
            if (m->s  != t->s_)  printf("  S mismatch: expected %02X, got %02X\n", t->s_, m->s);
            if (m->p  != t->p_)  printf("  P mismatch: expected %02X, got %02X\n", t->p_, m->p);
            failed++;
        } else {
            printf("Test passed: %s\n", name);
            passed++;
        }
    }

    printf("\nTest completed! Passed: %zu, Failed: %zu\n", passed, failed);

    json_decref(root);
}

int main(void) {
    m65xx_t m;
    tomharte_t t;

    printf("Starting 6502 test...\n");
    load_tomharte(&m, &t, "tests/6502/v1/00.json");

    printf("Test completed!\n");
    return 0;
}
