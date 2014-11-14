###############################################################
#
# Purpose: Makefile for "UVC Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.0
# License: GPL (inherited from luvcview)
#
###############################################################

PATH	:= $(PATH):/home/akrapivniy/ml3020/OpenWrt-Toolchain-ar71xx-for-mips_34kc-gcc-4.8-linaro_uClibc-0.9.33.2/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin

PREFIX   = mips-openwrt-linux-uclibc-

CC = mips-openwrt-linux-uclibc-gcc
LD = mips-openwrt-linux-uclibc-ld
APP_BINARY=uvc_stream


CFLAGS += -O2 -Wall
LFLAGS += -lpthread -ldl -lturbojpeg


OBJECTS=uvc_stream.o color.o utils.o v4l2uvc.o control.o fire.o vision.o

all: uga_buga

clean:
	@echo "Cleaning up directory."
	rm -f *.a *.o $(APP_BINARY) core *~ log errlog *.avi

# Applications:
uga_buga: $(OBJECTS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJECTS) -o $(APP_BINARY)
	chmod 755 $(APP_BINARY)

# useful to make a backup "make tgz"
tgz: clean
	mkdir -p backups
	tar czvf ./backups/uvc_streamer_`date +"%Y_%m_%d_%H.%M.%S"`.tgz --exclude backups *
