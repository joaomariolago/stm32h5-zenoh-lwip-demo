#ifndef _LWIP_APP_ETHERNET_H_
#define _LWIP_APP_ETHERNET_H_

#include "cmsis_os2.h"

#include "lwip/netif.h"
#include "LWIP/Target/lwipopts.h"

/** Definitions */

/* Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0        ((uint8_t) 192U)
#define IP_ADDR1        ((uint8_t) 168U)
#define IP_ADDR2        ((uint8_t) 1U)
#define IP_ADDR3        ((uint8_t) 101U)

/* NETMASK */
#define NETMASK_ADDR0   ((uint8_t) 255U)
#define NETMASK_ADDR1   ((uint8_t) 255U)
#define NETMASK_ADDR2   ((uint8_t) 255U)
#define NETMASK_ADDR3   ((uint8_t) 0U)

/* Gateway Address */
#define GW_ADDR0        ((uint8_t) 192U)
#define GW_ADDR1        ((uint8_t) 168U)
#define GW_ADDR2        ((uint8_t) 1U)
#define GW_ADDR3        ((uint8_t) 1U)

/* DHCP process states */
#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5

#define MAX_DHCP_TRIES             (4U)
#define DHCP_TASK_INTERVAL_MS      (250U)

/** Handlers */

extern struct netif h_netif;
extern osEventFlagsId_t h_net_ready_event;

/** Prototypes */

void net_if_config(void);

#endif /* _LWIP_APP_ETHERNET_H_ */
