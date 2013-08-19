
#ifdef LINUX
#include <stdio.h>
#include <string.h>
#else
#include <rflpc17xx/rflpc17xx.h>
#endif

#include "tinyLibC.h"

struct RamFileSystem {
	const char *base;
	const char *current;
};

struct RamFileSystem RFS = {
	NULL, NULL
};

void *rfs_open(const char *aMemoryBank) {
  RFS.base    = aMemoryBank;
  RFS.current = aMemoryBank;
  printf("rfs_open : base : 0x%x\r\n", aMemoryBank);
  return &RFS;
}

void rfs_close(void *aHandle) {
  RFS.base    = 0;
  RFS.current = 0;
}

int rfs_tell(const void *aHandle) {
  const struct RamFileSystem *rfs = (struct RamFileSystem *)aHandle;
  return (rfs->current - rfs->base);
}

int rfs_seek(void *aHandle, int anOffset) {
  struct RamFileSystem *rfs = (struct RamFileSystem *)aHandle;
/*  printf("SEEK rfs: %p, base %p offset: %d\r\n", rfs, rfs->base, anOffset);
  printf("SEEK current: %p\r\n", rfs->current);*/
  rfs->current = rfs->base + anOffset;
/*  printf("-SEEK current: %p\r\n", rfs->current);*/

  return anOffset;
}

int rfs_write(const void *aBuffer, int aLength, int aCount, void *aHandle) {
  return -1;
}

int rfs_read(void *aBuffer, int aLength, int aCount, void *aHandle) {
  struct RamFileSystem *rfs = (struct RamFileSystem *)aHandle;
/*  printf("read from %p->%p to %p (%d bytes)\r\n", rfs, rfs->current, aBuffer, aLength*aCount);*/
/*  printf("rfsRead 0x%x count %d\r\n", rfs->current, aCount * aLength);*/
  fake_memcpy(aBuffer, rfs->current, aLength * aCount);
  rfs->current += aLength * aCount;
  return (aLength * aCount);
}
