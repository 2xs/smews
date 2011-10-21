/*
  Author: Michael Hauspie <michael.hauspie@univ-lille1.fr>
  Created: 
  Time-stamp: <2011-08-26 10:06:06 (hauspie)>
*/
#ifndef __MBED_ETH_DEBUG_H__
#define __MBED_ETH_DEBUG_H__

#define ENABLE_MBED_DEBUG

#ifdef ENABLE_MBED_DEBUG
#define MBED_DEBUG printf
#else
static inline void nothing(const char *, ...){}
#define MBED_DEBUG nothing
#endif

#endif
