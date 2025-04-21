#include <string.h>

#include "stm32h5xx_hal.h"

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "eth.h"

#include "lwip/netifapi.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"

#include "netif/etharp.h"
#include "netif/ethernet.h"

#include "LWIP/Target/ifconfig.h"
#include "LWIP/Target/interface.h"
#include "LWIP/Target/link.h"

/** Typedefs */

typedef enum
{
  RX_ALLOC_OK     = 0x00,
  RX_ALLOC_ERROR  = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

/** Extern HAL variables */
extern ETH_TxPacketConfigTypeDef TxConfig;
extern ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT];
extern ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];

/* Memory Pool Declaration */
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

/* Local variables definitions */
static uint8_t RxAllocStatus;
osSemaphoreId_t RxPktSemaphore = NULL;
osSemaphoreId_t TxPktSemaphore = NULL;

/** Tasks */
osThreadId_t h_ethernet_input_task;
const osThreadAttr_t ethernet_input_task_attributes = {
  .name = "EthIfInput",
  .priority = (osPriority_t)osPriorityRealtime,
  .stack_size = configMINIMAL_STACK_SIZE
};

/* Forward Declarations */
static void ethernet_input_task(void *argument);
void pbuf_free_custom(struct pbuf *p);

/** Implementations */

static void low_level_init(struct netif *netif)
{
  MX_ETH_Init();

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = heth.Init.MACAddr[0];
  netif->hwaddr[1] = heth.Init.MACAddr[1];
  netif->hwaddr[2] = heth.Init.MACAddr[2];
  netif->hwaddr[3] = heth.Init.MACAddr[3];
  netif->hwaddr[4] = heth.Init.MACAddr[4];
  netif->hwaddr[5] = heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);

  /* create a binary semaphore used for informing ethernetif of frame reception */
  RxPktSemaphore = xSemaphoreCreateBinary();

  /* create a binary semaphore used for informing ethernetif of frame transmission */
  TxPktSemaphore = xSemaphoreCreateBinary();

  /* create the task that handles the ETH_MAC */
  h_ethernet_input_task = osThreadNew(ethernet_input_task, netif, &ethernet_input_task_attributes);

  target_link_init(netif);
}

static err_t low_level_output(struct netif * /**netif */, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];

  memset(Txbuffer, 0, ETH_TX_DESC_CNT * sizeof(ETH_BufferTypeDef));

  for (q = p; q != NULL; q = q->next)
  {
    if (i >= ETH_TX_DESC_CNT)
      return ERR_IF;

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if (i > 0)
    {
      Txbuffer[i - 1].next = &Txbuffer[i];
    }

    if (q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    ++i;
  }

  TxConfig.Length = p->tot_len;
  TxConfig.TxBuffer = Txbuffer;
  TxConfig.pData = p;

  pbuf_ref(p);

  do
  {
    if (HAL_ETH_Transmit_IT(&heth, &TxConfig) == HAL_OK)
    {
      errval = ERR_OK;
    }
    else
    {

      if (HAL_ETH_GetError(&heth) & HAL_ETH_ERROR_BUSY)
      {
        /* Wait for descriptors to become available */
        osSemaphoreAcquire(TxPktSemaphore, ETH_IF_TX_TIMEOUT);
        HAL_ETH_ReleaseTxPacket(&heth);
        errval = ERR_BUF;
      }
      else
      {
        /* Other error */
        pbuf_free(p);
        errval = ERR_IF;
      }
    }
  } while (errval == ERR_BUF);

  return errval;
}

static struct pbuf *low_level_input(struct netif * /**netif */)
{
  struct pbuf *p = NULL;

  if (RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
}

static void ethernet_input_task(void *argument)
{
  struct pbuf *p = NULL;
  struct netif *netif = (struct netif *)argument;

  for (;;)
  {
    if (osSemaphoreAcquire(RxPktSemaphore, ETH_IF_RX_TIMEOUT) == osOK)
    {
      do
      {
        p = low_level_input(netif);
        if (p != NULL)
        {
          if (netif->input(p, netif) != ERR_OK)
          {
            pbuf_free(p);
          }
        }
      } while (p != NULL);
    }
  }
}

err_t eth_interface_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = IF_HOSTNAME;
#endif

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IF_NAME_0;
  netif->name[1] = IF_NAME_1;

  /*
   * We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output() from it
   * if you have to do some checks before sending (e.g. if link is available...)
   */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom *custom_pbuf = (struct pbuf_custom *)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
    osSemaphoreRelease(RxPktSemaphore);
  }
}

u32_t sys_now(void)
{
  return HAL_GetTick();
}

/** ===== Callbacks ===== */

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef * /**heth */)
{
  osSemaphoreRelease(RxPktSemaphore);
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef * /**heth */)
{
  osSemaphoreRelease(TxPktSemaphore);
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
  if ((HAL_ETH_GetDMAError(heth) & ETH_DMACSR_RBU) == ETH_DMACSR_RBU)
  {
    osSemaphoreRelease(RxPktSemaphore);
  }

  if ((HAL_ETH_GetDMAError(heth) & ETH_DMACSR_TBU) == ETH_DMACSR_TBU)
  {
    osSemaphoreRelease(TxPktSemaphore);
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;

    /*
     * Initialize the struct pbuf.
     * This must be performed whenever a buffer's allocated because it may be
     * changed by lwIP or the app, e.g., pbuf_free decrements ref.
     */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
    RxAllocStatus = RX_ALLOC_ERROR;
    *buff = NULL;
  }
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd = p;

  /*
   * Update the total length of all the buffers of the chain.
   * Each pbuf in the chain should have it's tot_len set to its own length,
   * plus the length of all the following pbufs in the chain.
   */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
  pbuf_free((struct pbuf *)buff);
}
