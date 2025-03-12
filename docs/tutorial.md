# Zenoh-Pico & LWIP with STM32H5 Step By Step Tutorial

## Overview

This file is a detailed step-by-step tutorial on how to integrate **Zenoh-Pico** and **LWIP** with an **STM32H563ZI Nucleo board**. It covers all steps from the CubeMX project creation and peripherals setup, modification on CMakeLists.txt, to include required libraries, a basic pub/sub example and how to use **GitHub Actions** for automating builds and firmware releases.
This tutorial assumes that you have some basic knowledge of STM32CubeMX, CMake, and GitHub Actions. And that you are using a NUCLUEO-H563ZI board, if you are using a different board, you may need to adjust some aspects as pinout and clock configuration based on the board's datasheet.

## Project Setup

### CubeMX Project Creation

Open STM32CubeMX and on the top bar, uses File -> New Project.

<img src="cubemx/1.png" />
<img src="cubemx/2.png" />

When prompted about TrustZone, select the option "withoud TrustZone activated".

First step, go to Project Manager and uses the **browse** button to select the location of the project, and the project name to give a name to the project. Make sure also to change the **Toolcahin / IDE** to CMake.

<img src="cubemx/3.png" />

One extra step, this is optional, but I recommend since it keeps the code more organized. Go to the Project Manager -> Code Generator and set the **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**. This will make CubeMX split the peripheral initialization code into a .c and .h file, making it easier to modularize the code.
Also recommended to change to **Copy only the necessary library files** to avoid copying unnecessary files.

<img src="cubemx/4.png" />

### Clock Configuration

This step is specific to the NUCLEO-H563ZI board. If you are using a different board, you may need to adjust the clock configuration based on the board's datasheet.

Go to the Pinout & Configuration -> System Core -> RCC

1. HSE set to BYPASS Clock Source
2. LSE set to Crystal/Ceramic Resonator

<img src="cubemx/5.png" />

After this step go to the Clock Configuration and make the following changes:

1. Click on teh Input Frequency (HSE) and set it to 8MHz, just click on the field and type the value 8.
2. Set the PLL1 Source to HSE
3. Set System Clock Mux to PLLCLK

<img src="cubemx/6.png" />

After these steps you should have some errors on the clock sector, just go to the HCLK (MHz) and set it to 250MHz, and click enter. You should see a message stating that the configuration is being updated, after some seconds the errors should disappear and your configuration should be like this:

<img src="cubemx/7.png" />

### Configure the ICACHE

On the Pinout & Configuration -> System Core -> ICACHE, enable the ICACHE to 1 way direct mapped.

<img src="cubemx/8.png" />

### Configure USART3

On the NUCLEO-H563ZI board, the USART3 is connected to the ST-Link, so we can use it to print debug messages. Go to the Pinout & Configuration -> Connectivity -> USART3 and enable the USART3 setting it to asynchronous mode. All configurations are default.

<img src="cubemx/9.png" />

After that we need to fix the USART3 pinout, RX to PD9 and TX to PD8.

<img src="cubemx/10.png" />

### Configure the Ethernet

On the NUCLEO-H563ZI board, the Ethernet is connected to the RMII interface, so we need to configure it. Go to the Pinout & Configuration -> Connectivity -> ETH and enable the ETH setting it to RMII mode. YOu can play with the configuration after having the base project working, but for now just set the TX Descriptor length to 12 and RX Descriptor length to 8. MAC address you can change or leave the default.

<img src="cubemx/11.png" />

Make sure also to enable the Ethernet Global Interrupt on the NVIC settings.

<img src="cubemx/12.png" />

After this also make sure to change the Code generator to not automatically generate the ETH init function call since it will be called on the LWIP ethernetif.c file. Go to Project Manager -> Advanced Settings -> Do note Generate Function Call -> MX_ETH_Init

<img src="cubemx/13.png" />

On the pinout side make sure the following configurations are set:

EHT_TXD0 to PG13
THE_TX_EN to PG11

ETH_MDC to PC1
ETH_REF_CLK to PA1
ETH_MDIO to PA2

ETH_CRS_DV to PA7
ETH_RXD0 to PC4
ETH_RXD1 to PC5

ETH_TXD1 to PB15

For images, check the final pinout after the next section.

### Configuring user button and leds

For the user button:
- Set PC13 to GPIO_EXTI13, add a user label USER_BUTTON

For the user leds:
- Set PF4 to GPIO_Output, add a user label LED_YELLOW
- Set PB0 to GPIO_Output, add a user label LED_GREEN
- Set PG4 to GPIO_Output, add a user label LED_RED

### Final Pinout Configuration

After all these steps, your pinout configuration should look like this:

<img src="cubemx/14.png" />

### FreeRTOS

Even though LWIP can run using bare main loop, as we gonna use zenoh-pico we need to set a RTOS.
Go to Pinout & Configuration -> Middleware and Software Packs -> X-CUBE-FREERTOS and click on it. If it is the first time, you may be prompted to download the package, just click on download.
When everything is ready you should be able to select to use the package. Apply the following configurations and click on OK.

<img src="cubemx/15.png" />

After this step, before any other step click on generate code, since there is a bug on CubeMX in some version and platforms that can cause the IOC file to turn invalid.

> This step may be need or not depending on the CubeMX version and the platform, some versions of CubeMX when you select the enable FreeRTOS it will cause CubeMX to crash. If this happens to you, just close CubeMX and open the project using STM32CubeIDE and go to the Pinout & Configuration -> Middleware and Software Packs -> X-CUBE-FREERTOS and enable it there. After you can just close STM32CubeIDE and return to CubeMX. Make sure only to go to **Project Manager** and also to change the **Toolcahin / IDE** to CMake, since STM32CubeIDE will automatically change it to STM32CubeIDE. You can also delete the `.project` file that STM32CubeIDE generates.

After this we need to change the base system timer to use other than SysTick, since it will be used by FreeRTOS. Go to the Pinout & Configuration -> System Core -> SYS and change the **Timebase Source** to TIM12.

<img src="cubemx/16.png" />

The next step is a recommendation, but it will vary depending on your firmware needs, but its recommended to set the minimal stack size and the heap size to some value greater than the default since they are very small. I recommend at least 2kB for the stack size and 40kB for the heap size.

<img src="cubemx/17.png" />

After go to eh System Core -> NVIC and change the ETH global interrupt to a preemption priority of 7.

<img src="cubemx/18.png" />

Finally Generate the code.

### Coding

Now we gonna start the coding part of the project, we gonna make some steps to make the project ready to use Zenoh-Pico and LWIP.

1. Add connection for UART and printf so we can debug the project.
2. Change base CMake to Performance (Optional)
3. Add needed submodules
4. Configure the LWIP lib from ST
5. Configure the LAN8742 from ST
6. Add needed base ethernetif.c and lwipopts.h
7. Make a simple DHCP IP acquisition
8. Configure the Zenoh-Pico lib
9. Declare a simple pub/sub example

#### Add connection for UART and printf

First of all we need to add the connection for the UART and printf so we can debug / log the project. For now on I'll use each peripheral file, but if when configuring the project you didn't select to generate the peripheral initialization as a pair of '.c/.h' files per peripheral, you can just use the `main.c` file.
Add this to the `/* USER CODE BEGIN 1 */` section of the `Core/Src/usart.c` file:

```c
int _write(int fd, char *ptr, int len)
{
  /** In case of printf we keep synchonous since some parts use DMA */
  if (fd == 1 || fd == 2) {
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, HAL_MAX_DELAY);

    if (tx_status == HAL_OK) {
      return len;
    }
  }

  return -1;
}
```

This will allows us to use the `printf` function to print debug messages. After that we can add some test to see if everything is working.
On the `Core/Src/app_freertos.c` include the following code to the respective sections:

```c
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */
```

```c
/* USER CODE BEGIN defaultTask */
/* Infinite loop */
for(;;)
{
  printf("Hello World!\n");
  osDelay(1000);
}
/* USER CODE END defaultTask */
```

After that you can go to the [How to flash and debug](#how-to-flash-and-debug) section, flash the firmware and open some serial monitor to see the "Hello World!" message.

## How to flash and debug
