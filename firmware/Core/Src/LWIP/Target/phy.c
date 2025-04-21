#include "eth.h"
#include "LWIP/Target/phy.h"

/**
 * @brief  Initializes the MDIO interface GPIO and clocks.
 * @param  None
 * @retval 0 if OK, -1 if ERROR
 */
int32_t ETH_PHY_IO_Init(void)
{
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
 * @brief  De-Initializes the MDIO interface .
 * @param  None
 * @retval 0 if OK, -1 if ERROR
 */
int32_t ETH_PHY_IO_DeInit(void)
{
  return 0;
}

/**
 * @brief  Read a PHY register through the MDIO interface.
 * @param  DevAddr: PHY port address
 * @param  RegAddr: PHY register address
 * @param  pRegVal: pointer to hold the register value
 * @retval 0 if OK -1 if Error
 */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if (HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
 * @brief  Write a value to a PHY register through the MDIO interface.
 * @param  DevAddr: PHY port address
 * @param  RegAddr: PHY register address
 * @param  RegVal: Value to be written
 * @retval 0 if OK -1 if Error
 */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if (HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
 * @brief  Get the time in milliseconds used for internal PHY driver process.
 * @retval Time value
 */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}
