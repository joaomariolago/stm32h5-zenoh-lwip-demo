#ifndef _LWIP_APP_FTP_SERVER_H_
#define _LWIP_APP_FTP_SERVER_H_

/** FTP server user and password */

#define FTP_SERVER_USER_NAME      "blue"
#define FTP_SERVER_USER_PASSWORD  "robotics"

/** FTP server general config */

#define FTP_SERVER_BLOCK_SIZE       512U
#define FTP_SERVER_MAX_REQUEST_SIZE 1024U

/** FTP server process states */

#define FTP_SERVER_CLOSED 0U
#define FTP_SERVER_OPEN   1U

/** Define available FTP server requests */

#define FTP_USER_REQUEST           "USER"
#define FTP_PASS_REQUEST           "PASS"
#define FTP_QUIT_REQUEST           "QUIT"
#define FTP_PORT_REQUEST           "PORT"
#define FTP_NLST_REQUEST           "NLST"
#define FTP_STOR_REQUEST           "STOR"
#define FTP_RETR_REQUEST           "RETR"
#define FTP_DELE_REQUEST           "DELE"

/** Define available FTP server responses */

#define FTP_WELCOME_RESPONSE      "220 Service Ready\r\n"
#define FTP_USER_RESPONSE         "331 USER OK. PASS needed\r\n"
#define FTP_PASS_FAIL_RESPONSE    "530 NOT LOGGUED IN\r\n"
#define FTP_PASS_OK_RESPONSE      "230 USR LOGGUED IN\r\n"
#define FTP_PORT_OK_RESPONSE      "200 PORT OK\r\n"
#define FTP_NLST_OK_RESPONSE      "150 NLST OK\r\n"
#define FTP_RETR_OK_RESPONSE      "150 RETR OK\r\n"
#define FTP_STOR_OK_RESPONSE      "150 STOR OK\r\n"
#define FTP_DELE_OK_RESPONSE      "150 DELE OK\r\n"
#define FTP_QUIT_RESPONSE         "221 BYE OK\r\n"
#define FTP_TRANSFER_OK_RESPONSE  "226 Transfer OK\r\n"
#define FTP_WRITE_FAIL_RESPONSE   "550 File unavailable\r\n"
#define FTP_CMD_NOT_IMP_RESPONSE  "502 Command Unimplemented\r\n"
#define FTP_DATA_PORT_FAILED      "425 Cannot open Data Port\r\n"
#define FTP_UNKNOWN_RESPONSE      "500 Unrecognized Command\r\n"
#define FTP_BAD_SEQUENCE_RESPONSE "503 Bad Sequence of Commands\r\n"

/** Prototypes */

void ftpd_server_init(void);

#endif /* _LWIP_APP_FTP_SERVER_H_ */
