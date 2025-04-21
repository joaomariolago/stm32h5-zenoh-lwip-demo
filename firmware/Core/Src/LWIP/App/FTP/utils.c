#include <string.h>

#include "LWIP/App/FTP/utils.h"

void netconn_receive_request(struct netconn *connection, uint8_t *request_buffer)
{
  /** Buffers */
  struct netbuf *input_buffer = NULL;
  struct pbuf *q = NULL;

  /** Receive the packet */
  netconn_recv(connection, &input_buffer);

  uint16_t length = 0;
  if (input_buffer != NULL)
  {
    q = input_buffer->ptr;
    do
    {
      memcpy(&request_buffer[length], q->payload, q->len);
      length += q->len;
    } while ((q = q->next) != NULL);

    /** NULL char terminator for ASCII transfers */
    request_buffer[length] = '\0';

    netbuf_delete(input_buffer);
  }
}
