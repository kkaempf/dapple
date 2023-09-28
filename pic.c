/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  PIC: Parallel interface
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

#include <stdio.h>
#include <string.h>
#include <allegro.h>
#define COL_DRV_OFF  0x24
#define COL_DRV_ON   0x25

extern unsigned char virtscreen[128000];
#define virtplot(x,y,c) virtscreen[(y*640)+x]=c
//#define virtplot(x,y,c) putpixel(screen,x,y,c)
extern void virtscreencopy();
extern unsigned char virtcopy;

int tweakpic=0;

#ifdef DEFPAR
char parallel[128] = {DEFPAR};
#else
char parallel[128] = {"dapple.prn"};
#endif

#pragma argsused
void WriteParallel( unsigned int Address, unsigned char Data )
{
    FILE *file;

    switch ( Address & 0x0f )
    {
    case 0x00:
        virtplot(636,199,COL_DRV_ON);
        virtcopy = 1;
        virtscreencopy();
        /* load output port */
       /* if(!stricmp(parallel,"prn"))
        {
         fputc(tweakpic?Data:Data&0x7F,stdprn);
         fflush(stdprn);
         virtplot(636,199,0);
         virtcopy = 1;
         break;
        }
	*/
        file=fopen(parallel,"ab");
        if (file)
        {
         fputc(tweakpic?Data:Data&0x7F,file);
         fclose (file);
        }
        virtplot(636,199,0);
        virtcopy = 1;
    }
}
