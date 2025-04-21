#ifndef _LWIP_APP_ETHERNET_H_
#define _LWIP_APP_ETHERNET_H_

#include "cmsis_os2.h"
#include "lwip/netif.h"

#include "LWIP/lwipopts.h"
#include "LWIP/Target/ifconfig.h"

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

void network_start(void);

#endif /* _LWIP_APP_ETHERNET_H_ */
