// SPDX-License-Identifier: GPL-2.0+
/*
 * FTDI library loading and initialization
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __LIBFTDI_LINUX_H__
#define __LIBFTDI_LINUX_H__

#include <ftd2xx.h>

struct ftdi_funcptr_t {
	FT_STATUS(*get_libversion)(LPDWORD pversion);
	FT_STATUS(*createdevlist)(LPDWORD pnumdevs);
	FT_STATUS(*get_devinfo) \
		(FT_DEVICE_LIST_INFO_NODE * pdst, LPDWORD pnumdevs);
	FT_STATUS(*open)(int device, FT_HANDLE * handle);
	FT_STATUS(*close)(FT_HANDLE handle);
	FT_STATUS(*reset)(FT_HANDLE handle);
	FT_STATUS(*purge)(FT_HANDLE handle, DWORD mask);
	FT_STATUS(*set_usbpar) \
		(FT_HANDLE handle, DWORD insize, DWORD outsize);
	FT_STATUS(*set_chars)\
		(FT_HANDLE handle, \
		 UCHAR event, UCHAR event_en, UCHAR err, UCHAR err_en);
	FT_STATUS(*set_timeout) \
		(FT_HANDLE handle, DWORD readtimeout, DWORD writetimeout);
	FT_STATUS(*set_latency)(FT_HANDLE handle, DWORD time);
	FT_STATUS(*set_bitmode)(UCHAR mask, UCHAR mode);
	FT_STATUS(*get_queuestat)(FT_HANDLE handle, LPDWORD pavail);
	FT_STATUS(*get_stat)(FT_HANDLE handle, \
			LPDWORD rxqueue, LPDWORD txqueue, LPDWORD eventstatus);
	FT_STATUS(*read) \
		(FT_HANDLE handle, LPVOID buf, DWORD size, LPDWORD read);
	FT_STATUS(*write) \
		(FT_HANDLE handle, LPVOID buf, DWORD size, LPDWORD written);
	FT_STATUS(*get_deviceinfo) \
		(FT_HANDLE handle, \
		 FT_DEVICE * pdev, LPDWORD devid, \
		 PCHAR serial, PCHAR description, LPVOID dummy);
	FT_STATUS(*set_vidpid)(DWORD vid, DWORD pid);
	void *libftdi;
};

void ftdi_destroy(struct ftdi_funcptr_t *inst);
struct ftdi_funcptr_t *ftdi_create(void);

#endif /* __LIBFTDI_LINUX_H__ */
