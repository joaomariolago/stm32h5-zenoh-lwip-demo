#include "stm32h5xx_hal.h"

#include "FreeRTOS.h"

#include "eth.h"
#include "lan8742.h"

#include "lwip/netifapi.h"

#include "LWIP/Target/phy.h"
#include "LWIP/Target/link.h"

/** PHY Link variables */
lan8742_Object_t LAN8742;
lan8742_IOCtx_t LAN8742_IOCtx = {
  ETH_PHY_IO_Init,
  ETH_PHY_IO_DeInit,
  ETH_PHY_IO_WriteReg,
  ETH_PHY_IO_ReadReg,
  ETH_PHY_IO_GetTick
};

/**
 * @brief  Initializes the PHY link
 * @param netif LWIP network interface structure
 * @retval None
 */
void target_link_init(struct netif *netif)
{
  uint32_t duplex, speed = 0U;
  int32_t PHYLinkState = 0U;
  ETH_MACConfigTypeDef MACConf = {0};

  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  if (LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK)
  {
    netif_set_link_down(netif);
    netif_set_down(netif);
    return;
  }

  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  if (LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK)
  {
    netif_set_link_down(netif);
    netif_set_down(netif);
    return;
  }

  PHYLinkState = LAN8742_GetLinkState(&LAN8742);

  /* Get link state */
  if (PHYLinkState <= LAN8742_STATUS_LINK_DOWN)
  {
    netif_set_link_down(netif);
    netif_set_down(netif);
  }
  else
  {
    switch (PHYLinkState)
    {
      case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        break;
      case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        break;
      case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        break;
      case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        break;
      default:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        break;
    }

    /* Get MAC Config MAC */
    HAL_ETH_GetMACConfig(&heth, &MACConf);
    MACConf.DuplexMode = duplex;
    MACConf.Speed = speed;
    HAL_ETH_SetMACConfig(&heth, &MACConf);
    HAL_ETH_Start_IT(&heth);
    netif_set_up(netif);
    netif_set_link_up(netif);
  }
}

/**
 * @brief  Ethernet link state task
 * @param  argument: pointer to the netif structure
 * @retval None
 */
void ethernet_link_task(void *argument)
{
  ETH_MACConfigTypeDef MACConf = {0};
  int32_t PHYLinkState = 0U;
  uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;
  struct netif *netif = (struct netif *)argument;

  for (;;)
  {
    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

    if (netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
      HAL_ETH_Stop_IT(&heth);
      netifapi_netif_set_down(netif);
      netifapi_netif_set_link_down(netif);
    }
    else if (!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
      switch (PHYLinkState)
      {
        case LAN8742_STATUS_100MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_100MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_100M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_10MBITS_FULLDUPLEX:
          duplex = ETH_FULLDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          linkchanged = 1;
          break;
        case LAN8742_STATUS_10MBITS_HALFDUPLEX:
          duplex = ETH_HALFDUPLEX_MODE;
          speed = ETH_SPEED_10M;
          linkchanged = 1;
          break;
        default:
          break;
      }

      if (linkchanged)
      {
        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&heth, &MACConf);
        MACConf.DuplexMode = duplex;
        MACConf.Speed = speed;
        HAL_ETH_SetMACConfig(&heth, &MACConf);
        HAL_ETH_Start_IT(&heth);
        netifapi_netif_set_up(netif);
        netifapi_netif_set_link_up(netif);
      }
    }

    osDelay(100);
  }
}
