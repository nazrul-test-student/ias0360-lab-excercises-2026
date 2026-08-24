#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Required settings
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0
#define LWIP_IPV4                   1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DNS                    1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define MEM_ALIGNMENT               4
#define MEMP_NUM_PBUF               16
#define MEMP_NUM_UDP_PCB            4
#define MEMP_NUM_TCP_PCB            4
#define MEMP_NUM_TCP_PCB_LISTEN     2
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_SYS_TIMEOUT        10
#define PBUF_POOL_SIZE              16
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (2*TCP_MSS)
#define TCP_SND_QUEUELEN            (4*TCP_SND_BUF/TCP_MSS)
#define TCP_WND                     (2*TCP_MSS)

#endif /* _LWIPOPTS_H */