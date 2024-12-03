/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "string.h"
#include "usart.h"
#include "cJSON.h"
/* USER CODE END Includes */
#define DO_PACKET_LENGTH 17 // 溶解氧變送器數據包長度
#define PH_PACKET_LENGTH 9 // PH 變送器數據包長度
#define ORP_PACKET_LENGTH 9 // ORP 變送器數據包長度
#define FLOAT_DATA_LENGTH 4 // 每個浮點數據占 4 個字節

/* USER CODE BEGIN Variables */
uint8_t RX1Buffer[256];
uint8_t RX1Index=0;
uint8_t RX1Data;
char 	DissolvedOxygen[256];		//溶解養
float DO_Saturation; //溶解氧飽和度
float DO_Concentration; //溶解氧濃度
float Temperature; //溫度
float PH; //酸鹼值
float ORP; //氧化還原電位
extern int SentDataTask;
uint8_t address; // 取得地址碼
uint16_t expectedLength = 0;

uint8_t RX6Buffer[256];
uint8_t RX6Index=0;
uint8_t RX6Data;
char CopyRX6Buffer[256];
/* USER CODE END Variables */

/* USER CODE BEGIN Function */

uint16_t crc16(uint8_t *buffer, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= buffer[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

float ParseDO(uint8_t *data)
{
    uint32_t temp = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}
float ParsePH(uint8_t *data)
{
    uint16_t temp = (data[0] << 8) | data[1];
    // 將數據轉換為 PH 值
    float result = temp / 100.0f;
    return result;
}
float ParseORP(uint8_t *data)
{
    uint16_t temp = (data[0] << 8) | data[1];
    // 將數據轉換為 ORP 值（毫伏）
    float result = (float)temp;
    return result;
}

void UART_SEND1(uint8_t *data)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    HAL_UART_Transmit_IT(&huart1, &data[i], 1);
    while (huart1.gState != HAL_UART_STATE_READY);
  }
  osDelay(100);
}

void UART_SEND6(char *str)
{
	HAL_UART_Transmit_IT(&huart6, (uint8_t*)str,strlen(str));
	while(huart6.gState!=HAL_UART_STATE_READY);	
}

/**
 * @brief UART接收完成中斷回調函式
 *
 * @param huart UART句柄
 *
 * 此函式是UART接收完成中斷的回調函式，用於處理接收到的數據。
 * 根據不同的UART實例執行不同的操作。
 *
 * 對於USART1:
 * 1. 從RX1Buffer中提取8個2位元組的數值，並存入CurrentValue陣列中。
 * 2. 重新啟動USART1的中斷接收，等待下一次接收。
 *
 * 對於USART6:
 * 1. 將接收到的單個位元組存入RX2Buffer中。
 * 2. 檢查RX6Buffer中是否包含完整的JSON數據(以'{'開始，以'}'結束)。
 *    - 如果是完整的JSON數據，則複製RX6Buffer的內容到CopyRX6Buffer中，
 *    - 如果不是完整的JSON數據，則清空RX6Buffer。
 * 3. 重新啟動USART6的中斷接收，等待下一個位元組的接收。
 *
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
	if (huart->Instance == USART1)
	{
			// 將接收到的字節存入緩衝區
			RX1Buffer[RX1Index] = RX1Data;
			RX1Index++;

			// 檢查地址碼並根據不同變送器的數據包長度進行處理
			uint8_t address = RX1Buffer[0]; // 取得地址碼
			uint16_t expectedLength = 0;

			switch (address)
			{
			case 0x03: // 溶解氧變送器
					expectedLength = DO_PACKET_LENGTH;
					break;
			case 0x04: // PH 變送器
					expectedLength = PH_PACKET_LENGTH;
					break;
			case 0x05: // ORP 變送器
					expectedLength = ORP_PACKET_LENGTH;
					break;
			}

			// 如果接收到完整數據包
			if (RX1Index >= expectedLength)
			{
					// 計算接收到的數據的 CRC 校驗
					uint16_t receivedCRC = (RX1Buffer[expectedLength - 2] | (RX1Buffer[expectedLength - 1] << 8));
					uint16_t calculatedCRC = crc16(RX1Buffer, expectedLength - 2);

					if (receivedCRC == calculatedCRC) // crc校驗成功
					{
							switch (address)
							{
							case 0x03: // 溶解氧變送器
									DO_Saturation = ParseDO(&RX1Buffer[3]);  // 溶解氧飽和度
									DO_Concentration = ParseDO(&RX1Buffer[7]);  // 溶解氧濃度
									Temperature = ParseDO(&RX1Buffer[11]); // 溫度
									break;
							
							case 0x04: // PH 變送器
									PH = ParsePH(&RX1Buffer[3]); // PH 值
									break;

							case 0x05: // ORP 變送器
									ORP = ParseORP(&RX1Buffer[3]); // ORP 值
									SentDataTask=1;
									break;
							}
					}

					// 清空緩衝區並重置索引
					memset(RX1Buffer, 0, sizeof(RX1Buffer));
					RX1Index = 0;
			}
			// 重新啟動中斷接收
			HAL_UART_Receive_IT(&huart1, &RX1Data, 1);
	}
	if(huart->Instance==USART6)
	{
		// 處理從USART2接收到的數據
		RX6Buffer[RX6Index]=RX6Data;
		RX6Index++;
		
		// 檢查RX2Buffer中是否包含完整的JSON數據
		if(RX6Buffer[0]=='{'&&RX6Buffer[RX6Index-1]=='}')
		{
			// 複製完整的JSON數據到CopyRX2Buffer中
			memcpy(CopyRX6Buffer,RX6Buffer,RX6Index);
			// 從中斷程序中恢復Task03任務
			
			RX6Index=0;
		}	
		else if(RX6Buffer[0]!='{'||RX6Buffer[RX6Index-1]=='}')
		{	
			RX6Index=0;
		}
		// 重新啟動USART2的中斷接收
		HAL_UART_Receive_IT(&huart6, &RX6Data, 1);			
	}
}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//	UNUSED(huart);
//	if (huart->Instance == USART1)
//	{	
//			// 將接收到的字節存入緩衝區
//			RX1Buffer[RX1Index] = RX1Data;
//			RX1Index++;
//			// 如果接收到完整數據包
//			if (RX1Buffer[0]==0X03 && RX1Index >= 17)
//			{
//					memcpy(DissolvedOxygen,RX1Buffer,RX1Index);
//					SentDataTask=1;
//					memset(RX1Buffer, 0, sizeof(RX1Buffer));
//					RX1Index = 0;
//			}
//			// 重新啟動中斷接收
//			HAL_UART_Receive_IT(&huart1, &RX1Data, 1);
//	}
//	if(huart->Instance==USART6)
//	{
//		// 處理從USART2接收到的數據
//		RX6Buffer[RX6Index]=RX6Data;
//		RX6Index++;
//		
//		// 檢查RX2Buffer中是否包含完整的JSON數據
//		if(RX6Buffer[0]=='{'&&RX6Buffer[RX6Index-1]=='}')
//		{
//			// 複製完整的JSON數據到CopyRX2Buffer中
//			memcpy(CopyRX6Buffer,RX6Buffer,RX6Index);
//			// 從中斷程序中恢復Task03任務
//			
//			RX6Index=0;
//		}	
//		else if(RX6Buffer[0]!='{'||RX6Buffer[RX6Index-1]=='}')
//		{	
//			RX6Index=0;
//		}
//		// 重新啟動USART2的中斷接收
//		HAL_UART_Receive_IT(&huart6, &RX6Data, 1);			
//	}
//}
/* USER CODE  END  Function */