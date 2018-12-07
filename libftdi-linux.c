// SPDX-License-Identifier: GPL-2.0+
/*
 * FTDI library loading and initialization
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "libftdi-linux.h"

/* FTDI shared library */
static char *ftdi_symbols[] = {
	"FT_GetLibraryVersion",
	"FT_CreateDeviceInfoList",
	"FT_GetDeviceInfoList",
	"FT_Open",
	"FT_Close",
	"FT_ResetDevice",
	"FT_Purge",
	"FT_SetUSBParameters",
	"FT_SetChars",
	"FT_SetTimeouts",
	"FT_SetLatencyTimer",
	"FT_SetBitMode",
	"FT_GetQueueStatus",
	"FT_GetStatus",
	"FT_Read",
	"FT_Write",
	"FT_GetDeviceInfo",
	"FT_SetVIDPID",
	"FT_SetFlowControl",
	NULL,

};

void ftdi_destroy(struct ftdi_funcptr_t *inst)
{
	if (inst->libftdi != NULL)
		dlclose(inst->libftdi);

	free(inst);
}

struct ftdi_funcptr_t *ftdi_create(void)
{

	struct ftdi_funcptr_t *pfunc;
	void **ppfunc;
	unsigned int i;

	pfunc = calloc(1, sizeof(struct ftdi_funcptr_t));
	if (pfunc == NULL) {
		fprintf(stderr,
			"%s: no mem to create ftdi instance!\n", __func__);
		return NULL;
	}

	/* open the shared ftdi lib */
	pfunc->libftdi = dlopen("libftd2xx.so", RTLD_LAZY);
	if (pfunc->libftdi == NULL) {
		fprintf(stderr,
			"%s: cannot access libftd2xx.so!\n", __func__);
		ftdi_destroy(pfunc);

		return NULL;
	}
	i = 0;
	ppfunc = (void *)pfunc;
	do {
		while (ftdi_symbols[i] != NULL) {
			*ppfunc = dlsym(pfunc->libftdi, ftdi_symbols[i]);
			if (*ppfunc == NULL) {
				fprintf(stderr,
					"%s: error get symbol '%s' from libftdi!\n",
					__func__, ftdi_symbols[i]);
				break;
			}
			i++;
			ppfunc++;
		}
		/* check if all fct pointers are connected */
		if (i < ((sizeof(*pfunc) - sizeof(void *)) / sizeof(ppfunc))) {
			fprintf(stderr,
				"%s: could not attach all function pointers!\n",
				__func__);
			break;
		}

		return pfunc;
	} while (0);

	return NULL;
}
