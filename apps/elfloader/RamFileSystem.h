#ifndef __RAM_FILE_SYSTEM_H__
#define __RAM_FILE_SYSTEM_H__

extern void *rfs_open(const char *aMemoryBank);
extern void rfs_close(void *aHandle);

extern int rfs_tell(const void *aHandle);
extern int rfs_seek(void *handle, int anOffset);
extern int rfs_write(const void *aBuffer, int aLength, int aCount, void *aHandle);
extern int rfs_read(void *aBuffer, int aLength, int aCount, void *aHandle);

#endif
