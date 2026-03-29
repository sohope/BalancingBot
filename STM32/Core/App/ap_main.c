#include "ap_main.h"
#include "ROBOT_SVC/robot_svc.h"
#include "PID_SVC/PID_svc.h"
#include "../Driver/Button/button.h"

extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim3;

static hBtn_Def s_user_btn;

void ap_init()
{
	GyroImuSvc_Init(&hi2c1);
	UART_COM_Init(&huart6);
	Robot_Init();
	pid_svc_init(15.0f, 1.0f, 0.5f);
	DC_Motor_Init(&htim3);
	Button_Init(&s_user_btn, GPIOC, GPIO_PIN_13);
}

void ap_exe()
{
	// 버튼으로 Balance <-> Stop 토글
	if (Button_GetState(&s_user_btn) == ACT_RELEASED) {
		g_robot_mode = (g_robot_mode == 0) ? 1 : 0;
		if (g_robot_mode == 0) {
			DC_Motor_StopAll();
		}
	}

	GyroImuSvc_Execute();
	Robot_Execute();

	if (g_robot_mode == 1) {
		pid_svc_exe();
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART6) {
		UART_COM_RxEventHandler(Size);
	}
}
