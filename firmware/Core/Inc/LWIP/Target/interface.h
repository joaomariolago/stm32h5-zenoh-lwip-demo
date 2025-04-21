#ifndef _LWIP_APP_ETHERNET_IF_H_
#define _LWIP_APP_ETHERNET_IF_H_

#include "lwip/err.h"
#include "lwip/netif.h"

err_t eth_interface_init(struct netif *netif);

#endif /* _LWIP_APP_ETHERNET_IF_H_ */
