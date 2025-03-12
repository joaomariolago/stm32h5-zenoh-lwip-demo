#ifndef _LWIP_APP_ETHERNET_IF_H_
#define _LWIP_APP_ETHERNET_IF_H_

#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os2.h"

err_t ethernetif_init(struct netif *netif);
void ethernet_link_thread(void *argument);

#endif /* _LWIP_APP_ETHERNET_IF_H_ */
