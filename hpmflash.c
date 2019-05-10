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
#include <libaltusb.h>
#include <libhpmusb.h>
#include <libftdi.h>
#include <spihw.h>
#include <libM25Pxx_flash.h>

#include "osi.h"

#ifndef GITVERSION
#define GITVERSION "not a git build"
#endif

#define STDERR(...) fprintf(stderr, __VA_ARGS__)

#ifdef __linux__
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#else
#define ANSI_COLOR_RED     ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_COLOR_RESET   ""
#endif

void flash_progress(void *arg, unsigned int percent, unsigned int flag)
{
	char *str = (char *)arg;

	if (percent == 0)
		printf("[");
	else if (percent == 100)
		printf("*"ANSI_COLOR_RESET"] %s\n", str != NULL ? str : "");
	else if (flag == 0)
		printf(ANSI_COLOR_GREEN"*");
	else
		printf(ANSI_COLOR_YELLOW"*");
	fflush(stdout);
}

struct m25pxx_progress_t progprogress = {
	.fct = flash_progress,
	.arg = 0,
};

int main(int argc, char **argv)
{
	unsigned int i;
	int ret = 0;
	bool debug = false;

	/* FTDI stuff */
	struct ftdi_funcptr_t *ftdifunc = NULL;
	DWORD numdevs = 0;
	FT_STATUS rc;
	FT_DEVICE_LIST_INFO_NODE *devinfo_base, *devinfo;

	/* SPI hardware */
	int devidx = -1;
	char *devname = NULL;
	struct spihw_t *spihw = NULL;
	unsigned int speed = 16000000;
	unsigned int cs = 0;

	/* flash programming */
	struct m25pxxflash_t *flash = NULL;
	struct flashparam_t *chip;

	FILE *f;
	char *filename = NULL;
	size_t filesize;

	char txtbuf[64] = { };
	uint64_t ts_start, ts_end, t;
	float tdisp;

	uint32_t offset = 0, size = 0;
	uint8_t *buf = NULL;

	bool detectonly = false;
	bool erase = false;
	bool read = false;
	bool write = false;

	/* command line processing */
	char *end = NULL;
	int argrun;

	for (argrun = 1; argrun;) {
		switch (getopt(argc, argv, ":f:i:w:r:o:s:c:dexhv")) {
		case 'o':
			offset = strtod(optarg, &end);
			break;
		case 's':
			size = strtod(optarg, &end);
			break;
		case 'c':
			cs = strtod(optarg, &end);
			break;
		case 'd':
			detectonly = true;
			break;
		case 'w':
			if (optarg == NULL || strlen(optarg) < 1) {
				STDERR("invalid filename in -w argument!\n");
				return -1;
			}
			filename = strdup(optarg);
			write = true;
			break;
		case 'r':
			if (optarg == NULL || strlen(optarg) < 1) {
				STDERR("invalid filename in -r argument!\n");
				return -1;
			}
			filename = strdup(optarg);
			read = true;
			break;
		case 'e':
			erase = true;
			break;
		case 'i':
			if (optarg == NULL || strlen(optarg) < 1) {
				STDERR("invalid filename in -i argument!\n");
				return -1;
			}
			if (strcmp(optarg, "altusb") == 0) {
				devname = strdup("USB-Blaster");
			} else if (strcmp(optarg, "hpmusb") == 0) {
				devname = strdup("Quad RS232-HS A");
			} else {
				STDERR(
				"unknown interface '%s' !\n"
				"known interaces are:\n"
				"altusb - Altera USB-Blaster (old PX-blaster)\n"
				"hpmusb - FT4232 based hpm-blaster\n", optarg);
				return -1;
			}
			break;
		case 'f':
			if (optarg == NULL || strlen(optarg) < 1) {
				STDERR("invalid speed in -f argument!\n");
				STDERR("-f <speed_in_hz>\n");
				return -1;
			}
			speed = strtod(optarg, &end);
			break;
		case 'h':
			printf("hpmflash version '%s', commandlist:\n"
			       "-i <interface> select the SPI interface\n"
			       "-c <chipsel>   number of SPI-chipselect to use\n"
			       "-o <offset>    offset within flash\n"
			       "-s <size>      amount of bytes to read/write\n"
			       "               zero size always progresses the whole chip\n"
			       "-r <file>      reads flash into file\n"
			       "-w <file>      writes file into flash\n"
			       "-e             erase before write, or just erase\n"
			       "-f <speed>     SPI speed given in Hz\n"
			       "-d             just detect flash and exit\n"
			       "-v             version\n"
			       "-x             switch debug mode on\n"
			       , GITVERSION);
			return 0;
		case 'v':
			printf("hpmflash version '%s'\n", GITVERSION);
			return 0;
		case 'x':
			debug = true;
			break;
		case '?':
			STDERR("invalid arguments!\n");
			return -1;
		case -1:
			if (optind < argc) {
				STDERR("invalid argument!\n");
				return -1;
			}
			argrun = 0;
			break;
		}
	}

	if (devname == NULL) {
		STDERR("provide at least -i <interface> argument!\n");
		return -1;
	}

	/* create FTDI instance */
	ftdifunc = ftdi_create();
	if (ftdifunc == NULL) {
		fprintf(stderr, "ftdi_create() failed!\n");
		return -1;
	}

	/* add the altera usb-blaster to ftdi device list */
#ifndef __MINGW32__
	rc = ftdifunc->set_vidpid(0x09fb, 0x6001);
	if (rc != FT_OK) {
		printf("cannot add 0x09fb:6001 to ftdi devs!\n");
		ret = -1;
		goto out;
	}
#endif
	/* check for available FTDI devices */
	rc = ftdifunc->createdevlist(&numdevs);
	if (rc != FT_OK) {
		fprintf(stderr, "cannot create ftdi devlist!\n");
		ret = -1;
		goto out;
	}
	if (numdevs == 0) {
		fprintf(stderr, "no FTDI devices available!\n");
		ret = -1;
		goto out;
	} else if (numdevs > 20) {
		fprintf(stderr,
			"nobody in this world has more than 20 FTDI-devices!\n");
		ret = -1;
		goto out;
	}

	devinfo_base = calloc(numdevs, sizeof(*devinfo));
	ftdifunc->get_devinfo(devinfo_base, &numdevs);
	devinfo = devinfo_base;

	for (i = 0; i < numdevs; i++) {
		if (debug == true) {
			printf("-------------------------------------------\n");
			if (devinfo->Flags & 0x1) {
				printf("dev %d not usable (open-flag set).\n",
				       i);
				devinfo++;
				continue;
			}
			printf("device        : %d\n", i);
			printf("flags         : 0x%08x\n", devinfo->Flags);
			printf("type          : 0x%08x\n", devinfo->Type);
			printf("description   : %s\n", devinfo->Description);
			printf("serial        : %s\n", devinfo->SerialNumber);
			printf("ID            : %x\n", devinfo->ID);
			printf("LocID         : %x\n", devinfo->LocId);
		}

		if (strcmp(devinfo->Description, devname) == 0) {
			devidx = i;
			break;
		}
		devinfo++;
	}
	free(devinfo_base);
	if (debug == true)
		printf("-----------------------------------------------\n");

	if (devidx == -1) {
		STDERR(
		       "interface '%s' not in FTDI devicelist, not present? permissions?\n",
		       devname);
		ret = -1;
		goto out;
	}

	/* create the spi interface */
	if (strcmp(devname, "USB-Blaster") == 0) {
		spihw = altusb_create(devidx);
	} else if (strcmp(devname, "Quad RS232-HS A") == 0) {
		spihw = hpmusb_create(devidx);
	} else {
		STDERR("invalid devicename '%s' !\n", devname);
		ret = -1;
		goto out;
	}
	if (spihw == NULL) {
		fprintf(stderr, "cannot create spi hardware instance!\n");
		return -1;
	}

	/* create the flash handler instance */
	flash = m25pxxflash_create(spihw);
	if (flash == NULL) {
		fprintf(stderr, "cannot create M25Pxx flash instance!\n");

		return -1;
	}

	/* flash handling */
	spihw->ops->claim(spihw);
	spihw->ops->set_speed_mode(spihw, speed, 1);

	rc = -1;
	i = 0;
	printf("try with setting nCE low / TMS high.\n");
	while (rc != 0 && i < 4) {
		rc = m25pxx_detect(flash, cs);
		if (rc == 0) {
			chip = flash->flash_detected;
			printf("-----------------------------------------------\n");
			printf("----- M25Pxx detect ok (%-16s) -----\n",
			       flash->flash_detected->name);
			m25pxx_printflash(flash->flash_detected);

			m25pxx_rdsr(flash, txtbuf);
			printf("chip-status           : 0x%02x\n", txtbuf[0]);
			printf("-----------------------------------------------\n");
			if (detectonly == true)
				goto out;
		} else if (i == 0) {
			printf("retry with setting nCE low / TMS low.\n");
			spihw->ops->set_clr_tms(spihw, false);
		} else if (i == 1) {
			printf("retry with setting nCE high / TMS low.\n");
			spihw->ops->set_clr_nce(spihw, true);
		} else if (i == 2) {
			printf("retry with setting nCE high / TMS high.\n");
			spihw->ops->set_clr_tms(spihw, true);
		}
		i++;
	}
	if (rc != 0) {
		printf("M25Pxx detect @ CS %d failed.\n", cs);
		ret = -1;
		goto out;
	}

	/* create inout buffer */
	buf = calloc(1, chip->size);
	if (buf == NULL) {
		printf("no mem for creating flash buffer.\n");
		goto out;
	}

	if (read == true) {
		if (size == 0) {
			printf("WARN: no size given, assuming whole chip.\n");
			size = chip->size;
		}
		if ((offset + size) > chip->size) {
			printf(
			       "WARN: offset (0x%x) + size (0x%x) exceeds flash size (0x%x)!\n",
			       offset, size, chip->size);
			size = chip->size - offset;
		}
		printf("> read flash from offset 0x%x with size 0x%x ...\n",
		       offset, size);

		/* read flash */
		memset(buf, 0xFF, chip->size);
		ts_start = GetTimeStamp();
		rc = m25pxx_read(flash, buf, offset, size);
		if (rc != 0) {
			STDERR("read failed!\n");
			ret = -1;
			goto out;
		}
		ts_end = GetTimeStamp();
		t = ts_end - ts_start;
		tdisp = t / 1000.0f;
		printf("read %d bytes in time: %.2f %s\n",
		       size,
		       tdisp > 1000.0 ? tdisp / 1000.0 : tdisp,
		       tdisp > 1000.0 ? "s" : "ms");

		/* strip trailing 0xFF */
		i = 0;
		while (size > 0 && buf[size - 1] == 0xFF) {
			i++;
			size--;
		}
		printf("stripped %d trailing 0xFF\n", i);

		/* write file */
		if (size > 0) {
			f = fopen(filename, "wb");
			if (f == NULL) {
				STDERR("cannot open %s for write!\n", filename);
				ret = -1;
				goto out;
			}
			printf("write %d bytes to %s.\n", size, filename);
			if (fwrite(buf, 1, size, f) != size)
				STDERR("error writing to file %s!\n", filename);
			fclose(f);
		} else {
			printf("flash is blank, writing nothing to %s.\n",
			       filename);
		}
	}

	if (erase == true) {
		uint32_t eraseoffset = 0, erasesize = 0;
		unsigned int sectors = 0;

		if (write == true) {
			f = fopen(filename, "rb");
			if (f == NULL) {
				STDERR("cannot open %s for read!\n", filename);
				ret = -1;
				goto out;
			}
			filesize = fread(buf, 1, chip->size, f);
			fclose(f);
			if (size == 0) {
				printf(
				       "WARN: zero size given, asuming filesize.\n");
				erasesize = filesize;
			} else {
				erasesize = size;
			}
		} else {
			erasesize = size;
		}
		eraseoffset = offset - (offset % chip->sectorsize);
		if ((eraseoffset + erasesize) > chip->size) {
			printf(
			       "WARN: offset (0x%x) + size (0x%x) exceeds chip size (0x%x)!\n",
			       eraseoffset, erasesize, chip->size);
			erasesize = chip->size - eraseoffset;
		}
		printf("-> erase from offset 0x%x with size %d ...\n",
				eraseoffset, erasesize);

		sectors = erasesize / chip->sectorsize;
		if (eraseoffset < chip->sectorsize &&
		    (chip->sectortime * sectors) > chip->bulktime) {
			printf(
			       "using bulk erase instead erasing %d sectors ...\n",
				sectors);
			erasesize = 0;
		}
		if (erasesize == 0) {
			printf("WARN: zero size given, asuming chiperase.\n");
			printf("> starting chip erase ...\n");
			ts_start = GetTimeStamp();
			rc = m25pxx_chiperase(flash, &progprogress);
			if (rc != 0) {
				STDERR("chip erase failed!\n");
				ret = -1;
				goto out;
			}
			ts_end = GetTimeStamp();
			t = ts_end - ts_start;
			tdisp = t / 1000.0f;
			printf("chip erase time: %.2f %s\n",
			       tdisp > 1000.0 ? tdisp / 1000.0 : tdisp,
			       tdisp > 1000.0 ? "s" : "ms");

		} else {
			ts_start = GetTimeStamp();
			while (erasesize > 0) {
				sprintf(txtbuf, "0x%x", eraseoffset);
				progprogress.arg = txtbuf;
				rc = m25pxx_sectorerase(flash, eraseoffset,
							&progprogress);
				progprogress.arg = NULL;
				if (rc != 0) {
					STDERR("sector erase failed!\n");
					ret = -1;
					goto out;
				}
				eraseoffset += chip->sectorsize;
				if (erasesize > chip->sectorsize)
					erasesize -= chip->sectorsize;
				else
					erasesize = 0;
			}
			ts_end = GetTimeStamp();
			t = ts_end - ts_start;
			tdisp = t / 1000.0f;

			printf("erase done: %.2f %s\n",
			       tdisp > 1000.0 ? tdisp / 1000.0 : tdisp,
			       tdisp > 1000.0 ? "s" : "ms");
		}
	}

	if (write == true) {
		f = fopen(filename, "rb");
		if (f == NULL) {
			STDERR("cannot open %s for read!\n", filename);
			ret = -1;
			goto out;
		}
		filesize = fread(buf, 1, chip->size, f);
		fclose(f);

		if (filesize == 0) {
			STDERR("cannot program empty file!\n");
			ret = -1;
			goto out;
		}

		printf("%s contains %ld bytes.\n", filename, filesize);
		if (size > filesize) {
			printf("WARN: size (%d) is larger than file (%ld)!\n",
			       size, filesize);
			size = filesize;
		} else if (size == 0) {
			printf("WARN: zero size given, asuming filesize.\n");
			size = filesize;
		}
		if ((offset + size) > chip->size) {
			printf(
			       "WARN: offset (0x%x) + size (0x%x) exceeds chip size (0x%x)!\n",
			       offset, size, chip->size);
			size = chip->size - offset;
		}

		printf("programming %d bytes to offset 0x%x\n", size, offset);

		ts_start = GetTimeStamp();
		rc = m25pxx_program(flash, buf, offset, size, &progprogress);
		ts_end = GetTimeStamp();
		if (rc != 0) {
			STDERR("flash write failed!\n");
			ret = -1;
			goto out;
		}
		t = ts_end - ts_start;
		tdisp = t / 1000.0f;

		printf("flash program done: %.2f %s\n",
		       tdisp > 1000.0 ? tdisp / 1000.0 : tdisp,
		       tdisp > 1000.0 ? "s" : "ms");
	}

out:
	if (filename != NULL)
		free(filename);

	if (buf != NULL)
		free(buf);

	if (flash != NULL)
		m25pxxflash_destroy(flash);

	if (spihw != NULL) {
		spihw->ops->release(spihw);

		if (strcmp(devname, "USB-Blaster") == 0)
			altusb_destroy(spihw);
		else if (strcmp(devname, "Quad RS232-HS A") == 0)
			hpmusb_destroy(spihw);
	}

	if (devname != NULL)
		free(devname);

	if (ftdifunc != NULL)
		ftdi_destroy(ftdifunc);

	return ret;
}
