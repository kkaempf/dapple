/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  DEBUG: Dapple's debug/tracer (F5)
 * Revision:   (1.20) 2002.1229
 */

#include <stdio.h>
//#include <conio.h>
#include <string.h>
#include "dapple.h"

/* dapple.c */
extern unsigned char debugger;
extern unsigned char exitprogram;
extern unsigned char Rd6502(register unsigned short address);
extern void Wr6502(register unsigned short address, register unsigned char value);

/* cpu65c02.c */
extern void cpusetbreakpoint(unsigned short address);
extern unsigned short cpugetbreakpoint();
extern void cpuclearbreakpoint();
extern void cpusettracemode(unsigned char mode);
extern unsigned char cpugettracemode();
extern void cpusetpc(unsigned short address);
extern unsigned short cpugetpc();
extern unsigned short cpugetsp();
extern unsigned char cpugeta();
extern unsigned char cpugetx();
extern unsigned char cpugety();
extern unsigned char cpuflagsgen();
extern void cpuline();

/* type of CPU */
typedef enum {CPU6502, CPU65C02, CPU65SC02} CPUTYPE;

CPUTYPE getcputype (void);

/////////////////////////////////////////////////////////////////////////////

/* Information on the opcodes is defined in two components:
   the name of the opcode in printf() format, and a method used to determine
   what variables should be fed into the printf() and how far the IP should
   be pushed ahead.
   Opcodes are given in the format used by the Apple ][ disassembler. */

/* Rel16 is not used by the 65SC02 or earlier - but if I write a 65CE02
   disassembler, I can start from this codebase. -uso. */
typedef enum {Nothing, Byte, Word, Relative, TestBit, Rel16} NumberSize;

typedef struct
{
 char Description[20];
 NumberSize Method;
}
Opcode;

Opcode Ops6502[256]=
{
 "BRK",Nothing,                 /* $00 */
 "ORA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ORA $%02X",Byte,
 "ASL $%02X",Byte,
 "???",Nothing,
 "PHP",Nothing,                 /* $08 */
 "ORA #$%02X",Byte,
 "ASL",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ORA $%04X",Word,
 "ASL $%04X",Word,
 "???",Nothing,
 "BPL $%04X",Relative,          /* $10 */
 "ORA ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ORA $%02X,X",Byte,
 "ASL $%02X,X",Byte,
 "???",Nothing,
 "CLC",Nothing,                 /* $18 */
 "ORA $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ORA $%04X,X",Word,
 "ASL $%04X,X",Word,
 "???",Nothing,
 "JSR $%04X",Word,              /* $20 */
 "AND ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "BIT $%02X",Byte,
 "AND $%02X",Byte,
 "ROL $%02X",Byte,
 "???",Nothing,
 "PLP",Nothing,                 /* $28 */
 "AND #$%02X",Byte,
 "ROL",Byte,
 "???",Nothing,
 "BIT $%04X",Word,
 "AND $%04X",Word,
 "ROL $%04X",Word,
 "???",Nothing,
 "BMI $%04X",Relative,          /* $30 */
 "AND ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "AND $%02X,X",Byte,
 "ROL $%02X,X",Byte,
 "???",Nothing,
 "SEC",Nothing,                 /* $38 */
 "AND $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "AND $%04X,X",Word,
 "ROL $%04X,X",Word,
 "???",Nothing,
 "RTI",Nothing,                 /* $40 */
 "EOR ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X",Byte,
 "LSR $%02X",Byte,
 "???",Nothing,
 "PHA",Nothing,                 /* $48 */
 "EOR #$%02X",Byte,
 "LSR",Nothing,
 "???",Nothing,
 "JMP $%04X",Word,
 "EOR $%04X",Word,
 "LSR $%02X",Word,
 "???",Nothing,
 "BVC $%04X",Relative,          /* $50 */
 "EOR ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X,X",Byte,
 "LSR $%02X,X",Byte,
 "???",Nothing,
 "CLI",Nothing,                 /* $58 */
 "EOR $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%04X,X",Word,
 "LSR $%04X,X",Word,
 "???",Nothing,
 "RTS",Nothing,                 /* $60 */
 "ADC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ADC $%02X",Byte,
 "ROR $%02X",Byte,
 "???",Nothing,
 "PLA",Nothing,                 /* $68 */
 "ADC #$%02X",Byte,
 "ROR",Nothing,
 "???",Nothing,
 "JMP ($%04X)",Word,
 "ADC $%04X",Word,
 "ROR $%04X",Word,
 "???",Nothing,
 "BVS $%04X",Relative,          /* $70 */
 "ADC ($%02X),Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ADC $%02X,X",Byte,
 "ROR $%02X,X",Byte,
 "???",Nothing,
 "SEI",Nothing,                 /* $78 */
 "ADC $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "ADC $%04X,X",Word,
 "ROR $%04X,X",Word,
 "???",Nothing,
 "???",Nothing,                 /* $80 */
 "STA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "STY $%02X",Byte,
 "STA $%02X",Byte,
 "STX $%02X",Byte,
 "???",Nothing,
 "DEY",Nothing,                 /* $88 */
 "???",Nothing,
 "TXA",Nothing,
 "???",Nothing,
 "STY $%04X",Word,
 "STA $%04X",Word,
 "STX $%04X",Word,
 "???",Nothing,
 "BCC $%04X",Relative,          /* $90 */
 "STA ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "STY $%02X,X",Byte,
 "STA $%02X,X",Byte,
 "STX $%02X,Y",Byte,
 "???",Nothing,
 "TYA",Nothing,                 /* $98 */
 "STA $%04X,Y",Word,
 "TXS",Nothing,
 "???",Nothing,
 "???",Nothing,
 "STA $%04X,X",Word,
 "???",Nothing,
 "???",Nothing,
 "LDY #$%02X",Byte,             /* $A0 */
 "LDA ($%02X,X)",Byte,
 "LDX #$%02X",Byte,
 "???",Nothing,
 "LDY $%02X",Byte,
 "LDA $%02X",Byte,
 "LDX $%02X",Byte,
 "???",Nothing,
 "TAY",Nothing,                 /* $A8 */
 "LDA #$%02X",Byte,
 "TAX",Nothing,
 "???",Nothing,
 "LDY $%04X",Word,
 "LDA $%04X",Word,
 "LDX $%04X",Word,
 "???",Nothing,
 "BCS $%04X",Relative,          /* $B0 */
 "LDA ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "LDY $%02X,X",Byte,
 "LDA $%02X,X",Byte,
 "LDX $%02X,Y",Byte,
 "???",Nothing,
 "CLV",Nothing,                 /* $B8 */
 "LDA $%04X,Y",Word,
 "TSX",Nothing,
 "???",Nothing,
 "LDY $%04X,X",Word,
 "LDA $%04X,X",Word,
 "LDX $%04X,Y",Word,
 "???",Nothing,
 "CPY #$%02X",Byte,             /* $C0 */
 "CMP ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPY $%02X",Byte,
 "CMP $%02X",Byte,
 "DEC $%02X",Byte,
 "???",Nothing,
 "INY",Nothing,                 /* $C8 */
 "CMP #$%02X",Byte,
 "DEX",Nothing,
 "???",Nothing,
 "CPY $%04X",Word,
 "CMP $%04X",Word,
 "DEC $%04X",Word,
 "???",Nothing,
 "BNE $%04X",Relative,          /* $D0 */
 "CMP ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "CMP $%02X,X",Byte,
 "DEC $%02X,X",Byte,
 "???",Nothing,
 "CLD",Nothing,                 /* $D8 */
 "CMP $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "CMP $%04X,X",Word,
 "DEC $%04X,X",Word,
 "???",Nothing,
 "CPX #$%02X",Byte,             /* $E0 */
 "SBC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPX $%02X",Byte,
 "SBC $%02X",Byte,
 "INC $%02X",Byte,
 "???",Nothing,
 "INX",Nothing,                 /* $E8 */
 "SBC #$%02X",Byte,
 "NOP",Nothing,
 "???",Nothing,
 "CPX $%04X",Word,
 "SBC $%04X",Word,
 "INC $%04X",Word,
 "???",Nothing,
 "BEQ $%04X",Relative,          /* $F0 */
 "SBC ($%02X),Y",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "SBC $%02X,X",Byte,
 "INC $%02X,X",Byte,
 "???",Nothing,
 "SED",Nothing,                 /* $F8 */
 "SBC $%04X,Y",Word,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "SBC $%04X,X",Word,
 "INC $%04X,X",Word,
 "???",Nothing
};

Opcode Ops65C02[256]=
{
 "BRK",Nothing,                 /* $00 */
 "ORA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "TSB $%02X",Byte,
 "ORA $%02X",Byte,
 "ASL $%02X",Byte,
 "???",Nothing,
 "PHP",Nothing,                 /* $08 */
 "ORA #$%02X",Byte,
 "ASL",Nothing,
 "???",Nothing,
 "TSB $%04X",Word,
 "ORA $%04X",Word,
 "ASL $%04X",Word,
 "???",Nothing,
 "BPL $%04X",Relative,          /* $10 */
 "ORA ($%02X),Y",Byte,
 "ORA ($%02X)",Byte,
 "???",Nothing,
 "TRB $%02X",Byte,
 "ORA $%02X,X",Byte,
 "ASL $%02X,X",Byte,
 "???",Nothing,
 "CLC",Nothing,                 /* $18 */
 "ORA $%04X,Y",Word,
 "INC",Nothing,
 "???",Nothing,
 "TRB $%04X",Word,
 "ORA $%04X,X",Word,
 "ASL $%04X,X",Word,
 "???",Nothing,
 "JSR $%04X",Word,              /* $20 */
 "AND ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "BIT $%02X",Byte,
 "AND $%02X",Byte,
 "ROL $%02X",Byte,
 "???",Nothing,
 "PLP",Nothing,                 /* $28 */
 "AND #$%02X",Byte,
 "ROL",Byte,
 "???",Nothing,
 "BIT $%04X",Word,
 "AND $%04X",Word,
 "ROL $%04X",Word,
 "???",Nothing,
 "BMI $%04X",Relative,          /* $30 */
 "AND ($%02X),Y",Byte,
 "AND ($%02X)",Byte,
 "???",Nothing,
 "BIT $%02X,X",Byte,
 "AND $%02X,X",Byte,
 "ROL $%02X,X",Byte,
 "???",Nothing,
 "SEC",Nothing,                 /* $38 */
 "AND $%04X,Y",Word,
 "DEC",Nothing,
 "???",Nothing,
 "BIT $%04X,X",Word,
 "AND $%04X,X",Word,
 "ROL $%04X,X",Word,
 "???",Nothing,
 "RTI",Nothing,                 /* $40 */
 "EOR ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X",Byte,
 "LSR $%02X",Byte,
 "???",Nothing,
 "PHA",Nothing,                 /* $48 */
 "EOR #$%02X",Byte,
 "LSR",Nothing,
 "???",Nothing,
 "JMP $%04X",Word,
 "EOR $%04X",Word,
 "LSR $%04X",Word,
 "???",Nothing,
 "BVC $%04X",Relative,          /* $50 */
 "EOR ($%02X),Y",Byte,
 "EOR ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X,X",Byte,
 "LSR $%02X,X",Byte,
 "???",Nothing,
 "CLI",Nothing,                 /* $58 */
 "EOR $%04X,Y",Word,
 "PHY",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%04X,X",Word,
 "LSR $%04X,X",Word,
 "???",Nothing,
 "RTS",Nothing,                 /* $60 */
 "ADC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "STZ $%02X",Byte,
 "ADC $%02X",Byte,
 "ROR $%02X",Byte,
 "???",Nothing,
 "PLA",Nothing,                 /* $68 */
 "ADC #$%02X",Byte,
 "ROR",Nothing,
 "???",Nothing,
 "JMP ($%04X)",Word,
 "ADC $%04X",Word,
 "ROR $%04X",Word,
 "???",Nothing,
 "BVS $%04X",Relative,          /* $70 */
 "ADC ($%02X),Y",Byte,
 "ADC ($%02X)",Byte,
 "???",Nothing,
 "STZ $%02X,X",Byte,
 "ADC $%02X,X",Byte,
 "ROR $%02X,X",Byte,
 "???",Nothing,
 "SEI",Nothing,                 /* $78 */
 "ADC $%04X,Y",Word,
 "PLY",Nothing,
 "???",Nothing,
 "JMP ($%04X,X)",Word,
 "ADC $%04X,X",Word,
 "ROR $%04X,X",Word,
 "???",Nothing,
 "BRA $%04X",Relative,          /* $80 */
 "STA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "STY $%02X",Byte,
 "STA $%02X",Byte,
 "STX $%02X",Byte,
 "???",Nothing,
 "DEY",Nothing,                 /* $88 */
 "BIT #$%02X",Byte,
 "TXA",Nothing,
 "???",Nothing,
 "STY $%04X",Word,
 "STA $%04X",Word,
 "STX $%04X",Word,
 "???",Nothing,
 "BCC $%04X",Relative,          /* $90 */
 "STA ($%02X),Y",Byte,
 "STA ($%02X)",Byte,
 "???",Nothing,
 "STY $%02X,X",Byte,
 "STA $%02X,X",Byte,
 "STX $%02X,Y",Byte,
 "???",Nothing,
 "TYA",Nothing,                 /* $98 */
 "STA $%04X,Y",Word,
 "TXS",Nothing,
 "???",Nothing,
 "STZ $%04X",Word,
 "STA $%04X,X",Word,
 "STZ $%04X,X",Word,
 "???",Nothing,
 "LDY #$%02X",Byte,             /* $A0 */
 "LDA ($%02X,X)",Byte,
 "LDX #$%02X",Byte,
 "???",Nothing,
 "LDY $%02X",Byte,
 "LDA $%02X",Byte,
 "LDX $%02X",Byte,
 "???",Nothing,
 "TAY",Nothing,                 /* $A8 */
 "LDA #$%02X",Byte,
 "TAX",Nothing,
 "???",Nothing,
 "LDY $%04X",Word,
 "LDA $%04X",Word,
 "LDX $%04X",Word,
 "???",Nothing,
 "BCS $%04X",Relative,          /* $B0 */
 "LDA ($%02X),Y",Byte,
 "LDA ($%02X)",Byte,
 "???",Nothing,
 "LDY $%02X,X",Byte,
 "LDA $%02X,X",Byte,
 "LDX $%02X,Y",Byte,
 "???",Nothing,
 "CLV",Nothing,                 /* $B8 */
 "LDA $%04X,Y",Word,
 "TSX",Nothing,
 "???",Nothing,
 "LDY $%04X,X",Word,
 "LDA $%04X,X",Word,
 "LDX $%04X,Y",Word,
 "???",Nothing,
 "CPY #$%02X",Byte,             /* $C0 */
 "CMP ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPY $%02X",Byte,
 "CMP $%02X",Byte,
 "DEC $%02X",Byte,
 "???",Nothing,
 "INY",Nothing,                 /* $C8 */
 "CMP #$%02X",Byte,
 "DEX",Nothing,
 "???",Nothing,
 "CPY $%04X",Word,
 "CMP $%04X",Word,
 "DEC $%04X",Word,
 "???",Nothing,
 "BNE $%04X",Relative,          /* $D0 */
 "CMP ($%02X),Y",Byte,
 "CMP ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CMP $%02X,X",Byte,
 "DEC $%02X,X",Byte,
 "???",Nothing,
 "CLD",Nothing,                 /* $D8 */
 "CMP $%04X,Y",Word,
 "PHX",Nothing,
 "???",Nothing,
 "???",Nothing,
 "CMP $%04X,X",Word,
 "DEC $%04X,X",Word,
 "???",Nothing,
 "CPX #$%02X",Byte,             /* $E0 */
 "SBC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPX $%02X",Byte,
 "SBC $%02X",Byte,
 "INC $%02X",Byte,
 "???",Nothing,
 "INX",Nothing,                 /* $E8 */
 "SBC #$%02X",Byte,
 "NOP",Nothing,
 "???",Nothing,
 "CPX $%04X",Word,
 "SBC $%04X",Word,
 "INC $%04X",Word,
 "???",Nothing,
 "BEQ $%04X",Relative,          /* $F0 */
 "SBC ($%02X),Y",Byte,
 "SBC ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "SBC $%02X,X",Byte,
 "INC $%02X,X",Byte,
 "???",Nothing,
 "SED",Nothing,                 /* $F8 */
 "SBC $%04X,Y",Word,
 "PLX",Nothing,
 "???",Nothing,
 "???",Nothing,
 "SBC $%04X,X",Word,
 "INC $%04X,X",Word,
 "???",Nothing
};

Opcode Ops65SC02[256]=
{
 "BRK",Nothing,                 /* $00 */
 "ORA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "TSB $%02X",Byte,
 "ORA $%02X",Byte,
 "ASL $%02X",Byte,
 "RMB0 $%02X",Byte,
 "PHP",Nothing,                 /* $08 */
 "ORA #$%02X",Byte,
 "ASL",Nothing,
 "???",Nothing,
 "TSB $%04X",Word,
 "ORA $%04X",Word,
 "ASL $%04X",Word,
 "BBR0 $%02X $%04X",TestBit,
 "BPL $%04X",Relative,          /* $10 */
 "ORA ($%02X),Y",Byte,
 "ORA ($%02X)",Byte,
 "???",Nothing,
 "TRB $%02X",Byte,
 "ORA $%02X,X",Byte,
 "ASL $%02X,X",Byte,
 "RMB1 $%02X",Byte,
 "CLC",Nothing,                 /* $18 */
 "ORA $%04X,Y",Word,
 "INC",Nothing,
 "???",Nothing,
 "TRB $%04X",Word,
 "ORA $%04X,X",Word,
 "ASL $%04X,X",Word,
 "BBR1 $%02X $%04X",TestBit,
 "JSR $%04X",Word,              /* $20 */
 "AND ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "BIT $%02X",Byte,
 "AND $%02X",Byte,
 "ROL $%02X",Byte,
 "RMB2 $%02X",Byte,
 "PLP",Nothing,                 /* $28 */
 "AND #$%02X",Byte,
 "ROL",Byte,
 "???",Nothing,
 "BIT $%04X",Word,
 "AND $%04X",Word,
 "ROL $%04X",Word,
 "BBR2 $%02X $%04X",TestBit,
 "BMI $%04X",Relative,          /* $30 */
 "AND ($%02X),Y",Byte,
 "AND ($%02X)",Byte,
 "???",Nothing,
 "BIT $%02X,X",Byte,
 "AND $%02X,X",Byte,
 "ROL $%02X,X",Byte,
 "RMB3 $%02X,X",Byte,
 "SEC",Nothing,                 /* $38 */
 "AND $%04X,Y",Word,
 "DEC",Nothing,
 "???",Nothing,
 "BIT $%04X,X",Word,
 "AND $%04X,X",Word,
 "ROL $%04X,X",Word,
 "BBR3 $%02X $%04X",TestBit,
 "RTI",Nothing,                 /* $40 */
 "EOR ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X",Byte,
 "LSR $%02X",Byte,
 "RMB4 $%02X",Byte,
 "PHA",Nothing,                 /* $48 */
 "EOR #$%02X",Byte,
 "LSR",Nothing,
 "???",Nothing,
 "JMP $%04X",Word,
 "EOR $%04X",Word,
 "LSR $%04X",Word,
 "BBR4 $%02X $%04X",TestBit,
 "BVC $%04X",Relative,          /* $50 */
 "EOR ($%02X),Y",Byte,
 "EOR ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "EOR $%02X,X",Byte,
 "LSR $%02X,X",Byte,
 "RMB5 $%02X",Byte,
 "CLI",Nothing,                 /* $58 */
 "EOR $%04X,Y",Word,
 "PHY",Nothing,
 "???",Nothing,
 "???",Nothing,
 "EOR $%04X,X",Word,
 "LSR $%04X,X",Word,
 "BBR5 $%02X $%04X",TestBit,
 "RTS",Nothing,                 /* $60 */
 "ADC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "STZ $%02X",Byte,
 "ADC $%02X",Byte,
 "ROR $%02X",Byte,
 "RMB6 $%02X",Byte,
 "PLA",Nothing,                 /* $68 */
 "ADC #$%02X",Byte,
 "ROR",Nothing,
 "???",Nothing,
 "JMP ($%04X)",Word,
 "ADC $%04X",Word,
 "ROR $%04X",Word,
 "BBR6 $%02X $%04X",TestBit,
 "BVS $%04X",Relative,          /* $70 */
 "ADC ($%02X),Y",Byte,
 "ADC ($%02X)",Byte,
 "???",Nothing,
 "STZ $%02X,X",Byte,
 "ADC $%02X,X",Byte,
 "ROR $%02X,X",Byte,
 "RMB7 $%02X",Byte,
 "SEI",Nothing,                 /* $78 */
 "ADC $%04X,Y",Word,
 "PLY",Nothing,
 "???",Nothing,
 "JMP ($%04X,X)",Word,
 "ADC $%04X,X",Word,
 "ROR $%04X,X",Word,
 "BBR7 $%02X $%04X",TestBit,
 "BRA $%04X",Relative,          /* $80 */
 "STA ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "STY $%02X",Byte,
 "STA $%02X",Byte,
 "STX $%02X",Byte,
 "SMB0 $%02X",Byte,
 "DEY",Nothing,                 /* $88 */
 "BIT #$%02X",Byte,
 "TXA",Nothing,
 "???",Nothing,
 "STY $%04X",Word,
 "STA $%04X",Word,
 "STX $%04X",Word,
 "BBS0 $%02X $%04X",TestBit,
 "BCC $%04X",Relative,          /* $90 */
 "STA ($%02X),Y",Byte,
 "STA ($%02X)",Byte,
 "???",Nothing,
 "STY $%02X,X",Byte,
 "STA $%02X,X",Byte,
 "STX $%02X,Y",Byte,
 "SMB1 $%02X",Byte,
 "TYA",Nothing,                 /* $98 */
 "STA $%04X,Y",Word,
 "TXS",Nothing,
 "???",Nothing,
 "STZ $%04X",Word,
 "STA $%04X,X",Word,
 "STZ $%04X,X",Word,
 "BBS1 $%02X $%04X",TestBit,
 "LDY #$%02X",Byte,             /* $A0 */
 "LDA ($%02X,X)",Byte,
 "LDX #$%02X",Byte,
 "???",Nothing,
 "LDY $%02X",Byte,
 "LDA $%02X",Byte,
 "LDX $%02X",Byte,
 "SMB2 $%02X",Byte,
 "TAY",Nothing,                 /* $A8 */
 "LDA #$%02X",Byte,
 "TAX",Nothing,
 "???",Nothing,
 "LDY $%04X",Word,
 "LDA $%04X",Word,
 "LDX $%04X",Word,
 "BBS2 $%02X $%04X",TestBit,
 "BCS $%04X",Relative,          /* $B0 */
 "LDA ($%02X),Y",Byte,
 "LDA ($%02X)",Byte,
 "???",Nothing,
 "LDY $%02X,X",Byte,
 "LDA $%02X,X",Byte,
 "LDX $%02X,Y",Byte,
 "SMB3 $%02X",Byte,
 "CLV",Nothing,                 /* $B8 */
 "LDA $%04X,Y",Word,
 "TSX",Nothing,
 "???",Nothing,
 "LDY $%04X,X",Word,
 "LDA $%04X,X",Word,
 "LDX $%04X,Y",Word,
 "BBS3 $%02X $%04X",TestBit,
 "CPY #$%02X",Byte,             /* $C0 */
 "CMP ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPY $%02X",Byte,
 "CMP $%02X",Byte,
 "DEC $%02X",Byte,
 "SMB4 $%02X",Byte,
 "INY",Nothing,                 /* $C8 */
 "CMP #$%02X",Byte,
 "DEX",Nothing,
 "???",Nothing,
 "CPY $%04X",Word,
 "CMP $%04X",Word,
 "DEC $%04X",Word,
 "BBS4 $%02X $%04X",TestBit,
 "BNE $%04X",Relative,          /* $D0 */
 "CMP ($%02X),Y",Byte,
 "CMP ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CMP $%02X,X",Byte,
 "DEC $%02X,X",Byte,
 "SMB5 $%02X",Byte,
 "CLD",Nothing,                 /* $D8 */
 "CMP $%04X,Y",Word,
 "PHX",Nothing,
 "???",Nothing,
 "???",Nothing,
 "CMP $%04X,X",Word,
 "DEC $%04X,X",Word,
 "BBS5 $%02X $%04X",TestBit,
 "CPX #$%02X",Byte,             /* $E0 */
 "SBC ($%02X,X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "CPX $%02X",Byte,
 "SBC $%02X",Byte,
 "INC $%02X",Byte,
 "SMB6 $%02X",Byte,
 "INX",Nothing,                 /* $E8 */
 "SBC #$%02X",Byte,
 "NOP",Nothing,
 "???",Nothing,
 "CPX $%04X",Word,
 "SBC $%04X",Word,
 "INC $%04X",Word,
 "BBS6 $%02X $%04X",TestBit,
 "BEQ $%04X",Relative,          /* $F0 */
 "SBC ($%02X),Y",Byte,
 "SBC ($%02X)",Byte,
 "???",Nothing,
 "???",Nothing,
 "SBC $%02X,X",Byte,
 "INC $%02X,X",Byte,
 "SMB7 $%02X",Byte,
 "SED",Nothing,                 /* $F8 */
 "SBC $%04X,Y",Word,
 "PLX",Nothing,
 "???",Nothing,
 "???",Nothing,
 "SBC $%04X,X",Word,
 "INC $%04X,X",Word,
 "BBS7 $%02X $%04X",TestBit
};

Opcode *Ops=Ops65C02;

/* dasm()
   Disassemble one opcode:
   given a buffer in which to copy the disassembly and the current IP,
   returns the new IP. */

static int dasm(char *output, unsigned short int address)
{
 /* Addresses used by branch ops are relative to address+2, signed char */
 unsigned int num;
 signed char Rel;
 unsigned int p1,p2;
 unsigned int op;
 char Desc[128];

 /* We must keep the compiler from promoting signed values, so often we
    explicitly cast to unsigned char. */
 op=(unsigned char)Rd6502(address);

 if (Ops[op].Method==Nothing) /* Just the opcode */
 {
  sprintf (output,"%04X:  %02X            %s",address,op,Ops[op].Description);
  return address+1;
 }
 if (Ops[op].Method==Byte) /* Two byte opcode */
 {
  sprintf (Desc,Ops[op].Description,(unsigned char)(p1=Rd6502(address+1)));
  sprintf (output,"%04X:  %02X%02X          %s",address,op,(unsigned char)p1,Desc);
  return address+2;
 }
 if (Ops[op].Method==Word) /* Three byte opcode */
 {
  p1=(unsigned char)Rd6502(address+1);
  p2=(unsigned char)Rd6502(address+2);
  num=p1+(p2<<8); /* Construct low-endian number */
  sprintf (Desc,Ops[op].Description,num);
  sprintf (output,"%04X:  %02X%02X%02X        %s",address,op,p1,p2,Desc);
  return address+3;
 }
 if (Ops[op].Method==Relative) /* Two byte opcode */
 {
  /* This is a little tricky... The relative address is a signed char. */
  Rel=(signed char)Rd6502(address+1);
  num=(address+2);
  num+=Rel; /* this CAN subtract; remember a+(-b) is the same as a-b */
  sprintf (Desc,Ops[op].Description,(unsigned int)num);
  sprintf (output,"%04X:  %02X%02X          %s",address,op,(unsigned char)Rel,Desc);
  return address+2;
 }
 if (Ops[op].Method==TestBit) /* Three byte opcode */
 {
  p1=(unsigned char)Rd6502(address+1);
  Rel=(signed char)Rd6502(address+2);
  num=(address+2);
  num+=Rel; /* this CAN subtract; remember a+(-b) is the same as a-b */
  sprintf (Desc,Ops[op].Description,(unsigned int)num);
  sprintf (output,"%04X:  %02X%02X%02X        %s",address,op,p1,(unsigned char)Rel,Desc);
 }
 if (Ops[op].Method==Rel16) /* Three byte opcode */
 {
  int p16;

  p1=(unsigned char)Rd6502(address+1);
  p2=(unsigned char)Rd6502(address+2);
  num=p1+(p2<<8); /* Construct low-endian number */
  p16=(unsigned int)num+(unsigned int)(address+3);
  sprintf (Desc,Ops[op].Description,p16);
  sprintf (output,"%04X:  %02X%02X%02X        %s",address,op,p1,p2,Desc);
  return address+3;
 }
 /* error : should never reach this point but placate compilers */
 return -1;
}

void Debug6502 (void)
{
 int i, j;
 char input[128], dasmop[128];
 extern unsigned char virtcopy;
 unsigned char flags;

 debugger = 1; /* We are in debug mode now */

// gmode(3); clrscr(); //_setcursortype(_NORMALCURSOR);

// gotoxy(1,1);

vtop:
 dasm(dasmop, cpugetpc() );
 flags = cpuflagsgen();

 printf ("%s\n",dasmop);
 printf ("       A=%02X X=%02X Y=%02X S=%02X P=%02X M=%02X [%c%c%c%c%c%c%c%c]\n",
         cpugeta(), cpugetx(), cpugety(), cpugetsp(), flags, Rd6502( cpugetpc() ),
         flags&0x80?'N':'n',
         flags&0x40?'V':'v',
         flags&0x20?'R':'r',
         flags&0x10?'B':'b',
         flags&0x08?'D':'d',
         flags&0x04?'I':'i',
         flags&0x02?'Z':'z',
         flags&0x01?'C':'c');
 printf ("SP=01%02X -> %02X %02X %02X\n",
         (unsigned char)(cpugetsp()),
         Rd6502( cpugetsp() + 1 ),
         Rd6502( cpugetsp() + 2 ),
         Rd6502( cpugetsp() + 3 ));
 printf ("(? for help)\n");
top:
 switch (getcputype())
 {
  case CPU6502:   printf ("{6502} ");   break;
  case CPU65C02:  printf ("{65C02} ");  break;
  case CPU65SC02: printf ("{65SC02} "); break;
  default:        printf ("{CPU?} ");   break;
 }
 fgets (input,128,stdin);
 input[strlen(input)-1]=0;
 if (!*input) {
   cpusettracemode(1);
   goto btm;
 }

 switch (input[0])
 {
   case '?' :
   case 'h' :
   case 'H' :
     printf ("\
<Enter>    : Execute one opcode\n\
= [addr]   : Set breakpoint at addr (disable if none specified)\n\
+ offset   : Set breakpoint at current address + offset\n\
g          : Disable breakpoint and resume\n\
j addr     : Jump to addr\n\
d [addr]   : Dump memory at addr\n\
l [addr]   : Disassemble memory at addr\n\
v          : Display interrupt vectors\n\
h, ?       : Display this text\n\
q          : Exit to DOS\n\
");
    goto top;
   case 'q' :
   case 'Q' :
     exitprogram =1;
     cpuclearbreakpoint();
     cpusettracemode(0);
     goto btm;
  case '=':
   if(strlen(input)>=2)
   {
    unsigned short address;
    sscanf(input,"=%x",&address);
    cpusetbreakpoint(address);
    cpusettracemode(0);
    goto btm;
   }
   cpuclearbreakpoint();
   cpusettracemode(1);
   goto btm;
  case '+':
   if(strlen(input)>=2)
   {
    unsigned short address;
    sscanf(input,"+%x",&address);
    cpusetbreakpoint( cpugetpc() + address );
    cpusettracemode(0);
    goto btm;
   }
   printf ("Error: No address specified\n");
   goto top;
  case 'j':
  case 'J':
   if(strlen(input)>=2)
   {
    unsigned short address;             /* jump does not clear any breakpoint */
    if (sscanf(&(input[1]),"%x",&address))
    {
     cpusetpc(address);
     cpusettracemode(0);
    }
    else /* no address specified */
    {
     printf ("Error: No address specified\n");
     goto top;
    }
    goto btm;
   }
   break;
  case 'g':
  case 'G':
    cpuclearbreakpoint();
    cpusettracemode(0);
    goto btm;
  case 'v' :
  case 'V' :
   printf ("\
Reset  ($FFFC):  $%04X\n\
IRQ    ($FFFE):  $%04X\n\
NMI    ($FFFA):  $%04X\n\
", Rd6502(0xFFFC)+256*Rd6502(0xFFFD),
   Rd6502(0xFFFE)+256*Rd6502(0xFFFF),
   Rd6502(0xFFFA)+256*Rd6502(0xFFFB));
   goto top;
// break;
  case 'd': case 'D':
  {
   unsigned short Addr;

   if(strlen(input)>1)
    sscanf(&(input[1]),"%x",&Addr);
   else
    Addr = cpugetpc();

   printf ("\n");

   for (j=0; j<16; j++)
   {
    printf ("%04X: ",Addr);
    for (i=Addr; i<(Addr+16); i++)
     printf ("%02X ",Rd6502(i));
    for (i=Addr; i<(Addr+16); i++)
     if ((Rd6502(i)>0x1F) && (Rd6502(i)<0x80))
      printf ("%c",Rd6502(i));
     else
      printf (".");
    printf ("\n");
    Addr+=16;
   }
   goto top;
// break;
  case 'l': case 'L':
  {
   unsigned short Addr;

   if(strlen(input)>1)
    sscanf(&(input[1]),"%x",&Addr);
   else
    Addr = cpugetpc();
   puts("");
   for(j=0; j<16; j++)
   {
    Addr=dasm(input,Addr);
    printf ("%s\n",input);
   }
  }
  goto top;
//break;
 }
 }

 printf ("Error: Invalid command (? for help)\n");
 goto top;

btm:
 //clrscr();
  opengraph(); virtcopy=1; debugger = 0;

 return;
}
