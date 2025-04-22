/**
 * This code was based mostly on this application note:
 * https://www.nxp.com/docs/en/application-note-software/AN3931SW.zip
 * Thanks to the authors!
 */

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "main.h"

#include <string.h>
#include <stdio.h>

#include "lwip/api.h"

#include "LWIP/App/FTP/utils.h"
#include "LWIP/App/FTP/flash.h"
#include "LWIP/App/FTP/server.h"

/** Macros */
#define FTP_IS_REQUEST(type, request) (strstr((const char *)(request), (type)) != NULL)

#define FTP_DATA_PORT_ENTER(control_conn, data_conn, ip, port)                              \
  data_conn = ftpd_server_open_data_port(&(ip), (port));                                    \
  if ((data_conn) == NULL) {                                                                \
    netconn_write((control_conn), FTP_DATA_PORT_FAILED, STR_LEN(FTP_DATA_PORT_FAILED), 0U); \
    continue;                                                                               \
  }                                                                                         \

#define FTP_DATA_PORT_LEAVE(control_conn, data_conn)                                            \
  netconn_close(data_conn);                                                                     \
  netconn_delete(data_conn);                                                                    \
  netconn_write(control_conn, FTP_TRANSFER_OK_RESPONSE, STR_LEN(FTP_TRANSFER_OK_RESPONSE), 0U); \
  data_conn = NULL;                                                                             \
  continue;                                                                                     \

/** Task Handlers ------------------------------------------------------------*/

osThreadId_t h_ftpd_server_task;
const osThreadAttr_t ftpd_server_task_attributes = {
  .name = "FTPDTask",
  .priority = (osPriority_t)osPriorityBelowNormal,
  .stack_size = configMINIMAL_STACK_SIZE * 2U
};

/** Forward Declaration ------------------------------------------------------*/

static struct netconn *ftpd_server_open_data_port(ip_addr_t *ip_address, uint16_t port);
static void ftpd_server_data_control(struct netconn *control_connection, uint8_t *request);
static void ftpd_server_login_control(struct netconn *control_connection, uint8_t *request);
static void ftpd_server_get_data_port(uint8_t *request, ip4_addr_t *client_ip, uint16_t *port);
static void ftpd_server_list(struct netconn *data_connection);
static void ftpd_server_name_list(struct netconn *data_connection);
static void ftpd_server_program_flash(struct netconn *data_connection);
static void ftpd_server_read_flash(struct netconn *data_connection, uint8_t is_running_firmware);

/** Implementations ----------------------------------------------------------*/

static void ftpd_server_program_flash(struct netconn *data_connection)
{
  /** Prepares flash to be programmed */
  flash_open();

  struct netbuf *buf;
  while (netconn_recv(data_connection, &buf) == ERR_OK)
  {
    void *data;
    uint16_t len;
    netbuf_data(buf, &data, &len);

    /** Programs flash */
    program_flash((uint8_t *)data, len);

    netbuf_delete(buf);
  }
  program_flash_flush();

  /** Locks flash again */
  flash_close();
}

static void ftpd_server_read_flash(struct netconn *data_connection, uint8_t is_running_firmware)
{
  printf("Sending flash...\n");

  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    Error_Handler();
  }

  uint8_t *flash_address = NULL;
  uint32_t file_length = FLASH_BANK_SIZE;
  if (is_running_firmware)
  {
    printf("Sending running firmware...\n");
    flash_address = (uint8_t *)FLASH_BANK_1_START_ADDR;
  }
  else
  {
    printf("Sending slot firmware...\n");
    flash_address = (uint8_t *)FLASH_BANK_2_START_ADDR;
  }

  const size_t chunk = 512;
  size_t sent = 0;

  while (sent < file_length)
  {
    printf("Sending %zu bytes...\n", sent);
    size_t send_len = (file_length - sent) > chunk ? chunk : (file_length - sent);
    printf("Sending %zu bytes...\n", send_len);
    err_t err = netconn_write(data_connection, flash_address + sent, send_len, NETCONN_COPY);
    if (err != ERR_OK)
    {
      break;
    }
    sent += send_len;
  }

  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }

  printf("Sent %zu bytes\n", sent);
}

static void ftpd_server_name_list(struct netconn *data_connection)
{
  netconn_write(data_connection, "running.bin\r\nslot.bin\r\n", 24, NETCONN_COPY);
}

static void ftpd_server_list(struct netconn *data_connection)
{
  netconn_write(
    data_connection,
    "-rw-r--r-- 1 blue blue 1048576 Jan 01 00:00 running.bin\r\n-r--r--r-- 1 blue blue 1048576 Apr 22 00:00 slot.bin\r\n",
    112,
    NETCONN_COPY
  );
}

static struct netconn *ftpd_server_open_data_port(ip_addr_t *ip_address, uint16_t port)
{
  struct netconn *data_connection = netconn_new(NETCONN_TCP);

  if (data_connection == NULL)
  {
    return NULL;
  }

  if (netconn_connect(data_connection, ip_address, port) != ERR_OK)
  {
    netconn_delete(data_connection);
    return NULL;
  }

  return data_connection;
}

static void ftpd_server_get_data_port(uint8_t *request, ip4_addr_t *client_ip, uint16_t *port)
{
  char *head = (char *)request;
  char *end = NULL;

  uint8_t octets[4];
  uint8_t port_parts[2];

  /** Extract IPv4 Octets */
  for (uint32_t i = 0; i < 4; ++i)
  {
    end = strchr(head, ',');
    octets[i] = (uint8_t)strtoul(head, &end, 10U);
    head = end + 1;
  }

  /** Extract port bytes */
  end = strchr(head, ',');
  port_parts[0] = (uint8_t)strtoul(head, &end, 10U);
  head = end + 1U;
  end = strchr(head, '\r');
  port_parts[1] = (uint8_t)strtoul(head, &end, 10U);

  /** Mounts final IP and port */
  IP_ADDR4(client_ip, octets[0], octets[1], octets[2], octets[3]);
  *port = ((uint16_t)port_parts[0] << 8) | port_parts[1];
}

static void ftpd_server_data_control(struct netconn *control_connection, uint8_t *request)
{
  /** IP and port for data */
  ip_addr_t data_channel_ip;
  uint16_t data_channel_port = 0U;
  struct netconn *data_connection = NULL;

  /** FTP control processing loop */
  do
  {
    /** Receives incoming packets and check for session closing */
    netconn_receive_request(control_connection, request);

    printf("Req: %.64s\n", request);

    if (control_connection->pending_err != ERR_OK || FTP_IS_REQUEST(FTP_QUIT_REQUEST, request))
    {
      netconn_write(control_connection, FTP_QUIT_RESPONSE, STR_LEN(FTP_QUIT_RESPONSE), 0U);
      break;
    }

    /** Some file browsers may request a TYPE command */
    if (FTP_IS_REQUEST(FTP_TYPE_REQUEST, request))
    {
      netconn_write(control_connection, FTP_GENERIC_OK_RESPONSE, STR_LEN(FTP_GENERIC_OK_RESPONSE), 0U);
      continue;
    }

    /** In case of login, we are already authenticated */
    if (FTP_IS_REQUEST(FTP_USER_REQUEST, request) || FTP_IS_REQUEST(FTP_PASS_REQUEST, request))
    {
      netconn_write(control_connection, FTP_PASS_OK_RESPONSE, STR_LEN(FTP_PASS_OK_RESPONSE), 0U);
      continue;
    }

    /** Commands */
    if (FTP_IS_REQUEST(FTP_NOOP_REQUEST, request))
    {
      netconn_write(control_connection, FTP_NOOP_OK_RESPONSE, STR_LEN(FTP_NOOP_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_SYST_REQUEST, request))
    {
      netconn_write(control_connection, FTP_SYST_OK_RESPONSE, STR_LEN(FTP_SYST_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_CWD_REQUEST, request))
    {
      netconn_write(control_connection, FTP_CWD_OK_RESPONSE, STR_LEN(FTP_CWD_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_PWD_REQUEST, request))
    {
      netconn_write(control_connection, FTP_PWD_OK_RESPONSE, STR_LEN(FTP_PWD_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_SYST_REQUEST, request))
    {
      netconn_write(control_connection, FTP_SYST_OK_RESPONSE, STR_LEN(FTP_SYST_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_PORT_REQUEST, request))
    {
      ftpd_server_get_data_port(&request[sizeof(FTP_PORT_REQUEST)], &data_channel_ip, &data_channel_port);

      printf("Channel IP: %s\n", ipaddr_ntoa(&data_channel_ip));
      printf("Channel Port: %u\n", data_channel_port);

      netconn_write(control_connection, FTP_PORT_OK_RESPONSE, STR_LEN(FTP_PORT_OK_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_NLST_REQUEST, request))
    {
      FTP_DATA_PORT_ENTER(control_connection, data_connection, data_channel_ip, data_channel_port);
      netconn_write(control_connection, FTP_NLST_OK_RESPONSE, STR_LEN(FTP_NLST_OK_RESPONSE), 0U);

      ftpd_server_name_list(data_connection);

      FTP_DATA_PORT_LEAVE(control_connection, data_connection);
    }
    if (FTP_IS_REQUEST(FTP_LIST_REQUEST, request))
    {
      FTP_DATA_PORT_ENTER(control_connection, data_connection, data_channel_ip, data_channel_port);
      netconn_write(control_connection, FTP_LIST_OK_RESPONSE, STR_LEN(FTP_NLST_OK_RESPONSE), 0U);

      ftpd_server_list(data_connection);

      FTP_DATA_PORT_LEAVE(control_connection, data_connection);
    }
    if (FTP_IS_REQUEST(FTP_STOR_REQUEST, request))
    {
      /** Only allows writing to slot.bin */
      if (strncmp((char *)&request[sizeof(FTP_STOR_REQUEST)], "/slot.bin", 9U) == 0)
      {
        FTP_DATA_PORT_ENTER(control_connection, data_connection, data_channel_ip, data_channel_port);
        netconn_write(control_connection, FTP_STOR_OK_RESPONSE, STR_LEN(FTP_STOR_OK_RESPONSE), 0U);

        ftpd_server_program_flash(data_connection);

        FTP_DATA_PORT_LEAVE(control_connection, data_connection);
      }

      netconn_write(control_connection, FTP_WRITE_DENIED_RESPONSE, STR_LEN(FTP_WRITE_DENIED_RESPONSE), 0U);
      continue;
    }
    if (FTP_IS_REQUEST(FTP_RETR_REQUEST, request))
    {
      uint8_t is_slot_firm = strncmp((char *)&request[sizeof(FTP_RETR_REQUEST)], "/slot.bin", 9U) == 0;
      uint8_t is_running_firm = strncmp((char *)&request[sizeof(FTP_RETR_REQUEST)], "/running.bin", 12U) == 0;
      if (is_slot_firm || is_running_firm)
      {
        FTP_DATA_PORT_ENTER(control_connection, data_connection, data_channel_ip, data_channel_port);
        netconn_write(control_connection, FTP_STOR_OK_RESPONSE, STR_LEN(FTP_STOR_OK_RESPONSE), 0U);

        ftpd_server_read_flash(data_connection, is_running_firm);

        FTP_DATA_PORT_LEAVE(control_connection, data_connection);
      }

      netconn_write(control_connection, FTP_WRITE_DENIED_RESPONSE, STR_LEN(FTP_WRITE_DENIED_RESPONSE), 0U);
      continue;
    }

    /** Unsupported commands */
    if (
      FTP_IS_REQUEST(FTP_DELE_REQUEST, request) ||
      FTP_IS_REQUEST(FTP_LPRT_REQUEST, request)
    )
    {
      /**
       * DELETE: We do not allow users to delete, file system is emulated and read only.
       * LPRT: We just use the simpler active mode.
       */
      netconn_write(control_connection, FTP_CMD_NOT_IMP_RESPONSE, STR_LEN(FTP_CMD_NOT_IMP_RESPONSE), 0U);
      continue;
    }

    /** Invalid request */
    printf("Invalid request: %.64s\n", request);
    netconn_write(control_connection, FTP_UNKNOWN_RESPONSE, STR_LEN(FTP_UNKNOWN_RESPONSE), 0U);
  } while (1);
}

static void ftpd_server_login_control(struct netconn *control_connection, uint8_t *request)
{
  uint8_t is_user_accepted = 0U;
  uint8_t is_pass_accepted = 0U;

  /** Send Welcome message to client */
  netconn_write(control_connection, FTP_WELCOME_RESPONSE, STR_LEN(FTP_WELCOME_RESPONSE), 0U);

  /** Login process loop */
  do
  {
    /** Receives incoming packets and check for session closing */
    netconn_receive_request(control_connection, request);
    if (control_connection->pending_err != ERR_OK || FTP_IS_REQUEST(FTP_QUIT_REQUEST, request))
    {
      netconn_write(control_connection, FTP_QUIT_RESPONSE, STR_LEN(FTP_QUIT_RESPONSE), 0U);
      break;
    }

    if (FTP_IS_REQUEST(FTP_USER_REQUEST, request))
    {
      /** Validates provided username */
      if (!strncmp((char *)(&request[sizeof(FTP_USER_REQUEST)]), FTP_SERVER_USER_NAME, STR_LEN(FTP_SERVER_USER_NAME)))
      {
        /** User OK */
        is_user_accepted = 1U;
        netconn_write(control_connection, FTP_USER_RESPONSE, STR_LEN(FTP_USER_RESPONSE), 0U);

        /** Goes for password */
        continue;
      }
    }

    if (FTP_IS_REQUEST(FTP_PASS_REQUEST, request) && is_user_accepted == 1U)
    {
      if (!strncmp((char *)&request[sizeof(FTP_PASS_REQUEST)], FTP_SERVER_USER_PASSWORD, STR_LEN(FTP_SERVER_USER_PASSWORD)))
      {
        /** Password OK */
        is_pass_accepted = 1U;
        netconn_write(control_connection, FTP_PASS_OK_RESPONSE, STR_LEN(FTP_PASS_OK_RESPONSE), 0U);

        /** Goes for data */
        break;
      }
      else
      {
        /** Reset login state */
        is_user_accepted = 0U;
        is_pass_accepted = 0U;
      }
    }

    netconn_write(control_connection, FTP_PASS_FAIL_RESPONSE, STR_LEN(FTP_PASS_FAIL_RESPONSE), 0U);
  } while (1);

  /** If a valid login was provided */
  if (is_user_accepted == 1U && is_pass_accepted == 1U)
  {
    /** Logged in area */
    ftpd_server_data_control(control_connection, request);
  }

  /** Session close, cleanup */
  printf("Session closed\n");
  netconn_close(control_connection);
  netconn_delete(control_connection);
}

static void ftpd_server_task(void *argument)
{
  /** Suppres compiler warning */
  (void)argument;

  /** FTP Buffers */
  static uint8_t request[FTP_SERVER_MAX_REQUEST_SIZE];

  /** FTP locals */
  uint8_t server_state = FTP_SERVER_CLOSED;
  struct netconn *listen_connection = NULL, *control_connection = NULL;

  /** Initialize FTP connection listener socket on specified port */
  listen_connection = netconn_new(NETCONN_TCP);
  netconn_bind(listen_connection, NULL, FTP_SERVER_PORT);
  netconn_listen(listen_connection);
  (listen_connection)->recv_timeout = 0;

  /** Main FTP server loop */
  for (;;)
  {
    if (server_state == FTP_SERVER_CLOSED)
    {
      netconn_accept(listen_connection, &control_connection);
      if (control_connection != NULL)
      {
        server_state = FTP_SERVER_OPEN;
        control_connection->recv_timeout = 0;
      }
    }
    else
    {
      printf("FTP server: waiting for new connection...\n");
      /** Takes control of the task loop until session is closed */
      ftpd_server_login_control(control_connection, request);
      printf("FTP server: session closed\n");
      server_state = FTP_SERVER_CLOSED;
    }
  }

  return;
}

void ftpd_server_init(void)
{
  printf("Starting FTP server...\n");
  /** Creates server task */
  h_ftpd_server_task = osThreadNew(ftpd_server_task, NULL, &ftpd_server_task_attributes);
}
