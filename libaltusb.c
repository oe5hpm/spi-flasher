// SPDX-License-Identifier: GPL-2.0+
/*
 * Physical Layer for Altera USB-Blaster devices
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ftd2xx.h>
#include <libaltusb.h>

/* - Altera USB Blaster (version 1) - */
#define ALTUSB_BYTEMODE		0x80
#define ALTUSB_READ		0x40
#define ALTUSB_BIT_TCK		0x01
#define ALTUSB_BIT_nCONFIG	0x02
#define ALTUSB_BIT_nCE		0x04
#define ALTUSB_BIT_nCS		0x08
#define ALTUSB_BIT_DATA0	0x10
#define ALTUSB_BIT_LED		0x20
#define ALTUSB_DMASK		0x3F

#define DEFAULT_PORTSTATE	(ALTUSB_BIT_nCS | ALTUSB_BIT_nCONFIG)

#define FTDI_TIMEOUT		2000
#define FTDI_LATENCY		1

static const uint8_t bitreverse[256] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
	0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
	0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
	0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
	0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
	0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
	0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF,
};

static int spi_trx(struct spihw_t *spi, unsigned int cs,
		   uint8_t *out, uint8_t *in, size_t size)
{
	struct altusb_priv_t *priv = (struct altusb_priv_t *)spi->priv;
	uint8_t xbuf[0x10000] = { 0 };

	FT_STATUS rc;
	DWORD written, read;

	unsigned int bufsize, blksize, payloadsize, trxsize;

	if (cs > 0) {
		fprintf(stderr,
			"%s: cs %d out of range (0..0).\n",
			__func__, cs);
		return -1;
	}

	bufsize = 0;
	payloadsize = 0;

	/* assert chipselect */
	priv->portstate &= ~ALTUSB_BIT_nCS;
	xbuf[bufsize++] = priv->portstate;

	while (size) {
		blksize = size;
		payloadsize = 0;

		while (bufsize < sizeof(xbuf) && blksize != 0) {
			trxsize = blksize > 0x3F ? 0x3F : blksize;
			/* transfer job */
			xbuf[bufsize++] = 0xC0 + trxsize;
			/* payload */
			while (trxsize--) {
				xbuf[bufsize++] = bitreverse[*out++];
				payloadsize++;
				blksize--;
			}
			/* de-assert chipselect */
			if ((size - payloadsize) == 0) {
				priv->portstate |= ALTUSB_BIT_nCS;
				xbuf[bufsize++] = priv->portstate;
			}
		}
		/* start transfer */
		rc = spi->ftdifunc->write(spi->fthandle,
					  xbuf, bufsize, &written);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: write to FT245 failed!\n", __func__);

			return -1;
		} else if (written != bufsize) {
			fprintf(stderr,
				"%s: failed to write fifo %d != %d\n",
				__func__, written, bufsize);

			return -1;
		}
		/* fetch result */
		rc = spi->ftdifunc->read(spi->fthandle,
					 xbuf, payloadsize, &read);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: read from FT245 failed!\n", __func__);

			return -1;
		} else if (read != payloadsize) {
			fprintf(stderr,
				"%s: size mismatch on response %d != %d\n",
				__func__, read, payloadsize);
		}

		size -= payloadsize;

		bufsize = 0;
		while (payloadsize--)
			*in++ = bitreverse[xbuf[bufsize++]];

		bufsize = 0;
	}
	return size;
}


static int set_clr_tms(struct spihw_t *spi, bool set_nclear)
{
	struct altusb_priv_t *priv = (struct altusb_priv_t *)spi->priv;
	int rc;
	DWORD written;

	if (set_nclear)
		priv->portstate |= ALTUSB_BIT_nCONFIG;
	else
		priv->portstate &= ~ALTUSB_BIT_nCONFIG;

	/* start transfer */
	rc = spi->ftdifunc->write(spi->fthandle,
				  &priv->portstate, 1, &written);
	if (rc != FT_OK) {
		fprintf(stderr,
			"%s: write to FT245 failed!\n", __func__);

		return -1;
	} else if (written != 1) {
		fprintf(stderr,
			"%s: failed to write fifo %d != %d\n",
			__func__, written, 1);

		return -1;
	}

	return 0;
}

static int set_clr_nce(struct spihw_t *spi, bool set_nclear)
{
	struct altusb_priv_t *priv = (struct altusb_priv_t *)spi->priv;
	int rc;
	DWORD written;

	if (set_nclear)
		priv->portstate |= ALTUSB_BIT_nCE;
	else
		priv->portstate &= ~ALTUSB_BIT_nCE;

	/* start transfer */
	rc = spi->ftdifunc->write(spi->fthandle,
				  &priv->portstate, 1, &written);
	if (rc != FT_OK) {
		fprintf(stderr,
			"%s: write to FT245 failed!\n", __func__);

		return -1;
	} else if (written != 1) {
		fprintf(stderr,
			"%s: failed to write fifo %d != %d\n",
			__func__, written, 1);

		return -1;
	}

	return 0;
}

static int spi_setspeedmode(struct spihw_t *spi,
			    unsigned int speed, int mode)
{
	printf("WARN: Alera USB-Blaster runs fixed at 6MHz, and mode 1!\n");
	return 0;
}

static int spi_claim(struct spihw_t *spi)
{
	struct altusb_priv_t *priv = (struct altusb_priv_t *)spi->priv;
	int rc;
	DWORD written;

	priv->portstate |= ALTUSB_BIT_LED;

	/* start transfer */
	rc = spi->ftdifunc->write(spi->fthandle,
				  &priv->portstate, 1, &written);
	if (rc != FT_OK) {
		fprintf(stderr,
			"%s: write to FT245 failed!\n", __func__);

		return -1;
	} else if (written != 1) {
		fprintf(stderr,
			"%s: failed to write fifo %d != %d\n",
			__func__, written, 1);

		return -1;
	}
	return 0;
}

static int spi_release(struct spihw_t *spi)
{
	struct altusb_priv_t *priv = (struct altusb_priv_t *)spi->priv;

	priv->portstate &= ~ALTUSB_BIT_LED;

	/* reset TMS to initial (high) state */
	return set_clr_tms(spi, true);
}


static const struct spiops_t ops = {
	.trx = &spi_trx,
	.claim = &spi_claim,
	.release = &spi_release,
	.set_clr_tms = set_clr_tms,
	.set_clr_nce = set_clr_nce,
	.set_speed_mode = spi_setspeedmode,
};

void altusb_destroy(struct spihw_t *spi)
{
	uint8_t xbuf[256] = { DEFAULT_PORTSTATE };
	DWORD written;
	FT_STATUS rc;

	/* reset usb-blaster cpld statemachine */
	if (spi->fthandle != NULL) {
		rc = spi->ftdifunc->write(spi->fthandle,
					  xbuf, sizeof(xbuf), &written);
		if (rc != FT_OK)
			fprintf(stderr,
				"%s: cannot reset USB-Blaster!\n", __func__);
		spi->ftdifunc->close(spi->fthandle);
	}
	if (spi->priv != NULL)
		free(spi->priv);

	ftdi_destroy(spi->ftdifunc);
	free(spi);
}

struct spihw_t *altusb_create(unsigned int ftdi_devidx)
{
	struct spihw_t *spi;
	struct altusb_priv_t *priv;
	FT_STATUS rc;

	uint8_t xbuf[256];
	DWORD written, readbytes;

	spi = calloc(1, sizeof(*spi));
	if (spi == NULL) {
		fprintf(stderr,
			"%s: no memory for creating usb-blaster instance!\n",
			__func__);

		return NULL;
	}

	spi->priv = calloc(1, sizeof(struct altusb_priv_t));
	if (spi->priv == NULL) {
		fprintf(stderr,
			"%s: no memory for creating hpm-blaster priv!\n",
			__func__);
		free(spi);
		return NULL;
	}
	priv = (struct altusb_priv_t *)spi->priv;

	priv->portstate = DEFAULT_PORTSTATE;

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
		if (strcmp(xbuf, "USB-Blaster") != 0) {
			fprintf(stderr,
				"device description '%s' doesn't match '%s'.\n",
				xbuf, "USB-Blaster");
			break;
		}
		/* reset FTDI device */
		rc = spi->ftdifunc->reset(spi->fthandle);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot reset device.\n", __func__);
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

		//spi->ftdifunc->set_usbpar(spi->fthandle, 512, 512);
		/* reset usb-blaster cpld statemachine */
		memset(xbuf, priv->portstate, sizeof(xbuf));
		rc = spi->ftdifunc->write(spi->fthandle,
					  xbuf, sizeof(xbuf), &written);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s:cannot reset USB-Blaster!\n", __func__);
			break;
		}

		/* simple test if the attached ftdi acts like usb blaster */
		xbuf[0] = ALTUSB_READ;
		rc = spi->ftdifunc->write(spi->fthandle,
					  xbuf, 1, &written);
		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: cannot write to FT245!\n", __func__);
			break;
		}
		rc = spi->ftdifunc->read(spi->fthandle,
					 xbuf, 1, &readbytes);

		if (rc != FT_OK) {
			fprintf(stderr,
				"%s: read from FT245 failed!\n", __func__);

			break;
		} else if (readbytes != 1) {
			fprintf(stderr,
				"%s: cannot probe Altera USB-Blaster!\n",
				__func__);
			break;
		}

		return spi;
	} while (0);

	altusb_destroy(spi);
	return NULL;
}
