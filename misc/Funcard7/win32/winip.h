
#ifdef WIN32

#ifndef WIN32_NET_STRUCT
#define WIN32_NET_STRUCT

typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#define socklen_t int

/* ************************************* */

struct iphdr {
#if BYTE_ORDER == LITTLE_ENDIAN
        u_char  ihl:4,                /* header length */
                version:4;                 /* version */
#else
        u_char  version:4,                 /* version */
                ihl:4;                /* header length */
#endif
        u_char  tos;                 /* type of service */
        short   tot_len;                 /* total length */
        u_short id;                  /* identification */
        short   frag_off;                 /* fragment offset field */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
        u_char  ttl;                 /* time to live */
        u_char  protocol;                   /* protocol */
        u_short check;                 /* checksum */
        u_int32_t  saddr,daddr;  /* source and dest address */
};

#endif

#endif

