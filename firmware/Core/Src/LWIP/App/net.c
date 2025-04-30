#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "lwip/opt.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"

#include "LWIP/App/net.h"
#include "LWIP/Target/link.h"
#include "LWIP/Target/interface.h"
#include "LWIP/App/DHCP/client.h"
#include "LWIP/App/FTP/server.h"

/** Handlers */
struct netif netif;
osEventFlagsId_t h_net_ready_event;

/** Tasks Handlers */
osThreadId_t h_eth_link_task;
const osThreadAttr_t eth_link_task_attributes = {
  .name = "EthLink",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = configMINIMAL_STACK_SIZE
};

osThreadId_t h_dhcp_task;
const osThreadAttr_t dhcp_task_attributes = {
  .name = "DHCP",
  .priority = (osPriority_t)osPriorityBelowNormal,
  .stack_size = configMINIMAL_STACK_SIZE
};

/** Implementations */

void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
    start_dhcp();
  }
  else
  {
    /* Update DHCP state machine */
    stop_dhcp();
    osEventFlagsClear(h_net_ready_event, 0x01);
  }
}

void net_if_config(void)
{
  h_net_ready_event = osEventFlagsNew(NULL);

  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);

  /* Add the network interface */
  netif_add(&netif, &ipaddr, &netmask, &gw, NULL, &eth_interface_init, &tcpip_input);

  /* Registers the default network interface. */
  netif_set_default(&netif);
  ethernet_link_status_updated(&netif);
  netif_set_link_callback(&netif, ethernet_link_status_updated);

  /** Start Network tasks */
  h_eth_link_task = osThreadNew(ethernet_link_task, &netif, &eth_link_task_attributes);

  /** DHCP */
  h_dhcp_task = osThreadNew(dhcp_task, &netif, &dhcp_task_attributes);

  /** FTP */
  //ftpd_server_init();
}

void network_start(void)
{
  /* Initialize the LwIP stack */
  tcpip_init(NULL, NULL);

  /** Configures the interface and provides event to notify when ready */
  net_if_config();
}
