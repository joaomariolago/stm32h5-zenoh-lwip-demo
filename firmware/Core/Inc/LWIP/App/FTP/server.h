#ifndef _LWIP_APP_FTP_SERVER_H_
#define _LWIP_APP_FTP_SERVER_H_

/** FTP server user and password */

#define FTP_SERVER_USER_NAME      "blue"
#define FTP_SERVER_USER_PASSWORD  "robotics"

/** FTP server general config */

#define FTP_SERVER_PORT             21U
#define FTP_SERVER_MAX_REQUEST_SIZE 1024U

/** FTP server process states */

#define FTP_SERVER_CLOSED 0U
#define FTP_SERVER_OPEN   1U

/** Define available FTP server requests */

#define FTP_USER_REQUEST           "USER"
#define FTP_PASS_REQUEST           "PASS"
#define FTP_QUIT_REQUEST           "QUIT"
#define FTP_PORT_REQUEST           "PORT"
#define FTP_LPRT_REQUEST           "PORT"
#define FTP_NLST_REQUEST           "NLST"
#define FTP_LIST_REQUEST           "LIST"
#define FTP_STOR_REQUEST           "STOR"
#define FTP_RETR_REQUEST           "RETR"
#define FTP_DELE_REQUEST           "DELE"
#define FTP_FEAT_REQUEST           "FEAT"
#define FTP_TYPE_REQUEST           "TYPE"
#define FTP_SYST_REQUEST           "SYST"
#define FTP_CWD_REQUEST            "CWD"
#define FTP_PWD_REQUEST            "PWD"
#define FTP_NOOP_REQUEST           "NOOP"

/** Define available FTP server responses */

#define FTP_WELCOME_RESPONSE      "220 Service Ready\r\n"
#define FTP_USER_RESPONSE         "331 USER OK. PASS needed\r\n"
#define FTP_PASS_FAIL_RESPONSE    "530 NOT LOGGUED IN\r\n"
#define FTP_PASS_OK_RESPONSE      "230 USR LOGGUED IN\r\n"
#define FTP_GENERIC_OK_RESPONSE   "200 OK\r\n"
#define FTP_NOOP_OK_RESPONSE      "200 NOOP OK\r\n"
#define FTP_PORT_OK_RESPONSE      "225 PORT OK\r\n"
#define FTP_NLST_OK_RESPONSE      "150 NLST OK\r\n"
#define FTP_LIST_OK_RESPONSE      "150 LIST OK\r\n"
#define FTP_RETR_OK_RESPONSE      "150 RETR OK\r\n"
#define FTP_STOR_OK_RESPONSE      "150 STOR OK\r\n"
#define FTP_DELE_OK_RESPONSE      "150 DELE OK\r\n"
#define FTP_QUIT_RESPONSE         "221 BYE OK\r\n"
#define FTP_TRANSFER_OK_RESPONSE  "226 Transfer OK\r\n"
#define FTP_WRITE_FAIL_RESPONSE   "550 File unavailable\r\n"
#define FTP_WRITE_DENIED_RESPONSE "550 Permission denied\r\n"
#define FTP_CMD_NOT_IMP_RESPONSE  "502 Command Unimplemented\r\n"
#define FTP_DATA_PORT_FAILED      "425 Cannot open Data Port\r\n"
#define FTP_UNKNOWN_RESPONSE      "500 Unrecognized Command\r\n"
#define FTP_BAD_SEQUENCE_RESPONSE "503 Bad Sequence of Commands\r\n"
#define FTP_SYST_OK_RESPONSE      "215 UNIX Type: L8\r\n"
#define FTP_CWD_OK_RESPONSE       "250 Directory successfully changed\r\n"
#define FTP_PWD_OK_RESPONSE       "257 \"/\" is current directory\r\n"

/** Prototypes */

void ftpd_server_init(void);

#endif /* _LWIP_APP_FTP_SERVER_H_ */
