#!/bin/sh
gcc -o dap altbanks.c altkey.c binmanip.c clock.c cpu65c02.c dapple.c debug.c disk.c i18n.c \
           ini.c joystick.c lldisk.c massstor.c membanks.c pic.c prtsc.c video.c \
           `allegro-config --cflags --libs`
