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
