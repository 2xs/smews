#include "enc624J600.h"



void nicInit(void)
{
	ENC624J600Init();
	
}

//maj 04_05_12
void nicSend(unsigned int len, unsigned char* ptrpacket)
{
	ENC624J600PacketSend(len, ptrpacket);
}

//maj 04_05_12
#if 0
unsigned int nicPoll(unsigned int maxlen)
{
	return ENC624J600PacketReceive(maxlen);
}
#endif
void nicGetMacAddress(u08* macaddr)
{
	// read MAC address registers
	*macaddr++ = ENC624J600Read(MAADR1H);
	*macaddr++ = ENC624J600Read(MAADR1L);
	*macaddr++ = ENC624J600Read(MAADR2H);
	*macaddr++ = ENC624J600Read(MAADR2L);
	*macaddr++ = ENC624J600Read(MAADR3H);
	*macaddr++ = ENC624J600Read(MAADR3L);
}
//MAJ 01_06_12
void nicSetMacAddress(u08* macaddr)
{
	// write MAC address
	ENC624J600Write(MAADR1L, *macaddr++);
	ENC624J600Write(MAADR1H, *macaddr++);
	ENC624J600Write(MAADR2L, *macaddr++);
	ENC624J600Write(MAADR2H, *macaddr++);
	ENC624J600Write(MAADR3L, *macaddr++);
	ENC624J600Write(MAADR3H, *macaddr++);
}
