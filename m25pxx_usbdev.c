// SPDX-License-Identifier: GPL-2.0+
/*
 * test environment for spi, ftdi, flash library
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <libaltusb.h>
#include <libhpmusb.h>
#include <libftdi.h>
#include <spihw.h>
#include <libM25Pxx_flash.h>

#ifndef GITVERSION
#define GITVERSION "not a git build"
#endif

#ifdef __MINGW32__
#define DLLEXPORT	__declspec(dllexport)
#else
#define DLLEXPORT
#endif

#define STDERR(...) fprintf(stderr, __VA_ARGS__)

void DLLEXPORT m25pxx_usbdev_destroy(struct m25pxxflash_t *flash)
{
	if (flash == NULL)
		return;

	flash->spi->ops->release(flash->spi);
	hpmusb_destroy(flash->spi);
	m25pxxflash_destroy(flash);
}

struct m25pxxflash_t DLLEXPORT *m25pxx_usbdev_create(char *devname)
{
	unsigned int i;
	/* FTDI stuff */
	struct ftdi_funcptr_t *ftdifunc = NULL;
	DWORD numdevs;
	FT_STATUS rc;
	FT_DEVICE_LIST_INFO_NODE *devinfo_base, *devinfo;

	/* SPI hardware */
	int devidx = -1;
	struct spihw_t *spihw = NULL;

	/* flash programming */
	struct m25pxxflash_t *flash = NULL;

	/* create FTDI instance */
	ftdifunc = ftdi_create();
	if (ftdifunc == NULL) {
		fprintf(stderr, "ftdi_create() failed!\n");
		return NULL;
	}

	/* check for available FTDI devices */
	rc = ftdifunc->createdevlist(&numdevs);
	if (rc != FT_OK) {
		fprintf(stderr, "cannot create ftdi devlist!\n");
		goto out;
	}
	if (numdevs == 0) {
		fprintf(stderr, "no FTDI devices available!\n");
		goto out;
	}

	devinfo_base = calloc(numdevs, sizeof(*devinfo));
	ftdifunc->get_devinfo(devinfo_base, &numdevs);
	devinfo = devinfo_base;

	for (i = 0; i < numdevs; i++) {
		if (strcmp(devinfo->Description, devname) == 0) {
			devidx = i;
			break;
		}
		devinfo++;
	}
	free(devinfo_base);

	if (devidx == -1) {
		STDERR(
		       "interface '%s' not in FTDI devicelist, not present? permissions?\n",
		       devname);
		goto out;
	}

	/* create the spi interface */
	if (strcmp(devname, "USB-Blaster") == 0) {
		spihw = altusb_create(devidx);
	} else if (strcmp(devname, "Quad RS232-HS A") == 0) {
		spihw = hpmusb_create(devidx);
	} else {
		STDERR("invalid devicename '%s' !\n", devname);
		goto out;
	}
	if (spihw == NULL) {
		fprintf(stderr, "cannot create spi hardware instance!\n");
		goto out;
	}

	/* create the flash handler instance */
	flash = m25pxxflash_create(spihw);
	if (flash == NULL) {
		fprintf(stderr, "cannot create M25Pxx flash instance!\n");
		goto out;
	}
	return flash;
out:
	if (flash != NULL)
		m25pxxflash_destroy(flash);

	if (spihw != NULL) {
		spihw->ops->release(spihw);

		if (strcmp(devname, "USB-Blaster") == 0)
			altusb_destroy(spihw);
		else if (strcmp(devname, "Quad RS232-HS A") == 0)
			hpmusb_destroy(spihw);
	}

	if (ftdifunc != NULL)
		ftdi_destroy(ftdifunc);

	return NULL;
}
