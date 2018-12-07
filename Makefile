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
LIBS=libM25Pxx_flash.a libftdi-linux.a libaltusb.a libhpmusb.a
SOURCES=$(shell ls *.h *.c)

all: $(TARGET)

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
	@rm -f $(LIBS) *.o $(TARGET)
