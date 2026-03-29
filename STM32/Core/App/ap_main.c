#include "ap_main.h"
#include "ROBOT_SVC/robot_svc.h"
#include "PID_SVC/PID_svc.h"

extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim3;

void ap_init()
{
	GyroImuSvc_Init(&hi2c1);
	UART_COM_Init(&huart6);
	Robot_Init();
	pid_svc_init(15.0f, 1.0f, 0.5f);
	DC_Motor_Init(&htim3);
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
