#include "ap_main.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

void ap_init()
{
	GyroImuSvc_Init(&hi2c1, &huart2);
	UART_COM_Init(&huart6);
	Robot_Init();
	pid_svc_init(15.0f, 1.0f, 0.5f);
}

void ap_exe()
{
	GyroImuSvc_Execute(); 
	Robot_Execute();
	pid_svc_exe();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART6) {
		UART_COM_RxEventHandler(Size);
	}
}

// 타이머 인터럽트
void ap_timer_10ms_callback()
{
	ap_exe();
}
