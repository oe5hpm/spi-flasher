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
#include <sys/time.h>
#include <libaltusb.h>
#include <libftdi.h>
#include <spihw.h>
#include <libM25Pxx_flash.h>

uint64_t GetTimeStamp(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
}

void flash_progress(void *arg, unsigned int percent)
{
	if (percent == 0)
		printf("[");
	else if (percent == 100)
		printf("*]\n");
	else
		printf("*");

	fflush(stdout);
}

struct m25pxx_progress_t progprogress = {
	.fct = flash_progress,
	.arg = 0,
	.intervall = 0x8000
};

void flash_progress_erase(void *arg, unsigned int percent)
{
	printf("%d\n", percent);
	fflush(stdout);
}

struct m25pxx_progress_t eraserogress = {
	.fct = flash_progress_erase,
	.arg = 0,
	.intervall = 0
};

int main(int argc, char **argv)
{
	unsigned int i;
	struct ftdi_funcptr_t *ftdifunc;
	struct spihw_t *altusb;
	struct m25pxxflash_t *flash;
	DWORD libversion;
	DWORD numdevs;
	FT_STATUS rc;
	FT_DEVICE_LIST_INFO_NODE *devinfo_base, *devinfo;
	uint8_t *bufout;
	uint64_t ts_start, ts_end;

	bufout = calloc(1, 16 * 1024 * 1024);

	if (bufout == NULL) {
		printf("no mem.\n");
		return -1;
	}

	ftdifunc = ftdi_create();
	if (ftdifunc == NULL) {
		fprintf(stderr,
			"ftdi_create() failed!\n");

		return -1;
	}

	rc = ftdifunc->get_libversion(&libversion);
	printf("libversion (rc=%d) is 0x%x\n", rc, libversion);

	/* add the altera usb-blaster to ftdi device list */
	rc = ftdifunc->set_vidpid(0x09fb, 0x6001);
	if (rc != FT_OK) {
		printf("cannot add 0x09fb:6001 to ftdi devs!\n");

		return -1;
	}

	rc = ftdifunc->createdevlist(&numdevs);
	if (rc != FT_OK) {
		fprintf(stderr, "cannot create ftdi devlist!\n");

		return -1;
	}

	printf("there are %d (native) ftdi devices.\n", numdevs);
	devinfo_base = calloc(numdevs, sizeof(*devinfo));
	ftdifunc->get_devinfo(devinfo_base, &numdevs);
	devinfo = devinfo_base;

	for (i = 0; i < numdevs; i++) {
		printf("-----------------------------------------------\n");
		printf("dev           : %d\n", i);
		printf("flags         : 0x%08x\n", devinfo->Flags);
		printf("type          : 0x%08x\n", devinfo->Type);
		printf("description   : %s\n", devinfo->Description);
		printf("serial        : %s\n", devinfo->SerialNumber);
		printf("ID            : %x\n", devinfo->ID);
		printf("LocID         : %x\n", devinfo->LocId);

		devinfo++;
	}
	free(devinfo_base);

	altusb = altusb_create(0);
	if (altusb == NULL) {
		fprintf(stderr, "cannot create usb-blaster instance!\n");

		return -1;
	}

	flash = m25pxxflash_create(altusb);
	if (flash == NULL) {
		fprintf(stderr, "cannot create M25Pxx flash instance!\n");

		return -1;
	}
#if 1
	rc = m25pxx_detect(flash, 0);
	if (rc == 0) {
		printf("M25Pxx detect ok (%s).\n",
		       flash->flash_detected->name);
		m25pxx_printflash(flash->flash_detected);
	} else {
		printf("M25Pxx detect failed.\n");
	}
#endif
#if 1
	printf("> testing read status ...\n");
	ts_start = GetTimeStamp();
	rc = m25pxx_rdsr(flash, bufout);
	ts_end = GetTimeStamp();
	printf("read flash time: %zu (status = 0x%02x).\n",
	      ts_end - ts_start, bufout[0]);
#endif

#if 1
	printf("> testing read flash ...\n");
	ts_start = GetTimeStamp();
	rc = m25pxx_read(flash, bufout, 0, 512 * 1024);
	ts_end = GetTimeStamp();
	printf("read flash time: %zu\n", ts_end - ts_start);
#endif
#if 1
	printf("> testing sector erase ...\n");
	ts_start = GetTimeStamp();
	rc = m25pxx_sectorerase(flash, 0,  &progprogress);
	ts_end = GetTimeStamp();
	printf("sector erase time: %zu\n", ts_end - ts_start);
#endif
#if 1
	printf("> testing flash program ...\n");
	memset(bufout, 0x00, 8 * 1024 * 1024);
	for (i = 1; i < 1024; i++)
		bufout[i] = i;

	ts_start = GetTimeStamp();
	rc = m25pxx_program(flash, bufout, 0, 1 * 1024 * 1024, &progprogress);
	ts_end = GetTimeStamp();
	printf("page program time: %zu\n", ts_end - ts_start);
#endif

#if 1
	printf("> testing chip erase ...\n");
	ts_start = GetTimeStamp();
	rc = m25pxx_chiperase(flash, &progprogress);
	ts_end = GetTimeStamp();
	printf("chip erase time: %zu\n", ts_end - ts_start);
#endif

	free(bufout);
	ftdi_destroy(ftdifunc);
	m25pxxflash_destroy(flash);
	altusb_destroy(altusb);

	return 0;
}
