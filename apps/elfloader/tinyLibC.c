#include "tinyLibC.h"

/* not written for speed*/
int memcmp(const void *aPointer, const void *anotherPointer, int aSize) {
  const char *pointer = (const char *) aPointer;
  const char *nother  = (const char *) aPointer;
  int i = 0;
  for(i = 0; i < aSize; i++) {
    if(pointer[i] != nother[i])
	return pointer[i] - nother[i];
  }
  return 0;
}

int strlen(const char *aString) {
  int i = 0;
  while(aString[i] != '\0')
	i++;
  return i;
}

int strcmp(const char *aString, const char *anotherString) {
  while (*aString == *anotherString && *aString != '\0')
  {
	aString++;anotherString++;
  }
  return *aString - *anotherString;
/*

  int length = strlen(aString);
  int diff =  length - strlen(anotherString);
  if(diff == 0)
	return memcmp(aString, anotherString, length);
  return diff;*/
}

char toLower(char aChar) {
  if(aChar>= 'a' && (aChar <= 'z'))
	return aChar;
  return (aChar>= 'A' && (aChar <= 'Z')) ? aChar - 'A' + 'a' : aChar;
}

int strncmp(const char *aString, const char *anotherString, int aSize) {
  while (*aString == *anotherString && *aString != '\0' && --aSize) {
	aString++;anotherString++;
  }
  return *aString - *anotherString;
/*  int diff, i;
  for(i = 0; i < aSize; i++) {
    diff = toLower(aString[i]) - toLower(anotherString[i]);
    if(diff != 0)
      return diff;
  }
  return 0;*/
}

void fake_memcpy(void *dest, const void *source, int len) {
  int i;
  char *      dst = (char *)dest;
  const char *src = (const char *)source;
  for(i = 0; i < len; i++)
	dst[i] = src[i];
}
