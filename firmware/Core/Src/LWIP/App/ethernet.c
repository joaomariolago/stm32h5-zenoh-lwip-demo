#include <stdio.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "lwip/opt.h"
#include "lwip/netifapi.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif

#include "LWIP/App/ethernet.h"
#include "LWIP/Target/ethernetif.h"

/** Handlers */

struct netif h_netif;
osEventFlagsId_t h_net_ready_event;

/** Tasks Handlers */
osThreadId_t h_eth_link_task;
const osThreadAttr_t eth_link_task_attributes = {
  .name = "eth_link",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = configMINIMAL_STACK_SIZE * 8
};

#if LWIP_DHCP
/** State Machine Control */
__IO uint8_t dhcp_state = DHCP_OFF;

/** Task Control */
osThreadId_t h_dhcp_task;
const osThreadAttr_t dhcp_task_attributes = {
  .name = "dhcp",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = configMINIMAL_STACK_SIZE * 8
};
#endif

/** Tasks */

#if LWIP_DHCP
void dhcp_thread_entry(void *argument)
{
  struct netif *netif = (struct netif*)argument;

  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;
  char ip_str[16];

  for (;;)
  {
    /** Indicates to user that the DHCP is going on */
    if (dhcp_state == DHCP_START || dhcp_state == DHCP_WAIT_ADDRESS)
    {
      HAL_GPIO_TogglePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin);
    }
    else
    {
      HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_RESET);
    }

    switch (dhcp_state)
    {
      case DHCP_START:
        {
          ip_addr_set_zero_ip4(&netif->ip_addr);
          ip_addr_set_zero_ip4(&netif->netmask);
          ip_addr_set_zero_ip4(&netif->gw);

          dhcp_state = DHCP_WAIT_ADDRESS;

          netifapi_dhcp_start(netif);
        }
        break;
      case DHCP_WAIT_ADDRESS:
        {
          if (dhcp_supplied_address(netif))
          {
            dhcp_state = DHCP_ADDRESS_ASSIGNED;

            ip4_addr_set_u32(&ipaddr, netif_ip4_addr(netif)->addr);
            ip4addr_ntoa_r(&ipaddr, ip_str, sizeof(ip_str));

            osEventFlagsSet(h_net_ready_event, 0x01);
          }
          else
          {
            dhcp = (struct dhcp*)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

            /* DHCP timeout */
            if (dhcp->tries > MAX_DHCP_TRIES)
            {
              dhcp_state = DHCP_TIMEOUT;

              /* Static address used */
              IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
              IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
              IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
              netifapi_netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

              ip4_addr_set_u32(&ipaddr, netif_ip4_addr(netif)->addr);
              ip4addr_ntoa_r(&ipaddr, ip_str, sizeof(ip_str));

              osEventFlagsSet(h_net_ready_event, 0x01);
            }
          }
        }
        break;
      case DHCP_LINK_DOWN:
        {
          dhcp_state = DHCP_OFF;
        }
        break;
      default:
        break;
    }

    osDelay(DHCP_TASK_INTERVAL_MS);
  }
}
#endif /* LWIP_DHCP */

void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
#if LWIP_DHCP
    /* Update DHCP state machine */
    dhcp_state = DHCP_START;
#else
    ip_addr_t ipaddr;
    char ip_str[16];
    ip4_addr_set_u32(&ipaddr, netif_ip4_addr(netif)->addr);
    ip4addr_ntoa_r(&ipaddr, ip_str, sizeof(ip_str));

    osEventFlagsSet(h_net_ready_event, 0x01);
#endif

    /** Remove RED led when link is up */
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  }
  else
  {
#if LWIP_DHCP
    /* Update DHCP state machine */
    dhcp_state = DHCP_LINK_DOWN;
#endif

    osEventFlagsClear(h_net_ready_event, 0x01);

    /** RED led for user while link is down */
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
  }
}

/** Implementations */

void net_if_config(void)
{
  h_net_ready_event = osEventFlagsNew(NULL);

  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

#if LWIP_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
  IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

  /* Add the network interface */
  netif_add(&h_netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface. */
  netif_set_default(&h_netif);
  ethernet_link_status_updated(&h_netif);

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&h_netif, ethernet_link_status_updated);
  h_eth_link_task = osThreadNew(ethernet_link_thread, &h_netif, &eth_link_task_attributes);
#endif

#if LWIP_DHCP
  h_dhcp_task = osThreadNew(dhcp_thread_entry, &h_netif, &dhcp_task_attributes);
#endif
}
