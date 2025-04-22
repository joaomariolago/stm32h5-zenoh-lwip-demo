
#ifndef _LWIP_APP_FTP_FLASH_H_
#define _LWIP_APP_FTP_FLASH_H_

#define FLASH_BANK_1_START_ADDR 0x08000000U
#define FLASH_BANK_1_END_ADDR   0x080FFFFFU

#define FLASH_BANK_2_START_ADDR 0x08100000U
#define FLASH_BANK_2_END_ADDR   0x081FFFFFU

/** Prototypes */

void flash_open(void);
void flash_close(void);

void program_flash(uint32_t base_address, uint8_t *data, uint16_t length);

#endif /* _LWIP_APP_FTP_FLASH_H_ */
