#include "ap_main.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

void ap_init()
{
	GyroImuSvc_Init(&hi2c1, &huart2);
}

void ap_exe()
{
	GyroImuSvc_Execute();
}
