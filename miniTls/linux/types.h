/*
 * types.h
 *
 *  Created on: Aug 10, 2009
 *      Author: alex
 */

#ifndef TYPES_H_
#define TYPES_H_

#include "stdint.h"

/* types definition
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
 typedef signed char int8_t; TODO*/

#define CONST_VOID_P_VAR const void *
#define CONST_VAR(type,name) type const name
#define PERSISTENT_VAR(type,name) type name


#endif /* TYPES_H_ */
