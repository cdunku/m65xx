#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "m65xx.h"

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

static inline void set_nz(m65xx_t* const m, uint8_t data) {
  if((data & 0x80) >> 7) { m->p |= NF; } else { m->p &= ~NF; }
  if(data == 0) { m->p |= ZF; } else { m->p &= ~ZF; }
} 

static inline void set_p(m65xx_t* const m, uint8_t data) {
  if(data & 0x80) { m->p |= NF; } else { m->p &= ~NF; }
  if(data & 0x40) { m->p |= VF; } else { m->p &= ~VF; }
  if(data & 0x08) { m->p |= DF; } else { m->p &= ~DF; }
  if(data & 0x04) { m->p |= IDF; } else { m->p &= ~IDF; }
  if(data & 0x02) { m->p |= ZF; } else { m->p &= ~ZF; }
  if(data & 0x01) { m->p |= CF; } else { m->p &= ~CF; }
}

static inline void m6502_fetch(m65xx_t* const m) {
  set_abus(m, m->pc);
  on(m, SYNC);
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
    case 1:
      set_abus(m, m->pc);
      break;
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
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      set_dbus(m, m->a);
      m6502_opcode_table[m->ir].instr(m); 
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
      m6502_opcode_table[m->ir].instr(m); 

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for immediate\n");
      break;
  }
}
static inline void rela(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      m6502_opcode_table[m->ir].instr(m);
      set_abus(m, m->pc++);
      if(!m->bra) { m->ad = m->pc; m->tcu += 2; }
      break;
    case 2:
      uint8_t data = get_dbus(m);
      m->ad = (uint16_t)(m->pc + (int8_t)data);
      set_abus(m, m->pc);
      if(m->adh == m->pch) { m->tcu++; }
      break;
    case 3:
      set_abus(m, (m->pch << 8) | m->adl);
      break;
    case 4:
      set_abus(m, m->pc = m->ad);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for relative\n");
      break;
  }
}


static inline void zpgr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, get_dbus(m));
      break;
    case 3:
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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 3:
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
      off(m, RW);
      break;
    case 4:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 5:
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
      m6502_opcode_table[m->ir].instr(m); 

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);
      break;
    case 4:
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
      m6502_opcode_table[m->ir].instr(m); 
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
      m6502_opcode_table[m->ir].instr(m); 

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);  
      break;
    case 4:
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
      m6502_opcode_table[m->ir].instr(m); 

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 4:
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
      m6502_opcode_table[m->ir].instr(m); 
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
      set_abus(m, (m->adh << 8) | ((m->adl + m->x) & 0xFF));
      if (~(m->adh - ((m->ad + m->x) >> 8)) & 0x1) { m->tcu++; break; }
      break;
    case 4:
      set_abus(m, m->ad + m->x);
      break;
    case 5:
      m6502_opcode_table[m->ir].instr(m); 

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 5:
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
      set_abus(m, (m->adh << 8) | ((m->adl + m->x) & 0xFF));
      break;
    case 4:
      set_abus(m, (m->ad + m->x) & 0xFFFF);
      break;
    case 5:
      off(m, RW);
      break;
    case 6:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 7:
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

      set_abus(m, (m->adh << 8) | ((m->adl + m->y) & 0xFF));
      if (~(m->adh - ((m->ad + m->y) >> 8)) & 0x1) { m->tcu++; break; }
      break;
    case 4:
      set_abus(m, m->ad + m->y);
      break;
    case 5:
      m6502_opcode_table[m->ir].instr(m); 

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m); 
      break;
    case 5:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute, y write\n");
      break; 
  }
}
static inline void abym(m65xx_t* const m) {
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
      break;
    case 6:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);
      break;
    case 7:
      m->tcu = 0;
      m6502_fetch(m);      
      break;
    default:
      printf("Error: invalid cycle count for absolute, y read-modify-write\n");
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
      m6502_opcode_table[m->ir].instr(m);  

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

      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);  
      break;
    case 6:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, x write\n");
      break; 
  }
}
static inline void idxm(m65xx_t* const m) {
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
      break;
    case 7:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);  
      break;
    case 8:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, x read-modify-write\n");
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
      if (~(m->adh - ((m->ad + m->y) >> 8)) & 0x1) { 
        m->tcu++;       
        set_abus(m, m->ad + m->y);
        break; 
      }
      set_abus(m, (m->ad & 0xFF00) | ((m->ad + m->y) & 0xFF));
      break;
    case 5:
      set_abus(m, m->ad + m->y);
      break;
    case 6:
      m6502_opcode_table[m->ir].instr(m); 
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
      m6502_opcode_table[m->ir].instr(m); 
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
static inline void idym(m65xx_t* const m) {
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
      break;
    case 6:
      off(m, RW);
      break;
    case 7:
      off(m, RW);
      m6502_opcode_table[m->ir].instr(m);  
      break;
    case 8:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect, y read-modify-write\n");
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

static inline void brk(m65xx_t* const m) {
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
      set_abus_dbus(m, 0x100 | m->s--, m->p | BF);
      break;
    case 5:
      set_abus(m, 0xFFFE);
      break;
    case 6:
      m->p |= IDF;

      m->pcl = get_dbus(m);
      set_abus(m, 0xFFFF);
      break;
    case 7:
      m->pch = get_dbus(m);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for brk\n");
      break;
  }
}
static inline void nmi(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
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
      set_abus_dbus(m, 0x100 | m->s--, m->p & ~BF);
      break;
    case 5:
      m->p |= IDF;
      set_abus(m, 0xFFFA);
      break;
    case 6:
      m->pcl = get_dbus(m);
      set_abus(m, 0xFFFB);
      break;
    case 7:
      m->pch = get_dbus(m);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for nmi\n");
      break;
  }
}
static inline void irq(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      if(m->p & IDF) { 
        m->tcu = 0;
        m6502_fetch(m);

        return;
      }
      set_abus(m, m->pc);
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
      set_abus_dbus(m, 0x100 | m->s--, m->p & ~BF);
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

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for irq\n");
      break;
  }
}
static inline void res(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      // Hold during reset
      break;
    case 2:
      // First state starts here
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      set_abus(m, 0x100 | m->s--);
      break;
    case 4:
      set_abus(m, 0x100 | m->s--);
      break;
    case 5:
      set_abus(m, 0xFFFC);
      break;
    case 6:
      m->pcl = get_dbus(m);
      set_abus(m, 0xFFFD);
      break;
    case 7:
      m->pch = get_dbus(m);
      break;
    case 8:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for res\n");
      break;
  }
}



// Legal instructions 



static inline void nop(m65xx_t* const m) { (void) *m; }


// Clear/Set flags

static inline void clc(m65xx_t* const m) { m->p &= ~CF; }
static inline void sec(m65xx_t* const m) { m->p |= CF; }
static inline void cli(m65xx_t* const m) { m->p &= ~IDF; }
static inline void sei(m65xx_t* const m) { m->p |= IDF; }
static inline void clv(m65xx_t* const m) { m->p &= ~VF; }
static inline void cld(m65xx_t* const m) { m->p &= ~DF; }
static inline void sed(m65xx_t* const m) { m->p |= DF; }


// Jump instructions 

static inline void jsr(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      m->adl = get_dbus(m);
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->pch);
      break;
    case 4:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->pcl);
      break;
    case 5:
      set_abus(m, m->pc);
      break;
    case 6:
      m->adh = get_dbus(m);
      set_abus(m, m->pc = m->ad);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for jsr\n");
      break;
  }
}
static inline void abj(m65xx_t* const m) {
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
      m->pc = m->ad;

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for absolute jump\n");
      break;
  }
}
static inline void inj(m65xx_t* const m) {
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
      set_abus(m, (m->adh << 8) | ((m->adl + 1) & 0xFF));
      m->adl = get_dbus(m);
      break;
    case 5:
      m->adh = get_dbus(m);
      m->pc = m->ad;

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for indirect jump\n");
      break;
  }
}

// Stack instructions 

static inline void php(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->p | BF);
      break;
    case 3:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for php\n");
      break;
  }
}
static inline void plp(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      set_abus(m, 0x100| ++m->s);
      break;
    case 4:
      set_p(m, get_dbus(m));

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for plp\n");
      break;
  }
}
static inline void pha(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      off(m, RW);
      set_abus_dbus(m, 0x100 | m->s--, m->a);
      break;
    case 3:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for pha\n");
      break;
  }
}
static inline void pla(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      set_abus(m, 0x100| ++m->s);
      break;
    case 4:
      m->a = get_dbus(m);
      set_nz(m, m->a);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for pla\n");
      break;
  }
}

static inline void rti(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      set_abus(m, 0x100 | ++m->s);
      break;
    case 4:
      // Sets the 5th (always set) and disables the BF flag if set when value of P is updated
      m->p = (get_dbus(m) | 0x20) & (~BF);
      set_abus(m, 0x100 | ++m->s);
      break;
    case 5:
      m->pcl = get_dbus(m);
      set_abus(m, 0x100 | ++m->s);
      break;
    case 6:
      m->pch = get_dbus(m);

      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for rti\n");
      break;
  }
}
static inline void rts(m65xx_t* const m) {
  switch (m->tcu) { 
    case 1:
      set_abus(m, m->pc++);
      break;
    case 2:
      set_abus(m, 0x100 | m->s);
      break;
    case 3:
      set_abus(m, 0x100 | ++m->s);
      break;
    case 4:
      m->pcl = get_dbus(m);
      set_abus(m, 0x100 | ++m->s);
      break;
    case 5:
      m->pch = get_dbus(m);
      set_abus(m, m->pc++);
      break;
    case 6:
      m->tcu = 0;
      m6502_fetch(m);
      break;
    default:
      printf("Error: invalid cycle count for rts\n");
      break;
  }
}


// Branch instructions 

static inline void bpl(m65xx_t* const m) {
  if(!(m->p & NF)) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bmi(m65xx_t* const m) {
  if(m->p & NF) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bvc(m65xx_t* const m) {
  if(!(m->p & VF)) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bvs(m65xx_t* const m) {
  if(m->p & VF) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bcc(m65xx_t* const m) {
  if(!(m->p & CF)) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bcs(m65xx_t* const m) {
  if((m->p & CF)) { m->bra = 1; } else { m->bra = 0; }
}
static inline void bne(m65xx_t* const m) {
  if(!(m->p & ZF)) { m->bra = 1; } else { m->bra = 0; }
}
static inline void beq(m65xx_t* const m) {
  if((m->p & ZF)) { m->bra = 1; } else { m->bra = 0; }
}
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


// Logical, Rotation instructions 

static inline void and(m65xx_t* const m) {
  m->a &= get_dbus(m);
  set_nz(m, m->a);
}
static inline void eor(m65xx_t* const m) {
  m->a ^= get_dbus(m);
  set_nz(m, m->a);
}
static inline void ora(m65xx_t* const m) {
  m->a |= get_dbus(m);
  set_nz(m, m->a);
}
static inline void bit(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  if((m->a & data) == 0) { m->p |= ZF; } else { m->p &= ~ZF; }
  if(data & 0x80) { m->p |= NF; } else { m->p &= ~NF; }
  if(data & 0x40) { m->p |= VF; } else { m->p &= ~VF; }
}

static inline void rol(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  bool cf = m->p & CF;

  if(data & 0x80) { m->p |= CF; } else { m->p &= ~CF; }

  data = ((data << 1) | cf) & 0xFF;

  if(data & 0x80) { m->p |= NF; } else { m->p &= ~NF; }
  if(data == 0) { m->p |= ZF; } else { m->p &= ~ZF; }

  set_dbus(m, data);
}
static inline void ror(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  bool cf = m->p & CF;

  if(data & 0x1) { m->p |= CF; } else { m->p &= ~CF; }

  data = (data >> 1) | (cf << 7);
  set_nz(m, data);

  set_dbus(m, data);
}
static inline void asl(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  if((data & 0x80) >> 7) { m->p |= CF; } else { m->p &= ~CF; }

  data = (data << 1) & 0xFF;
  set_nz(m, data);

  set_dbus(m, data);
}
static inline void lsr(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  if(data & 0x1) { m->p |= CF; } else { m->p &= ~CF; }

  data = (data >> 1) & 0xFF;
  set_nz(m, data);

  set_dbus(m, data);
}


// Arithmethic, Increment/Decrement, Compare instructions

static inline void adc(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  bool cf = m->p & CF;

  if(m->p & DF) { 
    uint8_t al = (m->a & 0x0F) + (data & 0x0F) + cf;
    if (al > 0x09) { al += 0x06; }

    uint8_t ah = (m->a >> 4) + (data >> 4) + (al > 0x0F);

    if(ah & 0x08) { m->p |= NF; } else { m->p &= ~NF; }
    if(~(data ^ m->a) & ((ah << 4) ^ m->a) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }

    if(ah > 0x09) { ah += 0x06; }
    if(ah > 0x0F) { m->p |= CF; } else { m->p &= ~CF; }

    if(((m->a + data + cf) & 0xFF) == 0) { m->p |= ZF; } else { m->p &= ~ZF; }

    m->a = (ah << 4) | (al & 0x0F);
  } 
  else {
    uint16_t result = m->a + data + cf;

    set_nz(m, result & 0xFF);
    if(((m->a ^ result) & (data ^ result) & 0x80) != 0) { m->p |= VF; } else { m->p &= ~VF; }
    if(result > 0xFF) { m->p |= CF; } else { m->p &= ~CF; }

    m->a = result & 0xFF;
  }
}
static inline void sbc(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  uint16_t result;
  bool cf = m->p & CF;

  if(m->p & DF) {
    result = m->a - data - !cf;

    uint8_t al = (m->a & 0x0F) - (data & 0x0F) - !cf;
    if(al & 0x80) { al -= 0x06; }

    uint8_t ah = (m->a >> 4) - (data >> 4) - (al >> 7);

    set_nz(m, result & 0xFF);
    if((m->a ^ data) & (m->a ^ result) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }
    if(!(result > 0xFF)) { m->p |= CF; } else { m->p &= ~CF; }

    if(ah & 0x80) { ah -= 0x06; }
    
    m->a = (ah << 4) | (al & 0x0F);
  }
  else {
    result = m->a - data - !cf;

    set_nz(m, result & 0xFF);
    if((m->a ^ data) & (m->a ^ result) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }
    if(!(result > 0xFF)) { m->p |= CF; } else { m->p &= ~CF; }

    m->a = result & 0xFF;
  }
}

static inline void inc(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  data++;
  set_nz(m, data);

  set_dbus(m, data);
} 
static inline void iny(m65xx_t* const m) { set_nz(m, ++m->y); }
static inline void inx(m65xx_t* const m) { set_nz(m, ++m->x); }

static inline void dec(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  data--;
  set_nz(m, data);

  set_dbus(m, data);
}
static inline void dey(m65xx_t* const m) { set_nz(m, --m->y); }
static inline void dex(m65xx_t* const m) { set_nz(m, --m->x); }


static inline void cmp(m65xx_t* const m) {
  uint16_t data = m->a - get_dbus(m);
  set_nz(m, data & 0xFF);
  if(!((data >> 8) & 0x1)) { m->p |= CF; } else { m->p &= ~CF; }
}
static inline void cpy(m65xx_t* const m) {
  uint8_t val = get_dbus(m);
  uint16_t data = m->y - val;
  set_nz(m, data & 0xFF);
  if(!((data >> 8) & 0x1)) { m->p |= CF; } else { m->p &= ~CF; }
}
static inline void cpx(m65xx_t* const m) {
  uint16_t data = m->x - get_dbus(m);
  set_nz(m, data & 0xFF);
  if(!((data >> 8) & 0x1)) { m->p |= CF; } else { m->p &= ~CF; }
}

// Illegal/Undocumented instructions



static inline void slo(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  if((data & 0x80) >> 7) { m->p |= CF; } else { m->p &= ~CF; };
  data = (data << 1) & 0xFF;

  set_dbus(m, data);

  m->a |= data;
  set_nz(m, m->a);
}
static inline void anc(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  m->a &= data;
  set_nz(m, m->a);
  if(m->a & 0x80) { m->p |= CF; } else { m->p &= ~CF; }
}
static inline void rla(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  bool cf = m->p & CF;

  if(data & 0x80) { m->p |= CF; } else { m->p &= ~CF; }
  data = ((data << 1) | cf) & 0xFF;

  set_dbus(m, data);

  m->a &= data;
  set_nz(m, m->a);
}
static inline void sre(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  if(data & 0x1) { m->p |= CF; } else { m->p &= ~CF; }
  data >>= 1;
  set_nz(m, data);

  set_dbus(m, data);

  m->a ^= data;
  set_nz(m, m->a);
}
static inline void alr(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  m->a &= data;
  set_nz(m, m->a);

  if(m->a & 0x1) { m->p |= CF; } else { m->p &= ~CF; }
  m->a = (m->a >> 1) & 0xFF;
  set_nz(m, m->a);
}
static inline void rra(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  bool cf = m->p & CF;

  if(data & 0x1) { m->p |= CF; } else { m->p &= ~CF; }
  data = (data >> 1) | (cf << 7);
  set_nz(m, data);

  set_dbus(m, data);

  // Store the value of carry after ROR
  cf = m->p & CF;

  if(m->p & DF) {  
    uint8_t al = (m->a & 0x0F) + (data & 0x0F) + cf;
    if (al > 0x09) { al += 0x06; }

    uint8_t ah = (m->a >> 4) + (data >> 4) + (al > 0x0F);

    if(ah & 0x08) { m->p |= NF; } else { m->p &= ~NF; }
    if(~(data ^ m->a) & ((ah << 4) ^ m->a) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }
    if(((m->a + data + cf) & 0xFF) == 0) { m->p |= ZF; } else { m->p &= ~ZF; }

    if(ah > 0x09) { ah += 0x06; }
    if(ah > 0x0F) { m->p |= CF; } else { m->p &= ~CF; }


    m->a = (ah << 4) | (al & 0x0F);
  } 
  else {
    uint16_t result = m->a + data + cf;

    set_nz(m, result & 0xFF);
    if(((m->a ^ result) & (data ^ result) & 0x80) != 0) { m->p |= VF; } else { m->p &= ~VF; }
    if(result > 0xFF) { m->p |= CF; } else { m->p &= ~CF; }

    m->a = result & 0xFF;
  }
}
static inline void arr(m65xx_t* const m) {
  uint8_t data = get_dbus(m);

  m->a &= data;
  uint8_t a_ = m->a;
  bool cf = m->p & CF;

  m->a = (m->a >> 1) | (cf << 7);
  set_nz(m, m->a);

  if (m->p & DF) {
    if ((a_ & 0x0F) + (a_ & 0x01) > 5) { m->a = ((m->a + 0x06) & 0x0F) | (m->a & 0xF0); }

    if((m->a ^ (m->a << 1)) & VF) { m->p |= VF; } else { m->p &= ~VF; }
    if((a_ & 0xF0) + (a_ & 0x10) > 0x50) { m->p |= CF; } else { m->p &= ~CF; }
    
    if (m->p & CF) { m->a += 0x60; }
  } else {
    if((m->a >> 6) & 1) { m->p |= CF; } else { m->p &= ~CF; }
    if((m->a ^ (m->a << 1)) & 0x40) { m->p |= VF; } else { m->p &= ~VF; }
  }
}

static inline void sax(m65xx_t* const m) {
  set_dbus(m, m->a & m->x);
}
static inline void ane(m65xx_t* const m) {
  m->a = (m->a | 0xEE) & m->x & get_dbus(m);
  set_nz(m, m->a);
}

static inline void sha(m65xx_t* const m) {
  if(m->adh != (get_abus(m) >> 8)) {
    set_dbus(m, m->a & m->x & (m->adh + 1) & 0xFF);
    set_abus(m, (get_dbus(m) << 8) | ((m->adl + m->y) & 0xFF));
  }
  else {
    set_dbus(m, m->a & m->x & (m->adh + 1) & 0xFF);
  }
}
static inline void tas(m65xx_t* const m) {
  if(m->adh != (get_abus(m) >> 8)) {
    set_dbus(m, (m->s = m->a & m->x) & (m->adh + 1) & 0xFF);
    set_abus(m, (get_dbus(m) << 8) | ((m->adl + m->y) & 0xFF));
  }
  else {
    set_dbus(m, (m->s = m->a & m->x) & (m->adh + 1) & 0xFF);
  }
}
static inline void shy(m65xx_t* const m) {
  if(m->adh != (get_abus(m) >> 8)) {
    set_dbus(m, m->y & (m->adh + 1) & 0xFF);
    set_abus(m, (get_dbus(m) << 8) | ((m->adl + m->x) & 0xFF));
  }
  else {
    set_dbus(m, m->y & (m->adh + 1) & 0xFF);
  }
}
static inline void shx(m65xx_t* const m) {
  if(m->adh != (get_abus(m) >> 8)) {
    set_dbus(m, m->x & (m->adh + 1) & 0xFF);
    set_abus(m, (get_dbus(m) << 8) | ((m->adl + m->y) & 0xFF));
  }
  else {
     set_dbus(m, m->x & (m->adh + 1) & 0xFF);
  }
}
static inline void sbx(m65xx_t* const m) {
  m->x &= m->a;
  uint16_t data = m->x - get_dbus(m);
  m->x = data & 0xFF;
  if(!((data >> 8) & 0x1)) { m->p |= CF; } else { m->p &= ~CF; }
  set_nz(m, m->x);
}

static inline void lax(m65xx_t* const m) {
  set_nz(m, m->a = m->x = get_dbus(m));
}
static inline void lxa(m65xx_t* const m) {
  set_nz(m, m->a = m->x = (m->a | 0xEE) & get_dbus(m));
}
static inline void las(m65xx_t* const m) {
  set_nz(m, m->x = m->s = m->a = get_dbus(m) & m->s);
}

static inline void dcp(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  data--;

  set_dbus(m, data);

  uint16_t result = m->a - data;
  set_nz(m, result & 0xFF);
  if(!((result >> 8) & 0x1)) { m->p |= CF; } else { m->p &= ~CF; }
}
static inline void isc(m65xx_t* const m) {
  uint8_t data = get_dbus(m);
  data++;

  set_dbus(m, data);

  uint16_t result;
  bool cf = m->p & CF;

  if(m->p & DF) {
    result = m->a - data - !cf;

    uint8_t al = (m->a & 0x0F) - (data & 0x0F) - !cf;
    if(al & 0x80) { al -= 0x06; }

    uint8_t ah = (m->a >> 4) - (data >> 4) - (al >> 7);

    set_nz(m, result & 0xFF);
    if((m->a ^ data) & (m->a ^ result) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }
    if(!(result > 0xFF)) { m->p |= CF; } else { m->p &= ~CF; }

    if(ah & 0x80) { ah -= 0x06; }
    
    m->a = (ah << 4) | (al & 0x0F);
  }
  else {
    result = m->a - data - !cf;

    set_nz(m, result & 0xFF);
    if((m->a ^ data) & (m->a ^ result) & 0x80) { m->p |= VF; } else { m->p &= ~VF; }
    if(!(result > 0xFF)) { m->p |= CF; } else { m->p &= ~CF; }

    m->a = result & 0xFF;
  }
}


static inline void jam(m65xx_t* const m) {
  switch (m->tcu) {
    case 1:
      set_abus(m, m->pc);
      break;
    case 2:
      set_abus(m, 0xFFFF);
      break;
    case 3:
      set_abus(m, 0xFFFE);
      break;
    case 4:
      set_abus(m, 0xFFFE);
      break;
    case 5:
      set_abus(m, 0xFFFF);
      if(m->pins & RES) { 
        m->tcu = 0;
        m->ir = RES_OPCODE;
      }
      off(m, SYNC);
      break;
  }
}

/*
 *
 *
 * Opcode table(s)
 *
 *
*/

m65xx_opcodes_t m6502_opcode_table[0x103] = {
  [0x00] =  { .mode = brk, .instr = impl },
  [0x01] =  { .mode = idxr, .instr = ora },
  [0x02] =  { .mode = jam, .instr = impl },
  [0x03] =  { .mode = idxm, .instr = slo },
  [0x04] =  { .mode = zpgr, .instr = nop },
  [0x05] =  { .mode = zpgr, .instr = ora },
  [0x06] =  { .mode = zpgm, .instr = asl },
  [0x07] =  { .mode = zpgm, .instr = slo },
  [0x08] =  { .mode = php, .instr = impl },
  [0x09] =  { .mode = imme, .instr = ora },
  [0x0A] =  { .mode = accu, .instr = asl },
  [0x0B] =  { .mode = imme, .instr = anc },
  [0x0C] =  { .mode = absr, .instr = nop },
  [0x0D] =  { .mode = absr, .instr = ora },
  [0x0E] =  { .mode = absm, .instr = asl },
  [0x0F] =  { .mode = absm, .instr = slo },
  [0x10] =  { .mode = rela, .instr = bpl },
  [0x11] =  { .mode = idyr, .instr = ora },
  [0x12] =  { .mode = jam, .instr = impl },
  [0x13] =  { .mode = idym, .instr = slo },
  [0x14] =  { .mode = zpxr, .instr = nop },
  [0x15] =  { .mode = zpxr, .instr = ora },
  [0x16] =  { .mode = zpxm, .instr = asl },
  [0x17] =  { .mode = zpxm, .instr = slo }, 
  [0x18] =  { .mode = impl, .instr = clc },
  [0x19] =  { .mode = abyr, .instr = ora },
  [0x1A] =  { .mode = impl, .instr = nop },
  [0x1B] =  { .mode = abym, .instr = slo },
  [0x1C] =  { .mode = abxr, .instr = nop },
  [0x1D] =  { .mode = abxr, .instr = ora },
  [0x1E] =  { .mode = abxm, .instr = asl },
  [0x1F] =  { .mode = abxm, .instr = slo },
  [0x20] =  { .mode = jsr, .instr = impl },
  [0x21] =  { .mode = idxr, .instr = and },
  [0x22] =  { .mode = jam, .instr = impl },
  [0x23] =  { .mode = idxm, .instr = rla },
  [0x24] =  { .mode = zpgr, .instr = bit },
  [0x25] =  { .mode = zpgr, .instr = and },
  [0x26] =  { .mode = zpgm, .instr = rol },
  [0x27] =  { .mode = zpgm, .instr = rla },
  [0x28] =  { .mode = plp, .instr = impl },
  [0x29] =  { .mode = imme, .instr = and },
  [0x2A] =  { .mode = accu, .instr = rol },
  [0x2B] =  { .mode = imme, .instr = anc }, 
  [0x2C] =  { .mode = absr, .instr = bit },
  [0x2D] =  { .mode = absr, .instr = and },
  [0x2E] =  { .mode = absm, .instr = rol },
  [0x2F] =  { .mode = absm, .instr = rla },
  [0x30] =  { .mode = rela, .instr = bmi },
  [0x31] =  { .mode = idyr, .instr = and },
  [0x32] =  { .mode = jam, .instr = impl },
  [0x33] =  { .mode = idym, .instr = rla },
  [0x34] =  { .mode = zpxr, .instr = nop },
  [0x35] =  { .mode = zpxr, .instr = and },
  [0x36] =  { .mode = zpxm, .instr = rol },
  [0x37] =  { .mode = zpxm, .instr = rla },
  [0x38] =  { .mode = impl, .instr = sec },
  [0x39] =  { .mode = abyr, .instr = and },
  [0x3A] =  { .mode = impl, .instr = nop },
  [0x3B] =  { .mode = abym, .instr = rla },
  [0x3C] =  { .mode = abxr, .instr = nop },
  [0x3D] =  { .mode = abxr, .instr = and },
  [0x3E] =  { .mode = abxm, .instr = rol },
  [0x3F] =  { .mode = abxm, .instr = rla },
  [0x40] =  { .mode = rti, .instr = impl },
  [0x41] =  { .mode = idxr, .instr = eor },
  [0x42] =  { .mode = jam, .instr = impl },
  [0x43] =  { .mode = idxm, .instr = sre },
  [0x44] =  { .mode = zpgr, .instr = nop },
  [0x45] =  { .mode = zpgr, .instr = eor },
  [0x46] =  { .mode = zpgm, .instr = lsr },
  [0x47] =  { .mode = zpgm, .instr = sre },
  [0x48] =  { .mode = pha, .instr = impl },
  [0x49] =  { .mode = imme, .instr = eor },
  [0x4A] =  { .mode = accu, .instr = lsr },
  [0x4B] =  { .mode = imme, .instr = alr },
  [0x4C] =  { .mode = abj, .instr = impl },
  [0x4D] =  { .mode = absr, .instr = eor },
  [0x4E] =  { .mode = absm, .instr = lsr },
  [0x4F] =  { .mode = absm, .instr = sre },
  [0x50] =  { .mode = rela, .instr = bvc },
  [0x51] =  { .mode = idyr, .instr = eor },
  [0x52] =  { .mode = jam, .instr = impl },
  [0x53] =  { .mode = idym, .instr = sre },
  [0x54] =  { .mode = zpxr, .instr = nop },
  [0x55] =  { .mode = zpxr, .instr = eor },
  [0x56] =  { .mode = zpxm, .instr = lsr },
  [0x57] =  { .mode = zpxm, .instr = sre },
  [0x58] =  { .mode = impl, .instr = cli },
  [0x59] =  { .mode = abyr, .instr = eor },
  [0x5A] =  { .mode = impl, .instr = nop },
  [0x5B] =  { .mode = abym, .instr = sre },
  [0x5C] =  { .mode = abxr, .instr = nop },
  [0x5D] =  { .mode = abxr, .instr = eor },
  [0x5E] =  { .mode = abxm, .instr = lsr },
  [0x5F] =  { .mode = abxm, .instr = sre },
  [0x60] =  { .mode = rts, .instr = impl },
  [0x61] =  { .mode = idxr, .instr = adc },
  [0x62] =  { .mode = jam, .instr = impl },
  [0x63] =  { .mode = idxm, .instr = rra },
  [0x64] =  { .mode = zpgr, .instr = nop },
  [0x65] =  { .mode = zpgr, .instr = adc },
  [0x66] =  { .mode = zpgm, .instr = ror },
  [0x67] =  { .mode = zpgm, .instr = rra },
  [0x68] =  { .mode = pla, .instr = impl },
  [0x69] =  { .mode = imme, .instr = adc },
  [0x6A] =  { .mode = accu, .instr = ror },
  [0x6B] =  { .mode = imme, .instr = arr },
  [0x6C] =  { .mode = inj, .instr = impl },
  [0x6D] =  { .mode = absr, .instr = adc },
  [0x6E] =  { .mode = absm, .instr = ror },
  [0x6F] =  { .mode = absm, .instr = rra },
  [0x70] =  { .mode = rela, .instr = bvs },
  [0x71] =  { .mode = idyr, .instr = adc },
  [0x72] =  { .mode = jam, .instr = impl },
  [0x73] =  { .mode = idym, .instr = rra },
  [0x74] =  { .mode = zpxr, .instr = nop },
  [0x75] =  { .mode = zpxr, .instr = adc },
  [0x76] =  { .mode = zpxm, .instr = ror },
  [0x77] =  { .mode = zpxm, .instr = rra },
  [0x78] =  { .mode = impl, .instr = sei },
  [0x79] =  { .mode = abyr, .instr = adc },
  [0x7A] =  { .mode = impl, .instr = nop },
  [0x7B] =  { .mode = abym, .instr = rra },
  [0x7C] =  { .mode = abxr, .instr = nop },
  [0x7D] =  { .mode = abxr, .instr = adc },
  [0x7E] =  { .mode = abxm, .instr = ror },
  [0x7F] =  { .mode = abxm, .instr = rra },
  [0x80] =  { .mode = imme, .instr = nop },
  [0x81] =  { .mode = idxw, .instr = sta },
  [0x82] =  { .mode = imme, .instr = nop },
  [0x83] =  { .mode = idxw, .instr = sax }, 
  [0x84] =  { .mode = zpgw, .instr = sty },
  [0x85] =  { .mode = zpgw, .instr = sta },
  [0x86] =  { .mode = zpgw, .instr = stx },
  [0x87] =  { .mode = zpgw, .instr = sax },
  [0x88] =  { .mode = impl, .instr = dey },
  [0x89] =  { .mode = imme, .instr = nop },
  [0x8A] =  { .mode = impl, .instr = txa },
  [0x8B] =  { .mode = imme, .instr = ane },
  [0x8C] =  { .mode = absw, .instr = sty },
  [0x8D] =  { .mode = absw, .instr = sta },
  [0x8E] =  { .mode = absw, .instr = stx },
  [0x8F] =  { .mode = absw, .instr = sax },
  [0x90] =  { .mode = rela, .instr = bcc },
  [0x91] =  { .mode = idyw, .instr = sta },
  [0x92] =  { .mode = jam, .instr = impl },
  [0x93] =  { .mode = idyw, .instr = sha },
  [0x94] =  { .mode = zpxw, .instr = sty },
  [0x95] =  { .mode = zpxw, .instr = sta },
  [0x96] =  { .mode = zpyw, .instr = stx },
  [0x97] =  { .mode = zpyw, .instr = sax },
  [0x98] =  { .mode = impl, .instr = tya },
  [0x99] =  { .mode = abyw, .instr = sta },
  [0x9A] =  { .mode = impl, .instr = txs },
  [0x9B] =  { .mode = abyw, .instr = tas },
  [0x9C] =  { .mode = abxw, .instr = shy },
  [0x9D] =  { .mode = abxw, .instr = sta },
  [0x9E] =  { .mode = abyw, .instr = shx }, 
  [0x9F] =  { .mode = abyw, .instr = sha }, 
  [0xA0] =  { .mode = imme, .instr = ldy },
  [0xA1] =  { .mode = idxr, .instr = lda },
  [0xA2] =  { .mode = imme, .instr = ldx },
  [0xA3] =  { .mode = idxr, .instr = lax },
  [0xA4] =  { .mode = zpgr, .instr = ldy },
  [0xA5] =  { .mode = zpgr, .instr = lda },
  [0xA6] =  { .mode = zpgr, .instr = ldx },
  [0xA7] =  { .mode = zpgr, .instr = lax },
  [0xA8] =  { .mode = impl, .instr = tay },
  [0xA9] =  { .mode = imme, .instr = lda },
  [0xAA] =  { .mode = impl, .instr = tax },
  [0xAB] =  { .mode = imme, .instr = lxa },
  [0xAC] =  { .mode = absr, .instr = ldy },
  [0xAD] =  { .mode = absr, .instr = lda },
  [0xAE] =  { .mode = absr, .instr = ldx },
  [0xAF] =  { .mode = absr, .instr = lax },
  [0xB0] =  { .mode = rela, .instr = bcs },
  [0xB1] =  { .mode = idyr, .instr = lda },
  [0xB2] =  { .mode = jam, .instr = impl },
  [0xB3] =  { .mode = idyr, .instr = lax },
  [0xB4] =  { .mode = zpxr, .instr = ldy },
  [0xB5] =  { .mode = zpxr, .instr = lda },
  [0xB6] =  { .mode = zpyr, .instr = ldx },
  [0xB7] =  { .mode = zpyr, .instr = lax },
  [0xB8] =  { .mode = impl, .instr = clv },
  [0xB9] =  { .mode = abyr, .instr = lda },
  [0xBA] =  { .mode = impl, .instr = tsx },
  [0xBB] =  { .mode = abyr, .instr = las },
  [0xBC] =  { .mode = abxr, .instr = ldy },
  [0xBD] =  { .mode = abxr, .instr = lda },
  [0xBE] =  { .mode = abyr, .instr = ldx },
  [0xBF] =  { .mode = abyr, .instr = lax },
  [0xC0] =  { .mode = imme, .instr = cpy },
  [0xC1] =  { .mode = idxr, .instr = cmp },
  [0xC2] =  { .mode = imme, .instr = nop },
  [0xC3] =  { .mode = idxm, .instr = dcp },
  [0xC4] =  { .mode = zpgr, .instr = cpy },
  [0xC5] =  { .mode = zpgr, .instr = cmp },
  [0xC6] =  { .mode = zpgm, .instr = dec },
  [0xC7] =  { .mode = zpgm, .instr = dcp },
  [0xC8] =  { .mode = impl, .instr = iny },
  [0xC9] =  { .mode = imme, .instr = cmp },
  [0xCA] =  { .mode = impl, .instr = dex },
  [0xCB] =  { .mode = imme, .instr = sbx },
  [0xCC] =  { .mode = absr, .instr = cpy },
  [0xCD] =  { .mode = absr, .instr = cmp },
  [0xCE] =  { .mode = absm, .instr = dec },
  [0xCF] =  { .mode = absm, .instr = dcp },
  [0xD0] =  { .mode = rela, .instr = bne },
  [0xD1] =  { .mode = idyr, .instr = cmp },
  [0xD2] =  { .mode = jam, .instr = impl },
  [0xD3] =  { .mode = idym, .instr = dcp },
  [0xD4] =  { .mode = zpxr, .instr = nop },
  [0xD5] =  { .mode = zpxr, .instr = cmp },
  [0xD6] =  { .mode = zpxm, .instr = dec },
  [0xD7] =  { .mode = zpxm, .instr = dcp },
  [0xD8] =  { .mode = impl, .instr = cld },
  [0xD9] =  { .mode = abyr, .instr = cmp },
  [0xDA] =  { .mode = impl, .instr = nop },
  [0xDB] =  { .mode = abym, .instr = dcp },
  [0xDC] =  { .mode = abxr, .instr = nop },
  [0xDD] =  { .mode = abxr, .instr = cmp },
  [0xDE] =  { .mode = abxm, .instr = dec },
  [0xDF] =  { .mode = abxm, .instr = dcp },
  [0xE0] =  { .mode = imme, .instr = cpx },
  [0xE1] =  { .mode = idxr, .instr = sbc },
  [0xE2] =  { .mode = imme, .instr = nop },
  [0xE3] =  { .mode = idxm, .instr = isc },
  [0xE4] =  { .mode = zpgr, .instr = cpx },
  [0xE5] =  { .mode = zpgr, .instr = sbc },
  [0xE6] =  { .mode = zpgm, .instr = inc },
  [0xE7] =  { .mode = zpgm, .instr = isc },
  [0xE8] =  { .mode = impl, .instr = inx },
  [0xE9] =  { .mode = imme, .instr = sbc },
  [0xEA] =  { .mode = impl, .instr = nop },
  [0xEB] =  { .mode = imme, .instr = sbc },
  [0xEC] =  { .mode = absr, .instr = cpx },
  [0xED] =  { .mode = absr, .instr = sbc },
  [0xEE] =  { .mode = absm, .instr = inc },
  [0xEF] =  { .mode = absm, .instr = isc },
  [0xF0] =  { .mode = rela, .instr = beq },
  [0xF1] =  { .mode = idyr, .instr = sbc },
  [0xF2] =  { .mode = jam, .instr = impl },
  [0xF3] =  { .mode = idym, .instr = isc },
  [0xF4] =  { .mode = zpxr, .instr = nop },
  [0xF5] =  { .mode = zpxr, .instr = sbc },
  [0xF6] =  { .mode = zpxm, .instr = inc },
  [0xF7] =  { .mode = zpxm, .instr = isc },
  [0xF8] =  { .mode = impl, .instr = sed },
  [0xF9] =  { .mode = abyr, .instr = sbc },
  [0xFA] =  { .mode = impl, .instr = nop },
  [0xFB] =  { .mode = abym, .instr = isc },
  [0xFC] =  { .mode = abxr, .instr = nop },
  [0xFD] =  { .mode = abxr, .instr = sbc },
  [0xFE] =  { .mode = abxm, .instr = inc },
  [0xFF] =  { .mode = abxm, .instr = isc },
  [0x100] = { .mode = res, .instr = impl },
  [0x101] = { .mode = nmi, .instr = impl },
  [0x102] = { .mode = irq, .instr = impl },
}; 

void m65xx_init(m65xx_t* const m) {
  m->pins = 0;
  m->pins |= (RW | SYNC);
  m->a = m->x = m->y = m->p = m->tcu = 0;
  m->s = 0xFD;
  m->p |= 0x20;
  m->ir = 0x00; 
  m->pc = m->ad = 0;
  m->bra = 0;

  m->cpu_clock = 0;
  m->nmi_edge = m->nmi_ = m->irq_ = 0;
}

void m65xx_tick(m65xx_t* const m) {
  on(m, RW);
/*
  if((m->pins & NMI) && !(m->nmi_edge)) { m->nmi_ = 1; }
  m->nmi_edge = m->pins & NMI; // Updates if a edge case has occured

  if((m->pins & IRQ) && !(m->p & IDF)) { m->irq_ = 1; } else { m->irq_ = 0; }
*/
  if(m->pins & SYNC) {
    m->ir = get_dbus(m);
    off(m, SYNC);
    if(m->nmi_) {
      m->ir = NMI_OPCODE;
      m->nmi_ = 0;
    }
    else if(m->irq_) {
      m->ir = IRQ_OPCODE;
      m->irq_ = 0;
    }
    else { m->pc++; }
  }
  m->tcu++;
  m->cpu_clock++;
  // Call instruction/addressing mode 
  m6502_opcode_table[m->ir].mode(m);
}
