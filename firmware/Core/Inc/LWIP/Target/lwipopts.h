/**
 * Copyright (C) 2019, STMicroelectronics, all right reserved.
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 ******************************************************************************
 */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/**
 * SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#define SYS_LIGHTWEIGHT_PROT 0

/**
 * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities.
 */
#define NO_SYS 0

/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT 4

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#define MEM_SIZE (14 * 1024)

/* Relocate the LwIP RAM heap pointer */
#define LWIP_RAM_HEAP_POINTER (0x20084000)

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB 10

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG 64

/* ---------- Core options ---------- */
#define LWIP_ETHERNET                1
#define LWIP_RAW                     1
#define LWIP_IPV4                    1
#define LWIP_IGMP                    1

/* ---------- Pbuf options ---------- */

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool */
#define PBUF_POOL_SIZE 24

/* LWIP_SUPPORT_CUSTOM_PBUF == 1: to pass directly MAC Rx buffers to the stack
   no copy is needed */
   #define LWIP_SUPPORT_CUSTOM_PBUF 1

/* ---------- Buffers Sizes ---------- */
#define DEFAULT_RAW_RECVMBOX_SIZE    16
#define DEFAULT_UDP_RECVMBOX_SIZE    16
#define DEFAULT_TCP_RECVMBOX_SIZE    16
#define DEFAULT_ACCEPTMBOX_SIZE      16
#define TCPIP_MBOX_SIZE              16

/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_HOSTNAME 1

/* ---------- TCP options ---------- */
#define LWIP_TCP_KEEPALIVE           1
#define TCP_LISTEN_BACKLOG           1
#define LWIP_NETIF_TX_SINGLE_PBUF    1
#define LWIP_MULTICAST_TX_OPTIONS    1
#define LWIP_DHCP_DOES_ACD_CHECK     0

#define LWIP_TCP 1
#define TCP_TTL 255

/* TCP Maximum segment size. */
#define TCP_MSS (1500 - 40) /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF (16 * TCP_MSS)

/* TCP receive window. */
#define TCP_WND (16 * TCP_MSS)

#define TCP_SND_QUEUELEN ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

/* ---------- Socket options ---------- */
#define LWIP_SOCKET_POLL             0
#define LWIP_SO_LINGER               1
#define LWIP_TIMEVAL_PRIVATE         0

/* ---------- ARP options ---------- */
#define LWIP_ARP            1
#define MEMP_NUM_ARP_QUEUE  20
#define DHCP_DOES_ARP_CHECK 0

/* ---------- ICMP options ---------- */
#define LWIP_ICMP 1

/* ---------- DHCP options ---------- */
#define LWIP_DHCP 1

/* ---------- UDP options ---------- */
#define LWIP_UDP 1
#define UDP_TTL 255

/* ---------- Statistics options ---------- */
#define LWIP_STATS          0
#define LWIP_STATS_DISPLAY  0
#define MEM_STATS           0
#define SYS_STATS           0
#define MEMP_STATS          0
#define LINK_STATS          0

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/

/*
The STM32H5xx allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
*/
#define CHECKSUM_BY_HARDWARE

#ifdef CHECKSUM_BY_HARDWARE
/* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
#define CHECKSUM_GEN_IP 0
/* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
#define CHECKSUM_GEN_UDP 0
/* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
#define CHECKSUM_GEN_TCP 0
/* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
#define CHECKSUM_CHECK_IP 0
/* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
#define CHECKSUM_CHECK_UDP 0
/* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
#define CHECKSUM_CHECK_TCP 0
/* CHECKSUM_GEN_ICMP==1: Check checksums by hardware for outgoing ICMP packets.*/
/* Hardware TCP/UDP checksum insertion not supported when packet is an IPv4 fragment*/
#define CHECKSUM_GEN_ICMP 1
/* CHECKSUM_CHECK_ICMP==0: Check checksums by hardware for incoming ICMP packets.*/
#define CHECKSUM_CHECK_ICMP 0
#else
/* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
#define CHECKSUM_GEN_IP 1
/* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
#define CHECKSUM_GEN_UDP 1
/* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
#define CHECKSUM_GEN_TCP 1
/* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
#define CHECKSUM_CHECK_IP 1
/* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
#define CHECKSUM_CHECK_UDP 1
/* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
#define CHECKSUM_CHECK_TCP 1
/* CHECKSUM_GEN_ICMP==1: Check checksums by hardware for outgoing ICMP packets.*/
#define CHECKSUM_GEN_ICMP 1
/* CHECKSUM_CHECK_ICMP==1: Check checksums by hardware for incoming ICMP packets.*/
#define CHECKSUM_CHECK_ICMP 1
#endif

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#define LWIP_NETCONN 0

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET 1
#define LWIP_SO_RCVTIMEO 1   // Enable socket receive timeout option
#define LWIP_SO_SNDTIMEO 1   // Enable socket send timeout option

/*
   ------------------------------------
   ---------- DNS options -------------
   ------------------------------------
*/

#define LWIP_DNS                    1

/*
   ------------------------------------
   ------ LWIP_NETIF_API options ------
   ------------------------------------
*/
/**
 * LWIP_NETIF_API==1: Enable NETIF API
 */
#define LWIP_NETIF_API 1

/*
   ------------------------------------
   ---------- httpd options ----------
   ------------------------------------
*/
/** Set this to 1 to include "fsdata_custom.c" instead of "fsdata.c" for the
 * file system (to prevent changing the file included in CVS) */
#define HTTPD_USE_CUSTOM_FSDATA 1
#define MEMP_DEBUG LWIP_DBG_ON
#define MEM_DEBUG LWIP_DBG_ON

/*
   ---------------------------------
   ---------- OS options ----------
   ---------------------------------
*/

#define TCPIP_THREAD_NAME "TCP/IP"
#define TCPIP_THREAD_STACKSIZE 8192
#define DEFAULT_THREAD_STACKSIZE 4096
#define TCPIP_THREAD_PRIO osPriorityHigh

#define LWIP_DEBUG LWIP_DBG_OFF
// #define NETIF_DEBUG LWIP_DBG_ON
// #define DHCP_DEBUG LWIP_DBG_ON
// #define UDP_DEBUG  LWIP_DBG_ON
// #define TCP_DEBUG  LWIP_DBG_ON
// #define MEMP_DEBUG LWIP_DBG_ON
// #define MEM_DEBUG LWIP_DBG_ON
// #define ICMP_DEBUG LWIP_DBG_ON

#endif /* __LWIPOPTS_H__ */
