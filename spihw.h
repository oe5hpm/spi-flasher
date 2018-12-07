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
			      unsigned int speed, unsigned int mode);
	int (*set_port)(struct spihw_t *spi, uint8_t port);
};

#endif /* __SPIHW_H__ */
