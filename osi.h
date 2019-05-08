// SPDX-License-Identifier: GPL-2.0+
/*
 * OS interface / abstraction
 *
 * Copyright (C) 2019 Hannes Schmelzer <oe5hpm@oevsv.at>
 *
 */
#ifndef __MINGW32__
# include <unistd.h>
#else
# include <windows.h>
#endif /* __MINGW32__ */

#ifdef __linux__
static __inline__ void _usleep(unsigned int us)
{
	usleep(us);
}
#else
static __inline__ void _usleep(unsigned int us)
{
	__int64 t1, t2, freq, cmp;

	if (us >= 1000)
		return Sleep(us / 1000);

	QueryPerformanceCounter((LARGE_INTEGER *) &t1);
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	cmp = us * (freq / 1000000);
	do {
		QueryPerformanceCounter((LARGE_INTEGER *)&t2);
	} while ((t2 - t1) < cmp);
}
#endif /* __linux__ */
