/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usart.h"
#include "iwdg.h"
#include "cJSON.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t DO_AT[]={0x03, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC4, 0x2A};//DissolvedOxygen
uint8_t PH_AT[]={0x03, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC4, 0x2A};//PH 
uint8_t ORP_AT[]={0x03, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC4, 0x2A};//ORP 
extern uint8_t  DissolvedOxygen[256];		//溶解養
extern float DO_Saturation; //溶解氧飽和度
extern float DO_Concentration; //溶解氧濃度
extern float Temperature; //溫度
extern float PH; //酸鹼值
extern float ORP; //氧化還原電位
extern uint8_t RX1Data;
extern uint8_t RX1Buffer[256];
extern uint8_t RX6Data;
int TimeCount=0;
int SentDataTask=0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osThreadId myTask03Handle;
osMessageQId myQueue01Handle;
osTimerId myTimer01Handle;
osSemaphoreId myBinarySem01Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void SentDataSensor();
float ParseFloat(uint8_t *data);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);
void StartTask03(void const * argument);
void Callback01(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

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

  /* Create the semaphores(s) */
  /* definition and creation of myBinarySem01 */
  osSemaphoreDef(myBinarySem01);
  myBinarySem01Handle = osSemaphoreCreate(osSemaphore(myBinarySem01), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of myTimer01 */
  osTimerDef(myTimer01, Callback01);
  myTimer01Handle = osTimerCreate(osTimer(myTimer01), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of myQueue01 */
  osMessageQDef(myQueue01, 16, uint16_t);
  myQueue01Handle = osMessageCreate(osMessageQ(myQueue01), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityIdle, 0, 256);
  myTask02Handle = osThreadCreate(osThread(myTask02), NULL);

  /* definition and creation of myTask03 */
  osThreadDef(myTask03, StartTask03, osPriorityIdle, 0, 128);
  myTask03Handle = osThreadCreate(osThread(myTask03), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	HAL_UART_Receive_IT(&huart1, &RX1Data, 1);
	HAL_UART_Receive_IT(&huart6, &RX6Data, 1);	
	
  /* Infinite loop */
  for(;;)
  {
		UART_SEND1(DO_AT);
		//SentDataTask=1;
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
		if(SentDataTask==1)
		{
//			DO_Saturation = ParseFloat(&DissolvedOxygen[3]);  // 溶解氧飽和度
//			DO_Concentration = ParseFloat(&DissolvedOxygen[7]);  // 溶解氧濃度
//			Temperature = ParseFloat(&DissolvedOxygen[11]); // 溫度
//			memset(DissolvedOxygen, 0, sizeof(DissolvedOxygen));
			SentDataSensor();
			SentDataTask=0;
		}
		osDelay(500);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void const * argument)
{
  /* USER CODE BEGIN StartTask03 */
  /* Infinite loop */
  for(;;)
  {
		HAL_IWDG_Refresh(&hiwdg);
    osDelay(50);
  }
  /* USER CODE END StartTask03 */
}

/* Callback01 function */
void Callback01(void const * argument)
{
  /* USER CODE BEGIN Callback01 */
	TimeCount++;
	if(TimeCount%5)
	{
		//SentDataSensor();
	}
	if(TimeCount>59)
	{		
		TimeCount=0;	
	}
  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void SentDataSensor()
{
    cJSON *DataObject;
    char *data;

    // 創建 JSON 對象
    DataObject = cJSON_CreateObject();

    // 添加操作類型
    cJSON_AddItemToObject(DataObject, "Operating", cJSON_CreateString("UpdateSensorData"));

    // 添加溶解氧數據
    char DO_Saturation_String[16];
    char DO_Concentration_String[16];
    char Temperature_String[16];
		char PH_String[16];
		char ORP_String[16];
    // 將 float 數據轉換為字串
    snprintf(DO_Saturation_String, sizeof(DO_Saturation_String), "%.2f", DO_Saturation);
    snprintf(DO_Concentration_String, sizeof(DO_Concentration_String), "%.2f", DO_Concentration);
    snprintf(Temperature_String, sizeof(Temperature_String), "%.2f", Temperature);
    snprintf(PH_String, sizeof(PH_String), "%.2f", PH);
    snprintf(ORP_String, sizeof(ORP_String), "%.2f", ORP);	

    // 將字串添加到 JSON 對象
    cJSON_AddItemToObject(DataObject, "DO_Saturation", cJSON_CreateString(DO_Saturation_String));
    cJSON_AddItemToObject(DataObject, "DO_Concentration", cJSON_CreateString(DO_Concentration_String));
    cJSON_AddItemToObject(DataObject, "Temperature", cJSON_CreateString(Temperature_String));
		cJSON_AddItemToObject(DataObject, "PH", cJSON_CreateString(PH_String));
		cJSON_AddItemToObject(DataObject, "ORP", cJSON_CreateString(ORP_String));
		memset(DO_Saturation_String, 0, sizeof(DO_Saturation_String));
		memset(DO_Concentration_String, 0, sizeof(DO_Concentration_String));
		memset(Temperature_String, 0, sizeof(Temperature_String));
		memset(PH_String, 0, sizeof(PH_String));
		memset(ORP_String, 0, sizeof(ORP_String));
		DO_Saturation=0;
		DO_Concentration=0;
		Temperature=0;
		PH=0;
		ORP=0;
    // 生成 JSON 字串
    data = cJSON_Print(DataObject);

    // 發送 JSON 封包（假設 UART_SEND6 是發送函式）
    UART_SEND6(data);
		
    // 刪除 JSON 對象，釋放記憶體
    cJSON_Delete(DataObject);
    free(data);
}

//float ParseFloat(uint8_t *data)
//{
//    uint32_t temp = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
//    float result;
//    memcpy(&result, &temp, sizeof(float));
//    return result;
//}

/* USER CODE END Application */
