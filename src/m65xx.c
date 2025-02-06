#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "m65xx.h"


/*
 * NOTES> When creating a poll function for IRQ and NMI, create a list of opcodes that will either execute NMI or IRQ. NMI only occurs with implied instructions?????? idk i must check
 */


/*
 *
 *
 * Helper functions
 *
 *
*/ 

static inline void on(m65xx_t* const m, uint64_t pin) { m->pins |= pin; }
static inline void off(m65xx_t* const m, uint64_t pin) { m->pins &= ~pin; }

static inline void set_abus(m65xx_t* const m, uint16_t addr) { 
  m->pins = (m->pins & ~0xFFFFULL) | (addr & 0xFFFF);
}
static inline void set_dbus(m65xx_t* const m, uint8_t data) {
  m->pins = (m->pins & ~0xFF0000ULL) | ((data & 0xFF) << 16);
}
static inline void set_abus_dbus(m65xx_t* const m, uint16_t addr, uint8_t data) {
  m->pins = (m->pins & ~0xFFFFFFULL) | (addr & 0xFFFF) | ((data & 0xFF) << 16);
}
static inline uint16_t get_abus(m65xx_t* const m) { return (m->pins & 0xFFFF); }
static inline uint8_t get_dbus(m65xx_t* const m) { return ((m->pins & 0xFF0000) >> 16); }

static inline bool ret_cf(m65xx_t* const m) { return (m->p & CF); } // Returns carry flag
static inline bool ret_df(m65xx_t* const m) { return (m->p & DF); } // Returns decimal flag 

static inline void set_nz(m65xx_t* const m, uint8_t data) {
  if(data >> 7) { m->p |= NF; } else { m->p &= ~NF; }
  if(data == 0) { m->p |= ZF; } else { m->p &= ~ZF; }
} 

static inline void m6502_fetch(m65xx_t* const m) {
  on(m, SYNC);
  set_abus(m, m->pc);
}

/*
 *
 *
 * Addressing modes
 *
 *
*/ 

static inline void impl(m65xx_t* const m) {
  switch (m->tcu) {
    case 2:
      m6502_opcode_table[m->ir].instr(m); 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for implied\n");
      break;
  }
}
static inline void accu(m65xx_t* const m) {
  switch (m->tcu) {
    case 2:
      set_dbus(m, m->a);
      m6502_opcode_table[m->ir].instr(m); 

      // Execute Instruction 
      m->a = get_dbus(m);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for accumulator\n");
      break;
  }
}
static inline void imme(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      // Execute instruction
      m6502_opcode_table[m->ir].instr(m); 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for immediate\n");
      break;
  }
}
static inline void rela(m65xx_t* const m);


static inline void zpgr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, get_dbus(m));
      break;
    case 3:
      // Execute instruction
      m6502_opcode_table[m->ir].instr(m); 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage read\n");
      break;
  }
}
static inline void zpgw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, get_dbus(m));
      break;
    case 3:
      // Execute instruction
      off(m, RW);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage write\n");
      break;
  }
}
static inline void zpgm(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, get_dbus(m));
      break;
    case 3:
      // Reads from memory 
      break;
    case 4:
      // Dummy write
      off(m, RW);
      break;
    case 5:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 


      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage read-modify-write\n");
      break; 
  }
}

static inline void zpxr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, m->adl = get_dbus(m));
      break;
    case 3:
      m->adl = (m->adl + m->x) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage, x read\n");
      break; 
  }
}
static inline void zpxw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, m->adl = get_dbus(m));
      break;
    case 3:
      m->adl = (m->adl + m->x) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      off(m, RW);
      // Execute instruction 
      
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage, x write\n");
      break; 
  }
}
static inline void zpxm(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, m->adl = get_dbus(m));
      break;
    case 3:
      m->adl = (m->adl + m->x) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      off(m, RW);
      break;
    case 5:
      off(m, RW);
      // Execute instruction
      break;
    case 6:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage, x read-modify-write\n");
      break; 
  }
}

static inline void zpyr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, m->adl = get_dbus(m));
      break;
    case 3:
      m->adl = (m->adl + m->y) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage, y read\n");
      break; 
  }
}
static inline void zpyw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, m->adl = get_dbus(m));
      break;
    case 3:
      m->adl = (m->adl + m->y) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      off(m, RW);
      // Execute instruction 
      
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for zeropage, y write\n");
      break; 
  }
}

static inline void absr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      set_abus(m, m->ad);
      break;
    case 4:
      // Execute instruction

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute read\n");
      break; 
  }
}
static inline void absw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      set_abus(m, m->ad);
      break;
    case 4:
      // Execute instruction

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute write\n");
      break; 
  }
}
static inline void absm(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      set_abus(m, m->ad);
      break;
    case 4:
      off(m, RW);
      break;
    case 5:
      off(m, RW);
      // Execute instruction 
      break;
    case 6:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute read-modify-write\n");
      break; 
  }
}

static inline void abxr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      if(m->adh != ((m->ad + m->x) >> 8)) {
        m->cpu_clock++;
        set_abus(m, m->adl);
        break;
      }
      set_abus(m, (m->adh << 8) | ((m->adl + m->x) & 0xFF));
      break;
    case 4:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute, x read\n");
      break; 
  }
}
static inline void abxw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      set_abus(m, (m->adh << 8) | ((m->adl + m->x) & 0xFF));
      break;
    case 4:
      set_abus(m, m->ad + m->x);
      break;
    case 5:
      off(m, RW);
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute, x write\n");
      break; 
  }
}
static inline void abxm(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      break;
    case 4:
      set_abus(m, (m->adh << 8) | ((m->adl + m->x) & 0xFF));
      break;
    case 5:
      set_abus(m, m->ad + m->x);
      break;
    case 6:
      off(m, RW);
      break;
    case 7:
      off(m, RW);
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);      
      break;
    default:
      printf("Error: invalid cycle count for absolute, x read-modify-write\n");
      break; 
  }
}

static inline void abyr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      if(m->adh != ((m->ad + m->y) >> 8)) {
        m->cpu_clock++;
        set_abus(m, m->adl);
        break;
      }
      set_abus(m, (m->adh << 8) | ((m->adl + m->y) & 0xFF));
      break;
    case 4:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute, y read\n");
      break; 
  }
}
static inline void abyw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 3:
      m->adh = get_dbus(m);
      set_abus(m, (m->adh << 8) | ((m->adl + m->y) & 0xFF));
      break;
    case 4:
      set_abus(m, m->ad + m->y);
      break;
    case 5:
      off(m, RW);
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute, y write\n");
      break; 
  }
}

static inline void idxr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->adl);
      break;
    case 3:
      m->adl = (m->adl + m->x) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      set_abus(m, (m->adl + 1) & 0xFF);
      m->adl = get_dbus(m);
      break;
    case 5:
      m->adh = get_dbus(m);
      set_abus(m, m->ad);
      break;
    case 6:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, x read\n");
      break; 
  }
}
static inline void idxw(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->adl);
      break;
    case 3:
      m->adl = (m->adl + m->x) & 0xFF;
      set_abus(m, m->adl);
      break;
    case 4:
      set_abus(m, (m->adl + 1) & 0xFF);
      m->adl = get_dbus(m);
      break;
    case 5:
      m->adh = get_dbus(m);
      set_abus(m, m->ad);
      break;
    case 6:
      off(m, RW);
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, x write\n");
      break; 
  }
}

static inline void idyr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->adl);
      break;
    case 3:
      set_abus(m, (m->adl + 1) & 0xFF);
      m->adl = get_dbus(m);
      break;
    case 4:
      m->adh = get_dbus(m);
      if(m->adh != ((m->ad + m->y) >> 8)) {
        m->cpu_clock++;
        set_abus(m, m->ad + m->y);
        break;
      }
      set_abus(m, (m->adh << 8) | ((m->adl + m->y) & 0xFF));
      break;
    case 5:
      // Execute instruction 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, y read\n");
      break; 
  }
}
static inline void idyw(m65xx_t* const m) {
  switch (m->tcu) {  
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, m->adl);
      break;
    case 3:
      set_abus(m, (m->adl + 1) & 0xFF);
      m->adl = get_dbus(m);
      break;
    case 4:
      m->adh = get_dbus(m);
      set_abus(m, (m->adh << 8) | ((m->adl + m->y) & 0xFF));
      break;
    case 5:
      set_abus(m, m->ad + m->y);

      off(m, RW);
      // Execute instruction
      break;
    case 6:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, y write\n");
      break; 
  }
}



/*
 *
 *
 * Instructions
 *
 *
*/ 


// Interrupts


static inline void brkk(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->pch);
      break;
    case 3:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->pcl);
      break;
    case 4:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->p);
      break;
    case 5:
      m->p |= IDF;
      set_abus(m, 0xFFFE);
      break;
    case 6:
      m->pcl = get_dbus(m);
      set_abus(m, 0xFFFF);
      break;
    case 7:
      m->pch = get_dbus(m);
      set_abus(m, m->pc);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for brk\n");
      break;
  }
}

static inline void nmi(m65xx_t* const m);
static inline void irq(m65xx_t* const m);
static inline void res(m65xx_t* const m);



// Legal instructions 


// Load, Store, Transfer

static inline void lda(m65xx_t* const m) {
  set_nz(m, m->a = get_dbus(m));
}
static inline void ldx(m65xx_t* const m) {
  set_nz(m, m->x = get_dbus(m));
}
static inline void ldy(m65xx_t* const m) {
  set_nz(m, m->y = get_dbus(m));
}

static inline void sta(m65xx_t* const m) {
  set_dbus(m, m->a);
}
static inline void stx(m65xx_t* const m) {
  set_dbus(m, m->x);
}
static inline void sty(m65xx_t* const m) {
  set_dbus(m, m->y);
}

static inline void tax(m65xx_t* const m) {
  set_nz(m, m->x = m->a);
}
static inline void tay(m65xx_t* const m) {
  set_nz(m, m->y = m->a);
}
static inline void tsx(m65xx_t* const m) {
  set_nz(m, m->x = m->s);
}
static inline void txa(m65xx_t* const m) {
  set_nz(m, m->a = m->x);
}
static inline void txs(m65xx_t* const m) {
  m->s = m->x;
}
static inline void tya(m65xx_t* const m) {
  set_nz(m, m->a = m->y);
}


// Arithmethic, Increment/Decrement, Compare instructions

static inline void adc(m65xx_t* const m) {
  if(m->p & DF) {

  }
  else {
    uint16_t i_res = m->a + get_dbus(m); + (m->p & 1);
  }
}
static inline void sbc(m65xx_t* const m);



// Illegal/Undocumented instructions


static inline void jam(m65xx_t* const m) {
  m->pc--;
}
m65xx_opcodes_t m6502_opcode_table[0x100] = {
  [0x00] = { .mode = brkk, .instr = impl },
  // ...
  [0xA9] = { .mode = imme, .instr = lda },
};


void m65xx_init(m65xx_t* const m) {
  memset(m->ram, 0, 0x10000);
  m->pins = 0;
  on(m, (SYNC | RW));
  m->a = m->x = m->y = m->s = m->p = m->tcu = 0;
  m->ir = 0x00;
  m->p |= 0x20;
  m->pc = m->ad = 0;
  m->pcl = m->ram[0xFFFC];
  m->pch = m->ram[0xFFFD];
}
void m65xx_on(m65xx_t* const m);
void m65xx_reset(m65xx_t* const m);

static inline void m65xx_tick(m65xx_t* const m) {
  on(m, RW);
  // Check whether if Interrupt might occur
  if(m->pins & SYNC) {
    // m->ir = get_dbus(m);
    off(m, SYNC);
    m->pc++;
  }
  m->tcu += 1;
  // Call instruction/addressing mode 
  m6502_opcode_table[m->ir].mode(m);
}

static inline uint8_t rb(m65xx_t* const m, uint16_t addr) {
  return m->ram[addr];
}
static inline void wb(m65xx_t* const m, uint16_t addr, uint8_t data) {
  m->ram[addr] = data;
}

void m65xx_run(m65xx_t* const m) {
  if (m->pins & RW) {  // Read 
    set_dbus(m, rb(m, get_abus(m)));
  }
  m65xx_tick(m);

  if (!(m->pins & RW)) {  // Write  
    wb(m, get_abus(m), get_dbus(m));    
  }
}

