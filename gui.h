#define fdialog(a,b,c) file_select_ex(a,b,c,1024,0,0)
int yesno (char *message);
void ynexit();
void mbox (char *message);
int vga2alleg(unsigned char c);
void virtplot(int x,int y,unsigned char c);

#include <allegro.h>

extern DIALOG dialog[];

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

