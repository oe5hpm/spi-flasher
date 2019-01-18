// SPDX-License-Identifier: GPL-2.0+
/*
 * FTDI library loading and initialization
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
# include <dlfcn.h>
# define LIBNAME		"libftd2xx.so"
#elif __MINGW32__
# include <windows.h>
#ifdef _WIN32_
#  define LIBNAME		"ftd2xx.dll"
#else
#  define LIBNAME		"ftd2xx64.dll"
#endif
#else
# error "unsupported platform !"
#endif
#include "libftdi.h"

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
	"FT_SetFlowControl",
#ifndef __MINGW32__
	"FT_SetVIDPID",
#endif
	NULL,

};

void ftdi_destroy(struct ftdi_funcptr_t *inst)
{
	if (inst->libftdi != NULL)
#ifdef __linux__
		dlclose(inst->libftdi);
#else
		FreeLibrary(inst->libftdi);
#endif
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
#ifdef __linux__
	pfunc->libftdi = dlopen(LIBNAME, RTLD_LAZY);
#else
	pfunc->libftdi = LoadLibrary(LIBNAME);
#endif
	if (pfunc->libftdi == NULL) {
		fprintf(stderr,
			"%s: cannot access %s!\n", __func__, LIBNAME);
		ftdi_destroy(pfunc);

		return NULL;
	}
	i = 0;
	ppfunc = (void *)pfunc;
	do {
		while (ftdi_symbols[i] != NULL) {
#ifdef __linux
			*ppfunc = dlsym(pfunc->libftdi, ftdi_symbols[i]);
#else
			*ppfunc = GetProcAddress(pfunc->libftdi,
						 ftdi_symbols[i]);
#endif
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
