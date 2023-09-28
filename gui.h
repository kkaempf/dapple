#include <allegro.h>
#define PROGTITLE "Dapple"
#define fdialog(a,b,c) file_select_ex(a,b,c,1024,0,0)
int yesno (char *message)
{
 int x;

 x=alert (PROGTITLE,message,0,"&Yes","&No",'y','n');
 return x;
}

void ynexit()
{
 if (yesno("Really quit " PROGTITLE "?")==1) exit(0);
}

void mbox (char *message)
{
 alert (PROGTITLE,message,0,"O&K",0,'k',0);
}

void about();
int softreset();
int hardreset();
int fullreset();
int loadstate();
int savestate();
int seld1();
int seld2();
int loadbaszor();
int savebaszor();
int xbloadzor();

int vga2alleg(unsigned char c);
void virtplot(int x,int y,unsigned char c);

MENU FILE_MENU[] =  {
//{ "title", function, submenu, 0, 0 } 
{ "&Load State", loadstate,0,0,0},
{ "&Save State",savestate,0,0,0},
{ "",0,0,0,0},
{ "L&oad Basic...",loadbaszor,0,0,0},
{ "S&ave Basic...",savebaszor,0,0,0},
{ "Loa&d Program...",xbloadzor,0,0,0},
{ "Sav&e Program...",0,0,D_DISABLED,0},
{ "",0,0,0,0},
{ "E&xit",(void*)ynexit,0,0,0},
{ 0,0,0,0,0}
};
MENU DRIVES_MENU[] = {
{"Disk 1...",seld1,0,0,0},
{"Disk 2...",seld2,0,0,0},
{0,0,0,0,0}
};
MENU CPU_MENU[] = {
{"Basic 6502 Clone",0,0,D_DISABLED,0},
{"Synertek 6502",0,0,D_DISABLED,0},
{"WDC 65C02",0,0,D_DISABLED,0},
{"Rockwell 65C02",0,0,D_DISABLED,0},
{"Speed...",0,0,D_DISABLED,0},
{0,0,0,0,0}
};
MENU RAM_MENU[] = {
{"Language Card (][ and ][+ only)",0,0,D_DISABLED,0},
{"RAM Works/RAM Factor (//e and //c only)",0,0,D_DISABLED,0},
{0,0,0,0,0}
};
MENU ROM_MENU[] = {
{"Apple ][ (Monitor)",0,0,D_DISABLED,0},
{"Apple ][ (Revised)",0,0,D_DISABLED,0},
{"Apple ][+",0,0,D_DISABLED,0},
{"Apple ][+ (Dosius Plusrom)",0,0,D_DISABLED,0},
{"Franklin Ace",0,0,D_DISABLED,0},
{"Viking",0,0,D_DISABLED,0},
{"Apple //e (1982 version)",0,0,D_DISABLED,0},
{"Apple //e (1985 version)",0,0,D_DISABLED,0},
{"Apple //c (16K ROM)",0,0,D_DISABLED,0},
{"Apple //c (32K ROM)",0,0,D_DISABLED,0},
{0,0,0,0,0}
};
MENU DISPLAY_MENU[] = {
{"Mono",0,0,D_DISABLED,0},
{"Color",0,0,D_DISABLED,0},
{"Blurry",0,0,D_DISABLED,0},
{0,0,0,0,0}
};
MENU OPTIONS_MENU[] = {
{"Drives",0,DRIVES_MENU,0,0},
{"CPU",0,CPU_MENU,D_DISABLED,0},
{"RAM",0,RAM_MENU,D_DISABLED,0},
{"ROM",0,ROM_MENU,D_DISABLED,0},
{"Sound",0,0,D_DISABLED,0},
{"Display",0,DISPLAY_MENU,D_DISABLED,0},
{0,0,0,0,0}
};
MENU DEBUG_MENU[] = {
{"Set Execution Address...",0,0,0,0},
{0,0,0,0,0}
};
MENU RESET_MENU[] = {
{"Soft Reset",softreset,0,0,0},
{"Hard Reset",hardreset,0,0,0},
{"Full Reset",fullreset,0,0,0},
{0,0,0,0,0}
};
MENU MAIN_MENU[] = {
{"&File",0,FILE_MENU,0,0 },
{"&Options",0,OPTIONS_MENU,0,0},
{"&Debug",0,DEBUG_MENU,D_DISABLED,0},
{"&Reset",0,RESET_MENU,0,0},
{"&About",(void*)about,0,0,0},
{ 0,0,0,0,0}
};
DIALOG dialog[] = {
{ d_menu_proc,  0,   0,    0,    0,   0,  0,    0,      0,       0,   0,   MAIN_MENU,               NULL, NULL  },
{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0}   
};
void about()
{
 alert (PROGTITLE " - Apple ][ Emulator",
        "Version " DappleVersion,
        "Copyright (C) 2002, 2007 Steve Nickolas and Retro Emulation Project",
        "O&K",0,'k',0);
  do_dialog(dialog,0);
}
