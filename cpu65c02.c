/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  CPU65C02: CPU emulation (replaces M6502)
 * Revision:   (1.20) 2002.1229
 * The contributor of this code wishes to remain anonymous :p
 * ADC and SBC instructions contributed by Scott Hemphill.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include "dapple.h"
#include "cpu65c02.h"

#define readbus(addr) Rd6502(addr)
#define writebus(addr, value) Wr6502(addr, value)

/* type of CPU */

typedef enum {CPU6502, CPU65C02, CPU65SC02} CPUTYPE;
/*
#define CPU6502         0
#define CPU65C02        1
#define CPU65SC02       2
static unsigned char cputype;
 */
CPUTYPE cputype;

static unsigned char areg;              /* Accumulator          */
static unsigned char xreg;              /* X-Register           */
static unsigned char yreg;              /* Y-Register           */
static unsigned char stack;             /* Stack-Register       */
static unsigned short int pc;           /* Program counter      */

static unsigned char nflag;             /* N-Flag               */
static unsigned char vflag;             /* Overflow-Flag        */
static unsigned char iflag;             /* Interrupt-Flag       */
static unsigned char bflag;             /* Break-Flag           */
static unsigned char dflag;             /* Decimal-Flag         */
static unsigned char zflag;             /* Zero-Flag            */
static unsigned char cflag;             /* Carry-Flag           */

static unsigned char bytebuffer;
static unsigned short address;          /* 16-bit address       */
unsigned long cycle;                    /* Number of cycles     */
unsigned long lastcycle;
unsigned long linecycles;
static unsigned char traceflag;         /* set tracemode        */
static unsigned char breakflag;         /* allow breakpoint     */
static unsigned short breakpoint;
static unsigned char flashloop = 0;
unsigned int hold = 0;
unsigned short rasterline = 0;

/* state of the processor */
unsigned char stateflags;
#define STATEHALT  0x01
#define STATERESET 0x02
#define STATENMI   0x04
#define STATEIRQ   0x08
#define STATEBRK   0x10
#define STATETRACE 0x20
#define STATEBPT   0x40
#define STATEGURU  0x80

/* dapple.c */
extern unsigned char Rd6502(register unsigned short addr);
extern void Wr6502(register unsigned short addr, register unsigned char value);
extern unsigned char exitprogram;
extern unsigned int dvalue1;
extern unsigned int dvalue2;
extern unsigned int dvalue3;
extern unsigned int dvalue4;
extern void PC2APL();

/* video.c */
extern void virtpalettesetd(register unsigned char index, register unsigned char r,
                            register unsigned char b,     register unsigned char g);
extern void virtline(unsigned short rastline);
extern void virtscreencopy();

/* debug.c */
extern void Debug6502();

/*-------------------------------------*/

/* CPU */
/* - cpusetbreakpoint   */
/* - cpugetbreakpoint   */
/* - cpuclearbreakpoint */
/* - cpusettracemode    */
/* - cpugettracemode    */
/* - cpusetpc           */
/* - cpugetpc           */
/* - cpugeta            */
/* - cpugetx            */
/* - cpugety            */
/* - cpureset           */
/* - cpuinit            */
/* - cpustore           */
/* - cpurestore         */
/* - cpuline            */

/*-------------------------------------*/

/* flags                 */
/* - flagssetnz          */
/* - flagsgen            */
/* - flagsget            */
/* - flagsstring (nimp.) */

      void flagssetnz(register unsigned char value) {

        zflag = (value == 0) ? 1 : 0;
        nflag = (value > 0x7f) ? 1 : 0;

      } /* flagssetnz */


/*-------------------------------------*/


      unsigned char cpuflagsgen() {
        register unsigned char f;

        f = 0x20;
        if ( nflag != 0 ) { f = f | 0x80; }
        if ( vflag != 0 ) { f = f | 0x40; }
        if ( bflag != 0 ) { f = f | 0x10; }
        if ( dflag != 0 ) { f = f | 0x08; }
        if ( iflag != 0 ) { f = f | 0x04; }
        if ( zflag != 0 ) { f = f | 0x02; }
        if ( cflag != 0 ) { f = f | 0x01; }
        return(f);
      } /* flagsgen */


/*-------------------------------------*/


      void cpuflagsget(register unsigned char value) {

        nflag = ((value & 0x80) == 0) ? 0 : 1;
        vflag = ((value & 0x40) == 0) ? 0 : 1;
        bflag = ((value & 0x10) == 0) ? 0 : 1;
        dflag = ((value & 0x08) == 0) ? 0 : 1;
        iflag = ((value & 0x04) == 0) ? 0 : 1;
        zflag = ((value & 0x02) == 0) ? 0 : 1;
        cflag = ((value & 0x01) == 0) ? 0 : 1;
      } /* flagsget */

      void cpusetbreakpoint(unsigned short value) {

        breakpoint = value;
        breakflag  = 1;

      } /* cpusetbreakpoint */


/*-------------------------------------*/


      unsigned short cpugetbreakpoint() {

        if (breakflag) {
          return breakpoint;
        }
        else {
          return 0xffff;
        }

      } /* cpugetbreakpoint */


/*-------------------------------------*/


      void cpuclearbreakpoint() {

        breakflag = 0;

      } /* cpuclearbreakpoint */


/*-------------------------------------*/


      void cpureset() {

        stateflags = STATERESET;

      } /* cpureset */


/*-------------------------------------*/


      void cpusettracemode(unsigned char mode) {

        if (mode) {
          traceflag = 1;
        }
        else {
          traceflag = 0;
        }
      } /* cpusettracemode */


/*-------------------------------------*/


      unsigned char cpugettracemode() {

        return traceflag;

      } /* cougettracemode */


/*-------------------------------------*/


      void cpusetpc(unsigned short address) {

        pc = address;

      } /* cpusetpc */



/*-------------------------------------*/


      unsigned short cpugetpc() {

        return pc;

      } /* cpugetpc */


/*-------------------------------------*/


      unsigned short cpugetsp() {

        return (stack | 0x100);

      } /* cpugetsp */


/*-------------------------------------*/


      unsigned char cpugeta() {

        return areg;

      } /* cpugeta */


/*-------------------------------------*/


      unsigned char cpugetx() {

        return xreg;

      } /* cpugetx */


/*-------------------------------------*/


      unsigned char cpugety() {

        return yreg;

      } /* cpugety */


/*-------------------------------------*/


      void cpuinit() {

        areg    = 0;                    /* clear register */
        xreg    = 0;
        yreg    = 0;
        zflag   = 0;                    /* clear flags */
        nflag   = 0;
        cflag   = 0;
        vflag   = 0;
        iflag   = 0;
        dflag   = 0;
        bflag   = 0;
        stack   = 0xff;                 /* + 0x100 ! ==> pullstack/pushstack */

        cycle   = 0;                    /* clear cycle count */
        lastcycle = 0;
        linecycles = 65 * 1;

        cpusettracemode(0);             /* trace mode off */
        cpuclearbreakpoint();           /* no breakpoint */

        cpureset();

      } /* cpuinit */


/*-------------------------------------*/


      void cpustore(FILE *file) {

        unsigned char flags;

        fwrite(&areg,sizeof(areg),1,file);
        fwrite(&xreg,sizeof(xreg),1,file);
        fwrite(&yreg,sizeof(yreg),1,file);
        fwrite(&pc,sizeof(pc),1,file);
        fwrite(&stack,sizeof(stack),1,file);
        flags = cpuflagsgen();
        fwrite(&flags,sizeof(flags),1,file);
        fwrite(&stateflags,sizeof(stateflags),1,file);
        fwrite(&cycle,sizeof(cycle),1,file);
        fwrite(&lastcycle,sizeof(lastcycle),1,file);
        fwrite(&traceflag,sizeof(traceflag),1,file);
        fwrite(&breakflag,sizeof(breakflag),1,file);
        fwrite(&breakpoint,sizeof(breakpoint),1,file);
        fwrite(&hold,sizeof(hold),1,file);
        fwrite(&flashloop,sizeof(flashloop),1,file);
        fwrite(&rasterline,sizeof(rasterline),1,file);

      } /* cpustore */


/*-------------------------------------*/


      void cpurestore(FILE *file) {

        unsigned char flags;

        fread(&areg,sizeof(areg),1,file);
        fread(&xreg,sizeof(xreg),1,file);
        fread(&yreg,sizeof(yreg),1,file);
        fread(&pc,sizeof(pc),1,file);
        fread(&stack,sizeof(stack),1,file);
        fread(&flags,sizeof(flags),1,file);
        cpuflagsget(flags);
        fread(&stateflags,sizeof(stateflags),1,file);
        fread(&cycle,sizeof(cycle),1,file);
        fread(&lastcycle,sizeof(lastcycle),1,file);
        fread(&traceflag,sizeof(traceflag),1,file);
        fread(&breakflag,sizeof(breakflag),1,file);
        fread(&breakpoint,sizeof(breakpoint),1,file);
        fread(&hold,sizeof(hold),1,file);
        fread(&flashloop,sizeof(flashloop),1,file);
        fread(&rasterline,sizeof(rasterline),1,file);

      } /* cpurestore */


/*-------------------------------------*/


/*
      String flagsstring(int flags) {
        String st;

        if ( (flags & 0x80) == 0) { st="-"; }
        else                      { st="N"; }
        if ( (flags & 0x40) == 0) { st=st+"-"; }
        else                      { st=st+"V"; }
        if ( (flags & 0x20) == 0) { st=st+"-"; }
        else                      { st=st+"1"; }
        if ( (flags & 0x10) == 0) { st=st+"-"; }
        else                      { st=st+"B"; }
        if ( (flags & 0x08) == 0) { st=st+"-"; }
        else                      { st=st+"D"; }
        if ( (flags & 0x04) == 0) { st=st+"-"; }
        else                      { st=st+"I"; }
        if ( (flags & 0x02) == 0) { st=st+"-"; }
        else                      { st=st+"Z"; }
        if ( (flags & 0x01) == 0) { st=st+"-"; }
        else                      { st=st+"C"; }
        return(st);
      } /* flagsstring */


/*-------------------------------------*/


/* addressing modes */

      unsigned char readzp() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readzp */


      unsigned char readzpx() {
        address = (readbus(pc) + xreg) & 0xff;
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readzpx */


      unsigned char readzpy() {
        address = (readbus(pc) + yreg) & 0xff;
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readzpy */


      unsigned char readabs() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = address + ( readbus(pc) << 8 );
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readabs */


      unsigned char readabsx() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = (address + ( readbus(pc) << 8 ) + xreg) & 0xffff;
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readabsx */


      unsigned char readabsy() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = (address + ( readbus(pc) << 8 ) + yreg) & 0xffff;
        pc      = (pc + 1) & 0xffff;
        return readbus(address);
      } /* readabsy */


      unsigned char readindzp() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = readbus(address) + ( readbus((address + 1) & 0xff) << 8);
        return readbus(address);
      } /* readindzp */


      unsigned char readindzpx() {
        address = ( readbus(pc) + xreg ) & 0xff;
        pc      = (pc + 1) & 0xffff;
        address = readbus(address) + ( readbus((address + 1) & 0xff) << 8);
        return readbus(address);
      } /* readindzpx */


      unsigned char readindzpy() {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = ( readbus(address) + ( readbus((address + 1) & 0xff) << 8)
                                + yreg) & 0xffff;
        return readbus(address);
      } /* readindzpy */



      void writezp (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writezp */


      void writezpx (register unsigned char value) {
        address = ( readbus(pc) + xreg ) & 0xff;
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writezpx */


      void writezpy (register unsigned char value) {
        address = ( readbus(pc) + yreg ) & 0xff;
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writezpy */


      void writeabs (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = address + ( readbus(pc) << 8 );
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writeabs */


     void writeabsx (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = (address + ( readbus(pc) << 8 ) + xreg) & 0xffff;
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writeabsx */


      void writeabsy (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = (address + ( readbus(pc) << 8 ) + yreg) & 0xffff;
        pc      = (pc + 1) & 0xffff;
        writebus(address, value);
      } /* writeabsy */


      void writeindzp (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = readbus(address) + ( readbus((address + 1) & 0xff) << 8);
        writebus(address, value);
      } /* writeindzp */


      void writeindzpx (register unsigned char value) {
        address = ( readbus(pc) + xreg ) & 0xff;
        pc      = (pc + 1) & 0xffff;
        address = readbus(address) + ( readbus((address + 1) & 0xff) << 8);
        writebus(address, value);
      } /* writeindzpx */


      void writeindzpy (register unsigned char value) {
        address = readbus(pc);
        pc      = (pc + 1) & 0xffff;
        address = ( readbus(address) + ( readbus((address + 1) & 0xff) << 8 )
                    + yreg) & 0xffff;
        writebus(address, value);
      } /* writeindzpx */



/*-------------------------------------*/



/* Stack */

        unsigned char pullstack() {
          stack = (stack + 1) & 0xff;
          return readbus(stack | 0x100);
        } /* pullstack */


        void pushstack (register unsigned char value) {
          writebus(stack | 0x100, value);
          stack = (stack - 1) & 0xff;
        } /* pushstack */



/*-------------------------------------*/



/* Adc */

/* The following code was provided by Mr. Scott Hemphill. */
/* Thanks a lot! */

      void adc(register unsigned char value) {

        register unsigned short w;

        if ((areg ^ value) & 0x80) {
          vflag = 0;
        }
        else {
          vflag = 1;
        }

        if (dflag) {
          w = (areg & 0xf) + (value & 0xf) + cflag;
          if (w >= 10) {
            w = 0x10 | ((w+6)&0xf);
          }
          w += (areg & 0xf0) + (value & 0xf0);
          if (w >= 160) {
            cflag = 1;
            if (vflag && w >= 0x180) vflag = 0;
            w += 0x60;
          }
          else {
            cflag = 0;
            if (vflag && w < 0x80) vflag = 0;
          }
        }
        else {
          w = areg + value + cflag;
          if (w >= 0x100) {
            cflag = 1;
            if (vflag && w >= 0x180) vflag = 0;
          }
          else {
            cflag = 0;
            if (vflag && w < 0x80) vflag = 0;
          }
        }
        areg = (unsigned char)w;
        nflag = (areg >= 0x80) ? 1 : 0;
        zflag = (areg == 0)    ? 1 : 0;
      } /* adc */


/* Sbc */

/* The following code was provided by Mr. Scott Hemphill. */
/* Thanks a lot again! */

      void sbc(register unsigned char value) {

        register unsigned short w;
        register unsigned char temp;

        if ((areg ^ value) & 0x80) {
          vflag = 1;
        }
        else {
          vflag = 0;
        }

        if (dflag) {            /* decimal subtraction */
          temp = 0xf + (areg & 0xf) - (value & 0xf) + (cflag);
          if (temp < 0x10) {
            w = 0;
            temp -= 6;
          }
          else {
            w = 0x10;
            temp -= 0x10;
          }
          w += 0xf0 + (areg & 0xf0) - (value & 0xf0);
          if (w < 0x100) {
            cflag = 0;
            if (vflag && w < 0x80) vflag = 0;
            w -= 0x60;
          }
          else {
            cflag = 1;
            if ((vflag) && w >= 0x180) vflag = 0;
          }
          w += temp;
        }
        else {                  /* standard binary subtraction */
          w = 0xff + areg - value + cflag;
          if (w < 0x100) {
            cflag = 0;
            if (vflag && w < 0x80) vflag = 0;
          }
          else {
            cflag = 1;
            if (vflag && w >= 0x180) vflag = 0;
          }
        }
        areg = (unsigned char)w;
        nflag = (areg >= 0x80) ? 1 : 0;
        zflag = (areg == 0)    ? 1 : 0;
      } /* sbc */


/*-------------------------------------*/


/* Cmp */

      void cmp(register unsigned char value) {

        cflag = (areg >= value) ? 1 : 0;
        nflag = (((areg - value) & 0x80) == 0) ? 0 : 1;
        zflag = (areg == value) ? 1 : 0;

      } /* cmp */

/* Cpx */

      void cpx(register unsigned char value) {

        cflag = (xreg >= value) ? 1 : 0;
        nflag = (((xreg - value) & 0x80) == 0) ? 0 : 1;
        zflag = (xreg == value) ? 1 : 0;

      } /* cpx */

/* Cpy */

      void cpy(register unsigned char value) {

        cflag = (yreg >= value) ? 1 : 0;
        nflag = (((yreg - value) & 0x80) == 0) ? 0 : 1;
        zflag = (yreg == value) ? 1 : 0;

      } /* cpy */


/*-------------------------------------*/


      void cpuline() {


            do { /* ... while (!tracemode) */

              while (stateflags) {
                if (stateflags & STATERESET) {
                  stateflags = stateflags & ~(STATEHALT | STATERESET);
                  dflag = 0;            /* on a 65c02 the dflag gets cleared on reset */
                  pc = readbus(0xfffc) | (readbus(0xfffd) << 8);
                  cycle = cycle + 5;
                }
                if (stateflags & STATEHALT) {   /* processor still halted? */
                  virtscreencopy();
                  return;
                }
                if (stateflags & STATENMI) {
                  stateflags = stateflags & ~STATENMI;
                  pushstack(pc >> 8);
                  pushstack(pc & 0xff);
                  pushstack( cpuflagsgen() );
                  pc    = readbus(0xfffa) | (readbus(0xfffb) << 8);
                  cycle = cycle + 6;
                }
                if (stateflags & STATEIRQ) {
                  stateflags = stateflags & ~STATEIRQ;
                  pushstack(pc >> 8);
                  pushstack(pc & 0xff);
                  pushstack( cpuflagsgen() );
                  iflag = 1;
                  pc    = readbus(0xfffe) | (readbus(0xffff) << 8);
                  cycle = cycle + 6;
                }
                if (stateflags & STATEBRK) {
                  stateflags = stateflags & ~STATEBRK;
                  pc    = readbus(0xfffe) | (readbus(0xffff) << 8);
                  cycle = cycle + 7;
                }
                stateflags = stateflags &  ~(STATETRACE | STATEGURU | STATEBPT);        /* remove these */
               } /* while (stateflags) */


              if (pc == breakpoint) {
                if (breakflag) {        /* breakpoint allowed? */
                  stateflags = stateflags | STATEBPT;
                  virtscreencopy();
                  return;
                }
              }
              if ((cycle - lastcycle) >= linecycles) {
                lastcycle = lastcycle + linecycles;
                if (rasterline > 265) {
                  rasterline = 0;
                  flashloop++;
                  if ((flashloop&0x12)==0x12) {
                    virtpalettesetd(COL_TXT_WHT2,0x00,0x00,0x00);
                    virtpalettesetd(COL_TXT_WHT3,0xFF,0xFF,0xFF);
                    virtpalettesetd(COL_TXT_AMB2,0x00,0x00,0x00);
                    virtpalettesetd(COL_TXT_AMB3,0xbf,0x5f,0x00);
                    virtpalettesetd(COL_TXT_GRN2,0x00,0x00,0x00);
                    virtpalettesetd(COL_TXT_GRN3,0x00,0xAF,0x00);
                  }
                  else {
                    if ((flashloop&0x12)==0) {
                      virtpalettesetd(COL_TXT_WHT2,0xFF,0xFF,0xFF);
                      virtpalettesetd(COL_TXT_WHT3,0x00,0x00,0x00);
                      virtpalettesetd(COL_TXT_AMB2,0xbf,0x5f,0x00);
                      virtpalettesetd(COL_TXT_AMB3,0x00,0x00,0x00);
                      virtpalettesetd(COL_TXT_GRN2,0x00,0xAF,0x00);
                      virtpalettesetd(COL_TXT_GRN3,0x00,0x00,0x00);
                    }
                  }
                  virtscreencopy();
                  return;       /* jump back to PC2APL */
                } /* rasterline > 265 */
                else {
                  if (rasterline <= 191) {
                    virtline(rasterline);
                  }
                }
                rasterline++;
              } /* cycle >= 65 */

              /* interpret next opcode */
              bytebuffer        = readbus(pc);
              pc                = (pc + 1) & 0xffff;
              switch (bytebuffer) {
                case 0x00 :                             /* BRK */
                  stateflags = stateflags | STATEBRK;   /* give the surrounding environment     */
                  pc    = pc + 2;                       /* the chance to respond to a BRK       */
                  pushstack(pc >> 8);                   /* e.g. with a debugger                 */
                  pushstack(pc & 0xff);
                  pc    = address + ( readbus(pc) << 8 );
                  pushstack( cpuflagsgen() | 0x10 );
                  dflag = 0;                            /* The 65c02 clears the D-flag. */
                  iflag = 1;
                  virtscreencopy();
                  return;
                case 0x01 :                             /* ORA (zp,x) */
                  areg  = areg | readindzpx();
                  flagssetnz(areg);
                  cycle = cycle + 6;
                  break;
                case 0x02 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x03 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x04 :                             /* TSB zp */
                  bytebuffer    = readzp();
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  writebus(address, areg | bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x05 :                             /* ORA zp */
                  areg  = areg | readzp();
                  flagssetnz(areg);
                  cycle = cycle + 3;
                  break;
                case 0x06 :                             /* ASL zp */
                  bytebuffer    = readzp();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = (bytebuffer << 1) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = bytebuffer << 1;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x07 :                             /* RMB0 zp */
                  bytebuffer    = readzp() & 0xfe;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x08 :                             /* PHP */
                  pushstack( cpuflagsgen() );
                  cycle = cycle + 3;
                  break;
                case 0x09 :                             /* ORA # */
                  areg  = areg | readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x0a :                             /* ASL */
                  if ( areg > 0x7f ) {
                    areg        = (areg << 1) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    areg        = areg << 1;
                    cflag       = 0;
                  }
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x0b :                             /* NOP 65C02 */
                  cycle = cycle + 1;
                  break;
                case 0x0c :                             /* TSB abs */
                  bytebuffer    = readabs();
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  writebus(address, areg | bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x0d :                             /* ORA abs */
                  areg  = areg | readabs();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x0e :                             /* ASL abs */
                  bytebuffer    = readabs();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = (bytebuffer << 1) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = bytebuffer << 1;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x0f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x01)) {           /* BBR0 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x10 :                             /* BPL */
                  if ( nflag == 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0x11 :                             /* ORA (zp),y */
                  areg  = areg | readindzpy();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x12 :                             /* ORA (zp) */
                  areg  = areg | readindzp();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x13 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x14 :                             /* TRB zp */
                  bytebuffer    = readzp();
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  writebus(address, (~areg) & bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x15 :                             /* ORA zp,x */
                  areg  = areg | readzpx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x16 :                             /* ASL zp,x */
                  bytebuffer    = readzpx();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = (bytebuffer << 1) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = bytebuffer << 1;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x17 :                             /* RMB1 zp */
                  bytebuffer    = readzp() & 0xfd;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x18 :                             /* CLC */
                  cflag = 0;
                  cycle = cycle + 2;
                  break;
                case 0x19 :                             /* ORA abs,y */
                  areg  = areg | readabsy();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x1a :                             /* INA */
                  areg  = (areg + 1) & 0xff;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0x1b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x1c :                             /* TRB abs */
                  bytebuffer    = readabs();
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  writebus(address, (~areg) & bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x1d :                             /* ORA abs,x */
                  areg  = areg | readabsx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x1e :                             /* ASL abs,x */
                  bytebuffer    = readabsx();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = (bytebuffer << 1) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = bytebuffer << 1;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 7;
                  break;
                case 0x1f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x02)) {           /* BBR1 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x20 :                             /* JSR abs */
                  address       = readbus(pc);
                  pc            = (pc + 1) & 0xffff;
                  pushstack(pc >> 8);
                  pushstack(pc & 0xff);
                  pc            = address + ( readbus(pc) << 8 );
                  cycle = cycle + 6;
                  break;
                case 0x21 :                             /* AND (zp,x) */
                  areg  = areg & readindzpx();
                  flagssetnz(areg);
                  cycle = cycle + 6;
                  break;
                case 0x22 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x23 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x24 :                             /* BIT zp */
                  bytebuffer    = readzp();
                  vflag = ((bytebuffer & 0x40) == 0) ? 0 : 1;
                  nflag = ((bytebuffer & 0x80) == 0) ? 0 : 1;
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  cycle = cycle + 3;
                  break;
                case 0x25 :                             /* AND zp */
                  areg  = areg & readzp();
                  flagssetnz(areg);
                  cycle = cycle + 3;
                  break;
                case 0x26 :                             /* ROL zp */
                  bytebuffer    = readzp();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = ((bytebuffer << 1) | cflag) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = (bytebuffer << 1) | cflag;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x27 :                             /* RMB2 zp */
                  bytebuffer    = readzp() & 0xfb;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x28 :                             /* PLP */
                  cpuflagsget( pullstack() );
                  cycle = cycle + 4;
                  break;
                case 0x29 :                             /* AND # */
                  areg  = areg & readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x2a :                             /* ROL */
                  if (areg > 0x7f) {
                    areg        = ((areg << 1) | cflag) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    areg        = (areg << 1) | cflag;
                    cflag       = 0;
                  }
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x2b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x2c :                             /* BIT abs */
                  bytebuffer    = readabs();
                  vflag = ((bytebuffer & 0x40) == 0) ? 0 : 1;
                  nflag = ((bytebuffer & 0x80) == 0) ? 0 : 1;
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  cycle = cycle + 4;
                  break;
                case 0x2d :                             /* AND abs */
                  areg  = areg & readabs();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x2e :                             /* ROL abs */
                  bytebuffer    = readabs();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = ((bytebuffer << 1) | cflag) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = (bytebuffer << 1) | cflag;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x2f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x04)) {           /* BBR2 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x30 :                             /* BMI */
                  if ( nflag != 0 ) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f ) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0x31 :                             /* AND (zp),y */
                  areg  = areg & readindzpy();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x32 :                             /* AND (zp) */
                  areg  = areg & readindzp();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x33 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x34 :                             /* BIT zp,x */
                  bytebuffer    = readzpx();
                  vflag = ((bytebuffer & 0x40) == 0) ? 0 : 1;
                  nflag = ((bytebuffer & 0x80) == 0) ? 0 : 1;
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  cycle = cycle + 4;
                  break;
                case 0x35 :                             /* AND zp,x */
                  areg  = areg & readzpx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x36 :                             /* ROL zp,x */
                  bytebuffer    = readzpx();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = ((bytebuffer << 1) | cflag) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = (bytebuffer << 1) | cflag;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x37 :                             /* RMB3 zp */
                  bytebuffer    = readzp() & 0xf7;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x38 :                             /* SEC */
                  cflag = 1;
                  cycle = cycle + 2;
                  break;
                case 0x39 :                             /* AND abs,y */
                  areg  = areg & readabsy();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x3a :                             /* DEA */
                  areg  = (areg - 1) & 0xff;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0x3b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x3c :                             /* BIT abs,x */
                  bytebuffer    = readabsx();
                  vflag = ((bytebuffer & 0x40) == 0) ? 0 : 1;
                  nflag = ((bytebuffer & 0x80) == 0) ? 0 : 1;
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  cycle = cycle + 4;
                  break;
                case 0x3d :                             /* AND abs,x */
                  areg  = areg & readabsx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x3e :                             /* ROL abs,x */
                  bytebuffer    = readabsx();
                  if ( bytebuffer > 0x7f ) {
                    bytebuffer  = ((bytebuffer << 1) | cflag) & 0xff;
                    cflag       = 1;
                  }
                  else {
                    bytebuffer  = (bytebuffer << 1) | cflag;
                    cflag       = 0;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x3f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x08)) {           /* BBR3 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x40 :                             /* RTI */
                  cpuflagsget( pullstack() );
                  pc    = pullstack() + (pullstack() << 8);
                  cycle = cycle + 6;
                  break;
                case 0x41 :                             /* EOR (zp,x) */
                  areg  = areg ^ readindzpx();
                  flagssetnz(areg);
                  cycle = cycle + 6;
                  break;
                case 0x42 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x43 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x44 :                             /* NOP3 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 3;
                  break;
                case 0x45 :                             /* EOR zp */
                  areg  = areg ^ readzp();
                  flagssetnz(areg);
                  cycle = cycle + 3;
                  break;
                case 0x46 :                             /* LSR zp */
                  bytebuffer    = readzp();
                  cflag         = ((bytebuffer & 1) == 0) ? 0 : 1;
                  bytebuffer    = bytebuffer >> 1;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 5;
                  break;
                case 0x47 :                             /* RMB4 zp */
                  bytebuffer    = readzp() & 0xef;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x48 :                             /* PHA */
                  pushstack(areg);
                  cycle = cycle + 3;
                  break;
                case 0x49 :                             /* EOR # */
                  areg  = areg ^ readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x4a :                             /* LSR */
                  cflag = ((areg & 1) == 0) ? 0 : 1;
                  areg  = areg >> 1;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x4b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x4c :                             /* JMP abs */
                  pc    = readbus(pc) + ( readbus( (pc + 1) & 0xffff) << 8 );
                  cycle = cycle + 3;
                  break;
                case 0x4d :                             /* EOR abs */
                  areg  = areg ^ readabs();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x4e :                             /* LSR abs */
                  bytebuffer    = readabs();
                  cflag         = ((bytebuffer & 1) == 0) ? 0 : 1;
                  bytebuffer    = bytebuffer >> 1;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0x4f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x10)) {           /* BBR4 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x50 :                             /* BVC */
                  if ( vflag == 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0x51 :                             /* EOR (zp),y */
                  areg  = areg ^ readindzpy();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x52 :                             /* EOR (zp) */
                  areg  = areg ^ readindzp();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0x53 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x54 :                             /* NOP4 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 4;
                  break;
                case 0x55 :                             /* EOR zp,x */
                  areg  = areg ^ readzpx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x56 :                             /* LSR zp,x */
                  bytebuffer    = readzpx();
                  cflag         = ((bytebuffer & 1) == 0) ? 0 : 1;
                  bytebuffer    = bytebuffer >> 1;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0x57 :                             /* RMB5 zp */
                  bytebuffer    = readzp() & 0xdf;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x58 :                             /* CLI */
                  iflag = 0;
                  cycle = cycle + 2;
                  break;
                case 0x59 :                             /* EOR abs,y */
                  areg  = areg ^ readabsy();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x5a :                             /* PHY */
                  pushstack(yreg);
                  cycle = cycle + 3;
                  break;
                case 0x5b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x5c :                             /* NOP8 65C02 */
                  pc    = (pc + 2) & 0xffff;
                  cycle = cycle + 8;
                  break;
                case 0x5d :                             /* EOR abs,x */
                  areg  = areg ^ readabsx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x5e :                             /* LSR abs,x */
                  bytebuffer    = readabsx();
                  cflag         = ((bytebuffer & 1) == 0) ? 0 : 1;
                  bytebuffer    = bytebuffer >> 1;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 7;
                  break;
                case 0x5f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x20)) {           /* BBR5 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x60 :                             /* RTS */
                  pc    = ((pullstack() + (pullstack() << 8)) + 1) & 0xffff;
                  cycle = cycle + 6;
                  break;
                case 0x61 :                             /* ADC (zp,x) */
                  adc(readindzpx());
                  cycle = cycle + 6;
                  break;
                case 0x62 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x63 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x64 :                             /* STZ zp */
                  writezp(0);
                  cycle = cycle + 3;
                  break;
                case 0x65 :                             /* ADC zp */
                  adc(readzp());
                  cycle = cycle + 3;
                  break;
                case 0x66 :                             /* ROR zp */
                  bytebuffer    = readzp();
                  if (cflag) {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = (bytebuffer >> 1) | 0x80;
                  }
                  else {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = bytebuffer >> 1;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x67 :                             /* RMB6 zp */
                  bytebuffer    = readzp() & 0xbf;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x68 :                             /* PLA */
                  areg  = pullstack();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0x69 :                             /* ADC # */
                  adc(readbus(pc));
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x6a :                             /* ROR */
                  if (cflag) {
                    cflag       = (areg & 1) ? 1 : 0;
                    areg        = (areg >> 1) | 0x80;
                  }
                  else {
                    cflag       = (areg & 1) ? 1 : 0;
                    areg        = areg >> 1;
                  }
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x6b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x6c :                             /* JMP (abs) */
                  address       = readbus(pc) + ( readbus((pc + 1) & 0xffff) << 8 );
                  pc            = readbus(address)
                                  + ( readbus((address + 1) & 0xffff) << 8 );
                  cycle = cycle + 5;
                  break;
                case 0x6d :                             /* ADC abs */
                  adc(readabs());
                  cycle = cycle + 4;
                  break;
                case 0x6e :                             /* ROR abs */
                  bytebuffer    = readabs();
                  if (cflag) {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = (bytebuffer >> 1) | 0x80;
                  }
                  else {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = bytebuffer >> 1;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x6f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x40)) {           /* BBR6 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x70 :                             /* BVS */
                  if ( vflag != 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0x71 :                             /* ADC (zp),y */
                  adc(readindzpy());
                  cycle = cycle + 5;
                  break;
                case 0x72 :                             /* ADC (zp) */
                  adc(readindzp());
                  cycle = cycle + 5;
                  break;
                case 0x73 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x74 :                             /* STZ zp,x */
                  writezpx(0);
                  cycle = cycle + 4;
                  break;
                case 0x75 :                             /* ADC zp,x */
                  adc(readzpx());
                  cycle = cycle + 4;
                  break;
                case 0x76 :                             /* ROR zp,x */
                  bytebuffer    = readzpx();
                  if (cflag) {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = (bytebuffer >> 1) | 0x80;
                  }
                  else {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = bytebuffer >> 1;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 6;
                  break;
                case 0x77 :                             /* RMB7 zp */
                  bytebuffer    = readzp() & 0x7f;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x78 :                             /* SEI */
                  iflag = 1;
                  cycle = cycle + 2;
                  break;
                case 0x79 :                             /* ADC abs,y */
                  adc(readabsy());
                  cycle = cycle + 4;
                  break;
                case 0x7a :                             /* PLY */
                  yreg  = pullstack();
                  flagssetnz(yreg);
                  cycle = cycle + 4;
                  break;
                case 0x7b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x7c :                             /* JMP (abs,x) */
                  address       = (readbus(pc) + ( readbus((pc + 1) & 0xffff) << 8 )
                                  + xreg) & 0xffff;
                  pc            = readbus(address)
                                  + ( readbus((address + 1) & 0xffff) << 8 );
                  cycle = cycle + 5;
                  break;
                case 0x7d :                             /* ADC abs,x */
                  adc(readabsx());
                  cycle = cycle + 4;
                  break;
                case 0x7e :                             /* ROR abs,x */
                  bytebuffer    = readabsx();
                  if (cflag) {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = (bytebuffer >> 1) | 0x80;
                  }
                  else {
                    cflag       = (bytebuffer & 1) ? 1 : 0;
                    bytebuffer  = bytebuffer >> 1;
                  }
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle = cycle + 7;
                  break;
                case 0x7f :
                  if (cputype == CPU65SC02) {
                    if (!(readzp() & 0x80)) {           /* BBR7 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x80 :                             /* BRA */
                  bytebuffer= readbus(pc);
                  if ( bytebuffer > 0x7f) {
                    pc  = (pc - 255 + bytebuffer) & 0xffff;
                  }
                  else {
                    pc  = (pc + 1 + bytebuffer) & 0xffff;
                  }
                  cycle = cycle + 3;
                  break;
                case 0x81 :                             /* STA (zp,x) */
                  writeindzpx(areg);
                  cycle = cycle + 6;
                  break;
                case 0x82 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0x83 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x84 :                             /* STY zp */
                  writezp(yreg);
                  cycle = cycle + 3;
                  break;
                case 0x85 :                             /* STA zp */
                  writezp(areg);
                  cycle = cycle + 3;
                  break;
                case 0x86 :                             /* STX zp */
                  writezp(xreg);
                  cycle = cycle + 3;
                  break;
                case 0x87 :                             /* SMB0 zp */
                  bytebuffer    = readzp() | 0x01;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x88 :                             /* DEY */
                  yreg  = (yreg - 1) & 0xff;
                  flagssetnz(yreg);
                  cycle = cycle + 2;
                  break;
                case 0x89 :                             /* BIT # */
                  bytebuffer    = readzp();
                  zflag = ((bytebuffer & areg) == 0) ? 1 : 0;
                  cycle = cycle + 2;
                  break;
                case 0x8a :                             /* TXA */
                  areg  = xreg;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x8b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x8c :                             /* STY abs */
                  writeabs(yreg);
                  cycle = cycle + 4;
                  break;
                case 0x8d :                             /* STA abs */
                  writeabs(areg);
                  cycle = cycle + 4;
                  break;
                case 0x8e :                             /* STX abs */
                  writeabs(xreg);
                  cycle = cycle + 4;
                  break;
                case 0x8f :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x01) {              /* BBS0 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0x90 :                             /* BCC */
                  if ( cflag == 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                   pc           = (pc + 1) & 0xffff;
                   cycle        = cycle + 2;
                  }
                  break;
                case 0x91 :                             /* STA (zp),y */
                  writeindzpy(areg);
                  cycle = cycle + 6;
                  break;
                case 0x92 :                             /* STA (zp) */
                  writeindzp(areg);
                  cycle = cycle + 6;
                  break;
                case 0x93 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x94 :                             /* STY zp,x */
                  writezpx(yreg);
                  cycle = cycle + 4;
                  break;
                case 0x95 :                             /* STA zp,x */
                  writezpx(areg);
                  cycle = cycle + 4;
                  break;
                case 0x96 :                             /* STX zp,y */
                  writezpy(xreg);
                  cycle = cycle + 4;
                  break;
                case 0x97 :                             /* SMB1 zp */
                  bytebuffer    = readzp() | 0x02;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0x98 :                             /* TYA */
                  areg  = yreg;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0x99 :                             /* STA abs,y */
                  writeabsy(areg);
                  cycle = cycle + 5;
                  break;
                case 0x9a :                             /* TXS */
                  stack = xreg;
                  cycle = cycle + 2;
                  break;
                case 0x9b :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0x9c :                             /* STZ abs */
                  writeabs(0);
                  cycle = cycle + 4;
                  break;
                case 0x9d :                             /* STA abs,x */
                  writeabsx(areg);
                  cycle = cycle + 5;
                  break;
                case 0x9e :                             /* STZ abs,x */
                  writeabsx(0);
                  cycle = cycle + 5;
                  break;
                case 0x9f :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x02) {              /* BBS1 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xa0 :                             /* LDY # */
                  yreg  = readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(yreg);
                  cycle = cycle + 2;
                  break;
                case 0xa1 :                             /* LDA (zp,x) */
                  areg  = readindzpx();
                  flagssetnz(areg);
                  cycle = cycle + 6;
                  break;
                case 0xa2 :                             /* LDX # */
                  xreg  = readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0xa3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xa4 :                             /* LDY zp */
                  yreg  = readzp();
                  flagssetnz(yreg);
                  cycle = cycle + 3;
                  break;
                case 0xa5 :                             /* LDA zp */
                  areg  = readzp();
                  flagssetnz(areg);
                  cycle = cycle + 3;
                  break;
                case 0xa6 :                             /* LDX zp */
                  xreg  = readzp();
                  flagssetnz(xreg);
                  cycle = cycle + 3;
                  break;
                case 0xa7 :                             /* SMB2 zp */
                  bytebuffer    = readzp() | 0x04;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xa8 :                             /* TAY */
                  yreg  = areg;
                  flagssetnz(yreg);
                  cycle = cycle + 2;
                  break;
                case 0xa9 :                             /* LDA # */
                  areg  = readbus(pc);
                  pc    = (pc + 1) & 0xffff;
                  flagssetnz(areg);
                  cycle = cycle + 2;
                  break;
                case 0xaa :                             /* TAX */
                  xreg  = areg;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0xab :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xac :                             /* LDY abs */
                  yreg  = readabs();
                  flagssetnz(yreg);
                  cycle = cycle + 4;
                  break;
                case 0xad :                             /* LDA abs */
                  areg  = readabs();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0xae :                             /* LDX abs */
                  xreg  = readabs();
                  flagssetnz(xreg);
                  cycle = cycle + 4;
                  break;
                case 0xaf :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x04) {              /* BBS2 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xb0 :                             /* BCS */
                  if ( cflag != 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0xb1 :                             /* LDA (zp),y */
                  areg  = readindzpy();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0xb2 :                             /* LDA (zp) */
                  areg  = readindzp();
                  flagssetnz(areg);
                  cycle = cycle + 5;
                  break;
                case 0xb3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xb4 :                             /* LDY zp,x */
                  yreg  = readzpx();
                  flagssetnz(yreg);
                  cycle = cycle + 4;
                  break;
                case 0xb5 :                             /* LDA zp,x */
                  areg  = readzpx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0xb6 :                             /* LDX zp,y */
                  xreg  = readzpy();
                  flagssetnz(xreg);
                  cycle = cycle + 4;
                  break;
                case 0xb7 :                             /* SMB3 zp */
                  bytebuffer    = readzp() | 0x08;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xb8 :                             /* CLV */
                  vflag = 0;
                  cycle = cycle + 2;
                  break;
                case 0xb9 :                             /* LDA abs,y */
                  areg  = readabsy();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0xba :                             /* TSX */
                  xreg  = stack;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0xbb :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xbc :                             /* LDY abs,x */
                  yreg  = readabsx();
                  flagssetnz(yreg);
                  cycle = cycle + 4;
                  break;
                case 0xbd :                             /* LDA abs,x */
                  areg  = readabsx();
                  flagssetnz(areg);
                  cycle = cycle + 4;
                  break;
                case 0xbe :                             /* LDX abs,y */
                  xreg  = readabsy();
                  flagssetnz(xreg);
                  cycle = cycle + 4;
                  break;
                case 0xbf :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x08) {              /* BBS3 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xc0 :                             /* CPY # */
                  cpy(readbus(pc));
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xc1 :                             /* CMP (zp,x) */
                  cmp(readindzpx());
                  cycle = cycle + 6;
                  break;
                case 0xc2 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xc3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xc4 :                             /* CPY zp */
                  cpy(readzp());
                  cycle = cycle + 3;
                  break;
                case 0xc5 :                             /* CMP zp */
                  cmp(readzp());
                  cycle = cycle + 3;
                  break;
                case 0xc6 :                             /* DEC zp */
                  bytebuffer    = (readzp() - 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 5;
                  break;
                case 0xc7 :                             /* SMB4 zp */
                  bytebuffer    = readzp() | 0x10;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xc8 :                             /* INY */
                  yreg  = (yreg + 1) & 0xff;
                  flagssetnz(yreg);
                  cycle = cycle + 2;
                  break;
                case 0xc9 :                             /* CMP # */
                  cmp(readbus(pc));
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xca :                             /* DEX */
                  xreg  = (xreg - 1) & 0xff;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0xcb :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xcc :                             /* CPY abs */
                  cpy(readabs());
                  cycle = cycle + 4;
                  break;
                case 0xcd :                             /* CMP abs */
                  cmp(readabs());
                  cycle = cycle + 4;
                  break;
                case 0xce :                             /* DEC abs */
                  bytebuffer    = (readabs() - 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0xcf :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x10) {              /* BBS4 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xd0 :                             /* BNE */
                  if ( zflag == 0) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0xd1 :                             /* CMP (zp),y */
                  cmp(readindzpy());
                  cycle = cycle + 5;
                  break;
                case 0xd2 :                             /* CMP (zp) */
                  cmp(readindzp());
                  cycle = cycle + 5;
                  break;
                case 0xd3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xd4 :                             /* NOP4 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 4;
                  break;
                case 0xd5 :                             /* CMP zp,x */
                  cmp(readzpx());
                  cycle = cycle + 4;
                  break;
                case 0xd6 :                             /* DEC zp,x */
                  bytebuffer    = (readzpx() - 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0xd7 :                             /* SMB5 zp */
                  bytebuffer    = readzp() | 0x20;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xd8 :                             /* CLD */
                  dflag = 0;
                  cycle = cycle + 2;
                  break;
                case 0xd9 :                             /* CMP abs,y */
                  cmp(readabsy());
                  cycle = cycle + 4;
                  break;
                case 0xda :                             /* PHX */
                  pushstack(xreg);
                  cycle = cycle + 3;
                  break;
                case 0xdb :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xdc :                             /* NOP4 65C02 */
                  pc    = (pc + 2) & 0xffff;
                  cycle = cycle + 4;
                  break;
                case 0xdd :                             /* CMP abs,x */
                  cmp(readabsx());
                  cycle = cycle + 4;
                  break;
                case 0xde :                             /* DEC abs,x */
                  bytebuffer    = (readabsx() - 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 7;
                  break;
                case 0xdf :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x20) {              /* BBS5 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xe0 :                             /* CPX # */
                  cpx(readbus(pc));
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xe1 :                             /* SBC (zp,x) */
                  sbc(readindzpx());
                  cycle = cycle + 5;
                  break;
                case 0xe2 :                             /* NOP2 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xe3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xe4 :                             /* CPX zp */
                  cpx(readzp());
                  cycle = cycle + 3;
                  break;
                case 0xe5 :                             /* SBC zp */
                  sbc(readzp());
                  cycle = cycle + 3;
                  break;
                case 0xe6 :                             /* INC zp */
                  bytebuffer    = (readzp() + 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 5;
                  break;
                case 0xe7 :                             /* SMB6 zp */
                  bytebuffer    = readzp() | 0x40;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xe8 :                             /* INX */
                  xreg  = (xreg + 1) & 0xff;
                  flagssetnz(xreg);
                  cycle = cycle + 2;
                  break;
                case 0xe9 :                             /* SBC # */
                  sbc(readbus(pc));
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 2;
                  break;
                case 0xea :                             /* NOP */
                  cycle = cycle + 2;
                  break;
                case 0xeb :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xec :                             /* CPX abs */
                  cpx(readabs());
                  cycle = cycle + 4;
                  break;
                case 0xed :                             /* SBC abs */
                  sbc(readabs());
                  cycle = cycle + 4;
                  break;
                case 0xee :                             /* INC abs */
                  bytebuffer    = (readabs() + 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0xef :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x40) {              /* BBS6 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                case 0xf0 :                             /* BEQ */
                  if ( zflag != 0 ) {
                    bytebuffer= readbus(pc);
                    if ( bytebuffer > 0x7f ) {
                      pc        = (pc - 255 + bytebuffer) & 0xffff;
                    }
                    else {
                      pc        = (pc + 1 + bytebuffer) & 0xffff;
                    }
                    cycle       = cycle + 3;
                  }
                  else {
                    pc          = (pc + 1) & 0xffff;
                    cycle       = cycle + 2;
                  }
                  break;
                case 0xf1 :                             /* SBC (zp),y */
                  sbc(readindzpy());
                  cycle = cycle + 5;
                  break;
                case 0xf2 :                             /* SBC (zp) */
                  sbc(readindzp());
                  cycle = cycle + 5;
                  break;
                case 0xf3 :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xf4 :                             /* NOP4 65C02 */
                  pc    = (pc + 1) & 0xffff;
                  cycle = cycle + 4;
                  break;
                case 0xf5 :                             /* SBC zp,x */
                  sbc(readzpx());
                  cycle = cycle + 4;
                  break;
                case 0xf6 :                             /* INC zp,x */
                  bytebuffer    = (readzpx() + 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 6;
                  break;
                case 0xf7 :                             /* SMB7 zp */
                  bytebuffer    = readzp() | 0x80;
                  writebus(address, bytebuffer);
                  cycle = cycle + 5;
                  break;
                case 0xf8 :                             /* SED */
                  dflag = 1;
                  cycle = cycle + 2;
                  break;
                case 0xf9 :                             /* SBC abs,y */
                  sbc(readabsy());
                  cycle = cycle + 4;
                  break;
                case 0xfa :                             /* PLX */
                  xreg  = pullstack();
                  flagssetnz(xreg);
                  cycle = cycle + 4;
                  break;
                case 0xfb :                             /* NOP 65C02 */
                  cycle = cycle + 2;
                  break;
                case 0xfc :                             /* NOP4 65C02 */
                  pc    = (pc + 2) & 0xffff;
                  cycle = cycle + 4;
                  break;
                case 0xfd :                             /* SBC abs,x */
                  sbc(readabsx());
                  cycle = cycle + 4;
                  break;
                case 0xfe :                             /* INC abs,x */
                  bytebuffer    = (readabsx() + 1) & 0xff;
                  flagssetnz(bytebuffer);
                  writebus(address, bytebuffer);
                  cycle         = cycle + 7;
                  break;
                case 0xff :
                  if (cputype == CPU65SC02) {
                    if (readzp() & 0x80) {              /* BBS7 65SC02 */
                      bytebuffer= readbus(pc);
                      if ( bytebuffer > 0x7f) {
                        pc      = (pc - 255 + bytebuffer) & 0xffff;
                      }
                      else {
                        pc      = (pc + 1 + bytebuffer) & 0xffff;
                      }
                      cycle     = cycle + 5;
                    }
                    else {
                      pc        = (pc + 1) & 0xffff;
                      cycle     = cycle + 3;
                    }
                    break;
                  }
                  else {
                    pc  = (pc + 2) & 0xffff;            /* NOP3 65C02 */
                    break;
                  }
                default :
                  stateflags = stateflags | STATEHALT | STATEGURU;
                  pc    = (pc - 1) % 0xffff;            /* go back to last instruction */
                  virtscreencopy();
                  return;
              } /* switch */
              {
                register unsigned int delay;
                for (delay=hold; delay != 0; delay--);
              }
            } /* do */
            while (!traceflag);
            stateflags = stateflags | STATETRACE;

      } /* cpuline */


/*-------------------------------------*/


      void cpurun() {

        cputype = CPU65C02;

        cpuinit();
        do {
          cpuline();
          if ((stateflags & STATETRACE) | (stateflags & STATEGURU) | (stateflags & STATEBPT)
              /*| (stateflags & STATEBRK) */ ) {
            Debug6502();
          }
          PC2APL();
        }
        while (!exitprogram);
      } /* cpurun */


/***************************************************************************/

CPUTYPE getcputype (void)
{
 return cputype;
}
