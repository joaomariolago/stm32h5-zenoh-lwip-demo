#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include <string.h>
#include <stdio.h>

#include "LWIP/App/FTP/flash.h"

#define QUAD_WORD_SIZE 16U

/** Used for flashing operations */
static uint8_t flash_buffer[QUAD_WORD_SIZE] = {0};
static uint8_t flash_buffer_head = 0U;
static uint32_t flash_base_address = FLASH_BANK_2_START_ADDR;

/** Implementations */

void flash_open(void)
{
  flash_base_address = FLASH_BANK_2_START_ADDR;

  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    Error_Handler();
  }
}

void flash_close(void)
{
  flash_base_address = FLASH_BANK_2_START_ADDR;

  if (HAL_FLASH_OB_Lock() != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_FLASH_Lock() != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
}

void print_flash_write(uint32_t address, const uint8_t *buffer, size_t length)
{
  printf("0x%08lX: ", (unsigned long)address);
  for (size_t i = 0; i < length; ++i)
  {
      printf("%02X ", buffer[i]);
  }
  printf("\r\n");
}

void flash_program_single_buffer()
{
  uint8_t aligned_flash_buffer[QUAD_WORD_SIZE] = {0};

  /** Invert indianess */
  for (uint8_t i = 0; i < QUAD_WORD_SIZE; i += 4)
  {
    aligned_flash_buffer[i] = flash_buffer[i + 3];
    aligned_flash_buffer[i + 1] = flash_buffer[i + 2];
    aligned_flash_buffer[i + 2] = flash_buffer[i + 1];
    aligned_flash_buffer[i + 3] = flash_buffer[i];
  }

  uint32_t *flash_ptr = (uint32_t *)aligned_flash_buffer;
  print_flash_write(flash_base_address, aligned_flash_buffer, QUAD_WORD_SIZE);

  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, flash_base_address, (uint32_t)flash_ptr) != HAL_OK)
  {
    Error_Handler();
  }
}

void program_flash(uint8_t *data, uint16_t length)
{
  uint16_t data_index = 0;

  if (flash_buffer_head > 0U)
  {
    uint8_t fill = QUAD_WORD_SIZE - flash_buffer_head;

    if (length < fill)
    {
      memcpy(&flash_buffer[flash_buffer_head], data, length);
      flash_buffer_head += length;
      return;
    }

    memcpy(&flash_buffer[flash_buffer_head], data, fill);
    data_index += fill;

    flash_program_single_buffer();

    flash_base_address += QUAD_WORD_SIZE;
    flash_buffer_head = 0;
  }

  while (((uint16_t)(length - data_index)) >= QUAD_WORD_SIZE)
  {
    memcpy(flash_buffer, &data[data_index], QUAD_WORD_SIZE);

    flash_program_single_buffer();

    flash_base_address += QUAD_WORD_SIZE;
    data_index += QUAD_WORD_SIZE;
  }

  flash_buffer_head = length - data_index;
  if (flash_buffer_head > 0)
  {
    memcpy(flash_buffer, &data[data_index], flash_buffer_head);
  }
}

void program_flash_flush(void)
{
  if (flash_buffer_head == 0)
  {
    return;
  }

  memset(&flash_buffer[flash_buffer_head], 0xFF, QUAD_WORD_SIZE - flash_buffer_head);

  flash_program_single_buffer();

  flash_base_address += QUAD_WORD_SIZE;
  flash_buffer_head = 0;
}

void swap_flash_banks()
{
  /** Wait for ongoing flash operations */
  if (FLASH_WaitForLastOperation(HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  flash_open();

  FLASH_OBProgramInitTypeDef OBInit;

  /* Get the boot configuration status */
  HAL_FLASHEx_OBGetConfig(&OBInit);

  OBInit.OptionType = OPTIONBYTE_USER;
  OBInit.USERType = OB_USER_SWAP_BANK;

  /** Swap banks */
  if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE)
  {
    OBInit.USERConfig = OB_SWAP_BANK_ENABLE;
    HAL_FLASHEx_OBProgram(&OBInit);
  }
  else
  {
    OBInit.USERConfig = OB_SWAP_BANK_DISABLE;
    HAL_FLASHEx_OBProgram(&OBInit);
  }

  /* Launch Option bytes loading */
  HAL_FLASH_OB_Launch();

  /** Wait for ongoing flash operations */
  if (FLASH_WaitForLastOperation(HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  flash_close();
}

void erase_flash(void)
{
  /** Disable IRQs to avoid interrupting the flash operations */
  __disable_irq();

  /** Suspend task switching */
  vTaskSuspendAll();

  /** Wait for ongoing flash operations */
  if (FLASH_WaitForLastOperation(HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  flash_open();

  FLASH_EraseInitTypeDef erase_init;
  uint32_t sector_error = 0;

  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.Banks = FLASH_BANK_2;
  erase_init.Sector = 0;
  erase_init.NbSectors = 128;

  if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK)
  {
    Error_Handler();
  }

  if (sector_error != 0xFFFFFFFF)
  {
    Error_Handler();
  }

  flash_close();

  /** Wait for ongoing flash operations */
  if (FLASH_WaitForLastOperation(HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  /** Resume task switching */
  xTaskResumeAll();

  /** Enable IRQs */
  __enable_irq();
}
