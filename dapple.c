/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  DAPPLE:  main functions
 * Revision:   (1.25) 2003.0111
 *
 * Graphics chroma data (0x00-0x17) provided to Usenet by Robert Munafo
 * and given to the author by Holger Picker.  See dapple.h for alternate
 * names for the colors.
 *
 * Foreign character translation derived from experimentation and Epson
 * RX-80 documentation.
 *
 * Some information was provided to Usenet by Jon Bettencourt and given to
 * the author by Holger Picker.
 *
 * Some information regarding 128K memory banking was provided by David
 * Empson and Holger Picker.  The author greatly appreciates your help.
 * Thanks a million!
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

//#include "m6502.h"
//#include <conio.h>
//#include <dos.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#include "dapple.h"
#include "binmanip.h"
#include "cpu65c02.h"
#include "debug.h"
#include "disk.h"
#include "gui.h"
#include "joystick.h"
#include "lldisk.h"
#include "massstor.h"
#include "video.h"
#include <allegro.h>

#ifdef EMUZ80
int Z80_Execute (void);
#endif

BITMAP *bufferzor;

unsigned char keycapslock = 1;
unsigned char keyswitchyz = 0;
unsigned char joybutton0;
unsigned char joybutton1;

int sounddelay=2;
int dqhdv=1;

void uninst15 (void);
void uninst1B (void);

/* Memory Handling */
static unsigned char dappleram[49152];
       unsigned char *RAM=dappleram;

unsigned char DISKROM[256], PICROM[256];
extern unsigned char HDDROM[256]; /* no room here! */

unsigned char debugger;
unsigned char memauxread;               /* read motherboard ram or aux ram                      */
unsigned char memauxwrite;              /* write motherboard ram or aux ram                     */
unsigned char memstore80;
unsigned char memintcxrom;              /* read from slot rom $c100-$cfff or internal rom       */
unsigned char memslotc3;                /* read from slot rom $c300-$c3ff or internal rom       */
unsigned char memlcaux;                 /* read $0-$1ff + $d000-$ffff from main or aux memory   */
unsigned char memlcramr;                /* read LC RAM or ROM?                                  */
unsigned char memlcramw;                /* write LC RAM or ROM?                                 */
unsigned char memlcbank2;               /* read from LC bank 1 or LC bank 2                     */
unsigned char memann3;                  /* annunciator #3                                       */
unsigned char memnolc = 0;              /* Is there a LC card installed?                        */
extern unsigned char reload (char *bios);

Charset charmode=USA;

int ROM16K=0;

static int keepshift=0;
int shiftshutoff=0;

/* disk.c */
void InitDisk( int slot );
void ShutdownDisk( void );
void DiskReset( void );
unsigned char ReadDiskIO( unsigned short Address );
void WriteDiskIO( unsigned short Address, unsigned char Data );

void WriteParallel( unsigned short Address, unsigned char Data );

/* video.c: graphic display */
extern unsigned int virtcachemode;
extern unsigned char virt80col;
extern unsigned char virtaltchar;
extern unsigned char virtiou;
extern unsigned char virtdhres;
extern unsigned char virtvideobyte;
extern unsigned char virtscreen[128000];
//#define virtplot(x,y,c) virtscreen[(y*320)+x]=c
//#define virtplot(x,y,c) putpixel(screen,x,y,c)
extern void virtreset();
extern void virtsetmode();
extern void virtsethresmode(unsigned char mode);        /* 0 = standard, 1 = RGB */
extern unsigned char virtgethresmode();
extern void virtsetmonochrome(unsigned char mode);      /* 0 = colour, 1 = white, 2 = green, 3 = amber */
extern unsigned char virtgetmonochrome();
extern void virtpalettesetd(register unsigned char index, register unsigned char r,
                            register unsigned char b,     register unsigned char g);
extern void virtputpalette();
extern void virtline(unsigned int rastline);
extern void virtscreencopy();
extern void virtwrite0400(register unsigned int addr);
/* changed_this */
extern void virtwrite0400aux(register unsigned int addr);
extern void virtwrite0800(register unsigned int addr);
/* changed_this */
extern void virtwrite0800aux(register unsigned int addr);
extern void virtwrite2000(register unsigned int addr);
extern void virtwrite2000aux(register unsigned int addr);
extern void virtwrite4000(register unsigned int addr);
extern void virtwrite4000aux(register unsigned int addr);
// unsigned int rasterline = 0;

/* debug.c */
extern void Debug6502();

/* ini.c */
extern void wrini();
extern void rdini();

/* other variables */
unsigned char argquickstart = 0;
unsigned char argini = 0;

unsigned char exitprogram = 0;

unsigned int dvalue1 = 0;
unsigned int dvalue2 = 0;
unsigned int dvalue3 = 0;
unsigned int dvalue4 = 0;

// word XRun6502(M6502 *R);

#define _NOCURSOR      0
#define _SOLIDCURSOR   1
#define _NORMALCURSOR  2

int joyenabled=1;
// int hold=5;

void virtplot(int x, int y, unsigned char c){
virtscreen[(640*y)+x]=c;
putpixel(bufferzor,x,y*2,vga2alleg(c));
putpixel(bufferzor,x,1+(y*2),vga2alleg(c));
//printf("x:%d y:%d c:%d\n",x,y,c);
}

extern int hdd;
#define ROMDIR "/usr/share/dapple/"
char rompath[128]={ROMDIR "apple.rom"};
//static char cmap[16]={0,4,1,5,3,7,2,9,6,13,8,12,10,14,11,15};
//zD was here too ]xD
//#ifdef SAFE
//char *shiftstate=1047;
//#else
//char *shiftstate=MK_FP(0,1047);
//#endif
char oldshift;
int dqshift=1;

static int d=0,hd=0,osp;
int k=0; // used in altkey.c
int gm, smode=1;

/* changed_this */
/*int fontloaded=0;*/
/*int drawtext=1;*/

// int debugstep=0, debugtrace=0;
int dlight1=0, dlight2=0;
enum {emu6502, emuZ80} cpuinuse=emu6502;

/* changed_this */
/*#include "font.h"*/

enum DiskTypes
{
    UnknownType = 0,
    RawType     = 1,
    DOSType     = 2
};

extern struct DriveState
{
    /* variables for each disk */
    char DiskFN[ 80 ]; /* MS-DOS filename of disk image */
    int DiskFH;        /* MS-DOS file handle of disk image */
    long DiskSize;     /* length of disk image file in bytes */
    enum DiskTypes DiskType; /* Type of disk image */
    int WritePro;      /* 0:write-enabled, !0:write-protected */
    int TrkBufChanged; /* Track buffer has changed */
    int TrkBufOld;     /* Data in track buffer needs updating before I/O */
    int ShouldRecal;
    /* variables used during emulation */
    int Track;    /* 0-70 */
    int Phase;    /* 0- 3 */
    int ReadWP;   /* 0/1  */
    int Active;   /* 0/1  */
    int Writing;  /* 0/1  */
} DrvSt[ 2 ];


/*int critical_error_handler (int errval, int ax, int bp, int si)
{
 char far *deviceDriver;
 char AllowedActions;
 int CanIgnore=0, CanFail=0;
 char ah, al;
 deviceDriver=MK_FP(bp, si+10);
 ah=ax/256;
/* al=ax%256; */
 /* Generate ARIF message from AH flags */
 /*
 AllowedActions=ah;
 if ((ah&0x80)&&(((struct devhdr far *)(deviceDriver-10))->dh_attr&0x8000))
 {
  if (deviceDriver[0]=='P'&&
      deviceDriver[1]=='R'&&
      deviceDriver[2]=='N')
  {
   static char errnam[3];
   int travel;

   switch (errval)
   {
    case 1:  strcpy(errnam,"UN"); break;
    case 3:  strcpy(errnam,"RQ"); break;
    case 5:  strcpy(errnam,"PR"); break;
    case 9:  strcpy(errnam,"OP"); break;
    case 10: strcpy(errnam,"WF"); break;
    default: strcpy(errnam,"??"); break;
   }
   for (travel=0;travel<3;travel++) xputcs(32,travel,COL_LPTERR%256);
   for (travel=0;errnam[travel];travel++) xputcs(errnam[travel],travel,COL_LPTERR%256);
   if (kbhit())
   {
    for (travel=0;travel<3;travel++) xputcs(32,travel,COL_LPTERR%256);
    if (!getch()) getch();
    hardresume(3);
   }
   hardresume(1);
  }
 }
 if (AllowedActions & 32)  CanIgnore=-1;
 if (AllowedActions & 8)   CanFail=-1;
 if (CanIgnore) hardresume (0); /* Ignore it if you can, otherwise fail */
/* if (CanFail)   hardresume (3);
 gmode(3); /* If you can't ignore or fail, abort */
/* cprintf ("FATAL I/O ERROR, CANNOT RECOVER  #%04X  DEVICE @%04X:%04X",errval,si,bp);

#ifdef BOTH_ALT
 uninst15();
#endif
 uninst1B();

 hardresume(2); /* ABORT */
/* return 2;
}
*/

void setshiftmanip (int x)
{
 keepshift=shiftshutoff=x;
}
unsigned char reload (char *bios)
{
 FILE *file;
 extern int ROM16K;

 if (!*bios) return 0;
 printf ("Initializing Apple...");
 file=fopen(bios,"rb");
 if (!file)
 {
  return 0;
 }
 strcpy(rompath,bios);
 {
  unsigned int l;
  ROM16K=0;
  fseek(file,0,SEEK_END);
  l=ftell(file);
  fseek(file,0,SEEK_SET);
  ROM16K=(l==16128);
  if (l==20480) /* ApplePC/AppleWin 12K */
  {
   fseek(file,8192,SEEK_SET);
   ROM16K=0;
  }
  else if (l==32768) /* this gets false positive on //c BIOS */
  {
   fseek(file,16640,SEEK_SET);
   ROM16K=1;
  }
 }
 if (ROM16K) fread(&(ROM[12288]),3840,1,file);
 fread(ROM,12288,1,file);
 fclose(file);

/* init memory */
 memoryclear();
 memoryreset();

/* init display */
 virtinit();
 virtplot(314,2,COL_DRV_OFF);   /* slot #5 drive 1 off at the beginning */
 virtplot(314,4,COL_DRV_OFF);   /* slot #6 drive 1 off at the beginning */
 virtplot(316,4,COL_DRV_OFF);   /* slot #6 drive 2 off at the beginning */
 virtplot(314,6,COL_DRV_OFF);   /* slot #7 drive 1 off at the beginning */
 virtplot(316,6,COL_DRV_OFF);   /* slot #7 drive 2 off at the beginning */
 virtplot(314,199,cpuinuse?COL_DRV_ON:0);
 virtplot(316,199,0);

/* init processor */
 cpureset();
#ifdef EMUZ80
 Z80_Reset();
#endif
 cpuinuse=emu6502;
 return 1;
}
void click(void)
{
/* if (!smode) return;
 if (sounddelay!=2)
 {
  sound(40);
  delay(sounddelay);
  nosound();
 }
 else
 {
  int al;
  // Toggle the speaker
  al=inportb(0x61);
  al ^= 0x02;
  al &= 0xfe;
  outportb(0x61,al);
 }
 */
}

/*-------------------------------------*/


      void memoryreset() {
	memauxread    = 0;
	memauxwrite   = 0;
	memintcxrom   = 0;            /* read from slot rom $c100-$cfff               */
	memslotc3     = 0x80;         /* read slot rom                                */
	memlcaux      = 0;            /* read $0-$1ff + $d000-$ffff from main memory  */
	memlcramr     = 0;            /* read ROM                                     */
	memlcramw     = 0;            /* write ROM?                                   */
	memlcbank2    = 0;            /* read from LC bank 1                          */
	memstore80    = 0;            /* 80 store off                                 */
	memann3       = 0x80;         /* annunciator #3 on                            */
      } /* MemoryReset */


/*-------------------------------------*/


      void memoryclear() {
	memset(RAM,0,49152);
	memset(RAMEXT,0,16384);
	memset(bankram1,0,49152);
	memset(bankram2,0,16384);
      } /* MemoryClear */

/*-------------------------------------*/


      void memorystore(FILE *file) {

        fwrite(&ROM16K,         sizeof(ROM16K),         1,file);
        fwrite(&rompath,        sizeof(rompath),        1,file);
        fwrite(&ROM,            sizeof(ROM),            1,file);
        fwrite(&dappleram,      sizeof(dappleram),      1,file);
        if (!memnolc) {
          fwrite(&RAMEXT,       sizeof(RAMEXT),         1,file);
        }
        if (ROM16K) {
          fwrite(&bankram1,     sizeof(bankram1),       1,file);
          if (!memnolc) {
            fwrite(&bankram2,   sizeof(bankram2),       1,file);
          }
        }
        fwrite(&memauxread,     sizeof(memauxread),     1,file);
        fwrite(&memauxwrite,    sizeof(memauxwrite),    1,file);
        fwrite(&memintcxrom,    sizeof(memintcxrom),    1,file);
        fwrite(&memslotc3,      sizeof(memslotc3),      1,file);
        fwrite(&memlcaux,       sizeof(memlcaux),       1,file);
        fwrite(&memlcramr,      sizeof(memlcramr),      1,file);
        fwrite(&memlcramw,      sizeof(memlcramw),      1,file);
        fwrite(&memlcbank2,     sizeof(memlcbank2),     1,file);
        fwrite(&memstore80,     sizeof(memstore80),     1,file);
        fwrite(&memann3,        sizeof(memann3),        1,file);

      } /* memorystore */


/*-------------------------------------*/


      void memoryrestore(FILE *file) {

        fread(&ROM16K,          sizeof(ROM16K),         1,file);
        fread(&rompath,         sizeof(rompath),        1,file);
        fread(&ROM,             sizeof(ROM),            1,file);
        fread(&dappleram,       sizeof(dappleram),      1,file);
        //fread(&RAMEXT,          sizeof(RAMEXT),         1,file);
        if (!memnolc) {
          fread(&RAMEXT,        sizeof(RAMEXT),         1,file);
        }
        if (ROM16K) {
          fread(&bankram1,      sizeof(bankram1),       1,file);
          if (!memnolc) {
            fread(&bankram2,    sizeof(bankram2),       1,file);
          }
        }
        fread(&memauxread,      sizeof(memauxread),     1,file);
        fread(&memauxwrite,     sizeof(memauxwrite),    1,file);
        fread(&memintcxrom,     sizeof(memintcxrom),    1,file);
        fread(&memslotc3,       sizeof(memslotc3),      1,file);
        fread(&memlcaux,        sizeof(memlcaux),       1,file);
        fread(&memlcramr,       sizeof(memlcramr),      1,file);
        fread(&memlcramw,       sizeof(memlcramw),      1,file);
        fread(&memlcbank2,      sizeof(memlcbank2),     1,file);
        fread(&memstore80,      sizeof(memstore80),     1,file);
        fread(&memann3,         sizeof(memann3),        1,file);

      } /* memoryrestore */


/*-------------------------------------*/


void Wr6502(register unsigned short addr, register unsigned char val)
{

  if (addr < 0xc000) {
    if (addr < 0x2000) {
      if (addr < 0x400) {
        if (addr < 0x200) {
/* $0000 - $01ff */
          if (memlcaux)         { bankram1[addr] = val; return; }                       /* aux 64 */
          else                  { RAM[addr]      = val; return; }                       /* main 64 */
        }
        else {
/* $0200 - $03ff */
          if (memauxwrite)      { bankram1[addr] = val; return; }                       /* aux 64 */
          else                  { RAM[addr]      = val; return; }                       /* main 64 */
        }
      }
      else {
        if (addr < 0x800) {
/* $0400 - $07ff */
          if (memstore80) {
            if (gm&PG2) {
              if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
/* changed_this */
                                virtwrite0400aux(addr);
              }
              return;
            }
            else {
              if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite0400(addr);
              }
              return;
            }
          } /* if (memstore80) */
          else {
            if (memauxwrite) {
              if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
/* changed_this */
                                virtwrite0400aux(addr);
              }
              return;
            }
            else {
              if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite0400(addr);
              }
              return;
            }
          } /* else if (memstore80) */
        } /* if (addr < 0x800 */
        else {
          if (addr < 0xc00) {
/* $0800 - $0bff */
            if (memauxwrite) {
              if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
/* changed_this */
                                virtwrite0800aux(addr);
              }
              return;
            }
            else {
              if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite0800(addr);
              }
              return;
            }
          }
          else {
/* $0c00 - $1fff */
            if (memauxwrite)    { bankram1[addr] = val; return; }                       /* aux 64 */
            else                { RAM[addr]      = val; return; }                       /* main 64 */
          }
        }
      }
    } /* if (addr < 0x2000) */
    else {
      if (addr < 0x4000) {
/* $2000 - $3fff */
        if ((memstore80) && (gm&HRG)) {
          if (gm&PG2) {
            if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
                                virtwrite2000aux(addr);
            }
            return;
          }
          else {
            if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite2000(addr);
            }
            return;
          }
        }
        else {
          if (memauxwrite) {
            if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
                                virtwrite2000aux(addr);
            }
            return;
          }
          else {
            if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite2000(addr);
            }
            return;
          }
        }
      }
      else {
        if (addr < 0x6000) {
/* $4000 - $5fff */
          if (memauxwrite) {
            if (bankram1[addr] != val) {
                                bankram1[addr] = val;                                   /* aux 64 */
                                virtwrite4000aux(addr);
            }
            return;
          }
          else {
            if (RAM[addr] != val) {
                                RAM[addr] = val;                                        /* main 64 */
                                virtwrite4000(addr);
            }
            return;
          }
        }
        else {
/* $6000 - $bfff */
          if (memauxwrite)      { bankram1[addr] = val; return; }                       /* aux 64 */
          else                  { RAM[addr]      = val; return; }                       /* main 64 */
        }
      }
    }
  } /* if (addr < 0xc000) */
  else {
    if (addr >= 0xd000) {
      if ((!memlcramw) || (memnolc && !(ROM16K)) ) {
        return;         /* ROM is the same for whatever memlcaux is set to */
      }
      else {
        if (memlcaux) {
          if (addr < 0xe000) {
            if (memlcbank2) {
              bankram2[(addr-0xd000)+0x3000] = val;
              return;
            }
          }
          bankram2[addr-0xd000] = val;
          return;
        }
        else {
          if (addr < 0xe000) {
            if (memlcbank2) {
              RAMEXT[(addr-0xd000)+0x3000] = val;
              return;
            }
          }
          RAMEXT[addr-0xd000] = val;
          return;
        }
      } /* else if (memlcramw) */
    } /* if (addr => 0xd000 */
    else {

 /* test for softswitch area */
      if (addr <= 0xc0ff) {

      if (debugger) return;

        if (addr <= 0xc00f) {
          if (ROM16K) { /* only on Apple//e */
            switch (addr) {
              case 0xc000 : { if (memstore80) {
                                memstore80 = 0;
                                virtsetmode();
                              }
                              return;
                            }
              case 0xc001 : { if (!memstore80) {
                                memstore80 = 0x80;
                                virtsetmode();
                              }
                              return;
                            }
              case 0xc002 : { memauxread        = 0;    return; }
              case 0xc003 : { memauxread        = 0x80; return; }
              case 0xc004 : { memauxwrite       = 0;    return; }
              case 0xc005 : { memauxwrite       = 0x80; return; }
              case 0xc006 : { memintcxrom       = 0;    return; }        /* SLOTCXROM Off */
              case 0xc007 : { memintcxrom       = 0x80; return; }        /* SLOTCXROM On */
              case 0xc008 : { memlcaux          = 0;    return; }
              case 0xC009 : { memlcaux          = 0x80; return; }
              case 0xc00a : { memslotc3         = 0;    return; }        /* SLOTC3ROM Off */
              case 0xc00b : { memslotc3         = 0x80; return; }        /* SLOTC3ROM On */
              case 0xc00c : { if (virt80col) {
                                virt80col = 0;
//                              virtdhres = 0;
                                virtsetmode();
                              }
                              return;
                            }
              case 0xc00d : { if (!virt80col) {
                                virt80col = 0x80;
//                              if (!memann3) { virtdhres = 0x80; }
                                virtsetmode();
                              }
                              return;
                            }
              case 0xc00e : { if (virtaltchar) {                /* only update if there was a change */
                                virtaltchar = 0;
                                virtsetmode();
                              }
                              return;
                            }
              case 0xc00f : { if (!virtaltchar) {               /* only update if there was a change */
                                virtaltchar = 0x80;
                                virtsetmode();
                              }
                              return;
                            }
            } /* switch */
          } /* if (ROM16K) */
          else return;
        } /* if (addr <= 0xc00f */

        if (addr <= 0xc01f) {
          if (k!=-1) k&=0x7f;
          return;
        }

        if (addr <= 0xc02f)                             return;   /* Cassette output toggle */

        if (addr <= 0xc03f)     { click();  return; } /* Speaker */

        if (addr <= 0xc04f)                             return;   /* Utility output strobe - nani yo? */

        if (addr <= 0xc06f) {
          switch (addr) {
            case 0xc050 : { if (!(gm&GRX)) {
                              gm|=GRX;
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc051 : { if (gm&GRX) {
                              gm&=~GRX;
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc052 : { if (gm&SPL) {
                              gm&=~SPL;
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc053 : { if (!(gm&SPL)) {
                              gm|=SPL;
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc054 : { if (gm&PG2) {
                              gm &= ~PG2;
                              if (!memstore80) {
                                virtcachemode = (((virtcachemode & 0xfe) + 2) & 0xfe) | (virtcachemode & 0xff00);
                                virtsetmode();
                              }
                            }
                            return;
                          }
            case 0xc055 : { if (!(gm&PG2)) {
                              gm |= PG2;
                              if (!memstore80) {
                                virtsetmode();
                              }
                            }
                            return;
                          }
            case 0xc056 : { if (gm&HRG) {
                              gm&=~HRG;
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc057 : { if (!(gm&HRG)) {
                              gm|=HRG;
                              virtsetmode();
                            }
                            return;
                          }

            case 0xc058 : return; /* Out0-off - nani yo? */
            case 0xc059 : return; /* Out0-on - nani yo? */
            case 0xc05a : return; /* Out1-off - nani yo? */
            case 0xc05b : return; /* Out1-on - nani yo? */
            case 0xc05c : return; /* Out2-off - nani yo? */
            case 0xc05d : return; /* Out2-on - nani yo? */
/*
   C05E 49246 CLRAN3       OE G WR   If IOUDIS off: Annunciator 3 Off
              Y0EDGE         C  WR   If IOUDIS on: Interrupt on Y0 Rising
              DHIRESON      ECG WR   In 80-Column Mode: Double Width Graphics
*/
            case 0xc05e : {
                            if (!virtiou) {
                              memann3 = 0;              /* annunciator#3 off */
                            }
                            if (!virtdhres) {
                              virtdhres = 0x80; /* double hires on   */
                              virtsetmode();
                            }
                            return;
                          }
/*
    C05F 49247 SETAN3       OE G WR   If IOUDIS off: Annunciator 3 On
               Y0EDGE         C  WR   If IOUDIS on: Interrupt on Y0 Falling
               DHIRESOFF     ECG WR   In 80-Column Mode: Single Width Graphics
*/
            case 0xc05f : {
                            if (!virtiou) {
                              memann3 = 0x80;           /* annunciator#3 on */
                            }
                            if (virtdhres) {
                              virtdhres = 0;            /* double hires off */
                              virtsetmode();
                            }
                            return;
                          }
            case 0xc060 :
            case 0xc068 : return; /* Cassette input */
            case 0xc061 :
            case 0xc069 : return;
            case 0xc062 :
            case 0xc06a : return;
            case 0xc063 :
            case 0xc06b : return;
            case 0xc064 :
            case 0xc06c : return; /* Pdl0 */
            case 0xc065 :
            case 0xc06d : return; /* Pdl1 */
            case 0xc066 :
            case 0xc06e : return; /* Pdl2 */
            case 0xc067 :
            case 0xc06f : return; /* Pdl3 */
          } /* switch (addr) */
        } /* if (addr <= 0xc06f */

        if (addr == 0xc070) {
          extern unsigned char ResetGameTimer(unsigned short Address);
          ResetGameTimer(addr);
          return;
        }

        if (addr <= 0xc07f) {
          switch (addr) {
/* The following still has to be included :
C073 49367 BANKSEL       ECG W    Memory Bank Select for > 128K
C07E 49278 IOUDISON      EC  W    Disable IOU
           RDIOUDIS      EC   R7  Status of IOU Disabling
C07F 49279 IOUDISOFF     EC  W    Enable IOU
           RDDHIRES      EC   R7  Status of Double HiRes
*/
            case 0xc07e : { virtiou = 0;    return; }
            case 0xc07f : { virtiou = 0x80; return; }

          } /* switch */
          return;
        }

/* Language Card Handling */
        if (addr <= 0xc08f) {
          switch (addr) {
            case 0xc080 :
            case 0xc084 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0;     /* Write Rom */
                            return;
                          }
            case 0xc081 :
            case 0xc085 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0x80;  /* Write Bank */
                            return;
                          }
            case 0xc082 :
            case 0xc086 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0;     /* Write Rom */
                            return;
                          }
            case 0xc083 :
            case 0xc087 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0x80;  /* Write Bank */
                            return;
                          }
            case 0xc088 :
            case 0xc08c : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0;     /* Write Rom */
                            return;
                          }
            case 0xc089 :
            case 0xc08d : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0x80;  /* Write Bank */
                            return;
                          }
            case 0xc08a :
            case 0xc08e : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0;     /* Write Rom */
                            return;
                          }
            case 0xc08b :
            case 0xc08f : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0x80;  /* Write Bank */
                            return;
                          }
          } /* switch */
        }

/* Slot #1 Softswitches: Parallel Port */
        if (addr >= 0xc090 && addr <= 0xc09f) {
          WriteParallel (addr, val);    return;
        }

/* Slot #5 Softswitches */
        if (addr >= 0xc0d0 && addr <= 0xc0df) {
          WriteRawDiskIO (addr, val);   return;
        }

/* Slot #6 Softswitches */
        if (addr >= 0xc0e0 && addr <= 0xc0ef) {
          WriteDiskIO (addr, val);      return;
        }

/* Slot #7 Softswitches */
        if (addr >= 0xc0f0 && addr <= 0xc0ff && (!dqhdv)) {
          WriteMassStorIO (addr, val);  return;
        }


/* The remaining addresses between 0xc000 and 0xc0ff are simply ignored. */
        return;
      } /* if (addr <= 0xc0ff */

      if (memintcxrom) { return; }

/* Slot #4: Z80 */
// if (Addr>=50432 && Addr<=50687)
#ifdef EMUZ80
        if (addr==0xc400) {
          if (cpuinuse==emuZ80) { /* from manual */
            cpuinuse=emu6502;
            return;
          }
          cpuinuse=emuZ80;
          return;
        }
#endif
        return;
    } /* else (if addr >= 0xd000 */
  } /* else (if (addr < 0xc000 */
}




/*-------------------------------------*/


unsigned char Rd6502(register unsigned short addr)
{

  if (addr < 0xc000) {
    if (addr < 0x2000) {
      if (addr < 0x400) {
        if (addr < 0x200) {
/* $0000 - $01ff */
          if (memlcaux)         { return bankram1[addr]; }                      /* aux 64 */
          else                  { return RAM[addr]; }                           /* main 64 */
        }
        else {
/* $0200 - $03ff */
          if (memauxread)       { return bankram1[addr]; }                      /* aux 64 */
          else                  { return RAM[addr]; }                           /* main 64 */
        }
      }
      else {
        if (addr < 0x800) {
/* $0400 - $07ff */
          if (memstore80) {
            if (gm&PG2)         { return bankram1[addr]; }                      /* aux 64 */
            else                { return RAM[addr]; }                           /* main 64 */
          }
          else {
            if (memauxread)     { return bankram1[addr]; }                      /* aux 64 */
            else                { return RAM[addr]; }                           /* main 64 */
          }
        }
        else {
/* $0800 - $1fff */
          if (memauxread)       { return bankram1[addr]; }                      /* aux 64 */
          else                  { return RAM[addr]; }                           /* main 64 */
        }
      }
    } /* if (addr < 0x2000) */
    else {
      if (addr < 0x4000) {
/* $2000 - $3fff */
        if ((memstore80) && (gm&HRG)) {
          if (gm&PG2)           { return bankram1[addr]; }                      /* aux 64 */
          else                  { return RAM[addr]; }                           /* main 64 */
        }
        else {
          if (memauxread)       { return bankram1[addr]; }                      /* aux 64 */
          else                  { return RAM[addr]; }                           /* main 64 */
        }
      }
      else {
/* $4000 - $bfff */
        if (memauxread)         { return bankram1[addr]; }                      /* aux 64 */
        else                    { return RAM[addr]; }                           /* main 64 */
      }
    }
  } /* if (addr < 0xc000) */
  else {
    if (addr >= 0xd000) {
      if ((!memlcramr) || (memnolc && (!ROM16K)) ) {
        return ROM[addr-0xd000];        /* ROM is the same for whatever memlcaux is set to */
      }
      else {
        if (memlcaux) {
          if (addr < 0xe000) {
            if (memlcbank2) {
              return bankram2[(addr-0xd000)+0x3000];
            }
          }
          return bankram2[addr-0xd000];
        }
        else {
          if (addr < 0xe000) {
            if (memlcbank2) {
              return RAMEXT[(addr-0xd000)+0x3000];
            }
          }
          return RAMEXT[addr-0xd000];
        }
      } /* else if (memlcramr) */
    } /* if (addr => 0xd000 */
    else {

 /* test for softswitch area */
      if (addr <= 0xc0ff) {

        if (addr <= 0xc00f) { return (unsigned char) k; }   /* keyboard */

        if (debugger) return 0x0d;      /* avoid accessing softswitches from the debugger */

        if (addr <= 0xc01f) {
          if (ROM16K) {
            switch (addr) {
              case 0xc010 :     { if (k!=-1) k&=0x7f;
                                  return k&0x7f;        /* not correct (undefined so far) */
                                }
              case 0xc011 :     return (memlcbank2  | (k&0x7f));
              case 0xc012 :     return (memlcramr   | (k&0x7f));
              case 0xc013 :     return (memauxread  | (k&0x7f));
              case 0xc014 :     return (memauxwrite | (k&0x7f));
              case 0xc015 :     return (memintcxrom | (k&0x7f));
              case 0xc016 :     return (memlcaux    | (k&0x7f));
              case 0xc017 :     return (memslotc3   | (k&0x7f));
              case 0xc018 :     return (memstore80  | (k&0x7f));
              case 0xc019 :     return (rasterline<192) ? ((k&0x7f)|0x80) : (k&0x7f);
              case 0xc01a :     return (gm&GRX)     ? (k&0x7f) : ((k&0x7f)|0x80);
              case 0xc01b :     return (gm&SPL)     ? ((k&0x7f)|0x80) : (k&0x7f);
              case 0xc01c :     return (gm&PG2)     ? ((k&0x7f)|0x80) : (k&0x7f);
              case 0xc01d :     return (gm&HRG)     ? ((k&0x7f)|0x80) : (k&0x7f);
              case 0xc01e :     return (virtaltchar | (k&0x7f));
              case 0xc01f :     return (virt80col   | (k&0x7f));
            } /* switch */
          }
          else {
            if (k!=-1) k&=0x7f;
            return k&0x7f;      /* not correct (undefined so far) */
          }
        }


        if (addr <= 0xc02f)                             return k&0x7f;    /* Cassette output toggle */

        if (addr <= 0xc03f)     { click();              return k&0x7f; } /* Speaker */

        if (addr <= 0xc04f)                             return k&0x7f;   /* Utility output strobe - nani yo? */

        if (addr <= 0xc06f) {
          switch (addr) {
            case 0xc050 : { if (!(gm&GRX)) {
                              gm|=GRX;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc051 : { if (gm&GRX) {
                              gm&=~GRX;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc052 : { if (gm&SPL) {
                              gm&=~SPL;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc053 : { if (!(gm&SPL)) {
                              gm|=SPL;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc054 : { if (gm&PG2) {
                              gm &= ~PG2;
                              if (!memstore80) {
                                virtcachemode = (((virtcachemode & 0xfe) + 2) & 0xfe) | (virtcachemode & 0xff00);
                                virtsetmode();
                              }
                            }
                            return virtvideobyte;
                          }
            case 0xc055 : { if (!(gm&PG2)) {
                              gm |= PG2;
                              if (!memstore80) {
                                virtsetmode();
                              }
                            }
                            return virtvideobyte;
                          }
            case 0xc056 : { if (gm&HRG) {
                              gm&=~HRG;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc057 : { if (!(gm&HRG)) {
                              gm|=HRG;
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }

            case 0xc058 : return virtvideobyte; /* Out0-off - nani yo? */
            case 0xc059 : return virtvideobyte; /* Out0-on - nani yo? */
            case 0xc05a : return virtvideobyte; /* Out1-off - nani yo? */
            case 0xc05b : return virtvideobyte; /* Out1-on - nani yo? */
            case 0xc05c : return virtvideobyte; /* Out2-off - nani yo? */
            case 0xc05d : return virtvideobyte; /* Out2-on - nani yo? */
/*
   C05E 49246 CLRAN3       OE G WR   If IOUDIS off: Annunciator 3 Off
              Y0EDGE         C  WR   If IOUDIS on: Interrupt on Y0 Rising
              DHIRESON      ECG WR   In 80-Column Mode: Double Width Graphics
*/
            case 0xc05e : {
                            if (!virtiou) {
                              memann3 = 0;              /* annunciator#3 off */
                            }
                            if (!virtdhres) {
                              virtdhres = 0x80;         /* double hires on   */
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
/*
    C05F 49247 SETAN3       OE G WR   If IOUDIS off: Annunciator 3 On
               Y0EDGE         C  WR   If IOUDIS on: Interrupt on Y0 Falling
               DHIRESOFF     ECG WR   In 80-Column Mode: Single Width Graphics
*/
            case 0xc05f : {
                            if (!virtiou) {
                              memann3 = 0x80;           /* annunciator#3 on */
                            }
                            if (virtdhres) {
                              virtdhres = 0;            /* double hires off */
                              virtsetmode();
                            }
                            return virtvideobyte;
                          }
            case 0xc060 : return 255; // was nothing here -uso.
            case 0xc068 : return 0x00; /* Cassette input */
/* C061 49249 RDBTN0        EC   R7  Switch Input 0 / Open Apple */
            case 0xc061 :
            case 0xc069 : if (joya()) {
                            return 128;
                          } else {
                            return 0;
                          }
/* C062 49250 BUTN1         E    R7  Switch Input 1 / Solid Apple */
            case 0xc062 :
            case 0xc06a : if (joyb()) {
                            return 128;
                          } else {
                            return 0;
                          }
/* C063 49251 RD63          E    R7  Switch Input 2 / Shift Key
                             C   R7  Bit 7 = Mouse Button Not Pressed */
            case 0xc063 :
            case 0xc06b : if ((key[KEY_LSHIFT]||key[KEY_RSHIFT])) return dqshift?128:0; else return 128;
/* C064 49252 PADDL0       OECG  R7  Analog Input 0 */
            case 0xc064 :
            case 0xc06c :       {
                                  extern unsigned char ReadGameTimer(unsigned short Address);
                                  return ReadGameTimer(addr&0xFF);
                                }
/* C065 49253 PADDL1       OECG  R7  Analog Input 1 */
            case 0xc065 :
            case 0xc06d :       {
                                  extern unsigned char ReadGameTimer(unsigned short Address);
                                  return ReadGameTimer(addr&0xFF);
                                }
/* C066 49254 PADDL2       OE G  R7  Analog Input 2
              RDMOUX1        C   R7  Mouse Horiz Position */
            case 0xc066 :
            case 0xc06e : return 0xa0; /* Pdl2 */
/* C067 49255 PADDL3       OE G  R7  Analog Input 3
              RDMOUY1        C   R7  Mouse Vert Position */
            case 0xc067 :
            case 0xc06f : return 0xa0; /* Pdl3 */
          } /* switch (addr) */
        } /* if (addr <= 0xc06f */

        if (addr <= 0xc07f) {
          switch (addr) {
/* C070 49364 PTRIG         E    R   Analog Input Reset
                          C  WR   Analog Input Reset + Reset VBLINT Flag */
            case 0xc070 : {
                            extern unsigned char ResetGameTimer(unsigned short Address);
                            ResetGameTimer(addr);
                            return virtvideobyte;
                          }
/* The following still has to be included : */
/* C073 49367 BANKSEL       ECG W    Memory Bank Select for > 128K */
/* C078 49372                C  W    Disable IOU Access */
/* C079 49373                C  W    Enable IOU Access */

/* C07E 49278 IOUDISON      EC  W    Disable IOU
              RDIOUDIS      EC   R7  Status of IOU Disabling */
            case 0xc07e : {
                            if (ROM16K) { return ((virtiou)   | (virtvideobyte & 0x7f)); }
                            else { return virtvideobyte; }
                          }
/* C07F 49279 IOUDISOFF     EC  W    Enable IOU
              RDDHIRES      EC   R7  Status of Double HiRes */
            case 0xc07f : {
                            if (ROM16K) { return ((virtdhres) | (virtvideobyte & 0x7f)); }
                            else { return virtvideobyte; }
                          }

          } /* switch */
          return virtvideobyte;
        }

/* Language Card Handling */
        if (addr <= 0xc08f) {
          switch (addr) {
            case 0xc080 :
            case 0xc084 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0;     /* Write Rom */
                            return (k&0x7f);
                          }
            case 0xc081 :
            case 0xc085 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0x80;  /* Write Bank */
                            return (k&0x7f);
                          }
            case 0xc082 :
            case 0xc086 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0;     /* Write Rom */
                            return (k&0x7f);
                          }
            case 0xc083 :
            case 0xc087 : {
                            memlcbank2 = 0x80;  /* Bank 2 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0x80;  /* Write Bank */
                            return (k&0x7f);
                          }
            case 0xc088 :
            case 0xc08c : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0;     /* Write Rom */
                            return (k&0x7f);
                          }
            case 0xc089 :
            case 0xc08d : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0x80;  /* Write Bank */
                            return (k&0x7f);
                          }
            case 0xc08a :
            case 0xc08e : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0;     /* Read Rom */
                            memlcramw  = 0;     /* Write Rom */
                            return (k&0x7f);
                          }
            case 0xc08b :
            case 0xc08f : {
                            memlcbank2 = 0;     /* Bank 1 */
                            memlcramr  = 0x80;  /* Read Bank */
                            memlcramw  = 0x80;  /* Write Bank */
                            return (k&0x7f);
                          }
          } /* switch */
        }

/* Slot #1 Softswitches: Parallel Port */
        if (addr >= 0xc090 && addr <= 0xc09f) {
/*        return ReadParallel (addr); */
          return 0xa0;
        }

/* Slot #5 Softswitches */
        if (addr >= 0xc0d0 && addr <= 0xc0df) {
          return ReadRawDiskIO (addr);
        }

/* Slot #6 Softswitches */
        if (addr >= 0xc0e0 && addr <= 0xc0ef) {
          return ReadDiskIO (addr);
        }

/* Slot #7 Softswitches */
        if (addr >= 0xc0f0 && addr <= 0xc0ff && (!dqhdv)) {
          return dqhdv? 0xa0 : ReadMassStorIO (addr);
        }

/* The remaining addresses between 0xc000 and 0xc0ff are simply ignored. */
        return 0xa0;
      } /* if (addr <= 0xc0ff */
      else {
/* $c100 - $cfff */
        if (memintcxrom) return ROM[(addr-0xc100)+12288];
        else {
          if (addr <= 0xc1ff) return PICROM[addr-0xc100];       /* 1 */
          if (addr <= 0xc2ff) return 0xff;                      /* 2 */
          if (addr <= 0xc3ff) {                                 /* 3 */
            if (ROM16K) {
              if (memslotc3) { return 0xff; }
              else           { return ROM[(addr-0xc100)+12288]; }       /* copy of INTCXROM */
            }
            else             { return 0xff; }
          }
          if (addr <= 0xc4ff) return 0xff;                                      /* 4 */
          if (addr <= 0xc5ff) return HDDROM[addr-0xc500];                       /* 5 */
          if (addr <= 0xc6ff) return DISKROM[addr-0xc600];                      /* 6 */
          if (addr <= 0xc7ff) return dqhdv ? 0xff : HDDROM[addr-0xc700];        /* 7 */
          if (addr >= 0xc800) return ROM[(addr-0xc100)+12288];
          return 0xff;
        }
      } /* else if (addr <= 0xc0ff */
    } /* else if (addr >= 0xd000) */
  } /* else if (addr < 0xc000) */
} /* rd6502() */

int brun(char *filename)
{
 unsigned short int ca,address,length;
 unsigned long int fl;
 FILE *file;
 char *errstr;

 file=fopen(filename,"rb");
// gotoxy(1,25);
 if (!file)
 {
  perror(filename);
  return -1;
 }
 fseek(file,0,SEEK_END);
 fl=ftell(file)-4;
 fseek(file,0,SEEK_SET);
 fread(&address,2,1,file);
 fread(&length,2,1,file);
// gotoxy(1,25);

 if (fl+address>49152) return -2;

 for (ca=address; fl; fl--, ca++) Wr6502(ca,fgetc(file));

 fclose(file);

 cpusetpc(address);
 return 0;
}

/* Do nothing interrupt handler */
//void interrupt Int1B (void)
//{
//}
//
//void interrupt (*Old1B) (void);

//void inst1B (void)
//{
// Old1B=getvect(0x1B);
// setvect(0x1B,Int1B);
//}

//void uninst1B (void)
//{
// setvect(0x1B,Old1B);
//}

//zD
void uimain(){
do_dialog(dialog,0);
}
int softreset(){
DiskReset();
cpureset();
virtreset();
return D_CLOSE;
}
int hardreset(){
RAM[1012]=0;
DiskReset();
cpureset();
memoryreset();
virtreset();
return D_CLOSE;
}
int fullreset(){
DiskReset();
cpureset();
memoryreset();
memoryclear();
virtreset();
return D_CLOSE;
}
// yay
int loadstate(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Load State...",filename,"st");
if(x){
imrs(filename);
mbox("Loaded State!");
}
return D_CLOSE;
}
int savestate(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Save State...",filename,"st");
if(x){
immk(filename);
mbox("Saved State!");
}
return D_CLOSE;
}
int loadbaszor(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Load Basic...",filename,"bas");
if(x){
loadbas(filename);
mbox("Basic Loaded!");
}
return D_CLOSE;
}
int savebaszor(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Save Basic...",filename,"bas");
if(x){
savebas(filename);
mbox("Basic Saved!");
}
return D_CLOSE;
}
int xbloadzor(){
char filename[1024];
int x;
int addr;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Load Program...",filename,"pg2");
if(x){
addr = xbload(filename);
if(yesno("Execute the program ?") == 1) cpusetpc(addr);
}
return D_CLOSE;
}
int seld1(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Select Disk 1...",filename,"dsk");
if(x){
UnmountDisk(0);
strcpy(DrvSt[0].DiskFN, filename);
DrvSt[0].DiskType = UnknownType;
MountDisk(0);
}
return D_CLOSE;
}
int seld2(){
char filename[1024];
int x;
getcwd(filename,1024);
strcat(filename,"/");
x=fdialog("Select Disk 2...",filename,"dsk");
if(x){
UnmountDisk(1);
strcpy(DrvSt[1].DiskFN, filename);
DrvSt[1].DiskType = UnknownType;
MountDisk(1);
}
return D_CLOSE;
}
int main (int argc, char **argv)
{
 FILE  *rom;
FONT *myfont;
 RGB *mypalette;
 unsigned int travel;
 extern char hd1[128],hd2[128];

 allegro_init();
 install_keyboard();
 install_timer();
 install_mouse();
// install_joystick();
 //set_color_depth(8);
 bufferzor=create_bitmap(640,480);
 strcpy(hd1,"hdv1.hdv");
 strcpy(hd2,"hdv2.hdv");

 /* if (!isatty(fileno(stdin))) */
//   freopen("con","r",stdin);

// if (argc>3) {usage(); return;}

  rom=fopen(ROMDIR "disk.rom","rb");
  if (!rom) {
    printf ("disk ][ : ROM not available\n");
    memset(DISKROM,0xff,sizeof(DISKROM));       /* set rom to 0xff */
  }
  else {
    fread(DISKROM,256,1,rom);
    fclose(rom);
  }

  rom=fopen(ROMDIR "parallel.rom","rb");
  if (!rom) {
    printf ("Parallel port : ROM not available\n");
    memset(PICROM,0xff,sizeof(PICROM));         /* set rom to 0xff */
  }
  else {
    fread(PICROM,256,1,rom);
    fclose(rom);
  }

  rom=fopen(ROMDIR "massstor.rom","rb");
  if (!rom) {
    printf ("Mass storage devices : ROM not available\n");
    memset(HDDROM,0xff,sizeof(HDDROM));         /* set rom to 0xff */
  }
  else {
    fread(HDDROM,256,1,rom);
    fclose(rom);
  }

  rdini();

 InitDisk( 6 );

 argquickstart  = 0;    /* no quickstart        */
 argini         = 0;    /* save ini on exit     */

 if (argc>1)
 {
  int travel;

  for (travel=1; travel<argc; travel++)
  {
   if (!strcmp(argv[travel],"-nosaveini"))
   {
    argini=1;
   }
   if (!strcmp(argv[travel],"-nohd"))
   {
    dqhdv=1;
   }
   if (!strcmp(argv[travel],"-green"))
   {
    virtsetmonochrome(2);
   }
   if (!strcmp(argv[travel],"-white"))
   {
    virtsetmonochrome(1);
   }
   if (!strcmp(argv[travel],"-amber"))
   {
    virtsetmonochrome(3);
   }
   if (!strcmp(argv[travel],"-color"))
   {
    virtsetmonochrome(0);
   }
   if (!strcmp(argv[travel],"-rom"))
   {
    travel++;
    strcpy(rompath,argv[travel]);
   }
   if (!strcmp(argv[travel],"-d1"))
   {
    travel++;
    UnmountDisk(0);
    strcpy(DrvSt[0].DiskFN, argv[travel]);
    DrvSt[0].DiskType = UnknownType;
    MountDisk(0);
   }
   if (!strcmp(argv[travel],"-d2"))
   {
    travel++;
    UnmountDisk(1);
    strcpy(DrvSt[1].DiskFN, argv[travel]);
    DrvSt[1].DiskType = UnknownType;
    MountDisk(1);
   }
   if (!strcmp(argv[travel],"-h1"))
   {
    dqhdv=0;
    strcpy(hd1,argv[++travel]);
   }
   if (!strcmp(argv[travel],"-h2"))
   {
    dqhdv=0;
    strcpy(hd2,argv[++travel]);
   }
   if (!strcmp(argv[travel],"-q"))
   {
    argquickstart = 1;
   }
   if (!strcmp(argv[travel],"-nospeaker"))
   {
    smode=0;
   }
   if (!strcmp(argv[travel],"-nocapslock"))
   {
    keepshift=-1;
    shiftshutoff=1;
   }
  }
 }

 InitMassStor( 7 );

/* automatic search for rom file */

  rom=fopen(rompath,"rb");
  if (!rom) {
    strcpy(rompath,ROMDIR "apple.rom");
    rom=fopen(rompath,"rb");
    if (!rom) {
      strcpy(rompath,ROMDIR "apple2o.rom");
      rom=fopen(rompath,"rb");
      if (!rom) {
        strcpy(rompath,ROMDIR "apple2m.rom");
        rom=fopen(rompath,"rb");
        if (!rom) {
          strcpy(rompath,ROMDIR "apple2eo.rom");
          rom=fopen(rompath,"rb");
          if (!rom) {
            strcpy(rompath,ROMDIR "apple2ee.rom");
            rom=fopen(rompath,"rb");
            if (!rom) {
              printf ("No BIOS ROMs found.\n");
              goto veow;
            }
          }
        }
      }
    }
  }
 fclose(rom);
 /* load rom, reset softswitches and init the video display */
 reload(rompath);

 debugger = 0;

// signal(SIGINT,SIG_IGN);

bow:

  if (!argquickstart) {
    //uimain();           /* show menu on start */
  }
  if (!exitprogram) {
  //allegro_init();
//  install_keyboard();
//  install_timer();
  opengraph();
#ifndef __MSDOS__
  set_window_title("Dapple " DappleVersion);
#endif
myfont = (FONT*)load_bios_font("FONT.BIOS",0,0);
 if(!myfont) mbox("Font load was unsuccessful");
 else font = myfont;
 gui_bg_color=makecol(0,0,127);
 gui_fg_color=makecol(255,255,255);
  uimain();
cpurun();
    //cpurun();           /* start emulation */
  }

eow:
 if (!argini) wrini();
 // gmode(T80); <-- turn off the graphics
veow:
;
return 0;
}
