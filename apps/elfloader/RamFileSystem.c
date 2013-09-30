
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
  rfs->current = rfs->base + anOffset;

  return anOffset;
}

int rfs_write(const void *aBuffer, int aLength, int aCount, void *aHandle) {
  return -1;
}

int rfs_read(void *aBuffer, int aLength, int aCount, void *aHandle) {
  struct RamFileSystem *rfs = (struct RamFileSystem *)aHandle;

  fake_memcpy(aBuffer, rfs->current, aLength * aCount);
  rfs->current += aLength * aCount;
  return (aLength * aCount);
}
