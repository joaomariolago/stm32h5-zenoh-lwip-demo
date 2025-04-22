#include "main.h"

#include "LWIP/App/FTP/flash.h"

void flash_open(void)
{
  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    Error_Handler();
  }

  HAL_FLASH_Unlock();
}

void flash_close(void)
{
  HAL_FLASH_Lock();

  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
}

void program_flash(uint32_t base_address, uint8_t *data, uint16_t length)
{

}
