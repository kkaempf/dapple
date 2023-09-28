/* cpu65c02.c */
extern void cpuinit();
extern void cpureset();
extern void cpurun();
extern void cpusetpc(unsigned short address);
extern unsigned short cpugetpc();
extern unsigned short rasterline;
