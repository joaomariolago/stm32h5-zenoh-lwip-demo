#ifndef _LWIP_TARGET_IFCONFIG_H_
#define _LWIP_TARGET_IFCONFIG_H_

/** ===== Interface general configuration ===== */

/** Network interface name */
#define IF_NAME_0 'B'
#define IF_NAME_1 'R'

/** Fallback IP config (Used if no DHCP is IP is obtained) */
#define IF_IP_ADDR "192.168.2.100"
#define IF_GW_ADDR "255.255.255.0"
#define IF_MASK "192.168.2.1"

/** If hostname is enabled in `lwipopts.h` uses this as the name */
#define IF_HOSTNAME "lwip"

/** ===== Ethernet link buffers ===== */

/**
 * Size in bytes of each individual Ethernet Rx buffer
 * Recommended to match the size of heth.Init.RxBuffLen defined in `eth.c`
 */
#define ETH_RX_BUFFER_SIZE  (1524U)

/**
 * Total number of Rx buffers allocated from the LwIP Rx memory pool.
 * Must be greater than ETH_RX_DESC_CNT to avoid descriptor starvation.
 */
#define ETH_RX_BUFFER_CNT   ((ETH_RX_DESC_CNT) * 8U)

/**
 * Maximum number of Tx buffers allocated from the LwIP heap memory.
 * Determines how many transmissions can be queued simultaneously.
 */
#define ETH_TX_BUFFER_MAX   ((ETH_TX_DESC_CNT) * 4)

/** ===== Ethernet interface task RTOS timings ===== */

/** Maximum time receive input task will wait to acquire a lock for an input */
#define ETH_IF_RX_TIMEOUT         (osWaitForever)

/** Time to block waiting for transmissions to finish */
#define ETH_IF_TX_TIMEOUT         (2000U)

/** Maximum time awaiting DMA transmit */
#define ETH_DMA_TRANSMIT_TIMEOUT  (20U)

#endif /** !_LWIP_TARGET_IFCONFIG_H_ */
