/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "ic_app.h"
#include "ic_logger.h"
#include "usart.h"      // 為了拿到 huart2
#include "ic_logger.h"  // new log

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4, //128*4 too small
  .priority = (osPriority_t) osPriorityNormal,
};
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* IC: RTOS task define*/
osThreadId_t ledTaskHandle;
osThreadId_t tofTaskHandle;
osThreadId_t tempTaskHandle;

/* I2C mutex，用來保護 BMP280 / VL53L0X 的 I2C 存取 */
osMutexId_t i2cMutexHandle;

/* Logger queue + task */
osMessageQueueId_t g_log_queue;
osThreadId_t logTaskHandle;

osEventFlagsId_t g_app_init_done;
osEventFlagsId_t g_app_error_flags; //ic 260227


/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* IC: 建立 init 完成旗標 */
  g_app_init_done = osEventFlagsNew(NULL);
  g_app_error_flags = osEventFlagsNew(NULL); //ic 260227

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

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  

  /* IC: New log thread*/
  /* ---------- 建立 Log Queue ---------- */
  const osMessageQueueAttr_t logQueue_attributes = {
    .name = "logQueue"
  };
  g_log_queue = osMessageQueueNew(
      16,                     // queue 深度：一次最多排 16 條訊息
      sizeof(ic_log_msg_t),   // 每個元素大小
      &logQueue_attributes);

  /* ---------- 建立 Logger Task ---------- */
  const osThreadAttr_t logTask_attributes = {
    .name       = "logTask",
    .priority   = (osPriority_t)osPriorityLow, // log 優先權可以低一點
    //.priority   = (osPriority_t)osPriorityHigh,
    .stack_size =  512 * 4 //1024* 4
  };
  logTaskHandle = osThreadNew(ic_log_task, NULL, &logTask_attributes);

  /* IC: RTOS task define*/
  // 1) 建 I2C mutex
  const osMutexAttr_t i2cMutex_attributes = {
    .name = "i2cMutex"
  };
  i2cMutexHandle = osMutexNew(&i2cMutex_attributes);

  // 2) 建 LED task
  const osThreadAttr_t ledTask_attributes = {
    .name       = "ledTask",
    .priority   = (osPriority_t)osPriorityLow,
    .stack_size = 256 *4 //128 * 4    // 原本 128 * 4，先拉大到 4KB
  };
  ledTaskHandle = osThreadNew(ic_app_led_task, NULL, &ledTask_attributes);

  // 3) 建 TOF task
  const osThreadAttr_t tofTask_attributes = {
    .name       = "tofTask",
    .priority   = (osPriority_t)osPriorityNormal,
    .stack_size = 256 * 4    // 原本 128 * 4
  };
  tofTaskHandle = osThreadNew(ic_app_tof_task, NULL, &tofTask_attributes);

  // 4) 建 Temp task
  const osThreadAttr_t tempTask_attributes = {
    .name       = "tempTask",
    .priority   = (osPriority_t)osPriorityBelowNormal,
    .stack_size = 256 * 4    // 原本 256 * 4
  };
  tempTaskHandle = osThreadNew(ic_app_temp_task, NULL, &tempTask_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  //ic_log_init(&huart2);
  ic_app_init();
    
  /* Infinite loop */
  for(;;)
  {
    osDelay(10);      
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

