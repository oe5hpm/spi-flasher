// SPDX-License-Identifier: GPL-2.0+
/*
 * FTDI library loading and initialization (platform abstraction)
 *
 * Copyright (C) 2018 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __LIBFTDI_H__
#define __LIBFTDI_H__

#ifdef __linux__
#include <libftdi-linux.h>
#elif __win32__
#include <libftdi-windows.h>
#else
#error unsupported platform!
#endif

#endif /* __LIBFTDI_H__ */
