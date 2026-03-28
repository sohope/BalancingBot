#include "ap_main.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

void ap_init()
{
	GyroImuSvc_Init(&hi2c1, &huart2);
	UART_COM_Init(&huart6);
	Robot_Init();
}

void ap_exe()
{
	GyroImuSvc_Execute(); 
	Robot_Execute();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART6) {
		UART_COM_RxEventHandler(Size);
	}
}
