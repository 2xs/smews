#ifndef __TINY_LIB_C_H__
#define __TINY_LIB_C_H__

#ifndef LINUX
extern int memcmp(const void *aPointer, const void *anotherPointer, int aSize);
extern int strlen(const char *aString);
extern int strcmp(const char *aString, const char *anotherString);
extern int strncmp(const char *aString, const char *anotherString, int aSize);
extern char toLower(char aChar);
extern void fake_memcpy(void *dest, const void *source, int len);
#endif

#endif
