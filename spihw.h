// SPDX-License-Identifier: GPL-2.0+
/*
 * SPI Instance definition
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __SPIHW_H__
#define __SPIHW_H__

#include <ftd2xx.h>

struct spihw_t {
	FT_HANDLE		fthandle;
	struct ftdi_funcptr_t	*ftdifunc;
	uint8_t			portstate;
	struct spiops_t		*ops;
};

struct spiops_t {
	int (*trx)(struct spihw_t *spi,
		   uint8_t *out, uint8_t *in, size_t size);
	int (*set_speed_mode)(struct spihw_t *spi,
			      unsigned int speed, unsigned int mode);
	int (*set_port)(struct spihw_t *spi, uint8_t port);
	int (*set_cs)(struct spihw_t *spi, unsigned int cs);
};

#endif /* __SPIHW_H__ */
