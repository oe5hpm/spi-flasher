// SPDX-License-Identifier: GPL-2.0+
/*
 * Physical Layer for HPM Blaster devices
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ftd2xx.h>
#include <libhpmusb.h>
#include "osi.h"

#define FTDI_TIMEOUT		2500
#define FTDI_LATENCY		1

static int mpsse_probe(struct spihw_t *spi, bool retry, unsigned char probecmd)
{
	uint8_t xbuf[32] = { };
	DWORD writeb, readb, avail;
	bool checknext = false;
	FT_STATUS rc;
	unsigned int i, j;

	if (retry == false) {
		xbuf[0] = probecmd;
		rc = spi->ftdifunc->write(spi->fthandle, xbuf, 1, &writeb);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot write to FTx232.\n",
				__func__);
			return -1;
		}
	}

	i = 0;
	do {
		if (retry == true) {
			xbuf[0] = probecmd;
			rc = spi->ftdifunc->write(spi->fthandle,
						  xbuf, 1, &writeb);
			if (rc != FT_OK) {
				fprintf(stderr,
					"%s: cannot write to FTx232.\n",
					__func__);
				return -1;
			}
		}
		rc = spi->ftdifunc->get_queuestat(spi->fthandle, &avail);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot get queue status of FTx232.\n",
				__func__);
			return -1;
		}
		if (avail == 0) {
			_usleep(50 * 1000);
			i++;
			continue;
		} else if (avail > sizeof(xbuf)) {
			fprintf(stderr,
				"%s: response from FTx232 too large for xbuf.\n",
				__func__);
			return -1;
		}
		/* check result */
		rc = spi->ftdifunc->read(spi->fthandle, xbuf, avail, &readb);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot read from FTx232.\n",
				__func__);
			return -1;
		}
		j = 0;
		while (readb) {
			if (xbuf[j] == 0xFA) {
				checknext = true;
			} else if (checknext == true) {
				if (xbuf[j] == probecmd)
					return 0;
				checknext = false;
			}
			j++;
			readb--;
		}
		_usleep(10 * 1000);
		i++;
	} while (i < 512);

	return -1;
}

static int spi_trx(struct spihw_t *spi, unsigned int cs,
		   uint8_t *out, uint8_t *in, size_t size)
{
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;
	uint8_t xbuf[0x10000] = { };
	uint16_t trxsize;
	unsigned int i = 0;
	unsigned int payloadsize;
	DWORD writeb, readb;
	FT_STATUS rc;
	uint8_t csmasks[] = { 0x10, 0x20, 0x40, 0x80  };

	if (cs > 3) {
		fprintf(stderr,
			"%s: cs %d out of range (0 - 3).\n",
			__func__, cs);
		return -1;
	}
	priv->csmsk = csmasks[cs];

	/* assert chipselect */
	priv->portstate &= ~priv->csmsk;
	xbuf[i++] = 0x80;		/* setup port and drivers Bus A */
	xbuf[i++] = priv->portstate;	/* value */
	xbuf[i++] = 0xFB;		/* direction */

	while (size != 0) {
		trxsize = size > (0x10000 - 6) ? (0x10000 - 6) : size;

		/* setup transfer */
		xbuf[i++] = priv->trxcmd;
		xbuf[i++] = (trxsize - 1) & 0xFF;
		xbuf[i++] = ((trxsize - 1) & 0xFF00) >> 8;

		payloadsize = 0;
		while (trxsize--) {
			payloadsize++;
			xbuf[i++] = *out++;
		}

		/* is the transfer finished after this? de-assert chipselect */
		if ((size - payloadsize) == 0) {
			priv->portstate |= priv->csmsk;
			xbuf[i++] = 0x80;
			xbuf[i++] = priv->portstate;
			xbuf[i++] = 0xFB;
		}

		rc = spi->ftdifunc->write(spi->fthandle, xbuf, i, &writeb);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot write job to FTx232.\n",
				__func__);
			return -1;
		}

		rc = spi->ftdifunc->read(spi->fthandle,
					 in, payloadsize, &readb);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot read from FTx232.\n",
				__func__);
			return -1;
		}
		if (readb != payloadsize) {
			fprintf(stderr,
				"%s: FTx232 data out of sync.\n",
				__func__);
			return -1;
		}
		i = 0;
		size -= payloadsize;
		in += payloadsize;
	}

	return 0;
}

static int spi_setspeedmode(struct spihw_t *spi,
			    unsigned int speed, int mode)
{
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;
	uint8_t xbuf[32] = { };
	unsigned int i = 0;
	uint16_t div = 0;
	FT_STATUS rc;
	DWORD writeb = 0, availb = 0;

	if (mode == -1) {
	} else if (mode == 2 || mode == 3) {
		priv->portstate |= 0x01; /* CLK high */
		priv->trxcmd = 0x31;
		spi->mode = mode;
	} else if (mode == 0 || mode == 1) {
		priv->portstate &= ~0x01; /* CLK-low */
		priv->trxcmd = 0x31;
		spi->mode = mode;
	} else {
		fprintf(stderr,
			"%s: invalid mode %d.\n",
			__func__, mode);
		return -1;
	}

	if (speed != 0) {
		if (speed <= 6000000) {
			div = (6000000 / speed) - 1;
			xbuf[i++] = 0x8B;	/* enable clock prescaler */
		} else if (speed <= 30000000) {
			div = (30000000 / speed) - 1;
			if (speed < 30000000 && div == 0)
				div = 1;
			xbuf[i++] = 0x8A;	/* disable clock prescaler */
		} else {
			fprintf(stderr,
				"%s: invalid speed %d.\n",
				__func__, speed);
			return -1;
		}
		spi->speed = speed;

		/* setup clock divider */
		xbuf[i++] = 0x86;
		xbuf[i++] = div & 0xFF;
		xbuf[i++] = (div & 0xFF00) >> 8;
	}

	/* setup GPIO port */
	xbuf[i++] = 0x80;		/* setup port and drivers Bus A */
	xbuf[i++] = priv->portstate;	/* value */
	xbuf[i++] = 0xFB;		/* direction */
	/* loopback off */
	xbuf[i++] = 0x85;

	rc = spi->ftdifunc->write(spi->fthandle, xbuf, i, &writeb);
	if (rc != FT_OK) {
		fprintf(stderr,
			"%s: cannot write config to FTx232.\n",
			__func__);
		return -1;
	}

	rc = spi->ftdifunc->get_queuestat(spi->fthandle, &availb);
	if (rc != FT_OK) {
		fprintf(stderr,
			"%s: cannot get queue status of FTx232.\n",
			__func__);
		return -1;
	}

	if (availb != 0) {
		fprintf(stderr,
			"%s: unexpected response from MPSSE.\n",
			__func__);
		return -1;
	}

	return 0;
}

static int set_clr_tms(struct spihw_t *spi, bool set_nclear)
{
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;

	if (set_nclear == true)
		priv->portstate |= 0x8;
	else
		priv->portstate &= ~0x8;

	spi_setspeedmode(spi, 0, -1);

	return 0;
}

static int set_clr_nce(struct spihw_t *spi, bool set_nclear)
{
	struct spiops_t *ops = spi->ops;
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;
	int rc;
	uint8_t buf;

	if (set_nclear == true)
		priv->fpga_cfg |= 0x80;
	else
		priv->fpga_cfg &= ~0x80;

	rc = ops->trx(spi, 2, &priv->fpga_cfg, &buf, 1);
	if (rc != 0) {
		fprintf(stderr,
			"%s: FPGA setup failed!\n", __func__);
		return -1;
	}

	return 0;
}


static int spi_claim(struct spihw_t *spi)
{
	struct spiops_t *ops = spi->ops;
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;
	int rc;
	uint8_t xbuf;

	priv->portstate = 0xF8;

	rc = ops->set_speed_mode(spi, 6000000, 1);
	if (rc != 0) {
		fprintf(stderr,
			"%s: set_speed_mode failed!\n", __func__);
		return -1;
	}
	/*
	 * Bit7 = 0 ... TGT_nCE
	 * Bit6 = 0 ... assert target reset
	 * Bit5 = 0 ... turn on LED
	 * Bit4 = 0 ... TGT interface is flash
	 * Bit3 = 1 ... unused
	 * Bit2 = 1 ... unused
	 * Bit1 = 1 ... unused
	 * Bit0 = 1 ... intflash is connected to SPI
	 */
	priv->fpga_cfg = 0xF;

	ops->trx(spi, 2, &priv->fpga_cfg, &xbuf, 1);
	if (rc != 0) {
		fprintf(stderr,
			"%s: FPGA setup failed!\n", __func__);
		return -1;
	}

	printf("%s: FPGA-iostate 0x%02x\n", __func__, xbuf);

	return 0;
}

static int spi_release(struct spihw_t *spi)
{
	struct spiops_t *ops = spi->ops;
	struct hpmusb_priv_t *priv = (struct hpmusb_priv_t *)spi->priv;
	uint8_t xbuf;

	/* reset TMS to initial (high) state */
	set_clr_tms(spi, true);

	/* reset io-shiftregister */
	priv->fpga_cfg = 0xFF;
	return ops->trx(spi, 2, &priv->fpga_cfg, &xbuf, 1);

}

static const struct spiops_t ops = {
	.trx = &spi_trx,
	.claim = &spi_claim,
	.release = &spi_release,
	.set_clr_tms = set_clr_tms,
	.set_clr_nce = set_clr_nce,
	.set_speed_mode = spi_setspeedmode,
};

void hpmusb_destroy(struct spihw_t *spi)
{
	if (spi->priv != NULL)
		free(spi->priv);

	if (spi->fthandle != NULL)
		spi->ftdifunc->close(spi->fthandle);

	ftdi_destroy(spi->ftdifunc);

	free(spi);
}

struct spihw_t *hpmusb_create(unsigned int ftdi_devidx)
{
	struct spihw_t *spi;
	uint8_t xbuf[256] = { };
	DWORD avail, readb;
	FT_STATUS rc;
	struct hpmusb_priv_t *priv;


	spi = calloc(1, sizeof(*spi));
	if (spi == NULL) {
		fprintf(stderr,
			"%s: no memory for creating hpm-blaster instance!\n",
			__func__);

		return NULL;
	}

	spi->priv = calloc(1, sizeof(struct hpmusb_priv_t));
	if (spi->priv == NULL) {
		fprintf(stderr,
			"%s: no memory for creating hpm-blaster priv!\n",
			__func__);
		free(spi);
		return NULL;
	}
	priv = (struct hpmusb_priv_t *)spi->priv;

	priv->portstate = 0;
	priv->csmsk = 0x10;

	spi->ops = (struct spiops_t *)&ops;

	do {
		spi->ftdifunc = ftdi_create();
		if (spi->ftdifunc == NULL) {
			fprintf(stderr,
				"%s: could not connect function pointers!\n",
				__func__);
			break;
		}
		/* open the requested device */
		rc = spi->ftdifunc->open(ftdi_devidx, &spi->fthandle);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot open dev #%d, doesn't exist? permissions?\n",
				__func__, ftdi_devidx);
			break;
		}
		/* test for valid description string */
		rc = spi->ftdifunc->get_deviceinfo(spi->fthandle,
						   NULL, NULL,
						   NULL, xbuf, NULL);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot querry device info string!\n",
				__func__);
			break;
		}
		if (strcmp(xbuf, "Quad RS232-HS A") != 0) {
			fprintf(stderr,
				"device description '%s' doesn't match '%s'.\n",
				xbuf, "Quad RS232-HS A");
			break;
		}
		/* reset FTDI device */
		rc = spi->ftdifunc->reset(spi->fthandle);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot reset device.\n", __func__);
			break;
		}
		/* purge tx/rx queues */
		rc = spi->ftdifunc->purge(spi->fthandle,
					  FT_PURGE_RX | FT_PURGE_TX);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot reset rx/tx queue.\n", __func__);
			break;
		}
		/* purge input buffers */
		rc = spi->ftdifunc->get_queuestat(spi->fthandle, &avail);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot get queue status.\n", __func__);
			break;
		}
		while (avail > 0) {
			unsigned int xsize;

			xsize = avail > sizeof(xbuf) ? sizeof(xbuf) : avail;
			rc = spi->ftdifunc->read(spi->fthandle,
						 xbuf, xsize, &readb);
			if (rc != FT_OK) {
				fprintf(stderr,
					"%s: cannot read from FTx232.\n",
					__func__);
				break;
			}
			avail -= xsize;
		}
		/* setup paket size */
		rc = spi->ftdifunc->set_usbpar(spi->fthandle, 0x10000, 0x10000);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot set usb paket size.\n",
				__func__);
			break;
		}
		/* setup timeouts for usb-transfers */
		rc = spi->ftdifunc->set_timeout(spi->fthandle,
						FTDI_TIMEOUT, FTDI_TIMEOUT);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot setup timeouts.\n", __func__);
			break;
		}
		/* setup latency timer */
		rc = spi->ftdifunc->set_latency(spi->fthandle, FTDI_LATENCY);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot setup latency timer.\n", __func__);
			break;
		}
		/* disable event/error characters */
		rc = spi->ftdifunc->set_chars(spi->fthandle, 0, 0, 0, 0);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot disable event/error chars.\n",
				__func__);
			break;
		}
		/* setup flow control */
		rc = spi->ftdifunc->set_flowctrl(spi->fthandle,
						 FT_FLOW_RTS_CTS, 0x00, 0x00);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot setup flow control.\n",
				__func__);
			break;
		}
		/* reset MPSSE engine */
		rc = spi->ftdifunc->set_bitmode(spi->fthandle, 0, 0x00);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot assert reset.\n",
				__func__);
			break;
		}
		rc = spi->ftdifunc->set_bitmode(spi->fthandle, 0, 0x02);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot release reset.\n",
				__func__);
			break;
		}

		rc = mpsse_probe(spi, true, 0xAA);
		if (rc != 0) {
			fprintf(stderr,
				"%s: MPSSE probe (0xAA) failed.\n",
				__func__);
			break;
		}

		rc = mpsse_probe(spi, false, 0xAB);
		if (rc != 0) {
			fprintf(stderr,
				"%s: MPSSE probe (0xAB) failed.\n",
				__func__);
			break;
		}

		return spi;
	} while (0);

	hpmusb_destroy(spi);
	return NULL;
}
