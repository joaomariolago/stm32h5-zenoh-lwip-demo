
#ifndef _LWIP_APP_FTP_FLASH_H_
#define _LWIP_APP_FTP_FLASH_H_

#define FLASH_BANK_SIZE (8U * 1024U * 128U)

#define FLASH_BANK_1_START_ADDR 0x08000000U
#define FLASH_BANK_2_START_ADDR 0x08100000U

/** Prototypes */

void flash_open(void);
void flash_close(void);

void erase_flash(void);
void swap_flash_banks(void);

void program_flash(uint8_t *data, uint16_t length);
void program_flash_flush(void);

#endif /* _LWIP_APP_FTP_FLASH_H_ */
