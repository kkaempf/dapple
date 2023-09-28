/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  BINMANIP: binary image manipulation tools
 * Revision:   (1.20) 2002.1229
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
#include <errno.h>
#include <string.h>
//#include "m6502.h"
#include "dapple.h"

/* dapple.c */
extern int smode;
extern void memorystore(FILE *file);
extern void memoryrestore(FILE *file);

/* cpu65c02.c */
extern void cpustore(FILE *file);
extern void cpurestore(FILE *file);

/* video.c */
extern void virtstore(FILE *file);
extern void virtrestore(FILE *file);

extern char parallel[128];
extern int dqhdv;

/* Just to have something, even if it lacks some of the other types -uso. */
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

/* Saves RAM image, ROM image and processor status */
int immk (char *filename)
{
 FILE *file;

 file=fopen(filename,"wb");
 if (!file) return -1;

 fprintf(file,"DAPL");
 fwrite(&smode,sizeof(smode),1,file);

 memorystore(file);
 virtstore(file);
 cpustore(file);

 fwrite(DrvSt,sizeof(struct DriveState),2,file);
 fwrite(parallel,128,1,file);
 fwrite(&dqhdv,sizeof(dqhdv),1,file);

 fclose(file);
 return 0;
}


int imrs (char *filename)
{
 FILE *file;
 char uid[4];

 file=fopen(filename,"rb");
 if (!file) return -1;

 fread(uid,4,1,file);
 if (uid[0]!='D' || uid[1]!='A' || uid[2]!='P' || uid[3]!='L')
 {
  return -2;
 }
/* this has to be changed, since not only gm does store the current
   display but other new variables as well */

 fread(&smode,sizeof(smode),1,file);

 memoryrestore(file);
 virtrestore(file);
 cpurestore(file);

 fread(DrvSt,sizeof(struct DriveState),2,file);
 fread(parallel,128,1,file);
 fread(&dqhdv,sizeof(dqhdv),1,file);

 fclose(file);
 return 0;
}

int xbload (char *filename)
{
 unsigned short int ca,address,length;
 unsigned long int fl;
 FILE *file;
 char *errstr;

 file=fopen(filename,"rb");
// gotoxy(1,25);
 if (!file)
 {
  errstr=strerror(errno);
  fprintf(stderr,"%s: %s%c\r",filename,errstr,7);
  return -1370;
 }
 fseek(file,0,SEEK_END);
 fl=ftell(file)-4;
 fseek(file,0,SEEK_SET);
 fread(&address,2,1,file);
 fread(&length,2,1,file);

 for (ca=address; fl; fl--, ca++) Wr6502(ca,fgetc(file));

 fclose(file);
 return address;
}

/* this doesn't work very well... */

void savebas (char *filename)
{
 unsigned int startofprog;
 unsigned int endofprog;
 unsigned int lenofprog;
 unsigned int travel;
 FILE        *file;

 startofprog=RAM[103]+256*RAM[104];
 endofprog = RAM[175]+256*RAM[176];

 lenofprog = endofprog-startofprog;

 file=fopen(filename,"wb");
 if (!file) return;

 fwrite(&lenofprog,1,sizeof(lenofprog),file);

 for (travel=startofprog; travel<=endofprog; travel++)
  fwrite(&(RAM[travel]),1,1,file);

 fclose(file);
}

int loadbas(char *filename)
{
 unsigned int startofprog;
 unsigned int endofprog;
 unsigned int lenofprog;
 unsigned int travel;
 char         tmp;
 FILE        *file;

 file=fopen(filename,"rb");
 if (!file) return -1;

 fread(&lenofprog,1,sizeof(lenofprog),file);

 startofprog=RAM[103]+256*RAM[104];
 endofprog = startofprog+lenofprog;
 RAM[105]=endofprog%256;
 RAM[106]=endofprog>>8;
 RAM[175]=endofprog%256;
 RAM[176]=endofprog>>8;

 for (travel=startofprog; travel<=endofprog; travel++)
  fread(&(RAM[travel]),1,1,file);

 fclose(file);
 return 0;
}
