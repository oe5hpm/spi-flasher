// SPDX-License-Identifier: GPL-2.0+
/*
 * Physical Layer for Altera USB-Blaster devices
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __LIBALTUSB_H__
#define __LIBALTUSB_H__

#include <libftdi.h>
#include <spihw.h>

void altusb_destroy(struct spihw_t *spi);
struct spihw_t *altusb_create(unsigned int ftdi_devidx);

#endif /* LIBALTUSB_H_ */
