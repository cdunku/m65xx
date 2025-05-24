#pragma once

typedef enum {
  AM_IMPLIED,
  AM_IMMEDIATE,
  AM_ZERO_PAGE,
  AM_ZERO_PAGE_X,
  AM_ZERO_PAGE_Y,
  AM_ABSOLUTE,
  AM_ABSOLUTE_X,
  AM_ABSOLUTE_Y,
  AM_INDIRECT,          
  AM_INDEXED_INDIRECT,   
  AM_INDIRECT_INDEXED,   
  AM_RELATIVE            
} addressing_mode_t;

typedef struct { const char *fmt; addressing_mode_t mode; uint8_t size; } opcode_info_t;
extern const opcode_info_t m6502_opcodes[0x103];

void m6502_print(m65xx_t* const m);
