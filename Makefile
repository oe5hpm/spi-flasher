# SPDX-License-Identifier: GPL-2.0+
#
# Makefile for generating libs and test environemnt
#
# Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
#
CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

GIT_VERSION := $(shell (git describe --abbrev=4 --dirty --always --tags || \
			echo "?????") | sed s/-dirty/D/)

OBJS=hpmflash.o
TARGET=hpmflash
CFLAGS=-Wunused -I. -DGITVERSION=\"$(GIT_VERSION)\"
LFLAGS=-ldl
LIBS=libM25Pxx_flash.a libftdi.a libaltusb.a libhpmusb.a m25pxx_usbdev.a
SOURCES=$(shell ls *.h *.c)

ifeq ($(CROSS_COMPILE),x86_64-w64-mingw32-)
	CFLAGS += $(shell echo $(CROSS_COMPILE) | \
		    grep -c "\-w32">/dev/null && echo -D_WIN32_)
	LFLAGS :=
	TARGET := $(TARGET)64.exe
endif

all: $(TARGET)

m25pxx_usbdev.dll: libM25Pxx_flash.a libftdi.a libaltusb.a libhpmusb.a m25pxx_usbdev.o
	@echo [createDLL] $@
	@$(CC) -shared -o $@ $<

%.a: %.o
	@echo [ gen-lib ] $@
	@$(AR) rcs $@ $<

%.o: %.c
	@echo [compiling] $@
	@$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(LIBS) $(OBJS)
	@echo [ linking ] $@
	@$(CC) -o $@ $(OBJS) $(LIBS) $(LFLAGS)
	@echo [ .strip. ] $@
	@$(CROSS_COMPILE)strip $@

check:
	$(foreach f,$(SOURCES),scripts/check.sh $(f);)

clean:
	@rm -f $(LIBS) *.o $(TARGET) *.exe
