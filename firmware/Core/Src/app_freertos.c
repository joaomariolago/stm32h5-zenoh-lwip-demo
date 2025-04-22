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

#include "lwip/stats.h"
#include "LWIP/App/net.h"

#include "tim.h"
#include "stm32h5xx_ll_tim.h"
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

osThreadId_t h_zenoh_loop_task;
const osThreadAttr_t zenoh_loop_task_attributes = {
  .name = "zenoh_loop",
  .priority = (osPriority_t) osPriorityRealtime,
  .stack_size = configMINIMAL_STACK_SIZE
};

osThreadId_t h_zenoh_keep_alive_task;
const osThreadAttr_t h_zenoh_keep_alive_task_attributes = {
  .name = "zenoh_kp_al",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = configMINIMAL_STACK_SIZE
};

osThreadId_t h_stats_task;
const osThreadAttr_t stats_task_attributes = {
  .name = "stats_task",
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
 * @brief Zenoh loop task to keep the session alive and read messages.
 *
 * @param argument Not used
 */
void zenoh_loop_task(void */**argument */);

/**
 * @brief Zenoh keep alive is a task that sends keep alive messages to the zenoh session.
 *
 * @param argument Not used
 */
void zenoh_keep_alive_task(void *argument);

/**
 * @brief Report current OS stats to the console.
 *
 * @param argument Not used
 */
void stats_task(void */**argument */);

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
void get_motors_pulse_width(const char *message, uint16_t message_len, uint16_t *m1, uint16_t *m2, uint32_t *origin_time_us);

/**
 * @brief Update local jitter statistics.
 *
 * @param origin_time_us Time in us given from the mavlink message
 */
uint32_t update_jitter_statistics();

/**
 * @brief Set the motors PWM values and avoid optimizing away the function.
 *
 * @param m1 Motor 1 PWM value
 * @param m2 Motor 2 PWM value
 */
void __attribute__((optimize("O0"))) set_motors_pwm(uint16_t m1, uint16_t m2);

/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

__weak unsigned long getRunTimeCounterValue(void)
{
  return DWT->CYCCNT;
}
/* USER CODE END 1 */

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

  network_start();

  h_app_task = osThreadNew(app_task, NULL, &app_task_attributes);
  //h_stats_task = osThreadNew(stats_task, NULL, &stats_task_attributes);

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

  /** Wait a bit to let case a session is open it will reset */
  printf("Network is ready, waiting 2 seconds so old sessions are cleared...\n");
  osDelay(2000);

  /** Green LED for user, net is red! */
  printf("Net is ready, gonna start Zenoh...\n");
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

  /** Zenoh configuration */
  z_owned_config_t config;
  z_config_default(&config);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
  zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_LOCATOR);

  /** Open Zenoh session */
  printf("Opening Zenoh session...\n");
  z_owned_session_t s;
  for (uint32_t i = 0; (z_open(&s, z_move(config), NULL) < 0) && i < 10; ++i)
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

  printf("Creating subscriber for topic %s...\n", TOPIC_SERVO_OUT_RAW);
  z_result_t sub_creation_res = z_declare_subscriber(z_loan(s), &sub, z_loan(ke1), z_move(callback), NULL);

  if (sub_creation_res < 0) {
    return dead_end();
  }

  HAL_TIM_Base_Start_IT(&htim5);
  printf("Subscriber created successfully! Creating zenog main loop task...\n");

  h_zenoh_loop_task = osThreadNew(zenoh_loop_task, (void*)&s, &zenoh_loop_task_attributes);
  h_zenoh_loop_task = osThreadNew(zenoh_keep_alive_task, (void*)&s, &h_zenoh_keep_alive_task_attributes);

  for (;;)
  {
    /** We need to keep the watchdog alive */
    osDelay(1000);
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    //HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
  }

  z_drop(z_move(callback));
  z_drop(z_move(sub));
  z_drop(z_move(s));

  dead_end();
}

/**
 * @brief Zenoh task to read messages.
 *
 * @param argument Not used
 */
void zenoh_loop_task(void *argument)
{
  z_owned_session_t s = *(z_owned_session_t *)argument;

  while (1)
  {
    zp_read(z_loan(s), NULL);
  }
}

/**
 * @brief Zenoh keep alive is a task that sends keep alive messages to the zenoh session.
 *
 * @param argument Not used
 */
void zenoh_keep_alive_task(void *argument)
{
  z_owned_session_t s = *(z_owned_session_t *)argument;

  while (1)
  {
    zp_send_keep_alive(z_loan(s), NULL);
    osDelay(1000);
  }
}

/**
 * @brief Report current OS stats to the console.
 *
 * @param argument Not used
 */
void stats_task(void */**argument */)
{
  #define STATS_CHAR_BUFFER_SIZE 1024
  static char buffer[STATS_CHAR_BUFFER_SIZE];

  while (1)
  {
    /** RTOS statistics */
    printf("========================\n");
    vTaskGetRunTimeStats(buffer);
    printf("%s\n\n", buffer);

    /** LWIP Statistics */
    stats_display();

    osDelay(1000);
  }
}

/**
 * @brief Callback for when a message is received on topic SERVO_OUT_RAW
 *
 * @param sample The sample received
 * @param ctx Not used
 */
void sub_on_servo_out_raw_message(z_loaned_sample_t *sample, void */**ctx */)
{
  /** Print message as string */
  HAL_GPIO_TogglePin(ZENOH_FREQ_PIN_GPIO_Port, ZENOH_FREQ_PIN_Pin);

  uint32_t jitter_variance_us = update_jitter_statistics();
  printf("%lu\n", jitter_variance_us);

  // z_view_string_t keystr;
  // z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
  // z_owned_string_t value;
  // z_bytes_to_string(z_sample_payload(sample), &value);

  // uint32_t origin_time_us = 0U;
  // uint16_t m1 = 0U, m2 = 0U;
  // get_motors_pulse_width(z_string_data(z_loan(value)), (uint16_t)z_string_len(z_loan(value)), &m1, &m2, &origin_time_us);

  // set_motors_pwm(m1, m2);


  //printf("M1: %u, M2: %u\n", m1, m2);

  //z_drop(z_move(value));
}

/**
 * @brief Get the motors pulse width from a given SERVO_OUTPUT_RAW message.
 *
 * @param message String holding JSON content of the message
 * @param message_len Length of string holding the message
 * @param m1 Pointer to store motor 1 pulse width
 * @param m2 Pointer to store motor 2 pulse width
 */
void get_motors_pulse_width(const char *message, uint16_t message_len, uint16_t *m1, uint16_t *m2, uint32_t *origin_time_us)
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

        /** Get key time_usec */
        if (key_len >= 9 && strncmp(key, "time_usec", key_len) == 0) {
          sscanf(message + tokens[i + 1].start, "%lu", origin_time_us);
        }

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

/**
 * @brief Update local jitter statistics.
 *
 * @param origin_time_us Time in us given from the mavlink message
 */
uint32_t update_jitter_statistics()
{

  #define MSG_PERIOD_US 20000U

  uint32_t local_time_us = LL_TIM_GetCounter(TIM5);
  uint32_t local_jitter = (local_time_us > MSG_PERIOD_US ? local_time_us - MSG_PERIOD_US : MSG_PERIOD_US - local_time_us);
  LL_TIM_SetCounter(TIM5, 0);

  #define JITTER_MAX_SAMPLES 256
  static uint16_t jitter_buffer_index = 0U;
  static uint32_t jitter_buffer[JITTER_MAX_SAMPLES]= {0};

  jitter_buffer[jitter_buffer_index] = local_jitter;
  jitter_buffer_index = (jitter_buffer_index + 1) % JITTER_MAX_SAMPLES;

  uint32_t jitter_sum = 0U;
  for (uint16_t i = 0U; i < JITTER_MAX_SAMPLES; i++) {
    jitter_sum += jitter_buffer[i];
  }
  uint32_t jitter_avg = jitter_sum / JITTER_MAX_SAMPLES;

  uint32_t jitter_squared_sum = 0U;
  for (uint16_t i = 0U; i < JITTER_MAX_SAMPLES; i++) {
    jitter_squared_sum += (jitter_buffer[i] - jitter_avg) * (jitter_buffer[i] - jitter_avg);
  }
  uint32_t jitter_squared_average = jitter_squared_sum / JITTER_MAX_SAMPLES;

  return (uint32_t)jitter_squared_average;
}

/* USER CODE END Application */

