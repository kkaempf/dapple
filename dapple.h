/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  Header file for Dapple shared components
 * Revision:   (1.25) 2003.0111
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

#define DappleVersion "1.42 (1.5 Alpha 3)"
#include <allegro.h>
extern unsigned char *RAM;          /* 48K RAM bank */
extern unsigned char RAMEXT[16384]; /* 16K expanded RAM */
extern unsigned char bankram1[49152];
extern unsigned char bankram2[16384];
extern unsigned char ROM[16384];    /* 12K ROM bank, 4K expansion for //e */

extern char rompath[128];
extern int dverbose;

typedef enum {USA, France, Germany, UK, Denmark1, Sweden, Italy, Spain,
              Japan, Norway, Denmark2} Charset;

extern Charset charmode;

/* BINMANIP */
int bsave(char *filename);
int bload(char *filename);

/* Graphics flags */
#define HRG 1 /* 0001 */
#define PG2 2 /* 0010 */
#define GRX 4 /* 0100 */
#define SPL 8 /* 1000 */

#define T40 1
#define T80 3
#define G1 0x4
#define G2 0x6
#define G7 0xD
#define G8 0xE

/* VIDEO */
void opengraph(void);

#ifdef SAFE
#define GetAttrib(x) 0
#endif

/* Color definitions */

/* Lo-res graphics */
#define COL_LGR0 0x0
#define COL_LGR1 0x1
#define COL_LGR2 0x2
#define COL_LGR3 0x3
#define COL_LGR4 0x4
#define COL_LGR5 0x5
#define COL_LGR6 0x6
#define COL_LGR7 0x7
#define COL_LGR8 0x8
#define COL_LGR9 0x9
#define COL_LGRA 0xA
#define COL_LGRB 0xB
#define COL_LGRC 0xC
#define COL_LGRD 0xD
#define COL_LGRE 0xE
#define COL_LGRF 0xF

#define COL_HGR0 0x10
#define COL_HGR1 0x11
#define COL_HGR2 0x12
#define COL_HGR3 0x13
#define COL_HGR4 0x14
#define COL_HGR5 0x15
#define COL_HGR6 0x16
#define COL_HGR7 0x17

#define COL_TXT_WHT0 0x18
#define COL_TXT_WHT1 0x19
#define COL_TXT_WHT2 0x1A
#define COL_TXT_WHT3 0x1B

#define COL_TXT_GRN0 0x1C
#define COL_TXT_GRN1 0x1D
#define COL_TXT_GRN2 0x1E
#define COL_TXT_GRN3 0x1F

#define COL_TXT_AMB0 0x20
#define COL_TXT_AMB1 0x21
#define COL_TXT_AMB2 0x22
#define COL_TXT_AMB3 0x23

#define COL_DRV_OFF  0x24
#define COL_DRV_ON   0x25

#define COL_LPT_OFF  0x26
#define COL_LPT_ON   0x27

#define COL_DASM     0x28
#define COL_FLAG     0x29
#define COL_DASM80   0x2A
#define COL_FLAG80   0x2B

#define COL_HDV_OFF  0x2C
#define COL_HDV_ON   0x2D

#define COL_DISKFLAG 0x2E
#define COL_LPTERR   0x2F

struct rgb
{
 unsigned char red;
 unsigned char green;
 unsigned char blue;
};

typedef unsigned char byte;
typedef unsigned short int word;
//zD
BITMAP *bufferzor;
