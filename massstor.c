/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  MASSSTOR: Native interface to Andrew Gregory's MASSSTOR.ROM
 * Revision:   (1.20) 2002.1229
 * Code was taken from Andrew Gregory's emulator
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

/* MASSSTOR.ROM is the Apple interface to the mass storage disk images. */

#include <string.h>
#include <stdio.h>
//#include <dos.h>
#include <fcntl.h>
//#include <io.h>
#include <unistd.h>
#include <allegro.h>
//#include "dapple.h"

#define COL_DRV_OFF  0x24
#define COL_DRV_ON   0x25

extern unsigned char virtscreen[128000];
#define virtplot(x,y,c) virtscreen[(y*320)+x]=c
//#define virtplot(x,y,c) putpixel(screen,x,y,c)
extern void virtscreencopy();
extern unsigned char virtcopy;

char hd1[128],hd2[128];

void InitMassStor( int slot );
void ShutdownMassStor( void );
unsigned char ReadMassStorIO( unsigned short Address );
void WriteMassStorIO( unsigned short Address, unsigned char Data );

/* NOTE: in theory ProDOS can support 4 volumes in slots 5 & 6. However,
   it never seems to perform STATUS or FORMAT calls for the 2 extra units.
   Hence, the mass-storage routines supply ProDOS with incorrect block
   numbers, and won't format correctly. So extra volumes are disabled for
   now. */

/* #define MS_DMA to create DMA (direct memory access) version
   #undef  MS_DMA to create slow I/O (but more standard) version
                  where the card does not access memory
   Make sure the correct version of MASSSTOR.ASM has been assembled. */
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

int MassStorSlot;

unsigned short MaxBlock = 65535; //280;

char VolumeFN[ 4 ][ 80 ]; /* volume file names */
int VolumeFH[ 4 ];        /* volume file handles */
long VolumeSize[ 4 ];     /* size of volumes in bytes */
int WritePro[ 4 ];        /* if a volume is write-protected */
int ValidBlock[ 4 ];      /* if current block buffer contains valid data */
unsigned short int Block[ 4 ];          /* block # in buffer */
unsigned char Buffer[ 4 ][ 512 ];  /* 512 byte buffer for each unit */

char HDDROM[256]; /* no room in dapple.c, moved here -uso. */

int hdd=8;

unsigned char Unit;            /* current unit */
unsigned short ProDOSBuf;       /* Apple memory location to ProDOS buffer */
unsigned char Byte[ 4 ];       /* current byte offset in buffer */
unsigned char DataReg;         /* current value of data register */
unsigned char Status;          /* current status of device */

#define STATUS_OFF 0
#define STATUS_RD  1
#define STATUS_WR  2

unsigned int GetAttrib( char *filename );

/* Updates light for current unit based on command */
/* cmd:0=nothing,1=reading,2=writing */
void UpdateMassStorStatus( int cmd )
{
 hdd=(cmd?((cmd==1)?2:4):8);
}

void InitMassStor( int slot )
{
    int i;
    unsigned int attr;

    MassStorSlot = slot;

    for ( i = 0; i < 4; i++ )
    {
        VolumeFN[ i ][ 0 ] = 0;
    }

    strcpy( VolumeFN[ 0 ], hd1 );
    strcpy( VolumeFN[ 1 ], hd2 );

    for ( i = 0; i < 4; i++ )
    {
        Unit=i;
        UpdateMassStorStatus( STATUS_OFF );
        attr = GetAttrib( VolumeFN[ i ] );
        if ( attr == 0xffff )
        {
            attr = 0;
        }
        //WritePro[ i ] = ( attr & FA_RDONLY ) ? 1 : 0;
	WritePro[i] = 0;
        //VolumeFH[ i ] = open( VolumeFN[ i ], WritePro[ i ] ? O_RDONLY : O_RDWR );
	VolumeFH[i]=open(VolumeFN[i],O_RDWR);
        if ( VolumeFH[ i ] > 0 )
        {
            VolumeSize[ i ] = FileSize( VolumeFH[ i ] );
        }
        else
        {
            VolumeSize[ i ] = 0L;
        }
        ValidBlock[ i ] = 0;
    }
    Status = 0;
    Unit = 0;
}

void ShutdownMassStor( void )
{
    int i;

    for ( i = 0; i < 4; i++ )
    {
        if ( VolumeFH[ i ] > 0 )
        {
            close( VolumeFH[ i ] );
            if ( VolumeSize[ i ] == 0L )
            {
                /* nothing in the file - might as well delete it */
                remove( VolumeFN[ i ] );
            }
        }
        VolumeFH[ i ] = 0;
    }
}


unsigned char ReadMassStorIO( unsigned short Address )
{
    unsigned char v;

    v = 0xff;
    switch ( Address & 0x0f )
    {
    case 0x01:
        v = Unit;
        break;
    case 0x02:
        #ifdef MS_DMA
        v = ProDOSBuf & 0xff;
        #else
        v = Byte[ Unit ] & 0xff;
        #endif
        break;
    case 0x03:
        #ifdef MS_DMA
        v = ProDOSBuf >> 8;
        #else
        v = Byte[ Unit ] >> 8;
        #endif
        break;
    case 0x04:
        v = Block[ Unit ] & 0xff;
        break;
    case 0x05:
        v = Block[ Unit ] >> 8;
        break;
    case 0x06:
        v = DataReg;
        break;
    case 0x07:
        v = Status;
        break;
    }
    return v;
}

/* this does not actually erase any data - it only attempts to create a
   volume of the size given in MaxBlock */
void FormatVolume( void )
{
    if ( WritePro[ Unit ] ) {
        Status = PS_WRPRO;
    }
    else {
        lseek( VolumeFH[ Unit ], 512L * (long)MaxBlock -1L, SEEK_SET );
        write( VolumeFH[ Unit ], Buffer[ Unit ], 1 );
        VolumeSize[ Unit ] = (long)MaxBlock * 512L;
    }
}

void ReadBlock( void )
{
    #ifdef MS_DMA
    int bufptr;
    #endif

    if ( !ValidBlock[ Unit ] )
    {
        /* get block from disk */
        if ( VolumeFH[ Unit ] < 1 )
        {
            Status = PS_NODEV;
        }
        else
        {
            virtplot(634+(Unit*2),6,COL_DRV_ON);
            virtcopy = 1;
            virtscreencopy();
            lseek( VolumeFH[ Unit ], 512L * (long)Block[ Unit ], SEEK_SET );
            read( VolumeFH[ Unit ], Buffer[ Unit ], 512 );
            virtplot(634+(Unit*2),6,COL_DRV_OFF);
            virtcopy = 1;
            virtscreencopy();
            ValidBlock[ Unit ] = 1;
            Byte[ Unit ] = 0;
        }
    }
    if ( Status == PS_OK )
    {
        #ifdef MS_DMA
        for ( bufptr = 0; bufptr < 512; bufptr++ )
        {
            Wr6502( ProDOSBuf + bufptr, Buffer[ Unit ][ bufptr ] );
        }
        #else
        DataReg = Buffer[ Unit ][ Byte[ Unit ] ];
        Byte[ Unit ]++;
        if ( Byte[ Unit ] == 0x200 )
        {
            Byte[ Unit ] = 0;
        }
        #endif
    }
}

void WriteBlock( void )
{
    #ifdef MS_DMA
    int bufptr;
    #endif

    #ifdef MS_DMA
    for ( bufptr = 0; bufptr < 512; bufptr++ )
    {
        Buffer[ Unit ][ bufptr ] = Rd6502( ProDOSBuf + bufptr );
    }
    Byte[ Unit ] = 0;
    #else
    Buffer[ Unit ][ Byte[ Unit ] ] = DataReg;
    Byte[ Unit ]++;
    if ( Byte[ Unit ] == 0x200 )
    {
        Byte[ Unit ] = 0;
    }
    #endif
    /* If not a valid filehandle, then no device error */
    if ( VolumeFH[ Unit ] < 1 )
    {
        Status = PS_NODEV;
        return;
    }
    /* If write-protected, then write-protected error */
    if ( WritePro[ Unit ] )
    {
        Status = PS_WRPRO;
        return;
    }
    /* Only write when the buffer is full */
    if ( Byte[ Unit ] == 0 )
    {
            virtplot(634+(Unit*2),6,COL_DRV_ON);
            virtcopy = 1;
            virtscreencopy();
        /* Try to seek to the blocks location in the file */
        if ( lseek( VolumeFH[ Unit ], 512L * (long)Block[ Unit ], SEEK_SET ) == -1L )
        {
            /* Error */
            Status = PS_IOERR;
            virtplot(634+(Unit*2),6,COL_DRV_OFF);
            virtcopy = 1;
            virtscreencopy();
            return;
        }
        /* If the file is newly created (zero-length) then it needs to be
           formatted first */
        if ( VolumeSize[ Unit ] == 0L )
        {
            /* Check if the volume should be formatted */
//          if ( ShouldFormat )
//          {
//              FormatVolume();
//          }
        }
        /* Everything appears to be OK up until now - write the buffer */
        if ( write( VolumeFH[ Unit ], Buffer[ Unit ], 512 ) == 0 )
        {
            /* Error on write */
            Status = PS_IOERR;
        }
    }
    virtplot(634+(Unit*2),6,COL_DRV_OFF);
    virtcopy = 1;
}

void WriteMassStorIO( unsigned short Address, unsigned char Data )
{
    Status = PS_OK;
    switch ( Address & 0x0f )
    {
    case 0x00:
        switch ( Data )
        {
        case 0x00:
            /* Status */
            /* Set # blocks */
            Block[ Unit ] = ( VolumeSize[ Unit ] == 0L ) ?
                            MaxBlock : (unsigned short)( VolumeSize[ Unit ] / 512L );
            break;
        case 0x01:
            UpdateMassStorStatus( STATUS_RD );
            ReadBlock();
            break;
        case 0x02:
            UpdateMassStorStatus( STATUS_WR );
            WriteBlock();
            break;
        case 0x03:
            UpdateMassStorStatus( STATUS_WR );
            FormatVolume();
            break;
        }
        UpdateMassStorStatus( STATUS_OFF );
        break;
    case 0x01:
        Unit = Data;
        break;
    case 0x02:
        #ifdef MS_DMA
        ProDOSBuf = ( ProDOSBuf & 0xff00 ) | Data;
        #else
        Byte[ Unit ] = ( Byte[ Unit ] & 0xff00 ) | Data;
        #endif
        break;
    case 0x03:
        #ifdef MS_DMA
        ProDOSBuf = ( ProDOSBuf & 0x00ff ) | ( Data << 8 );
        #else
        Byte[ Unit ] = ( Byte[ Unit] & 0x00ff ) | ( Data << 8 );
        #endif
        break;
    case 0x04:
        Block[ Unit ] = ( Block[ Unit ] & 0xff00 ) | Data;
        ValidBlock[ Unit ] = 0;
        break;
    case 0x05:
        Block[ Unit ] = ( Block[ Unit ] & 0x00ff ) | ( Data << 8 );
        ValidBlock[ Unit ] = 0;
        break;
    case 0x06:
        DataReg = Data;
        break;
  }
}
