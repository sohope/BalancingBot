#ifndef APP_AP_MAIN_H_
#define APP_AP_MAIN_H_

#include "stm32f4xx_hal.h"
#include "../driver/UART_COM/uart_com.h"
#include "../driver/Dc_motor/dc_motor.h"
#include "Gyro_imu_svc/gyro_imu_svc.h"
#include "PID_SVC/PID_svc.h"
#include "ROBOT_SVC/robot_svc.h"

extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim3;

void ap_init();
void ap_exe();

#endif /* APP_AP_MAIN_H_ */
