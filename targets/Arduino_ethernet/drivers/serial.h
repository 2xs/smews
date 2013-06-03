#ifndef __SERIAL_H__
#define __SERIAL_H__

extern volatile char stringBuffer[];
//extern char *stringBuffer;

void serialInit(void);
void putchr(unsigned char c);
void displayString(char *string);

#endif
