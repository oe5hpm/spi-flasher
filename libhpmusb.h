// SPDX-License-Identifier: GPL-2.0+
/*
 * Physical Layer for HPM Blaster devices
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __LIBHPMUSB_H__
#define __LIBHPMUSB_H__

#include <libftdi.h>
#include <spihw.h>

struct hpmusb_priv_t {
	uint8_t		portstate;
	uint8_t		fpga_cfg;
	uint8_t		csmsk;
	uint8_t		trxcmd;
};

void hpmusb_destroy(struct spihw_t *spi);
struct spihw_t *hpmusb_create(unsigned int ftdi_devidx);

#endif /* __LIBHPMUSB_H__ */
