#ifndef _LWIP_APP_DHCP_DHCP_H_
#define _LWIP_APP_DHCP_DHCP_H_

/* DHCP process states */
#define DHCP_OFF              (uint8_t) 0
#define DHCP_START            (uint8_t) 1
#define DHCP_WAIT_ADDRESS     (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED (uint8_t) 3
#define DHCP_TIMEOUT          (uint8_t) 4
#define DHCP_LINK_DOWN        (uint8_t) 5

/** DHCP config */
#define MAX_DHCP_TRIES        (4U)
#define DHCP_TASK_INTERVAL_MS (250U)

/** Prototypes */
void start_dhcp(void);
void stop_dhcp(void);

void dhcp_task(void *argument);

#endif /* _LWIP_APP_DHCP_DHCP_H_ */
