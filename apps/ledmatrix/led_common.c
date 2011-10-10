/*
* Copyright or © or Copr. 2011, Michael Hauspie
*
* Author e-mail: michael.hauspie@lifl.fr
*
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
*
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use,
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info".
*
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability.
*
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or
* data to be ensured and,  more generally, to use and operate it in the
* same conditions as regards security.
*
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/
#include <stdint.h>

char text[128];
uint8_t color;
char color_text[10];
volatile int position;
int text_size;


uint8_t htoi(char hexa)
{
   if (hexa >= '0' && hexa <= '9')
      return hexa - '0';
   if (hexa >= 'a' && hexa <= 'f')
      return hexa - 'a' + 10;
   if (hexa >= 'A' && hexa <= 'F')
      return hexa - 'A' + 10;
   return 0;
}


int strlen(const char *str)
{
    const char *s = str;
    while (*s++);
    return s-str;
}


void strcpy(char *dst, const char *src)
{
    for (;*src;dst++,src++)
    {
	if (*src == '%')
	{
	    int val;
	    src++;
	    if (*src == '%')
	    {
		*dst = *src;
		continue;
	    }
	    if (*src == '\0' || *(src+1) == '\0')
	    {
		*dst = '\0';
		return;
	    }
	    val = htoi(*src)*16 + htoi(*(src+1));
	    src++;
	    *dst = val;
	}
	else
	    *dst = *src;
    }
    *dst = '\0';
}
