/*
*Lib Universal TUN (libutun) Copyright(C) Geoffroy Cogniaux < geoffroy@cogniaux.com >
*
*Lib Universal TUN is free software; you can redistribute it and / or
*modify it under the terms of the GNU General Public
*License as published by the Free Software Foundation; either
*version 2.1 of the License, or(at your option)any later version.
*
*Lib Universal TUN is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*General Public License for more details.
*
*You should have received a copy of the GNU General Public
*License along with Lib Universal TUN; if not, write to the Free Software
*Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA
*
**
*Lib Universal TUN uses TAP-Win32 on Windows(r)
* -- A kernel driver to provide virtual tap device
*             functionality on Windows.  Originally derived
*             from the CIPE-Win32 project by Damion K. Wilson,
*             with extensive modifications by James Yonan.
*
*All original source code using TAP-Win32 which derives from the CIPE-Win32 project is
*Copyright (C) Damion K. Wilson, 2003, and is released under the
*GPL version 2.
*
*All other original source code using TAP-Win32 is 
*Copyright (C) 2002-2006 OpenVPN Solutions LLC,
*and is released under the GPL version 2 .



On Windows, point-to-point IP support (i.e. --dev tun)
is emulated by the TAP-Win32 driver.  The major limitation
imposed by this approach is that the --ifconfig local and
remote endpoints must be part of the same 255.255.255.252
subnet.  The following list shows examples of endpoint
pairs which satisfy this requirement.  Only the final
component of the IP address pairs is at issue.

As an example, the following option would be correct:
    --ifconfig 10.7.0.5 10.7.0.6 (on host A)
    --ifconfig 10.7.0.6 10.7.0.5 (on host B)
because [5,6] is part of the below list.

[  1,  2] [  5,  6] [  9, 10] [ 13, 14] [ 17, 18]
[ 21, 22] [ 25, 26] [ 29, 30] [ 33, 34] [ 37, 38]
[ 41, 42] [ 45, 46] [ 49, 50] [ 53, 54] [ 57, 58]
[ 61, 62] [ 65, 66] [ 69, 70] [ 73, 74] [ 77, 78]
[ 81, 82] [ 85, 86] [ 89, 90] [ 93, 94] [ 97, 98]
[101,102] [105,106] [109,110] [113,114] [117,118]
[121,122] [125,126] [129,130] [133,134] [137,138]
[141,142] [145,146] [149,150] [153,154] [157,158]
[161,162] [165,166] [169,170] [173,174] [177,178]
[181,182] [185,186] [189,190] [193,194] [197,198]
[201,202] [205,206] [209,210] [213,214] [217,218]
[221,222] [225,226] [229,230] [233,234] [237,238]
[241,242] [245,246] [249,250] [253,254]

*/


#ifdef WIN32
#include "utun.h"

int  utun_write (utun *t, unsigned char *buf, int len)
{
  DWORD write_size, last_err;

  ResetEvent(t->overlap_write.hEvent);
  if(WriteFile(t->device_handle,buf,len,&write_size,&t->overlap_write)) 
  {
    return write_size;
  }
  switch (last_err = GetLastError()) 
  {
  case ERROR_IO_PENDING:
      {
        WaitForSingleObject(t->overlap_write.hEvent, INFINITE);
        GetOverlappedResult(t->device_handle, &t->overlap_write,
			&write_size, FALSE);
        return write_size;
      }
    break;
  default:
      {
        LPVOID lpMsgBuf; 
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            last_err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        printf( "utun_write failed with error %d: %s\n", last_err, lpMsgBuf );
      }
    break;
  }

  return -1;
}

int  utun_read  (utun *t, unsigned char *buf, int len)
{
  DWORD read_size, last_err;

  ResetEvent(t->overlap_read.hEvent);
  if (ReadFile(t->device_handle, buf, len, &read_size, &t->overlap_read)) 
  {
    return read_size;
  }
  switch (last_err = GetLastError()) 
  {
  case ERROR_IO_PENDING:
      {
        WaitForSingleObject(t->overlap_read.hEvent, INFINITE);
        GetOverlappedResult(t->device_handle, &t->overlap_read, &read_size, FALSE);    
        return read_size;
      }
    break;
  default:
      {
        LPVOID lpMsgBuf; 
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            last_err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        printf( "utun_read failed with error %d: %s\n", last_err, lpMsgBuf );
      }
    break;
  }

  return -1;
}

int  utun_open  (utun *t, char *ip_src, char *ip_dest,unsigned long mtu)
{
  HKEY key, key2;
  long rc;
  char regpath[1024], cmd[256];
  char adapterid[1024];
  char adaptername[1024];
  char tapname[1024];
  long len;
  int found = 0;
  int err, i;
  unsigned long status = TRUE;
#ifdef _DEBUG
  ULONG info[3];
#endif

  memset(t, 0, sizeof(utun));
  t->device_handle = INVALID_HANDLE_VALUE;
  t->ip_src = utun_inet_addr(ip_src);

  /* Open registry and look for a working TAP-WIN32 network adapters */
  if((rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NETWORK_CONNECTIONS_KEY, 0, KEY_READ, &key))) 
  {
    printf("Unable to read registry: [rc=%d]\n", rc);
    return -1;
  }

  for (i = 0; ; i++) 
  {
    len = sizeof(adapterid);
    if(RegEnumKeyEx(key, i, (LPCWSTR)adapterid, &len, 0, 0, 0, NULL))
      break;

    /* Find out more about this adapter */

    _snprintf(regpath, sizeof(regpath), "%s\\%s\\Connection", NETWORK_CONNECTIONS_KEY, adapterid);
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCSTR)regpath, 0, KEY_READ, &key2))
      continue;

    len = sizeof(adaptername);
    err = RegQueryValueEx(key2, "Name", 0, 0, adaptername, &len);

    RegCloseKey(key2);

    if(err)
      continue;

    _snprintf(tapname, sizeof(tapname), USERMODEDEVICEDIR "%s" TAPSUFFIX, adapterid);
    t->device_handle = CreateFile(tapname, 
                       GENERIC_WRITE | GENERIC_READ,
				       0,0, 
                       OPEN_EXISTING, 
                       FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
                       0);
    if(t->device_handle != INVALID_HANDLE_VALUE) 
    {
      found = 1;
      break;
    }
    else
    {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        
        if(dw!=2)//File not found .
        {
            //could inform that a TAP is found but isn't working well.
            printf("Adapter %s, file %s\n",adaptername,tapname);
            printf( "utun_open failed with error %d: %s\n", dw, lpMsgBuf );
        }
    }
  }//end for

  RegCloseKey(key);

  if(!found) 
  {
    printf("No Windows tap device found!\n");
    return -1;
  }

  /* Try to open the useful TAP-Win32 driver !! */

  if(t->device_handle == INVALID_HANDLE_VALUE) 
  {
    _snprintf(tapname, sizeof(tapname), USERMODEDEVICEDIR "%s" TAPSUFFIX, adapterid);
    t->device_handle = 
        CreateFile(tapname, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, 
        FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
  }

  if(t->device_handle == INVALID_HANDLE_VALUE) 
  {
    printf("%s (%s) is not a usable TAP-Win32\n", adaptername, adapterid);
    return -1;
  }

#ifdef _DEBUG
  if (DeviceIoControl (t->device_handle, TAP_IOCTL_GET_VERSION,
			 &info, sizeof (info),
			 &info, sizeof (info), &len, NULL))
  {
	printf( "Loading TAP-Win32 Driver Version %d.%d %s\n",
	     (int) info[0],
	     (int) info[1],
	     (info[2] ? "(DEBUG)" : ""));

  }
#endif

  //Configure as PtoP, require to emulate TUN
  if(t->device_handle)
  {
	  ULONG ep[2];
	  ep[0] = utun_inet_addr(ip_src);
	  ep[1] = utun_inet_addr(ip_dest);

	  if (!DeviceIoControl (t->device_handle, TAP_IOCTL_CONFIG_POINT_TO_POINT,
				ep, sizeof (ep),
				ep, sizeof (ep), &len, NULL))
      {
	    printf( "ERROR: The TAP-Win32 driver rejected a DeviceIoControl call to set Point-to-Point mode");
        return -1;
      }
  }

  _snprintf(cmd, sizeof(cmd),
	    "netsh interface ip set address \"%s\" static %s 255.255.255.252",adaptername,ip_src);
#ifdef _DEBUG
  printf("Setting to \"%s\" device a correct address, please wait...\n", adaptername);
  printf("Excuting: %s\n", cmd);
#endif
  if(system(cmd) == 0) 
  {
    t->ip_src=  utun_inet_addr(ip_src);
    t->ip_dest= utun_inet_addr(ip_dest);
#ifdef _DEBUG
    printf("Device \"%s\" set from %s to %s\n", adaptername, ip_src, ip_dest);
#endif
  } 
  else
  {
    _snprintf(cmd, sizeof(cmd),"netsh interface ip show config \"%s\"",adaptername);
    printf("WNG: Unable to set correctly IP addresses, please check it manually.\n", adaptername);
    system(cmd);
  }

  /* try to change MTU */
  if(mtu >= 100 && mtu <= 1500)// TAP-WIN32 V8 constraint
  {
      char cfgid[1024];
      char keyid[32];
      char str_mtu[32];

      itoa(mtu,str_mtu,10);

      if((rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ADAPTER_KEY, 0, KEY_READ, &key))) 
      {
        printf("Unable to read registry: [rc=%d]\n", rc);
        return -1;
      }
      for (i = 0; ; i++) 
      {
        int len = sizeof(keyid);
        if(RegEnumKeyEx(key, i, (LPCWSTR)keyid, &len, 0, 0, 0, NULL))
          break;

        _snprintf(regpath, sizeof(regpath), "%s\\%s\\", ADAPTER_KEY, keyid);
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCSTR)regpath, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &key2))
          continue;

        len = sizeof(cfgid);
        err = RegQueryValueEx(key2, "NetCfgInstanceId", 0, 0, cfgid, &len);

        if(strcmp(adapterid,cfgid)==0)
        {
            RegSetValueEx(key2,"MTU",0,REG_SZ,(LPCTSTR)&str_mtu,sizeof(DWORD)); 
                
            RegCloseKey(key2);
            break;
        }
        else
            continue;
      }
      RegCloseKey(key);

  }
  
  
  /* set driver media status to 'connected' (i.e. set the interface up) */
  if (!DeviceIoControl (t->device_handle, TAP_IOCTL_SET_MEDIA_STATUS,
			&status, sizeof (status),
			&status, sizeof (status), &len, NULL))
  {
    printf("WARNING: Unable to enable TAP adapter\n");
    return -1;
  }


  t->overlap_read.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  t->overlap_write.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  if (!t->overlap_read.hEvent || !t->overlap_write.hEvent) 
  {
    printf("WARNING: Unable to create Overlapped Events\n");
    return -1;
  }

  return 0;
}

void utun_close (utun *t)
{
    CancelIo(t->device_handle);
    CloseHandle(t->overlap_read.hEvent);
    CloseHandle(t->overlap_write.hEvent);
    CloseHandle(t->device_handle);
}

int  utun_select(utun *t)
{
    
    return 0;
}


__inline unsigned short utun_htons(unsigned short n)
{
  return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

__inline unsigned short utun_ntohs(unsigned short n)
{
  return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

__inline unsigned long utun_htonl(unsigned long n)
{
  return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

__inline unsigned long utun_ntohl(unsigned long n)
{
  return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

unsigned long utun_inet_addr(const char *src)
{
    unsigned int val;
    int base, n;
    unsigned char c;
    unsigned int parts[4];
    unsigned int *pp = parts;

    c = *src;
    for (;;) {
            /*
             * Collect number up to ``.''.
             * Values are specified as for C:
             * 0x=hex, 0=octal, isdigit=decimal.
             */
            if (!isdigit(c))
                    return (0);
            val = 0; base = 10;
            if (c == '0') {
                    c = *++src;
                    if (toupper(c) == 'X')
                            base = 16, c = *++src;
                    else
                            base = 8;
            }
            for (;;) {
                    if (isdigit(c)) {
                            val = (val * base) + (c - '0');
                            c = *++src;
                    } else if (base == 16 && isxdigit(toupper(c))) {
                            val = (val << 4) |
                                    (toupper(c) + 10 - 'A');
                            c = *++src;
                    } else
                    break;
            }
            if (c == '.') {
                    /*
                     * Internet format:
                     *      a.b.c.d
                     *      a.b.c   (with c treated as 16 bits)
                     *      a.b     (with b treated as 24 bits)
                     */
                    if (pp >= parts + 3)
                            return (0);
                    *pp++ = val;
                    c = *++src;
            } else
                    break;
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && !isspace(c))
            return (0);
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = pp - parts + 1;
    switch (n) {

    case 0:
            return (0);             /* initial nondigit */

    case 1:                         /* a -- 32 bits */
            break;

    case 2:                         /* a.b -- 8.24 bits */
            if (val > 0xffffff)
                    return (0);
            val |= parts[0] << 24;
            break;

    case 3:                         /* a.b.c -- 8.8.16 bits */
            if (val > 0xffff)
                    return (0);
            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

    case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
            if (val > 0xff)
                    return (0);
            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
    }

    val = utun_htonl(val);
    return (val);
}




#endif

