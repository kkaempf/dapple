/*
 * Dapple ][ Emulator  Version 0.02
 * Copyright (C) 2002, 2003 Dapple ][ Development.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// joystick.c 2002.1211 dosius
//#include <conio.h>
//#include <dos.h>
#include <stdio.h>
#include <allegro.h>
//#include <go32.h>
//#include <sys/farptr.h>

#ifdef USE_M6502
#include "m6502.h"
#else
typedef unsigned char byte;
typedef unsigned short int word;
#endif

unsigned int GameMinX = 26, GameMinY = 26;
unsigned int GameMaxX = 27, GameMaxY = 27;
unsigned long ClockX, ClockY;                /* Timeouts for 556 timer */
unsigned int LeftAltDown = 0, RightAltDown = 0;

int joya(void)
{
if(key[KEY_ALT]) return 255;
if(joy[0].button[0].b) return 255;
return 0; 
}
int joyb(void)
{
if(key[KEY_ALTGR]) return 255;
if(joy[0].button[1].b) return 255;
return 0;
}

/////////////////////////////////////////////////////////////////////////////

#ifndef USE_M6502
#define ClockTick cycle
#endif

extern unsigned long ClockTick;

#pragma argsused
byte ResetGameTimer( word Address )
{
    unsigned int ReadX, ReadY;

    /* Get the current position of the game controller */
    if (joy[0].num_sticks == 0) goto nojoystick;
    ReadX=joy[0].stick[0].axis[0].pos; ReadY=joy[0].stick[0].axis[1].pos;

    /* Update maximum ranges */
    if ( ReadX < GameMinX )
    {
        GameMinX = ReadX;
    }
    if ( ReadY < GameMinY )
    {
        GameMinY = ReadY;
    }
    if ( ReadX > GameMaxX )
    {
        GameMaxX = ReadX;
    }
    if ( ReadY > GameMaxY )
    {
        GameMaxY = ReadY;
    }
    /* Convert PC readings to Apple */
    ReadX = ( ReadX - GameMinX ) * 256 / ( GameMaxX - GameMinX );
    ReadY = ( ReadY - GameMinY ) * 256 / ( GameMaxY - GameMinY );
    /* Calculate CPU clock tick at timeout */
    ClockX = ClockTick +7 + 11 * ReadX;
    ClockY = ClockTick +7 + 11 * ReadY;

nojoystick:
    return 0;
}

byte ReadGameTimer( word Address )
{
  if ( Address == 0x64 )
  {
      return ( ClockTick > ClockX ) ? 0 : 255;  /* Game 0 has timed out */
  }
  if ( Address == 0x65 )
  {
      return ( ClockTick > ClockY ) ? 0 : 255;  /* Game 1 has timed out */
  }
  return 0;
}

