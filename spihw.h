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
#include <stdbool.h>

struct spihw_t {
	FT_HANDLE		fthandle;
	struct ftdi_funcptr_t	*ftdifunc;
	unsigned int		speed;
	unsigned int		mode;
	void			*priv;
	struct spiops_t		*ops;
};

struct spiops_t {
	int (*claim)(struct spihw_t *spi);
	int (*release)(struct spihw_t *spi);
	int (*trx)(struct spihw_t *spi, unsigned int cs,
		   uint8_t *out, uint8_t *in, size_t size);
	int (*set_speed_mode)(struct spihw_t *spi,
			      unsigned int speed, int mode);
	int (*set_clr_tms)(struct spihw_t *spi, bool set_nclear);
	int (*set_clr_nce)(struct spihw_t *spi, bool set_nclear);
};

#endif /* __SPIHW_H__ */
