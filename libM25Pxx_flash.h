// SPDX-License-Identifier: GPL-2.0+
/*
 * M25Pxx Flash Library
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include <spihw.h>

struct flashparam_t {
	uint8_t		type;
	char		name[12];
	uint8_t		memtype;
	uint8_t		capacity;
	uint8_t		signature;
	uint32_t	size;
	uint32_t	sectorsize;
	uint32_t	pagesize;
	uint32_t	bulktime;
	uint32_t	bulktime_max;
	uint32_t	sectortime;
	uint32_t	sectortime_max;
	uint32_t	pagetime;
	uint32_t	pagetime_max;
};

struct m25pxxflash_t {
	struct flashparam_t	*flash_db;
	struct flashparam_t	*flash_detected;

	struct spihw_t		*spi;
	unsigned int		cs;
	uint8_t			*xbuf;
	size_t			xbufsize;
};

struct m25pxx_progress_t {
	void (*fct)(void *arg, unsigned int prcnt_done, unsigned int flag);
	void *arg;
};

void m25pxxflash_destroy(struct m25pxxflash_t *inst);
struct m25pxxflash_t *m25pxxflash_create(struct spihw_t *spi);
int m25pxx_detect(struct m25pxxflash_t *inst, uint8_t cs);
int m25pxx_rdsr(struct m25pxxflash_t *inst, uint8_t *reg);
int m25pxx_wrsr(struct m25pxxflash_t *inst, uint8_t reg);
int m25pxx_read(struct m25pxxflash_t *inst,
		void *dst, uint32_t addr, size_t size);
int m25pxx_program(struct m25pxxflash_t *inst,
		   void *src, uint32_t addr, size_t size,
		   struct m25pxx_progress_t *progress);
void m25pxx_printflash(struct flashparam_t *pflash);
int m25pxx_chiperase(struct m25pxxflash_t *inst,
		     struct m25pxx_progress_t *progress);
int m25pxx_sectorerase(struct m25pxxflash_t *inst, uint32_t addr,
		       struct m25pxx_progress_t *progress);
