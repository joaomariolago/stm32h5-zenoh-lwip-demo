#include "FreeRTOS.h"

#include "main.h"

#include "lwip/dhcp.h"
#include "lwip/netifapi.h"

#include "LWIP/App/DHCP/client.h"
#include "LWIP/Target/ifconfig.h"

/** Reference to network IP ready flag */
extern osEventFlagsId_t h_net_ready_event;

/** Local DHCP state machine */
__IO uint8_t dhcp_state = DHCP_OFF;

/**
 * @brief DHCP task
 *
 * @param argument Pointer to the network interface
 */
void dhcp_task(void *argument)
{
  struct netif *netif = (struct netif*)argument;

  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;
  char ip_str[16];

  for (;;)
  {
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
              ip4addr_aton(IF_IP_ADDR, &ipaddr);
              ip4addr_aton(IF_GW_ADDR, &netmask);
              ip4addr_aton(IF_MASK, &gw);
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

void start_dhcp(void)
{
  dhcp_state = DHCP_START;
}

void stop_dhcp(void)
{
  dhcp_state = DHCP_LINK_DOWN;
}
