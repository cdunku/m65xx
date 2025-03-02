#include <stdio.h>

#include "m65xx.h"
#include "debug.h"


static const char* m6502_opcode_lookup[0x103] = {
  "BRK", "ORA (ind,X)", "KIL", "SLO (ind,X)", "NOP zp", "ORA zp", "ASL zp", "SLO zp",
  "PHP", "ORA #", "ASL A", "ANC #", "NOP abs", "ORA abs", "ASL abs", "SLO abs",
  "BPL rel", "ORA (ind),Y", "KIL", "SLO (ind),Y", "NOP zp,X", "ORA zp,X", "ASL zp,X", "SLO zp,X",
  "CLC", "ORA abs,Y", "NOP", "SLO abs,Y", "NOP abs,X", "ORA abs,X", "ASL abs,X", "SLO abs,X",
  "JSR abs", "AND (ind,X)", "KIL", "RLA (ind,X)", "BIT zp", "AND zp", "ROL zp", "RLA zp",
  "PLP", "AND #", "ROL A", "ANC #", "BIT abs", "AND abs", "ROL abs", "RLA abs",
  "BMI rel", "AND (ind),Y", "KIL", "RLA (ind),Y", "NOP zp,X", "AND zp,X", "ROL zp,X", "RLA zp,X",
  "SEC", "AND abs,Y", "NOP", "RLA abs,Y", "NOP abs,X", "AND abs,X", "ROL abs,X", "RLA abs,X",
  "RTI", "EOR (ind,X)", "KIL", "SRE (ind,X)", "NOP zp", "EOR zp", "LSR zp", "SRE zp",
  "PHA", "EOR #", "LSR A", "ALR #", "JMP abs", "EOR abs", "LSR abs", "SRE abs",
  "BVC rel", "EOR (ind),Y", "KIL", "SRE (ind),Y", "NOP zp,X", "EOR zp,X", "LSR zp,X", "SRE zp,X",
  "CLI", "EOR abs,Y", "NOP", "SRE abs,Y", "NOP abs,X", "EOR abs,X", "LSR abs,X", "SRE abs,X",
  "RTS", "ADC (ind,X)", "KIL", "RRA (ind,X)", "NOP zp", "ADC zp", "ROR zp", "RRA zp",
  "PLA", "ADC #", "ROR A", "ARR #", "JMP (ind)", "ADC abs", "ROR abs", "RRA abs",
  "BVS rel", "ADC (ind),Y", "KIL", "RRA (ind),Y", "NOP zp,X", "ADC zp,X", "ROR zp,X", "RRA zp,X",
  "SEI", "ADC abs,Y", "NOP", "RRA abs,Y", "NOP abs,X", "ADC abs,X", "ROR abs,X", "RRA abs,X",
  "NOP #", "STA (ind,X)", "NOP #", "SAX (ind,X)", "STY zp", "STA zp", "STX zp", "SAX zp",
  "DEY", "NOP #", "TXA", "XAA #", "STY abs", "STA abs", "STX abs", "SAX abs",
  "BCC rel", "STA (ind),Y", "KIL", "AHX (ind),Y", "STY zp,X", "STA zp,X", "STX zp,Y", "SAX zp,Y",
  "TYA", "STA abs,Y", "TXS", "TAS abs,Y", "SHY abs,X", "STA abs,X", "SHX abs,Y", "AHX abs,Y",
  "LDY #", "LDA (ind,X)", "LDX #", "LAX (ind,X)", "LDY zp", "LDA zp", "LDX zp", "LAX zp",
  "TAY", "LDA #", "TAX", "LAX #", "LDY abs", "LDA abs", "LDX abs", "LAX abs",
  "BCS rel", "LDA (ind),Y", "KIL", "LAX (ind),Y", "LDY zp,X", "LDA zp,X", "LDX zp,Y", "LAX zp,Y",
  "CLV", "LDA abs,Y", "TSX", "LAS abs,Y", "LDY abs,X", "LDA abs,X", "LDX abs,Y", "LAX abs,Y",
  "CPY #", "CMP (ind,X)", "NOP #", "DCP (ind,X)", "CPY zp", "CMP zp", "DEC zp", "DCP zp",
  "INY", "CMP #", "DEX", "AXS #", "CPY abs", "CMP abs", "DEC abs", "DCP abs",
  "BNE rel", "CMP (ind),Y", "KIL", "DCP (ind),Y", "NOP zp,X", "CMP zp,X", "DEC zp,X", "DCP zp,X",
  "CLD", "CMP abs,Y", "NOP", "DCP abs,Y", "NOP abs,X", "CMP abs,X", "DEC abs,X", "DCP abs,X",
  "CPX #", "SBC (ind,X)", "NOP #", "ISC (ind,X)", "CPX zp", "SBC zp", "INC zp", "ISC zp",
  "INX", "SBC #", "NOP", "SBC #", "CPX abs", "SBC abs", "INC abs", "ISC abs",
  "BEQ rel", "SBC (ind),Y", "KIL", "ISC (ind),Y", "NOP zp,X", "SBC zp,X", "INC zp,X", "ISC zp,X",
  "SED", "SBC abs,Y", "NOP", "ISC abs,Y", "NOP abs,X", "SBC abs,X", "INC abs,X", "ISC abs,X",
  "RES", "NMI", "IRQ" 
};

void m6502_print(m65xx_t* const m) {
  char flags[] = "........";

  if(m->p & NF) { flags[0] = 'n'; }
  if(m->p & VF) { flags[1] = 'v'; }
  flags[2] = '1'; 
  if(m->p & BF) { flags[3] = 'b'; }
  if(m->p & DF) { flags[4] = 'd'; }
  if(m->p & IDF) { flags[5] = 'i'; }
  if(m->p & ZF) { flags[6] = 'z'; }
  if(m->p & CF) { flags[7] = 'c'; }

  printf("\n[PC]: %04X, [A]: %02X, [X]: %02X, [Y]: %02X, [S]: %02X, [P]: %02X (%s), [CYC]: %lu\n",
         m->pc, m->a, m->x, m->y, m->s, m->p, flags, m->cpu_clock);
  
  // For 6502 and its derivatives
  printf("[ADDR]: %04X, [DATA]: %02X, [RDY]: %d, [IRQ]: %d, [NMI]: %d, [SYNC]: %d, [RES]: %d, [RW]: %d\n"
         "[AEC]: %d, [P0]: %d, [P1]: %d, [P2]: %d, [P3]: %d, [P4]: %d, [P5]: %d\n", // For 6510
         (uint16_t)(m->pins & 0xFFFF), 
         (uint8_t)((m->pins & 0xFF0000) >> 16),
         (m->pins & RDY) ? 1 : 0,
         (m->pins & IRQ) ? 1 : 0,
         (m->pins & NMI) ? 1 : 0,
         (m->pins & SYNC) ? 1 : 0,
         (m->pins & RES) ? 1 : 0,
         (m->pins & RW) ? 1 : 0,
         (m->pins & AEC) ? 1 : 0,
         (m->pins & P0) ? 1 : 0,
         (m->pins & P1) ? 1 : 0,
         (m->pins & P2) ? 1 : 0,
         (m->pins & P3) ? 1 : 0,
         (m->pins & P4) ? 1 : 0,
         (m->pins & P5) ? 1 : 0);

  printf("--> %s\n", m6502_opcode_lookup[m->ir]);
}
