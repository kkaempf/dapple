/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  PRTSC:  Print Dapple screen to Epson FX-80/RX-80 printer
 *             * Test code *
 *
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

unsigned int point (unsigned int x, unsigned int y);

extern unsigned char virtscreen[128000];
#define virtgetpixel(x,y) virtscreen[(y*640)+x]

void prtsc (void)
{
/* int i,j;

 fprintf (stdprn,"%cA%c",27,2);
 for (i=0;i<200;i++)
 {
  fprintf (stdprn,"%c*%c%c%c",27,4,128,2);
  for (j=0; j<319; j++)
  {
   if (virtgetpixel(j,i))
     fprintf (stdprn,"%c%c",0,0);
    else
     fprintf (stdprn,"%c%c",192,192);
  }
 }
 fprintf (stdprn,"%c%c2",0,27);
 */
}
