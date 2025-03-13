/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "zenoh-pico.h"

#include "lwip/tcpip.h"
#include "LWIP/App/ethernet.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/** client or peer */
#define ZENOH_MODE "client"
/** Locator to connect to, if empty it will scout */
#define ZENOH_LOCATOR "tcp/192.168.2.2:7447"

#define ZENOH_TOPIC_PING_KEYEXPR "demo/example/topic/1"
#define ZENOH_TOPIC_PONG_KEYEXPR "mavlink/1/1/HEARTBEAT"

#define ZENOH_N_MESSAGES 1000000
#define ZENOH_MESSAGE_SIZE 2

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

z_owned_publisher_t pub;
z_owned_subscriber_t sub;

volatile char zenoh_buffer[ZENOH_MESSAGE_SIZE];

osThreadId_t h_app_task;
const osThreadAttr_t app_task_attributes = {
  .name = "app_task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = configMINIMAL_STACK_SIZE
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void dead_end(void);
void app_task(void */**argument */);
void data_handler(z_loaned_sample_t *sample, void *ctx);

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */

  /** We start with no LED on */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

  /* Initialize the LwIP stack */
  tcpip_init(NULL, NULL);

  /** Configures the interface and provides event to notify when ready */
  net_if_config();

  h_app_task = osThreadNew(app_task, NULL, &app_task_attributes);

  /* Delete the Init Thread */
  osThreadTerminate(defaultTaskHandle);

  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void dead_end(void)
{
  for (;;)
  {
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    osDelay(500);
  }
}

void app_task(void */**argument */)
{
  printf("At App Task, awaiting network to be ready to use...\n");

  /** We want to have net ready before starting this task */
  osEventFlagsWait(h_net_ready_event, 0x01, osFlagsWaitAny, osWaitForever);

  /** Green LED for user, net is red! */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

  printf("Net is ready, gonna start Zenoh task...\n");

  /** Zenoh configuration */
  z_owned_config_t config;
  z_config_default(&config);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
  if (strcmp(ZENOH_LOCATOR, "") != 0)
  {
    if (strcmp(ZENOH_MODE, "client") == 0)
    {
      zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_LOCATOR);
    }
    else
    {
      zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_LOCATOR);
    }
  }

  /** Open Zenoh session */
  z_owned_session_t s;
  while (z_open(&s, z_move(config), NULL) < 0)
  {
    printf("Failed to open Zenoh session, retrying...\n");
    osDelay(1000);
  }

  /** Fill with random Letters */
  // for (int idx = 0; idx < ZENOH_MESSAGE_SIZE; ++idx)
  // {
  //   zenoh_buffer[idx] = 'A' + (idx % 26);
  // }
  // zenoh_buffer[ZENOH_MESSAGE_SIZE - 1] = '\0';

  /** Create a publisher */
  // z_view_keyexpr_t ke;
  // z_view_keyexpr_from_str_unchecked(&ke, ZENOH_TOPIC_PING_KEYEXPR);
  // if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0)
  // {
  //   return dead_end();
  // }

  /** Create a subscriber */
  z_owned_closure_sample_t callback;
  z_closure(&callback, data_handler, NULL, NULL);
  z_view_keyexpr_t ke1;
  z_view_keyexpr_from_str_unchecked(&ke1, ZENOH_TOPIC_PONG_KEYEXPR);
  if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke1), z_move(callback), NULL) < 0) {
    return dead_end();
  }

  printf("Zenoh task started, ready to send and receive data...\n");

  while (1)
  {
    zp_read(z_loan(s), NULL);
    zp_send_keep_alive(z_loan(s), NULL);
    //zp_send_join(z_loan(s), NULL); Probably not needed since its not implemented
  }

  printf("Zenoh task finished, exiting...\n");

  for (;;)
  {
    /** Blink Green LED to inform user that main task is finished */
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    osDelay(1000);
  }
}

void data_handler(z_loaned_sample_t *sample, void *ctx) {
  (void)(ctx);

  HAL_GPIO_WritePin(ZENOH_FREQ_PIN_GPIO_Port, ZENOH_FREQ_PIN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_SET);

  printf("Received data: \n");

  // z_owned_bytes_t payload;
  // z_bytes_from_str(&payload, zenoh_buffer, NULL, NULL);
  //z_publisher_put(z_loan(pub), z_move(payload), NULL);

  HAL_GPIO_WritePin(ZENOH_FREQ_PIN_GPIO_Port, ZENOH_FREQ_PIN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_RESET);
}
/* USER CODE END Application */
