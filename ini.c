/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  INI: DAPPLE.INI file handling
 * Revision:   (1.23) 2003.0104
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
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "dapple.h"

extern void virtsetmonochrome(unsigned int mode);
extern unsigned char virtgetmonochrome();
extern void virtsetpalette(unsigned char mode);
extern unsigned char virtgetpalette();

extern int sounddelay, smode,
           dqhdv,
           tweakpic,
           hold,
           dqshift;

extern int joyenabled;
extern int shiftshutoff;
extern int nof8;
extern int tweakbksp;

extern unsigned char memnolc;

extern unsigned int GameMinX, GameMinY, GameMaxX, GameMaxY;

extern char parallel[128], rompath[128];

void wrini(void)
{
 char inipath[PATH_MAX];
 FILE *file;
 unsigned char vpal;

 vpal=virtgetpalette();

 sprintf(inipath, "%s/.dapple", getenv("HOME"));
 mkdir(inipath, 0755);
 strcat(inipath, "/dapple.ini");
 file=fopen(inipath,"wt");
 if (!file) return;
 fprintf(file,"[Dapple]\n");
 fprintf(file,"Sound=%s\n",smode?"Yes":"No");
 fprintf(file,"HardDisk=%s\n",dqhdv?"No":"Yes");
 fprintf(file,"Video=%s\n",virtgetmonochrome()==0?"Color":
                           virtgetmonochrome()==1?"Mono":
                           virtgetmonochrome()==2?"Green":
                                   "Amber");

 fprintf(file,"LanguageCard=%s\n",memnolc?"No":"Yes");
 fprintf(file,"HighParallel=%s\n",tweakpic?"Yes":"No");
 fprintf(file,"Charset=%s\n",charmode==USA     ?"USA":
                             charmode==France  ?"France":
                             charmode==Germany ?"Germany":
                             charmode==UK      ?"UK":
                             charmode==Denmark1?"Denmark1":
                             charmode==Sweden  ?"Sweden":
                             charmode==Italy   ?"Italy":
                             charmode==Spain   ?"Spain":
                             charmode==Japan   ?"Japan":
                             charmode==Norway  ?"Norway":
                             charmode==Denmark2?"Denmark2":"???");
 fprintf(file,"ParallelOutput=%s\n",parallel);
 fprintf(file,"DefaultBios=%s\n",rompath);
 fprintf(file,"Delay=$%04X\n",hold);
 fprintf(file,"ShiftMod=%s\n",dqshift?"No":"Yes");
 fprintf(file,"ShiftManip=%s\n",shiftshutoff?"No":"Yes");
 fprintf(file,"Joystick=%s\n",joyenabled?"Yes":"No");

/*
 fprintf(file,"JoyCalibrateMinX=%u\n",GameMinX);
 fprintf(file,"JoyCalibrateMinY=%u\n",GameMinY);
 fprintf(file,"JoyCalibrateMaxX=%u\n",GameMaxX);
 fprintf(file,"JoyCalibrateMaxY=%u\n",GameMaxY);
 */

 fprintf(file,"Palette=%s\n", vpal==0?"IIgs":
                              vpal==1?"Munafo":
                              vpal==2?"ApplePC":
                              vpal==3?"MESS":
                                      "EGA");

 fprintf(file,"PrtScF8=%s\n", nof8?"No":"Yes");
 fprintf(file,"TweakBksp=%s\n", tweakbksp?"Yes":"No");

 fclose(file);
}

void rdini(void)
{
 char inipath[PATH_MAX];
 FILE *file;
 unsigned char mono;

 sprintf(inipath, "%s/.dapple/dapple.ini", getenv("HOME"));
 file=fopen(inipath,"rt");
 if (!file) return;
 while (!feof(file))
 {
  char wip[128];

  fgets(wip,128,file);
  wip[strlen(wip)-1]=0;
  if (!strcasecmp(wip,"Sound=Yes")) smode=1;
  if (!strcasecmp(wip,"Sound=No")) smode=0;
  if (!strcasecmp(wip,"HardDisk=No")) dqhdv=1;
  if (!strcasecmp(wip,"HardDisk=Yes")) dqhdv=0;

  if (!strcasecmp(wip,"Video=Color")) mono=0;
  if (!strcasecmp(wip,"Video=Mono"))  mono=1;
  if (!strcasecmp(wip,"Video=Green")) mono=2;
  if (!strcasecmp(wip,"Video=Amber")) mono=3;

  if (!strcasecmp(wip,"Console=Color")) mono=0;
  if (!strcasecmp(wip,"Console=Amber")) mono=3;

  if (!strcasecmp(wip,"LanguageCard=Yes")) memnolc=0;
  if (!strcasecmp(wip,"LanguageCard=No")) memnolc=1;
  if (!strcasecmp(wip,"HighParallel=Yes")) tweakpic=1;
  if (!strcasecmp(wip,"HighParallel=No")) tweakpic=0;
  if (!strcasecmp(wip,"Charset=USA")) charmode=USA;
  if (!strcasecmp(wip,"Charset=France")) charmode=France;
  if (!strcasecmp(wip,"Charset=Germany")) charmode=Germany;
  if (!strcasecmp(wip,"Charset=UK")) charmode=UK;
  if (!strcasecmp(wip,"Charset=Denmark1")) charmode=Denmark1;
  if (!strcasecmp(wip,"Charset=Sweden")) charmode=Sweden;
  if (!strcasecmp(wip,"Charset=Italy")) charmode=Italy;
  if (!strcasecmp(wip,"Charset=Spain")) charmode=Spain;
  if (!strcasecmp(wip,"Charset=Japan")) charmode=Japan;
  if (!strcasecmp(wip,"Charset=Norway")) charmode=Norway;
  if (!strcasecmp(wip,"Charset=Denmark2")) charmode=Denmark2;
  if (!strncasecmp(wip,"ParallelOutput=",15)) strcpy(parallel,&wip[15]);
  if (!strncasecmp(wip,"DefaultBios=",12)) strcpy(rompath,&wip[12]);
  if (!strncasecmp(wip,"Delay=$",7)) sscanf(wip,"Delay=$%x",&hold);
  if (!strcasecmp(wip,"ShiftMod=No")) dqshift=1;
  if (!strcasecmp(wip,"ShiftMod=Yes")) dqshift=0;
  if (!strcasecmp(wip,"Palette=IIgs")) virtsetpalette(0);
  if (!strcasecmp(wip,"Palette=Munafo")) virtsetpalette(1);
  if (!strcasecmp(wip,"Palette=ApplePC")) virtsetpalette(2);
  if (!strcasecmp(wip,"Palette=MESS")) virtsetpalette(3);
  if (!strcasecmp(wip,"Palette=EGA")) virtsetpalette(4);
  if (!strcasecmp(wip,"ShiftManip=Yes")) setshiftmanip(0);
  if (!strcasecmp(wip,"ShiftManip=No")) setshiftmanip(1);
  if (!strcasecmp(wip,"Joystick=Yes")) joyenabled=1;
  if (!strcasecmp(wip,"Joystick=No")) joyenabled=0;
  if (!strcasecmp(wip,"PrtScF8=No")) nof8=1;
  if (!strcasecmp(wip,"PrtScF8=Yes")) nof8=0;
  if (!strcasecmp(wip,"TweakBksp=Yes")) tweakbksp=1;
  if (!strcasecmp(wip,"TweakBksp=No")) tweakbksp=0;

/*
  if (!strncasecmp(wip,"JoyCalibrateMinX=",17))
    sscanf(wip,"JoyCalibrateMinX=%u",&GameMinX);
  if (!strncasecmp(wip,"JoyCalibrateMinY=",17))
    sscanf(wip,"JoyCalibrateMinY=%u",&GameMinY);
  if (!strncasecmp(wip,"JoyCalibrateMaxX=",17))
    sscanf(wip,"JoyCalibrateMaxX=%u",&GameMaxX);
  if (!strncasecmp(wip,"JoyCalibrateMaxY=",17))
    sscanf(wip,"JoyCalibrateMaxY=%u",&GameMaxY);
 */
 }
 fclose(file);
 virtsetmonochrome(mono);
}
