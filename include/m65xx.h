#pragma once

#include <stdint.h>
#include <stdbool.h>

enum {
  // Pins of the control bus
  // ADDR_PINS, - 0th-15th bit
  DATA_PINS = 16, 
  RDY_PIN = 24,
  IRQ_PIN,
  NMI_PIN,
  SYNC_PIN,
  RES_PIN,
  RW_PIN,
  M6510_AEC_PIN,
  M6510_P0_PIN,
  M6510_P1_PIN,
  M6510_P2_PIN,
  M6510_P3_PIN,
  M6510_P4_PIN,
  M6510_P5_PIN,
};

static const uint64_t RDY  = (1ULL << RDY_PIN);
static const uint64_t IRQ  = (1ULL << IRQ_PIN);
static const uint64_t NMI  = (1ULL << NMI_PIN);
static const uint64_t SYNC = (1ULL << SYNC_PIN);
static const uint64_t RES  = (1ULL << RES_PIN);
static const uint64_t RW   = (1ULL << RW_PIN);

// 6510 pin group
static const uint64_t AEC  = (1Ull << M6510_AEC_PIN);
static const uint64_t P0   = (1ULL << M6510_P0_PIN);
static const uint64_t P1   = (1ULL << M6510_P1_PIN);
static const uint64_t P2   = (1ULL << M6510_P2_PIN);
static const uint64_t P3   = (1ULL << M6510_P3_PIN);
static const uint64_t P4   = (1ULL << M6510_P4_PIN);
static const uint64_t P5   = (1ULL << M6510_P5_PIN);


static const uint8_t NF  = (1 << 7);
static const uint8_t VF  = (1 << 6);
static const uint8_t BF  = (1 << 4);
static const uint8_t DF  = (1 << 3);
static const uint8_t IDF = (1 << 2);
static const uint8_t ZF  = (1 << 1);
static const uint8_t CF  = (1 << 0);

static const uint16_t RES_OPCODE = 0x100;
static const uint16_t NMI_OPCODE = 0x101;
static const uint16_t IRQ_OPCODE = 0x102;

typedef struct {
  uint8_t ram[0x10000];
  uint64_t pins;
  uint8_t a, x, y, s, p, tcu;
  uint16_t ir;
  union { struct { uint8_t pcl; uint8_t pch; }; uint16_t pc; };
  union { struct { uint8_t adl; uint8_t adh; }; uint16_t ad; };
  bool bra;
  uint64_t cpu_clock;

  // nmi_edge holds the edge case value, nmi_ executes a non-maskable interrupt.
  bool nmi_edge, nmi_, irq_;
} m65xx_t;

typedef struct { void (*mode)(m65xx_t*); void (*instr)(m65xx_t*); } m65xx_opcodes_t;
extern m65xx_opcodes_t m6502_opcode_table[0x103];

void m65xx_tick(m65xx_t* const m);
void m65xx_init(m65xx_t* const m);

