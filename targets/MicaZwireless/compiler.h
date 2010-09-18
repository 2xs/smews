#ifndef COMPILER_H
#define COMPILER_H



/******************************************************************************
*******************              Commonly used types        *******************
******************************************************************************/
typedef unsigned char       BOOL;

// Data
typedef unsigned char       BYTE;
typedef unsigned int        WORD;
typedef unsigned long int   DWORD;

// Unsigned numbers
typedef unsigned char       UINT8;
typedef unsigned int        UINT16;
typedef unsigned long int   UINT32;

// Signed numbers
typedef signed char         INT8;
typedef signed int          INT16;
typedef signed long int     INT32;

#ifndef FALSE
   #define FALSE 0
#endif

#ifndef TRUE
   #define TRUE 1
#endif

#ifndef NULL
   #define NULL 0
#endif


#define BM(n) (1 << (n))
#define BF(x,b,s) (((x) & (b)) >> (s))
#define MIN(n,m) (((n) < (m)) ? (n) : (m))
#define MAX(n,m) (((n) < (m)) ? (m) : (n))
#define ABS(n) ((n < 0) ? -(n) : (n))




#endif


