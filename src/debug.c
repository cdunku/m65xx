#include <stdio.h>

#include "m65xx.h"
#include "debug.h"

const opcode_info_t m6502_opcodes[0x103] = {
  [0x00] = { "BRK", AM_IMPLIED, 1 },
  [0x01] = { "ORA ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x02] = { "JAM", AM_IMPLIED, 1 },
  [0x03] = { "SLO ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x04] = { "NOP $%02X", AM_ZERO_PAGE, 2 },
  [0x05] = { "ORA $%02X", AM_ZERO_PAGE, 2 },
  [0x06] = { "ASL $%02X", AM_ZERO_PAGE, 2 },
  [0x07] = { "SLO $%02X", AM_ZERO_PAGE, 2 },
  [0x08] = { "PHP", AM_IMPLIED, 1 },
  [0x09] = { "ORA #$%02X", AM_IMMEDIATE, 2 },
  [0x0A] = { "ASL A", AM_IMPLIED, 1 },
  [0x0B] = { "ANC #$%02X", AM_IMMEDIATE, 2 },
  [0x0C] = { "NOP $%04X", AM_ABSOLUTE, 3 },
  [0x0D] = { "ORA $%04X", AM_ABSOLUTE, 3 },
  [0x0E] = { "ASL $%04X", AM_ABSOLUTE, 3 },
  [0x0F] = { "SLO $%04X", AM_ABSOLUTE, 3 },

  [0x10] = { "BPL $%04X", AM_RELATIVE, 2 },
  [0x11] = { "ORA ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x12] = { "JAM", AM_IMPLIED, 1 },
  [0x13] = { "SLO ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x14] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x15] = { "ORA $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x16] = { "ASL $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x17] = { "SLO $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x18] = { "CLC", AM_IMPLIED, 1 },
  [0x19] = { "ORA $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x1A] = { "NOP", AM_IMPLIED, 1 },
  [0x1B] = { "SLO $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x1C] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x1D] = { "ORA $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x1E] = { "ASL $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x1F] = { "SLO $%04X,X", AM_ABSOLUTE_X, 3 },

  [0x20] = { "JSR $%04X", AM_ABSOLUTE, 3 },
  [0x21] = { "AND ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x22] = { "JAM", AM_IMPLIED, 1 },
  [0x23] = { "RLA ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x24] = { "BIT $%02X", AM_ZERO_PAGE, 2 },
  [0x25] = { "AND $%02X", AM_ZERO_PAGE, 2 },
  [0x26] = { "ROL $%02X", AM_ZERO_PAGE, 2 },
  [0x27] = { "RLA $%02X", AM_ZERO_PAGE, 2 },
  [0x28] = { "PLP", AM_IMPLIED, 1 },
  [0x29] = { "AND #$%02X", AM_IMMEDIATE, 2 },
  [0x2A] = { "ROL A", AM_IMPLIED, 1 },
  [0x2B] = { "ANC #$%02X", AM_IMMEDIATE, 2 },
  [0x2C] = { "BIT $%04X", AM_ABSOLUTE, 3 },
  [0x2D] = { "AND $%04X", AM_ABSOLUTE, 3 },
  [0x2E] = { "ROL $%04X", AM_ABSOLUTE, 3 },
  [0x2F] = { "RLA $%04X", AM_ABSOLUTE, 3 },

  [0x30] = { "BMI $%04X", AM_RELATIVE, 2 },
  [0x31] = { "AND ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x32] = { "JAM", AM_IMPLIED, 1 },
  [0x33] = { "RLA ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x34] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x35] = { "AND $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x36] = { "ROL $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x37] = { "RLA $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x38] = { "SEC", AM_IMPLIED, 1 },
  [0x39] = { "AND $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x3A] = { "NOP", AM_IMPLIED, 1 },
  [0x3B] = { "RLA $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x3C] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x3D] = { "AND $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x3E] = { "ROL $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x3F] = { "RLA $%04X,X", AM_ABSOLUTE_X, 3 },

  [0x40] = { "RTI", AM_IMPLIED, 1 },
  [0x41] = { "EOR ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x42] = { "JAM", AM_IMPLIED, 1 },
  [0x43] = { "SRE ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x44] = { "NOP $%02X", AM_ZERO_PAGE, 2 },
  [0x45] = { "EOR $%02X", AM_ZERO_PAGE, 2 },
  [0x46] = { "LSR $%02X", AM_ZERO_PAGE, 2 },
  [0x47] = { "SRE $%02X", AM_ZERO_PAGE, 2 },
  [0x48] = { "PHA", AM_IMPLIED, 1 },
  [0x49] = { "EOR #$%02X", AM_IMMEDIATE, 2 },
  [0x4A] = { "LSR A", AM_IMPLIED, 1 },
  [0x4B] = { "ALR #$%02X", AM_IMMEDIATE, 2 },
  [0x4C] = { "JMP $%04X", AM_ABSOLUTE, 3 },
  [0x4D] = { "EOR $%04X", AM_ABSOLUTE, 3 },
  [0x4E] = { "LSR $%04X", AM_ABSOLUTE, 3 },
  [0x4F] = { "SRE $%04X", AM_ABSOLUTE, 3 },

  [0x50] = { "BVC $%04X", AM_RELATIVE, 2 },
  [0x51] = { "EOR ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x52] = { "JAM", AM_IMPLIED, 1 },
  [0x53] = { "SRE ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x54] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x55] = { "EOR $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x56] = { "LSR $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x57] = { "SRE $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x58] = { "CLI", AM_IMPLIED, 1 },
  [0x59] = { "EOR $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x5A] = { "NOP", AM_IMPLIED, 1 },
  [0x5B] = { "SRE $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x5C] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x5D] = { "EOR $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x5E] = { "LSR $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x5F] = { "SRE $%04X,X", AM_ABSOLUTE_X, 3 },

  [0x60] = { "RTS", AM_IMPLIED, 1 },
  [0x61] = { "ADC ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x62] = { "JAM", AM_IMPLIED, 1 },
  [0x63] = { "RRA ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x64] = { "NOP $%02X", AM_ZERO_PAGE, 2 },
  [0x65] = { "ADC $%02X", AM_ZERO_PAGE, 2 },
  [0x66] = { "ROR $%02X", AM_ZERO_PAGE, 2 },
  [0x67] = { "RRA $%02X", AM_ZERO_PAGE, 2 },
  [0x68] = { "PLA", AM_IMPLIED, 1 },
  [0x69] = { "ADC #$%02X", AM_IMMEDIATE, 2 },
  [0x6A] = { "ROR A", AM_IMPLIED, 1 },
  [0x6B] = { "ARR #$%02X", AM_IMMEDIATE, 2 },
  [0x6C] = { "JMP ($%04X)", AM_INDIRECT, 3 },
  [0x6D] = { "ADC $%04X", AM_ABSOLUTE, 3 },
  [0x6E] = { "ROR $%04X", AM_ABSOLUTE, 3 },
  [0x6F] = { "RRA $%04X", AM_ABSOLUTE, 3 },

  [0x70] = { "BVS $%04X", AM_RELATIVE, 2 },
  [0x71] = { "ADC ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x72] = { "JAM", AM_IMPLIED, 1 },
  [0x73] = { "RRA ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x74] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x75] = { "ADC $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x76] = { "ROR $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x77] = { "RRA $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x78] = { "SEI", AM_IMPLIED, 1 },
  [0x79] = { "ADC $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x7A] = { "NOP", AM_IMPLIED, 1 },
  [0x7B] = { "RRA $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x7C] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x7D] = { "ADC $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x7E] = { "ROR $%04X,X", AM_ABSOLUTE_X, 3 },
  [0x7F] = { "RRA $%04X,X", AM_ABSOLUTE_X, 3 },

  [0x80] = { "NOP #$%02X", AM_IMMEDIATE, 2 },
  [0x81] = { "STA ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x82] = { "NOP #$%02X", AM_IMMEDIATE, 2 },
  [0x83] = { "SAX ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0x84] = { "STY $%02X", AM_ZERO_PAGE, 2 },
  [0x85] = { "STA $%02X", AM_ZERO_PAGE, 2 },
  [0x86] = { "STX $%02X", AM_ZERO_PAGE, 2 },
  [0x87] = { "SAX $%02X", AM_ZERO_PAGE, 2 },
  [0x88] = { "DEY", AM_IMPLIED, 1 },
  [0x89] = { "NOP #$%02X", AM_IMMEDIATE, 2 },
  [0x8A] = { "TXA", AM_IMPLIED, 1 },
  [0x8B] = { "ANE #$%02X", AM_IMMEDIATE, 2 },
  [0x8C] = { "STY $%04X", AM_ABSOLUTE, 3 },
  [0x8D] = { "STA $%04X", AM_ABSOLUTE, 3 },
  [0x8E] = { "STX $%04X", AM_ABSOLUTE, 3 },
  [0x8F] = { "SAX $%04X", AM_ABSOLUTE, 3 },

  [0x90] = { "BCC $%04X", AM_RELATIVE, 2 },
  [0x91] = { "STA ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x92] = { "JAM", AM_IMPLIED, 1 },
  [0x93] = { "AHX ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0x94] = { "STY $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x95] = { "STA $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0x96] = { "STX $%02X,Y", AM_ZERO_PAGE_Y, 2 },
  [0x97] = { "SAX $%02X,Y", AM_ZERO_PAGE_Y, 2 },
  [0x98] = { "TYA", AM_IMPLIED, 1 },
  [0x99] = { "STA $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x9A] = { "TXS", AM_IMPLIED, 1 },
  [0x9B] = { "TAS $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0x9C] = { "SHY $%04X,X", AM_ABSOLUTE, 3 },
  [0x9D] = { "STA $%04X,X", AM_ABSOLUTE, 3 },
  [0x9E] = { "SHX $%04X,Y", AM_ABSOLUTE, 3 },
  [0x9F] = { "AHX $%04X,Y", AM_ABSOLUTE, 3 },

  [0xA0] = { "LDY #$%02X", AM_IMMEDIATE, 2 },
  [0xA1] = { "LDA ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xA2] = { "LDX #$%02X", AM_IMMEDIATE, 2 },
  [0xA3] = { "LAX ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xA4] = { "LDY $%02X", AM_ZERO_PAGE, 2 },
  [0xA5] = { "LDA $%02X", AM_ZERO_PAGE, 2 },
  [0xA6] = { "LDX $%02X", AM_ZERO_PAGE, 2 },
  [0xA7] = { "LAX $%02X", AM_ZERO_PAGE, 2 },
  [0xA8] = { "TAY", AM_IMPLIED, 1 },
  [0xA9] = { "LDA #$%02X", AM_IMMEDIATE, 2 },
  [0xAA] = { "TAX", AM_IMPLIED, 1 },
  [0xAB] = { "LAX #$%02X", AM_IMMEDIATE, 2 },
  [0xAC] = { "LDY $%04X", AM_ABSOLUTE, 3 },
  [0xAD] = { "LDA $%04X", AM_ABSOLUTE, 3 },
  [0xAE] = { "LDX $%04X", AM_ABSOLUTE, 3 },
  [0xAF] = { "LAX $%04X", AM_ABSOLUTE, 3 },

  [0xB0] = { "BCS $%04X", AM_RELATIVE, 2 },
  [0xB1] = { "LDA ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xB2] = { "JAM", AM_IMPLIED, 1 },
  [0xB3] = { "LAX ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xB4] = { "LDY $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xB5] = { "LDA $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xB6] = { "LDX $%02X,Y", AM_ZERO_PAGE_Y, 2 },
  [0xB7] = { "LAX $%02X,Y", AM_ZERO_PAGE_Y, 2 },
  [0xB8] = { "CLV", AM_IMPLIED, 1 },
  [0xB9] = { "LDA $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xBA] = { "TSX", AM_IMPLIED, 1 },
  [0xBB] = { "LAS $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xBC] = { "LDY $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xBD] = { "LDA $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xBE] = { "LDX $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xBF] = { "LAX $%04X,Y", AM_ABSOLUTE_Y, 3 },

  [0xC0] = { "CPY #$%02X", AM_IMMEDIATE, 2 },
  [0xC1] = { "CMP ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xC2] = { "NOP #$%02X", AM_IMMEDIATE, 2 },
  [0xC3] = { "DCP ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xC4] = { "CPY $%02X", AM_ZERO_PAGE, 2 },
  [0xC5] = { "CMP $%02X", AM_ZERO_PAGE, 2 },
  [0xC6] = { "DEC $%02X", AM_ZERO_PAGE, 2 },
  [0xC7] = { "DCP $%02X", AM_ZERO_PAGE, 2 },
  [0xC8] = { "INY", AM_IMPLIED, 1 },
  [0xC9] = { "CMP #$%02X", AM_IMMEDIATE, 2 },
  [0xCA] = { "DEX", AM_IMPLIED, 1 },
  [0xCB] = { "AXS #$%02X", AM_IMMEDIATE, 2 },
  [0xCC] = { "CPY $%04X", AM_ABSOLUTE, 3 },
  [0xCD] = { "CMP $%04X", AM_ABSOLUTE, 3 },
  [0xCE] = { "DEC $%04X", AM_ABSOLUTE, 3 },
  [0xCF] = { "DCP $%04X", AM_ABSOLUTE, 3 },

  [0xD0] = { "BNE $%04X", AM_RELATIVE, 2 },
  [0xD1] = { "CMP ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xD2] = { "JAM", AM_IMPLIED, 1 },
  [0xD3] = { "DCP ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xD4] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xD5] = { "CMP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xD6] = { "DEC $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xD7] = { "DCP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xD8] = { "CLD", AM_IMPLIED, 1 },
  [0xD9] = { "CMP $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xDA] = { "NOP", AM_IMPLIED, 1 },
  [0xDB] = { "DCP $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xDC] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xDD] = { "CMP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xDE] = { "DEC $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xDF] = { "DCP $%04X,X", AM_ABSOLUTE_X, 3 },

  [0xE0] = { "CPX #$%02X", AM_IMMEDIATE, 2 },
  [0xE1] = { "SBC ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xE2] = { "NOP #$%02X", AM_IMMEDIATE, 2 },
  [0xE3] = { "ISC ($%02X,X)", AM_INDEXED_INDIRECT, 2 },
  [0xE4] = { "CPX $%02X", AM_ZERO_PAGE, 2 },
  [0xE5] = { "SBC $%02X", AM_ZERO_PAGE, 2 },
  [0xE6] = { "INC $%02X", AM_ZERO_PAGE, 2 },
  [0xE7] = { "ISC $%02X", AM_ZERO_PAGE, 2 },
  [0xE8] = { "INX", AM_IMPLIED, 1 },
  [0xE9] = { "SBC #$%02X", AM_IMMEDIATE, 2 },
  [0xEA] = { "NOP", AM_IMPLIED, 1 },
  [0xEB] = { "SBC #$%02X", AM_IMMEDIATE, 2 },
  [0xEC] = { "CPX $%04X", AM_ABSOLUTE, 3 },
  [0xED] = { "SBC $%04X", AM_ABSOLUTE, 3 },
  [0xEE] = { "INC $%04X", AM_ABSOLUTE, 3 },
  [0xEF] = { "ISC $%04X", AM_ABSOLUTE, 3 },

  [0xF0] = { "BEQ $%04X", AM_RELATIVE, 2 },
  [0xF1] = { "SBC ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xF2] = { "JAM", AM_IMPLIED, 1 },
  [0xF3] = { "ISC ($%02X),Y", AM_INDIRECT_INDEXED, 2 },
  [0xF4] = { "NOP $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xF5] = { "SBC $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xF6] = { "INC $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xF7] = { "ISC $%02X,X", AM_ZERO_PAGE_X, 2 },
  [0xF8] = { "SED", AM_IMPLIED, 1 },
  [0xF9] = { "SBC $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xFA] = { "NOP", AM_IMPLIED, 1 },
  [0xFB] = { "ISC $%04X,Y", AM_ABSOLUTE_Y, 3 },
  [0xFC] = { "NOP $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xFD] = { "SBC $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xFE] = { "INC $%04X,X", AM_ABSOLUTE_X, 3 },
  [0xFF] = { "ISC $%04X,X", AM_ABSOLUTE_X, 3 },

  [0x100] = { "RES", AM_IMPLIED, 1 },
  [0x101] = { "NMI", AM_IMPLIED, 1 },
  [0x102] = { "IRQ", AM_IMPLIED, 1 },
};

void m6502_print(m65xx_t* const m) {
  opcode_info_t info = m6502_opcodes[m->ir];
  uint16_t pc_ = m->pc - info.size;

  char flags[9] = "........";
  flags[8] = '\0';
  flags[0] = (m->p & NF) ? 'N' : '-';
  flags[1] = (m->p & VF) ? 'V' : '-';
  flags[2] = '1'; 
  flags[3] = (m->p & BF) ? 'B' : '-';
  flags[4] = (m->p & DF) ? 'D' : '-';
  flags[5] = (m->p & IDF) ? 'I' : '-';
  flags[6] = (m->p & ZF) ? 'Z' : '-';
  flags[7] = (m->p & CF) ? 'C' : '-';

  uint8_t instr_bytes[3] = {0};
  for (int i = 0; i < info.size; i++) {
    instr_bytes[i] = m->ram[pc_ + i];
  }
  uint8_t operand1 = (info.size > 1) ? instr_bytes[1] : 0;
  uint8_t operand2 = (info.size > 2) ? instr_bytes[2] : 0;
  uint16_t addr = (operand2 << 8) | operand1;
  uint16_t branch_target = pc_ + info.size + (int8_t)operand1;

  char op_bytes[9] = {0};
  if (info.size == 1) {
    snprintf(op_bytes, sizeof(op_bytes), "%02x----", instr_bytes[0]);
  } else if (info.size == 2) {
    snprintf(op_bytes, sizeof(op_bytes), "%02x%02x--", instr_bytes[0], instr_bytes[1]);
  } else if (info.size == 3) {
    snprintf(op_bytes, sizeof(op_bytes), "%02x%02x%02x", instr_bytes[0], instr_bytes[1], instr_bytes[2]);
  }

  char mnemonic[32] = {0};
  switch (info.mode) {
    case AM_IMPLIED:
      snprintf(mnemonic, sizeof(mnemonic), "%s", info.fmt);
      break;
    case AM_IMMEDIATE:
      snprintf(mnemonic, sizeof(mnemonic), info.fmt, operand1);
      break;
    case AM_ZERO_PAGE:
    case AM_ZERO_PAGE_X:
    case AM_ZERO_PAGE_Y:
    case AM_INDEXED_INDIRECT:
    case AM_INDIRECT_INDEXED:
      snprintf(mnemonic, sizeof(mnemonic), info.fmt, operand1);
      break;
    case AM_ABSOLUTE:
    case AM_ABSOLUTE_X:
    case AM_ABSOLUTE_Y:
    case AM_INDIRECT:
      snprintf(mnemonic, sizeof(mnemonic), info.fmt, addr);
      break;
    case AM_RELATIVE:
      snprintf(mnemonic, sizeof(mnemonic), info.fmt, branch_target);
      break;
    default:
      snprintf(mnemonic, sizeof(mnemonic), "%s", info.fmt);
      break;
  }
  printf("PC:%04X A:%02X X:%02X Y:%02X S:%02X [%s]| cyc=%lu %-6s %s\n",
         pc_, m->a, m->x, m->y, m->s, flags, m->cpu_clock, op_bytes, mnemonic);
}

