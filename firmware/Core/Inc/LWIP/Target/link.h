#ifndef _LWIP_TARGET_LINK_H_
#define _LWIP_TARGET_LINK_H_

/**
 * @brief  Initializes the PHY link
 * @param netif LWIP network interface structure
 * @retval None
 */
void target_link_init(struct netif *netif);

/**
 * @brief  Ethernet link state task
 * @param  argument: pointer to the netif structure
 * @retval None
 */
void ethernet_link_task(void *argument);

#endif /** !_LWIP_TARGET_LINK_H_ */
