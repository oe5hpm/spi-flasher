// SPDX-License-Identifier: GPL-2.0+
/*
 * M25Pxx Flash Library
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <spihw.h>
#include "libM25Pxx_flash.h"

#ifdef DEBUG
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif

static const struct flashparam_t fltab[] = {
	{
	  .type			= 0x00,
	  .name			= "M25P40",
	  .memtype		= 0x20,
	  .capacity		= 0x13,
	  .signature		= 0x12,
	  .size			= 0x100000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 5000000,
	  .bulktime_max		= 10000000,
	  .sectortime		= 1000000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 1500,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25P80",
	  .memtype		= 0x20,
	  .capacity		= 0x14,
	  .signature		= 0x13,
	  .size			= 0x100000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 8000000,
	  .bulktime_max		= 20000000,
	  .sectortime		= 650000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 650,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25Px80",
	  .memtype		= 0x71,
	  .capacity		= 0x14,
	  .signature		= 0x13,
	  .size			= 0x100000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 8000000,
	  .bulktime_max		= 80000000,
	  .sectortime		= 600000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 800,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25P16",
	  .memtype		= 0x20,
	  .capacity		= 0x15,
	  .signature		= 0x14,
	  .size			= 0x200000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 13000000,
	  .bulktime_max		= 40000000,
	  .sectortime		= 600000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 650,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "W25Q16",
	  .memtype		= 0x40,
	  .capacity		= 0x15,
	  .signature		= 0x14,
	  .size			= 0x200000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 3000000,
	  .bulktime_max		= 10000000,
	  .sectortime		= 60000,
	  .sectortime_max	= 500000,
	  .pagetime		= 500,
	  .pagetime_max		= 4000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25P32",
	  .memtype		= 0x20,
	  .capacity		= 0x16,
	  .signature		= 0x15,
	  .size			= 0x400000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 23000000,
	  .bulktime_max		= 80000000,
	  .sectortime		= 600000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 650,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "SST25VF032",
	  .memtype		= 0x25,
	  .capacity		= 0x4A,
	  .signature		= 0x15,
	  .size			= 0x400000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  /* no useable values found in datasheet, so use them of M25P32 */
	  .bulktime		= 23000000,
	  .bulktime_max		= 80000000,
	  .sectortime		= 600000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 650,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "MX25U3235F",
	  .memtype		= 0x25,
	  .capacity		= 0x36,
	  .signature		= 0x36,
	  .size			= 0x400000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 25000000,
	  .bulktime_max		= 50000000,
	  .sectortime		= 350000,
	  .sectortime_max	= 2000000,
	  .pagetime		= 500,
	  .pagetime_max		= 3000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25P64",
	  .memtype		= 0x20,
	  .capacity		= 0x17,
	  .signature		= 0x16,
	  .size			= 0x800000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 68000000,
	  .bulktime_max		= 160000000,
	  .sectortime		= 1000000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 1400,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "S25FL064L",
	  .memtype		= 0x40,
	  .capacity		= 0x17,
	  .signature		= 0x16,
	  .size			= 0x800000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 55000000,
	  .bulktime_max		= 150000000,
	  .sectortime		= 450000,
	  .sectortime_max	= 1150000,
	  .pagetime		= 500,
	  .pagetime_max		= 1350,
	},
	{
	  .type			= 0x00,
	  .name			= "MX25U6435F",
	  .memtype		= 0x25,
	  .capacity		= 0x37,
	  .signature		= 0x37,
	  .size			= 0x800000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 50000000,
	  .bulktime_max		= 75000000,
	  .sectortime		= 350000,
	  .sectortime_max	= 2000000,
	  .pagetime		= 500,
	  .pagetime_max		= 3000,
	},
	{
	  .type			= 0x00,
	  .name			= "M25P128",
	  .memtype		= 0x20,
	  .capacity		= 0x18,
	  .signature		= 0x00,
	  .size			= 0x1000000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 130000000,
	  .bulktime_max		= 250000000,
	  .sectortime		= 1600000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 600,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "N25Q128",
	  .memtype		= 0xBA,
	  .capacity		= 0x18,
	  .signature		= 0x00,
	  .size			= 0x1000000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 170000000,
	  .bulktime_max		= 250000000,
	  .sectortime		= 700000,
	  .sectortime_max	= 3000000,
	  .pagetime		= 500,
	  .pagetime_max		= 5000,
	},
	{
	  .type			= 0x00,
	  .name			= "W25Q128",
	  .memtype		= 0x17,
	  .capacity		= 0x18,
	  .signature		= 0x40,
	  .size			= 0x1000000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 40000000,
	  .bulktime_max		= 200000000,
	  .sectortime		= 150000,
	  .sectortime_max	= 2000000,
	  .pagetime		= 700,
	  .pagetime_max		= 3000,
	},
	{
	  .type			= 0x00,
	  .name			= "S25FL128S",
	  .memtype		= 0x02,
	  .capacity		= 0x18,
	  .signature		= 0x17,
	  .size			= 0x1000000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 66000000,
	  .bulktime_max		= 330000000,
	  .sectortime		= 520000,
	  .sectortime_max	= 2600000,
	  .pagetime		= 250,
	  .pagetime_max		= 750,
	},
	{
	  .type			= 0x00,
	  .name			= "S25FL256S",
	  .memtype		= 0x02,
	  .capacity		= 0x19,
	  .signature		= 0x18,
	  .size			= 0x2000000,
	  .sectorsize		= 0x10000,
	  .pagesize		= 0x100,
	  .bulktime		= 66000000,
	  .bulktime_max		= 330000000,
	  .sectortime		= 520000,
	  .sectortime_max	= 2600000,
	  .pagetime		= 250,
	  .pagetime_max		= 750,
	},
	{ .type = 0xFF },
};

static struct flashparam_t *m25pxx_search(struct flashparam_t *ptab,
					  uint8_t typ, uint8_t cap, uint8_t sig)
{
	while (ptab->type != 0xFF) {
		if ((typ == 0xFF || ptab->memtype == typ) &&
		    (cap == 0xFF || ptab->capacity == cap) &&
		    (sig == 0xFF || (ptab->signature != 0 &&
				     ptab->signature == sig)))
			return ptab;
		ptab++;
	}
	return NULL;
}

int DLLEXPORT m25pxx_detect(struct m25pxxflash_t *inst, uint8_t cs)
{
	struct spiops_t *spi = inst->spi->ops;
	int rc;
	uint8_t xbuf[32] = { 0 };

	if (inst == NULL)
		return -1;

	/* try 'READID' command */
	xbuf[0] = 0x9F;
	rc = spi->trx(inst->spi, cs, xbuf, xbuf, 4);
	if (rc != 0) {
		fprintf(stderr, "%s: spi trx failed!\n", __func__);
		return -1;
	}

	if (xbuf[2] != 0xFF && xbuf[3] != 0xFF)
		inst->flash_detected = m25pxx_search(inst->flash_db,
						     xbuf[2], xbuf[3], 0xFF);

	/* not found, try 'read-signature' command */
	if (inst->flash_detected == NULL)  {
		xbuf[0] = 0xAB;
		rc = spi->trx(inst->spi, cs, xbuf, xbuf, 8);
		if (rc != 0) {
			fprintf(stderr, "%s: spi trx failed!\n", __func__);
			return -1;
		}

		if (xbuf[4] != 0xFF)
			inst->flash_detected = m25pxx_search(inst->flash_db,
							     0xFF,
							     0xFF,
							     xbuf[4]);
	}
	/* still not found, try 'read_id' (spansion) command */
	if (inst->flash_detected == NULL)  {
		xbuf[0] = 0x90;
		rc = spi->trx(inst->spi, cs, xbuf, xbuf, 4);
		if (rc != 0) {
			fprintf(stderr, "%s:spi trx failed!\n", __func__);
			return -1;
		}

		if (xbuf[2] != 0xFF && xbuf[3] != 0xFF)
			inst->flash_detected = m25pxx_search(inst->flash_db,
							     xbuf[2],
							     xbuf[3],
							     0xFF);
	}

	if (inst->flash_detected == NULL) {
		printf("typ: 0x%02x, cap: 0x%02x, sig: 0x%02x\n",
		       xbuf[2], xbuf[3], xbuf[4]);
		return -1;
	}

	if ((inst->flash_detected->size + 0x10) != inst->xbufsize) {
		inst->xbufsize = 0;
		if (inst->xbuf)
			free(inst->xbuf);

		inst->xbuf = calloc(inst->flash_detected->size + 0x10, 1);
		if (inst->xbuf == NULL) {
			fprintf(stderr,
				"%s: no mem for xbuf!\n",
				__func__);

			return -1;
		}
		inst->xbufsize = inst->flash_detected->size + 0x10;
	}
	inst->cs = cs;

	return 0;
}

int DLLEXPORT m25pxx_read(struct m25pxxflash_t *inst,
			  void *dst, uint32_t addr, size_t size)
{
	struct spiops_t *spi = inst->spi->ops;
	int rc;

	if (inst == NULL)
		return -1;

	if (inst->flash_detected == NULL) {
		fprintf(stderr,
			"%s: no valid flash detected!\n", __func__);
		return -1;
	}

	if (dst == NULL)
		return -1;

	if ((addr + size) > inst->flash_detected->size) {
		fprintf(stderr,
			"%s: addr 0x%x with size 0x%lx exceeds flash size 0x%x !\n",
			__func__, addr, size, inst->flash_detected->size);
		return -1;
	}

	inst->xbuf[0] = 0x03;
	inst->xbuf[1] = (addr & 0x00FF0000) >> 16;
	inst->xbuf[2] = (addr & 0x0000FF00) >> 8;
	inst->xbuf[3] = (addr & 0x000000FF) >> 0;

	rc = spi->trx(inst->spi, inst->cs, inst->xbuf, inst->xbuf, size + 4);
	if (rc != 0) {
		fprintf(stderr,
			"%s: spi trx returned error (%d)!\n", __func__, rc);
		return -1;
	}

	memcpy(dst, inst->xbuf + 4, size);

	return 0;
}

int DLLEXPORT m25pxx_rdsr(struct m25pxxflash_t *inst, uint8_t *reg)
{
	struct spiops_t *spi = inst->spi->ops;
	uint8_t xbuf[2] = { 0x05, 0x00 };
	int rc;

	if (inst == NULL)
		return -1;

	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 2);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot poll status register!\n",
			__func__);
		return -1;
	}

	if (reg)
		*reg = xbuf[1];

	return 0;
}


int DLLEXPORT m25pxx_wdsr(struct m25pxxflash_t *inst, uint8_t reg)
{
	struct spiops_t *spi = inst->spi->ops;
	uint8_t xbuf[2] = { 0x01, reg };
	int rc;

	if (inst == NULL)
		return -1;

	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 2);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot write status register!\n",
			__func__);
		return -1;
	}

	return 0;
}

int DLLEXPORT m25pxx_chiperase(struct m25pxxflash_t *inst,
			       struct m25pxx_progress_t *progress)
{
	struct spiops_t *spi = inst->spi->ops;
	uint8_t xbuf[4] = { 0 };
	unsigned int cnt = 0, cntx;
	unsigned int percent, percentx;
	int rc;

	if (inst == NULL)
		return -1;

	if (inst->flash_detected == NULL) {
		fprintf(stderr,
			"%s: no valid flash detected!\n", __func__);
		return -1;
	}

	if (progress)
		progress->fct(progress->arg, 0, 0);

	xbuf[0] = 0x06;	/* write enable */
	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 1);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set write enable!\n", __func__);
		return -1;
	}

	xbuf[0] = 0xC7;	/* bulk erase */
	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 1);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set bulk erase!\n", __func__);
		return -1;
	}

	cnt = inst->flash_detected->bulktime_max /
	      (inst->flash_detected->bulktime / 64);
	cntx = cnt;
	do {
		usleep(inst->flash_detected->bulktime / 64);
		rc = m25pxx_rdsr(inst, &xbuf[0]);
		if (rc != 0)
			return -1;
		DBG("%s: (%02x) delay %d, retry #%d\n",
		    __func__,
		    xbuf[0], inst->flash_detected->bulktime / 64, cnt);
		cnt--;
		percent = (cntx - cnt) / (cntx >= 100 ? (cntx / 100) : 1);
		if (percent != 0 && percentx != percent) {
			percentx = percent;
			if (progress)
				progress->fct(progress->arg, percent, 0);
		}
	} while ((xbuf[0] & 0x1) == 0x1 && cnt > 0);

	if ((xbuf[0] & 0x1) != 0)
		return -1;

	if (progress) {
		while (percent++ < 99)
			progress->fct(progress->arg, percent, 1);
		progress->fct(progress->arg, 100, 0);
	}

	return 0;
}

int DLLEXPORT m25pxx_sectorerase(struct m25pxxflash_t *inst, uint32_t addr,
				 struct m25pxx_progress_t *progress)
{
	struct spiops_t *spi = inst->spi->ops;
	uint8_t xbuf[4] = { 0 };
	unsigned int cnt = 0, cntx;
	unsigned int percent, percentx;
	int rc;

	if (inst == NULL)
		return -1;

	if (inst->flash_detected == NULL) {
		fprintf(stderr,
			"%s: no valid flash detected!\n", __func__);
		return -1;
	}

	if (progress)
		progress->fct(progress->arg, 0, 0);

	xbuf[0] = 0x06;	/* write enable */
	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 1);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set write enable!\n", __func__);
		return -1;
	}

	xbuf[0] = 0xD8;
	xbuf[1] = (addr & 0x00FF0000) >> 16;
	xbuf[2] = (addr & 0x0000FF00) >> 8;
	xbuf[3] = (addr & 0x000000FF) >> 0;
	rc = spi->trx(inst->spi, inst->cs, xbuf, xbuf, 4);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set sector erase!\n", __func__);
		return -1;
	}

	cnt = inst->flash_detected->sectortime_max /
	      (inst->flash_detected->sectortime / 64);
	cntx = cnt;
	do {
		usleep(inst->flash_detected->sectortime / 64);
		rc = m25pxx_rdsr(inst, &xbuf[0]);
		if (rc != 0)
			return -1;
		DBG("%s: (%02x) delay %d, retry #%d\n",
		    __func__,
		    xbuf[0], inst->flash_detected->sectortime / 64, cnt);
		cnt--;
		percent = (cntx - cnt) / (cntx >= 100 ? (cntx / 100) : 1);
		if (percent != 0 && percentx != percent) {
			percentx = percent;
			if (progress)
				progress->fct(progress->arg, percent, 0);
		}
	} while ((xbuf[0] & 0x1) == 0x1 && cnt > 0);

	if ((xbuf[0] & 0x1) != 0)
		return -1;


	if (progress) {
		while (percent++ < 99)
			progress->fct(progress->arg, percent, 1);
		progress->fct(progress->arg, 100, 0);
	}
	return 0;
}

static int m25pxx_progpage(struct m25pxxflash_t *inst,
			   void *src, uint32_t addr, size_t size)
{
	struct spiops_t *spi = inst->spi->ops;
	unsigned int cnt = 0;
	int rc;

	inst->xbuf[0] = 0x06;	/* write enable */
	rc = spi->trx(inst->spi, inst->cs, inst->xbuf, inst->xbuf, 1);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set write enable!\n", __func__);
		return -1;
	}

	inst->xbuf[0] = 0x02;
	inst->xbuf[1] = (addr & 0x00FF0000) >> 16;
	inst->xbuf[2] = (addr & 0x0000FF00) >> 8;
	inst->xbuf[3] = (addr & 0x000000FF) >> 0;
	memcpy(&inst->xbuf[4], src, size);
	rc = spi->trx(inst->spi, inst->cs, inst->xbuf, inst->xbuf, size + 4);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot set page program!\n", __func__);
		return -1;
	}

	cnt = inst->flash_detected->pagetime_max /
	      (inst->flash_detected->pagetime / 8);
	do {
		usleep(inst->flash_detected->pagetime / 8);
		rc = m25pxx_rdsr(inst, &inst->xbuf[0]);
		if (rc != 0)
			return -1;
		DBG("%s: (%02x) delay %d, retry #%d\n",
		    __func__,
		   inst->xbuf[0], inst->flash_detected->pagetime / 8, cnt);
		cnt--;
	} while ((inst->xbuf[0] & 0x1) == 0x1 && cnt > 0);

	if ((inst->xbuf[0] & 0x1) != 0)
		return -1;

	return 0;
}

int DLLEXPORT m25pxx_program(struct m25pxxflash_t *inst,
			     void *src, uint32_t addr, size_t size,
			     struct m25pxx_progress_t *progress)
{
	int rc;
	size_t prog;
	uint8_t status;
	size_t size_x = size;
	unsigned int percent, percentx = 0;

	if (inst == NULL)
		return -1;

	if (inst->flash_detected == NULL) {
		fprintf(stderr,
			"%s: no valid flash detected!\n", __func__);
		return -1;
	}

	rc = m25pxx_rdsr(inst, &status);
	if (rc != 0)
		return -1;

	if (status & 0x80) {
		fprintf(stderr,
			"%s: flash has write protect asserted status 0x%02x.\n",
			__func__, status);
		return -1;
	}

	if (progress)
		progress->fct(progress->arg, 0, 0);

	if (status & 0x1C) {
		fprintf(stderr,
			"%s: WARN: flash has some block protect bits set 0x%02x\n",
			__func__, status);
	}

	while (size) {
		if (progress) {
			percent = (size_x - size) /
				  (size_x >= 100 ? (size_x / 100) : 1);
			if (percent != 0 && percentx != percent) {
				percentx = percent;
				progress->fct(progress->arg, percent, 0);
			}
		}
		prog = size > inst->flash_detected->pagesize ?
			      inst->flash_detected->pagesize : size;

		memset(inst->xbuf, 0xFF, inst->flash_detected->pagesize);

		if (memcmp(src, inst->xbuf, prog) == 0) {
			DBG("%s: skip empty page @ 0x%x\n", __func__, addr);
		} else {
			rc = m25pxx_progpage(inst, src, addr, prog);
			if (rc != 0) {
				fprintf(stderr,
					"%s: cannot program page @ 0x%x\n",
					__func__, addr);
				return -1;
			}
		}
		src += prog;
		addr += prog;
		size -= prog;
	}

	if (progress && percent != 100)
		progress->fct(progress->arg, 100, 0);

	return 0;
}

void DLLEXPORT m25pxx_printflash(struct flashparam_t *pflash)
{
	if (pflash == NULL)
		return;

	printf("Name                  : %s\n", pflash->name);
	printf("Memtype               : 0x%02x\n", pflash->memtype);
	printf("Capacity              ; 0x%02x\n", pflash->capacity);
	printf("Signature             : 0x%02x\n", pflash->signature);
	printf("Size                  : 0x%x bytes\n", pflash->size);
	printf("Sector-Size           : 0x%x\n", pflash->sectorsize);
	printf("Sector-Cnt            : %d\n",
	       pflash->size / pflash->sectorsize);
	printf("Page-Size             : 0x%x\n", pflash->pagesize);
	printf("BulkErase Time typ    : %.2f ms\n", pflash->bulktime / 1000.0);
	printf("BulkErase Time max    : %.2f ms\n",
	       pflash->bulktime_max / 1000.0);
	printf("SectorErase Time typ  : %.2f ms\n",
	       pflash->sectortime / 1000.0);
	printf("SectorErase Time max  : %.2f ms\n",
	       pflash->sectortime_max / 1000.0);
	printf("PageProgram Time typ  : %.2f ms\n", pflash->pagetime / 1000.0);
	printf("PageProgram Time max  : %.2f ms\n",
	       pflash->pagetime_max / 1000.0);
}

void m25pxxflash_destroy(struct m25pxxflash_t *inst)
{
	if (inst == NULL)
		return;

	if (inst->xbuf != NULL)
		free(inst->xbuf);
	free(inst);
}

struct m25pxxflash_t *m25pxxflash_create(struct spihw_t *spi)
{
	struct m25pxxflash_t *inst;

	inst = calloc(1, sizeof(*inst));
	if (inst == NULL) {
		fprintf(stderr, "no mem flash M25Pxx flash instance!\n");
		return NULL;
	}

	do {
		inst->flash_db = (struct flashparam_t *)&fltab[0];
		inst->spi = spi;

		return inst;
	} while (0);

	return NULL;
}
