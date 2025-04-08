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

#define JSMN_HEADER
#include "jsmn.h"
#include "zenoh-pico.h"

#include "lwip/tcpip.h"
#include "LWIP/App/ethernet.h"

#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/** Zenoh session */
#define ZENOH_MODE "client"
#define ZENOH_LOCATOR "tcp/192.168.2.2:7447"

/** Lets listen to SERVO messages to control the motor */
#define TOPIC_SERVO_OUT_RAW "mavlink/1/1/SERVO_OUTPUT_RAW"

/** We want to control the boat so motor 1 and 3 used */
#define MOTOR_1_SERVO_INDEX 1
#define MOTOR_2_SERVO_INDEX 3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
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

/**
 * @brief If something goes wrong in the initialization process, we end up here.
 */
void dead_end(void);

/**
 * @brief Main application loop task.
 *
 * @param argument Not used
 */
void app_task(void */**argument */);

/**
 * @brief Callback for when a message is received on topic SERVO_OUT_RAW
 *
 * @param sample The sample received
 * @param ctx Not used
 */
void sub_on_servo_out_raw_message(z_loaned_sample_t *sample, void *ctx);

/**
 * @brief Get the motors pulse width from a given SERVO_OUTPUT_RAW message.
 *
 * @param message String holding JSON content of the message
 * @param message_len Length of string holding the message
 * @param m1 Pointer to store motor 1 pulse width
 * @param m2 Pointer to store motor 2 pulse width
 */
void get_motors_pulse_width(const char *message, uint16_t message_len, uint16_t *m1, uint16_t *m2);

/**
 * @brief Set the motors PWM values and avoid optimizing away the function.
 *
 * @param m1 Motor 1 PWM value
 * @param m2 Motor 2 PWM value
 */
void __attribute__((optimize("O0"))) set_motors_pwm(uint16_t m1, uint16_t m2);

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

  /** We start PWM for motors */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

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

/**
 * @brief If something goes wrong in the initialization process, we end up here.
 */
void dead_end(void)
{
  for (;;)
  {
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    osDelay(500);
  }
}

/**
 * @brief Main application loop task.
 *
 * @param argument Not used
 */
void app_task(void */**argument */)
{
  printf("At App Task, awaiting network to be ready to use...\n");
  /** We want to have net ready before starting this task */
  osEventFlagsWait(h_net_ready_event, 0x01, osFlagsWaitAny, osWaitForever);

  /** Green LED for user, net is red! */
  printf("Net is ready, gonna start Zenoh task...\n");
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

  /** Zenoh configuration */
  z_owned_config_t config;
  z_config_default(&config);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_LOCATOR);

  /** Open Zenoh session */
  z_owned_session_t s;
  while (z_open(&s, z_move(config), NULL) < 0)
  {
    printf("Failed to open Zenoh session, retrying...\n");
    osDelay(1000);
  }

  /** Create a subscriber */
  z_owned_subscriber_t sub;
  z_owned_closure_sample_t callback;
  z_closure(&callback, sub_on_servo_out_raw_message, NULL, NULL);
  z_view_keyexpr_t ke1;
  z_view_keyexpr_from_str_unchecked(&ke1, TOPIC_SERVO_OUT_RAW);

  if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke1), z_move(callback), NULL) < 0) {
    return dead_end();
  }

  printf("Zenoh task started, ready to send and receive data...\n");

  while (1)
  {
    zp_read(z_loan(s), NULL);
    zp_send_keep_alive(z_loan(s), NULL);
  }

  printf("Zenoh task finished, exiting...\n");

  z_drop(z_move(callback));
  z_drop(z_move(sub));
  z_drop(z_move(s));

  dead_end();
}

/**
 * @brief Callback for when a message is received on topic SERVO_OUT_RAW
 *
 * @param sample The sample received
 * @param ctx Not used
 */
void sub_on_servo_out_raw_message(z_loaned_sample_t *sample, void */**ctx */) {
  /** Print message as string */
  z_view_string_t keystr;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
  z_owned_string_t value;
  z_bytes_to_string(z_sample_payload(sample), &value);

  uint16_t m1 = 0, m2 = 0;
  get_motors_pulse_width(z_string_data(z_loan(value)), (uint16_t)z_string_len(z_loan(value)), &m1, &m2);

  set_motors_pwm(m1, m2);

  //printf("M1: %u, M2: %u\n", m1, m2);

  z_drop(z_move(value));
}

/**
 * @brief Get the motors pulse width from a given SERVO_OUTPUT_RAW message.
 *
 * @param message String holding JSON content of the message
 * @param message_len Length of string holding the message
 * @param m1 Pointer to store motor 1 pulse width
 * @param m2 Pointer to store motor 2 pulse width
 */
void get_motors_pulse_width(const char *message, uint16_t message_len, uint16_t *m1, uint16_t *m2)
{
  #define MAX_TOKENS 80
  static jsmn_parser parser;
  static jsmntok_t tokens[MAX_TOKENS];

  jsmn_init(&parser);
  int r = jsmn_parse(&parser, message, message_len, tokens, MAX_TOKENS);

  if (r < 0) {
    /** Invalid message */
    return;
  }

  /** Extract all servo<x>_raw values */
  for (int i = 0; i < r; i++) {
    if (tokens[i].type == JSMN_STRING) {
        int key_len = tokens[i].end - tokens[i].start;
        const char *key = message + tokens[i].start;

        // Check if key starts with "servo" and ends with "_raw"
        if (key_len >= 9 && strncmp(key, "servo", 5) == 0 && strncmp(key + key_len - 4, "_raw", 4) == 0) {
          /** Extract servo index number and its duty value */
            uint16_t servo_number = 0, servo_value = 0;
            sscanf(key + 5, "%hu", &servo_number);
            sscanf(message + tokens[i + 1].start, "%hu", &servo_value);

            if (servo_number == MOTOR_1_SERVO_INDEX) {
              *m1 = servo_value;
            } else if (servo_number == MOTOR_2_SERVO_INDEX) {
              *m2 = servo_value;
            }
        }
    }
  }
}

/**
 * @brief Set the motors PWM values and avoid optimizing away the function.
 *
 * @param m1 Motor 1 PWM value
 * @param m2 Motor 2 PWM value
 */
void set_motors_pwm(uint16_t m1, uint16_t m2)
{
  if (m1 != 0) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, m1);
  }
  if (m2 != 0) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, m2);
  }
}
/* USER CODE END Application */
