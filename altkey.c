/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  ALTKEY:  Alternate keyboard handler
 * Revision:   (1.24) 2003.0105
 * Some code was taken from Andrew Gregory's emulator
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

//#include <dos.h>
#include <allegro.h>
#include "cpu65c02.h"
#include "dapple.h"
#include "debug.h"
#include "disk.h"
#include "prtsc.h"
#include "video.h"

extern int k;

extern enum {emu6502, emuZ80} cpuinuse;

extern unsigned char keycapslock;
extern unsigned char keyswitchyz;
extern unsigned char exitprogram;

extern unsigned int LeftAltDown, RightAltDown;

int tweakbksp=0;

int nof8=1;
void uimain();
void PC2APL (void)
{
    unsigned int Key = 0;

    /* Get next key if any */
//    _AH=0x11; /* Check for extended keystroke */
//    geninterrupt(0x16);
//    if (_FLAGS&0x40) return; /* "jz" */
//    Key=_AX;
//    _AH=0x10;
//    geninterrupt(0x16);


    /* There is a keystroke */
    /* Handle special function keys */
    poll_joystick();
    poll_keyboard();
    if(keypressed()){
    Key = readkey();
    if(!Key) return;

    if (Key==0x0300) /* ^@ */
    {
     k=128;
     return;
    }

    if(Key>>8==KEY_F3)//0x3C00)
    {
     virtsethresmode((virtgethresmode() + 1) & 0x1);
     return;
    }

    if(Key>>8==KEY_F4)//0x3D00)
    {
     virtsetmonochrome((virtgetmonochrome() + 1) & 0x3);
     return;
    }

    if (Key==0x4200)
    {
     if (!nof8) prtsc();
     return;
    }

    if (Key>>8 == KEY_F9)//0x4300)
    {
     /* F9 - debugger */
     Debug6502();
    }

    if (Key>>8==KEY_F10)//0x4400)
{ uimain();} 
/*    {
       uimain();
       return;
      }

    if (Key>>8==KEY_F4 && key[KEY_ALT])//0x3E00 && (LeftAltDown || RightAltDown))
    */ /* Alt-F4 */

    if (Key>>8==KEY_F12)// == 0x8a00 || Key == 0x8C00 || Key == 0x0000)
    {
      /* Ctrl-F12, Ctrl-Break */
     k=0;
     cpuinuse=emu6502;
     DiskReset();
     cpureset();
#ifdef EMUZ80
     Z80_Reset();
#endif
     memoryreset();
     virtreset();
     return;
    }

    /* Handle key re-mappings */

    /* Alt-A..Z -> A..Z */
    if ( Key == 0x1e00 ) Key = 0x1e61;
    if ( Key == 0x3000 ) Key = 0x3062;
    if ( Key == 0x2e00 ) Key = 0x2e63;
    if ( Key == 0x2000 ) Key = 0x2064;
    if ( Key == 0x1200 ) Key = 0x1265;
    if ( Key == 0x2100 ) Key = 0x2166;
    if ( Key == 0x2200 ) Key = 0x2267;
    if ( Key == 0x2300 ) Key = 0x2368;
    if ( Key == 0x1700 ) Key = 0x1769;
    if ( Key == 0x2400 ) Key = 0x246a;
    if ( Key == 0x2500 ) Key = 0x256b;
    if ( Key == 0x2600 ) Key = 0x266c;
    if ( Key == 0x3200 ) Key = 0x326d;
    if ( Key == 0x3100 ) Key = 0x316e;
    if ( Key == 0x1800 ) Key = 0x186f;
    if ( Key == 0x1900 ) Key = 0x1970;
    if ( Key == 0x1000 ) Key = 0x1071;
    if ( Key == 0x1300 ) Key = 0x1372;
    if ( Key == 0x1f00 ) Key = 0x1f73;
    if ( Key == 0x1400 ) Key = 0x1474;
    if ( Key == 0x1600 ) Key = 0x1675;
    if ( Key == 0x2f00 ) Key = 0x2f76;
    if ( Key == 0x1100 ) Key = 0x1177;
    if ( Key == 0x2d00 ) Key = 0x2d78;
    if ( Key == 0x1500 ) Key = 0x1579;
    if ( Key == 0x2c00 ) Key = 0x2c7a;

    /* Alt-1..0 -> 1..0 */
    if ( Key == 0x7800 ) Key = 0x0231;
    if ( Key == 0x7900 ) Key = 0x0332;
    if ( Key == 0x7a00 ) Key = 0x0433;
    if ( Key == 0x7b00 ) Key = 0x0534;
    if ( Key == 0x7c00 ) Key = 0x0635;
    if ( Key == 0x7d00 ) Key = 0x0736;
    if ( Key == 0x7e00 ) Key = 0x0837;
    if ( Key == 0x7f00 ) Key = 0x0938;
    if ( Key == 0x8000 ) Key = 0x0a39;
    if ( Key == 0x8100 ) Key = 0x0b30;

    /* Alt-Misc -> Misc */
    if ( Key == 0x8200 ) Key = 0x0c2d; /* - */
    if ( Key == 0x8300 ) Key = 0x0d3d; /* = */
    if ( Key == 0x1a00 ) Key = 0x1a5b; /* [ */
    if ( Key == 0x1b00 ) Key = 0x1b5d; /* ] */
    if ( Key == 0x2700 ) Key = 0x273b; /* : */
    if ( Key == 0x2600 ) Key = 0x2b5c; /* \ */
    if ( Key == 0x0e00 ) Key = 0x0e08; /* Backspace */
    if ( Key == 0xa000 ) Key = 0x50e0; /* Down arrow */
    if ( Key == 0xa600 ) Key = 0x1c0d; /* Enter */
    if ( Key == 0x0100 ) Key = 0x011b; /* Esc */
    if ( Key == 0x3700 ) Key = 0x372a; /* Keypad * */
    if ( Key == 0x4a00 ) Key = 0x4a2d; /* Keypad - */
    if ( Key == 0x4e00 ) Key = 0x4e2b; /* Keypad + */
    if ( Key == 0xa400 ) Key = 0x352f; /* Keypad / */
    if ( Key == 0x9b00 ) Key = 0x4be0; /* Left arrow */
    if ( Key == 0x9d00 ) Key = 0x4de0; /* Right arrow */
    if ( Key == 0xa500 ) Key = 0x0f09; /* Tab */
    if ( Key == 0x9800 ) Key = 0x48e0; /* Up arrow */
    if ( Key == 0x3500 ) Key = 0x352f; /* / */

//  if ( Key == 0x0e08 ) Key = 0x4be0; /* Backspace -> Left arrow */
    {
     extern int ROM16K;

     if (tweakbksp && ROM16K) if (Key==0x0E08) Key=0x0E7F;
    }

    if ( Key>>8 == KEY_LEFT/*0x4be0*/ ) Key = 0x2308; /* Left arrow  -> Ctrl-H */
    if ( Key>>8 == KEY_RIGHT/*0x4de0*/ ) Key = 0x1615; /* Right arrow -> Ctrl-U */
    if ( Key>>8 == KEY_UP/*0x48e0*/ ) Key = 0x250b; /* Up arrow    -> Ctrl-K */
    if ( Key>>8 == KEY_DOWN/*0x50e0*/ ) Key = 0x240a; /* Down arrow  -> Ctrl-J */

    if ( Key == 0x53e0 ) Key = 0x007f; /* Delete -> Apple Del */

    /* Process key */

    Key &= 0xff;                    /* Get ASCII code */

    if (Key>=0&&Key<=127)
    {
     if (keyswitchyz)
     {
      switch (Key)
      {
       case 'y' : Key = 'z'; break;
       case 'Y' : Key = 'Z'; break;
       case 'z' : Key = 'y'; break;
       case 'Z' : Key = 'Y'; break;
      }
     }
     if (keycapslock) /* Flip the CAPS LOCK setting */
     {
      if ((Key >= 'a') && (Key <= 'z'))
      {
       Key -= 32;
      }
      else
      {
       if ((Key >= 'A') && (Key <= 'Z'))
       {
        Key += 32;
       }
      }
     }
    }

    if ( Key < 1 || Key > 127 )
    {
        return;   /* Don't consider out-of-range chars */
    }
    k = Key | 0x80;                    /* Set the strobe */
}
}
