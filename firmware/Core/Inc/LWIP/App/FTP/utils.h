#ifndef _LWIP_APP_FTP_UTILS_H_
#define _LWIP_APP_FTP_UTILS_H_

#include "lwip/api.h"

/** Macros */
#define STR_LEN(x) (sizeof(x) - 1U)

/** Prototypes */

void netconn_receive_request(struct netconn *connection, uint8_t *request_buffer);

#endif /* _LWIP_APP_FTP_UTILS_H_ */
