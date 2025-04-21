/**
 * This code was based mostly on this application note:
 * https://www.nxp.com/docs/en/application-note-software/AN3931SW.zip
 * Thanks to the authors!
 */

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>
#include <stdio.h>

#include "lwip/api.h"

#include "LWIP/App/FTP/utils.h"
#include "LWIP/App/FTP/server.h"

/** Locals */

static uint8_t ftpd_server_buffer[FTP_SERVER_BLOCK_SIZE];
static uint8_t ftpd_server_request_buffer[FTP_SERVER_MAX_REQUEST_SIZE];

/** Task Handlers ------------------------------------------------------------*/

osThreadId_t h_ftpd_server_task;
const osThreadAttr_t ftpd_server_task_attributes = {
    .name = "FTPDTask",
    .priority = (osPriority_t)osPriorityBelowNormal,
    .stack_size = configMINIMAL_STACK_SIZE};

/** Forward Declaration ------------------------------------------------------*/

static void ftpd_server_task(void *argument);
static void ftpd_server_socket_init(struct netconn *conn);
static void ftpd_server_connection_handler(struct netconn *connection, uint8_t *request_buffer);
static uint8_t ftpd_server_handle_data_request(struct netconn *connection, uint8_t *request_buffer);
static uint8_t *ftpd_server_get_client_info(uint8_t *request_buffer, ip_addr_t *client_ip, uint16_t *port);
static uint8_t ftpd_server_login_handler(struct netconn *connection, uint8_t *request_buffer, uint8_t *accepted);

/** Implementations ----------------------------------------------------------*/

uint8_t ftpd_server_is_quit_request(struct netconn *connection, uint8_t *request_buffer)
{
  if (strstr((const char *)request_buffer, FTP_QUIT_REQUEST) != NULL)
  {
    /** QUIT request, sending goodbye */
    netconn_write(connection, FTP_QUIT_RESPONSE, STR_LEN(FTP_QUIT_RESPONSE), 0U);
    return 1;
  }

  /** Invalid request */
  netconn_write(connection, FTP_UNKNOWN_RESPONSE, STR_LEN(FTP_UNKNOWN_RESPONSE), 0U);

  return 0;
}

static uint8_t *ftpd_server_get_client_info(uint8_t *request_buffer, ip4_addr_t *client_ip, uint16_t *port)
{
  char *head = (char *)request_buffer;
  char *end = NULL;

#define OCTETS_NUMBER 4U
#define PORT_BYTES 2U

  uint8_t octets[OCTETS_NUMBER];
  uint8_t port_parts[PORT_BYTES];

  /** Extract IPv4 Octets */
  for (uint32_t i = 0; i < OCTETS_NUMBER; ++i)
  {
    end = strchr(head, ',');
    if (end == NULL)
    {
      return NULL;
    }

    octets[i] = (uint8_t)strtoul(head, NULL, 10);
    head = end + 1;
  }

  /** Extract Port */
  for (uint32_t i = 0; i < PORT_BYTES; ++i)
  {
    end = strchr(head, (i == 0) ? ',' : '\r');
    if (end == NULL)
    {
      return NULL;
    }

    port_parts[i] = (uint8_t)strtoul(head, NULL, 10);
    head = end + 1;
  }

  /** Mounts final IP and port */
  IP_ADDR4(client_ip, octets[0], octets[1], octets[2], octets[3]);
  *port = ((uint16_t)port_parts[0] << 8) | port_parts[1];

  return (uint8_t *)head;
}

static uint8_t ftpd_server_handle_data_request(struct netconn *connection, uint8_t *request_buffer)
{
  struct netconn *connection_data = NULL;

  /** IP and port for data */
  ip4_addr_t client_ip_address;
  uint16_t ftp_data_port;

  /** Get client IP and return how much to advance the head */
  char *request_head = (char *)ftpd_server_get_client_info(request_buffer, &client_ip_address, &ftp_data_port);

  if (request_head == NULL)
  {
    /** Invalid request */
    netconn_write(connection, FTP_DATA_PORT_FAILED, STR_LEN(FTP_DATA_PORT_FAILED), 0U);
    return 1;
  }

  /** Sends OK to client*/
  netconn_write(connection, FTP_PORT_OK_RESPONSE, STR_LEN(FTP_PORT_OK_RESPONSE), 0U);

  /** Awaits for response on control port */
  netconn_receive_request(connection, request_buffer);
  if (connection->pending_err != ERR_OK)
  {
    /** Session closed by client */
    return 1;
  }

  if (strstr(request_buffer, FTP_NLST_REQUEST) != NULL)
  {
  }
  else if (strstr(request_buffer, FTP_RETR_REQUEST) != NULL)
  {
  }
  else if (strstr(request_buffer, FTP_STOR_REQUEST) != NULL)
  {
  }
  else
  {
    netconn_write(connection, FTP_CMD_NOT_IMP_RESPONSE, STR_LEN(FTP_CMD_NOT_IMP_RESPONSE), 0U);
  }

  /** OK */
  return 0U;
}

static uint8_t ftpd_server_login_handler(struct netconn *connection, uint8_t *request_buffer, uint8_t *accepted)
{
  /** Performs the login */
  if (strstr((const char *)request_buffer, FTP_USER_REQUEST) != NULL)
  {
    /** Username */
    if (
        !strncmp(
            (const char *)(&request_buffer[sizeof(FTP_USER_REQUEST)]),
            FTP_SERVER_USER_NAME,
            STR_LEN(FTP_SERVER_USER_NAME)))
    {
      netconn_write(connection, FTP_USER_RESPONSE, STR_LEN(FTP_USER_RESPONSE), 0U);

      /** Username is OK, awaits user sends password */
      netconn_receive_request(connection, request_buffer);

      if (connection->pending_err != ERR_OK)
      {
        /** Session closed by client */
        return 1U;
      }

      if (strstr((const char *)request_buffer, FTP_PASS_REQUEST) != NULL)
      {
        /*authentication process: password matchs exactly?*/
        if (
            !strncmp(
                (const char *)&request_buffer[sizeof(FTP_PASS_REQUEST)],
                FTP_SERVER_USER_PASSWORD,
                STR_LEN(FTP_SERVER_USER_PASSWORD)))
        {
          /** Login OK */
          netconn_write(connection, FTP_PASS_OK_RESPONSE, STR_LEN(FTP_PASS_OK_RESPONSE), 0U);

          *accepted = 1U;
        }
        else
        {
          /** Invalid password */
          netconn_write(connection, FTP_PASS_FAIL_RESPONSE, STR_LEN(FTP_PASS_FAIL_RESPONSE), 0U);
        }
      }
      else
      {
        /** Expects a password request */
        netconn_write(connection, FTP_BAD_SEQUENCE_RESPONSE, STR_LEN(FTP_BAD_SEQUENCE_RESPONSE), 0U);
      }
    }
    else
    {
      /** Invalid User */
      netconn_write(connection, FTP_PASS_FAIL_RESPONSE, STR_LEN(FTP_PASS_FAIL_RESPONSE), 0U);
    }
  }
  else
  {
    /** If client requested to quit */
    if (ftpd_server_is_quit_request(connection, request_buffer))
    {
      return 1U;
    }
  }

  return 0U;
}

static void ftpd_server_connection_handler(struct netconn *connection, uint8_t *request_buffer)
{
  uint8_t is_login_accepted = 0U;

  /** Send Welcome message */
  netconn_write(connection, FTP_WELCOME_RESPONSE, STR_LEN(FTP_WELCOME_RESPONSE), 0U);

  do
  {
    netconn_receive_request(connection, request_buffer);

    if (connection->pending_err != ERR_OK)
    {
      /** Session closed by client */
      break;
    }

    /** If already logged in */
    if (is_login_accepted == 1U)
    {
      if (strstr((const char *)request_buffer, FTP_PORT_REQUEST) != NULL)
      {
        /** PORT request */
        if (ftpd_server_handle_data_request(connection, &request_buffer[sizeof(FTP_PORT_REQUEST)]))
        {
          /** Session closed by client */
          break;
        }
      }
      else if (strstr((const char *)request_buffer, FTP_DELE_REQUEST) != NULL)
      {
        /** DELETE Request, we do not allow users to delete, file system is emulated and read only */
        netconn_write(connection, FTP_CMD_NOT_IMP_RESPONSE, STR_LEN(FTP_CMD_NOT_IMP_RESPONSE), 0U);
      }
      else
      {
        /** Check for a possible QUIT */
        if (ftpd_server_is_quit_request(connection, request_buffer))
        {
          /** QUIT */
          break;
        }
      }
    }
    else
    {
      if (ftpd_server_login_handler(connection, request_buffer, &is_login_accepted))
      {
        /** Invalid state os client quit */
        break;
      }
    }
  } while (1);

  /** Session close */
  netconn_close(connection);
  netconn_delete(connection);
}

static void ftpd_server_socket_init(struct netconn *conn)
{
  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 21);
  netconn_listen(conn);
  conn->recv_timeout = 0;
}

static void ftpd_server_task(void *argument)
{
  /** Suppres compiler warning */
  (void)argument;

  /** Local FTP server control */
  uint8_t server_state = FTP_SERVER_CLOSED;

  /** NET con handlers */
  struct netconn *conn = NULL, *new_conn = NULL;

  /** Initialize FTP sockets configuration */
  ftpd_server_socket_init(conn);

  /** Main FTP server loop */
  for (;;)
  {
    if (server_state == FTP_SERVER_CLOSED)
    {
      netconn_accept(conn, &new_conn);
      if (new_conn != NULL)
      {
        server_state = FTP_SERVER_OPEN;
        new_conn->recv_timeout = 0;
      }
    }
    else
    {
      /** Takes control of the task loop until session is closed */
      ftpd_server_connection_handler(new_conn, ftpd_server_request_buffer);
      /** Reset FTP server to closed state */
      server_state = FTP_SERVER_CLOSED;
    }
  }

  return;
}

void ftpd_server_init(void)
{
  /** Creates server task */
  h_ftpd_server_task = osThreadNew(ftpd_server_task, NULL, &ftpd_server_task_attributes);
}
