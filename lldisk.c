/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  LLDISK:  code for handling raw 1.44MB disks in drive A:
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

/*
 * Error listing:  Unless noted, all errors return 2.  TC++ 1.01
 *
 *   0: No error (data accepted)
 *   1: Bad command
 *   2: Address mark not found
 *   3: Write protected (returns 1)
 *   4: Sector not found
 *   5: Reset failed (hard disk)
 *   6: Disk changed
 *   7: Drive parameter activity failed
 *   8: DMA overrun
 *   9: DMA crosses segment boundary
 *  10: Bad sector detected
 *  11: Bad track detected
 *  12: Unsupported track
 *  16: CRC error
 *  17: CRC error, compensated (data accepted)
 *  32: Controller failure
 *  64: Seek error
 * 128: Attachment failed to respond
 * 170: Hard disk not ready
 * 187: ???
 * 204: Write fault
 * 224: Status error
 * 255: Sense operation failed
 */

extern unsigned char virtscreen[128000];
//#define virtplot(x,y,c) virtscreen[(y*320)+x]=c
//#define virtplot(x,y,c) putpixel(screen,x,y,c)
extern void virtscreencopy();
extern unsigned char virtcopy;

//#include <bios.h>
#define COL_DRV_OFF  0x24
#define COL_DRV_ON   0x25

typedef struct
{
 unsigned int head;
 unsigned int track;
 unsigned int sector;
} drvptr;

void sec2hts (drvptr *hts, unsigned int sector)
{
 unsigned int avgtrk;

 hts->sector=sector%18;
 avgtrk=(sector-hts->sector)/18;
 hts->head=avgtrk%2;
 hts->track=(avgtrk-hts->head)/2;

// hts->track++;
 hts->sector++;
}

int llread (int sector, unsigned char *buf)
{
 /*drvptr hts;
 int e;

 virtplot(634,2,COL_DRV_ON);
 virtcopy = 1; /* force redraw of screen */
/* virtscreencopy();

 sec2hts(&hts,sector);

 e=biosdisk(2,0,hts.head,hts.track,hts.sector,1,buf);

 virtplot(634,2,COL_DRV_OFF);
 virtcopy = 1; /* force redraw of screen */
/* virtscreencopy();

 if (e==0||e==17) return 0;
 if (e==3) return 1;
 return -1;
 */
}

int llwrite (int sector, unsigned char *buf)
{
/* drvptr hts;
 int e;

 virtplot(634,2,COL_DRV_ON);
 virtcopy = 1; /* force redraw of screen */
 /*virtscreencopy();

 sec2hts(&hts,sector);

 e=biosdisk(3,0,hts.head,hts.track,hts.sector,1,buf);

 virtplot(634,2,COL_DRV_OFF);
 virtcopy = 1; /* force redraw of screen */
/* virtscreencopy();

 if (e==0||e==17) return 0;
 if (e==3) return 1;
 return -1;
 */
}

#include <string.h>
#include <stdio.h>
//#include <dos.h>
#include <fcntl.h>
//#include <io.h>
#include "dapple.h"

void InitMassStor( int slot );
void ShutdownMassStor( void );
byte ReadMassStorIO( word Address );
void WriteMassStorIO( word Address, byte Data );

#define MS_DMA

/*
;Mass-storage device I/O
; $0 - Writing the ProDOS command here will perform that command.
;      It is important to setup the other locations first.
;      0=Status,1=Read,2=Write,3=Format
; $1 - Unit number $0-$3
; $2 - ProDOS buffer low byte
; $3 - ProDOS buffer high byte
; $4 - Block low byte
; $5 - Block high byte
; $6 - Data register
*/

#define PS_OK    0x00
#define PS_IOERR 0x27
#define PS_NODEV 0x28
#define PS_WRPRO 0x2b

int RawDiskSlot;

word RMaxBlock = 2880;

long RVolumeSize[ 4 ];     /* size of volumes in bytes */
int RWritePro[ 4 ];        /* if a volume is write-protected */
int RValidBlock[ 4 ];      /* if current block buffer contains valid data */
word RBlock[ 4 ];          /* block # in buffer */
byte RBuffer[ 4 ][ 512 ];  /* 512 byte buffer for each unit */

byte RUnit;            /* current unit */
word RProDOSBuf;       /* Apple memory location to ProDOS buffer */
word RByte[ 4 ];       /* current byte offset in buffer */
byte RDataReg;         /* current value of data register */
byte RStatus;          /* current status of device */

#define STATUS_OFF 0
#define STATUS_RD  1
#define STATUS_WR  2

void InitRawDisk( int slot )
{
    int i;
    unsigned int attr;

    RawDiskSlot = slot;

    for ( i = 0; i < 4; i++ )
    {
        RUnit=i;
        RValidBlock[ i ] = 0;
    }
    RVolumeSize[ 0 ] = 1474560;
    RStatus = 0;
    RUnit = 0;
}

byte ReadRawDiskIO( word Address )
{
    byte v;

    v = 0xff;
    switch ( Address & 0x0f )
    {
    case 0x01:
        v = RUnit;
        break;
    case 0x02:
        #ifdef MS_DMA
        v = RProDOSBuf & 0xff;
        #else
        v = RByte[ RUnit ] & 0xff;
        #endif
        break;
    case 0x03:
        #ifdef MS_DMA
        v = RProDOSBuf >> 8;
        #else
        v = RByte[ RUnit ] >> 8;
        #endif
        break;
    case 0x04:
        v = RBlock[ RUnit ] & 0xff;
        break;
    case 0x05:
        v = RBlock[ RUnit ] >> 8;
        break;
    case 0x06:
        v = RDataReg;
        break;
    case 0x07:
        v = RStatus;
        break;
    }
    return v;
}

/* This doesn't actually format the disk yet, we've got to work on that... */
void RFormatVolume( void )
{
 /* RStatus = PS_WRPRO; if write protected. */

// int travel, error;
// char zeroes[9216];

// for (travel=0; travel<9216; travel++)
// {
//  zeroes[travel]=0;
// }

// for (travel=0; travel<80; travel++)
// {
//   virtplot(314,2,COL_DRV_ON);
//   virtcopy = 1; /* force redraw of screen */
//   virtscreencopy();

//   error=biosdisk(3,0,0,travel,0,18,zeroes);

//   virtplot(314,2,COL_DRV_OFF);
//   virtcopy = 1; /* force redraw of screen */
//   virtscreencopy();

// if (error!=0&&error!=17) {RStatus=PS_IOERR; return;}
//   if (error==3) {RStatus=PS_WRPRO; return;}

//   error=biosdisk(3,0,1,travel,0,18,zeroes);
// if (error!=0&&error!=17) {RStatus=PS_IOERR; return;}
//   if (error==3) {RStatus=PS_WRPRO; return;}
// }
}

void RReadBlock( void )
{
    #ifdef MS_DMA
    int bufptr;
    #endif

    if ( !RValidBlock[ RUnit ] )
    {
        /* get block from disk */
        if ( RUnit )
        {
            RStatus = PS_NODEV;
        }
        else
        {
            int xs;

            xs=llread(RBlock[RUnit],RBuffer[RUnit]);
            if ( xs==2 )
            {
                RStatus = PS_IOERR;
            }
            else
            {
                RValidBlock[ RUnit ] = 1;
                RByte[ RUnit ] = 0;
            }
        }
    }
    if ( RStatus == PS_OK )
    {
        #ifdef MS_DMA
        for ( bufptr = 0; bufptr < 512; bufptr++ )
        {
            Wr6502( RProDOSBuf + bufptr, RBuffer[ RUnit ][ bufptr ] );
        }
        #else
        RDataReg = RBuffer[ RUnit ][ RByte[ RUnit ] ];
        RByte[ RUnit ]++;
        if ( RByte[ RUnit ] == 0x200 )
        {
            RByte[ RUnit ] = 0;
        }
        #endif
    }
}

void RWriteBlock( void )
{
    #ifdef MS_DMA
    int bufptr;
    #endif

    #ifdef MS_DMA
    for ( bufptr = 0; bufptr < 512; bufptr++ )
    {
        RBuffer[ RUnit ][ bufptr ] = Rd6502( RProDOSBuf + bufptr );
    }
    RByte[ RUnit ] = 0;
    #else
    RBuffer[ RUnit ][ RByte[ RUnit ] ] = RDataReg;
    RByte[ RUnit ]++;
    if ( RByte[ RUnit ] == 0x200 )
    {
        RByte[ RUnit ] = 0;
    }
    #endif
    /* If not a valid filehandle, then no device error */
    if ( RUnit )
    {
        RStatus = PS_NODEV;
        return;
    }
    /* Only write when the buffer is full */
    if ( RByte[ RUnit ] == 0 )
    {
        int ex;

        ex=llwrite(RBlock[RUnit],RBuffer[RUnit]);
        if (ex==1) {RStatus=PS_WRPRO; return;}
        if (ex) {RStatus=PS_IOERR; return;}
    }
}

void WriteRawDiskIO( word Address, byte Data )
{
    RStatus = PS_OK;
    switch ( Address & 0x0f )
    {
    case 0x00:
        switch ( Data )
        {
        case 0x00:
            /* Status */
            /* Set # blocks */
            RBlock[ RUnit ] = 2880;
            break;
        case 0x01:
            RReadBlock();
            break;
        case 0x02:
            RWriteBlock();
            break;
        case 0x03:
            RFormatVolume();
            break;
        }
        break;
    case 0x01:
        RUnit = Data;
        break;
    case 0x02:
        #ifdef MS_DMA
        RProDOSBuf = ( RProDOSBuf & 0xff00 ) | Data;
        #else
        RByte[ RUnit ] = ( RByte[ RUnit ] & 0xff00 ) | Data;
        #endif
        break;
    case 0x03:
        #ifdef MS_DMA
        RProDOSBuf = ( RProDOSBuf & 0x00ff ) | ( Data << 8 );
        #else
        RByte[ RUnit ] = ( RByte[ RUnit] & 0x00ff ) | ( Data << 8 );
        #endif
        break;
    case 0x04:
        RBlock[ RUnit ] = ( RBlock[ RUnit ] & 0xff00 ) | Data;
        RValidBlock[ RUnit ] = 0;
        break;
    case 0x05:
        RBlock[ RUnit ] = ( RBlock[ RUnit ] & 0x00ff ) | ( Data << 8 );
        RValidBlock[ RUnit ] = 0;
        break;
    case 0x06:
        RDataReg = Data;
        break;
  }
}
