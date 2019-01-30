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
#elif __MINGW32__
# include <windows.h>
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
#ifndef __MINGW32__
	"FT_SetVIDPID",
#endif
	"FT_SetFlowControl",
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
	char *libname = "ftdi-null-lib";
	unsigned int i;
	unsigned int errorcode = 0;

	pfunc = calloc(1, sizeof(struct ftdi_funcptr_t));
	if (pfunc == NULL) {
		fprintf(stderr,
			"%s: no mem to create ftdi instance!\n", __func__);
		return NULL;
	}

	/* open the shared ftdi lib */
#ifdef __linux__
	libname = "libftd2xx.so";
	pfunc->libftdi = dlopen(libname, RTLD_LAZY);
#else
	SYSTEM_INFO sysinfo = { 0 };
	GetNativeSystemInfo(&sysinfo);

	if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		libname = "ftd2xx.dll";
	else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		libname = "ftd2xx64.dll";
	else
		fprintf(stderr, "%s: unknown architecure!\n", __func__);

	pfunc->libftdi = LoadLibrary(libname);
#endif
	if (pfunc->libftdi == NULL) {
#ifdef __MINGW32__
		errorcode = GetLastError();
#endif
		fprintf(stderr,
			"%s: cannot access %s (err=%d)!\n",
			__func__, libname, errorcode);
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
