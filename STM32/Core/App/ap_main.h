#ifndef APP_AP_MAIN_H_
#define APP_AP_MAIN_H_

#include "stm32f4xx_hal.h"
#include "Gyro_imu_svc/gyro_imu_svc.h"


#include "../driver/UART_COM/uart_com.h"
#include "ROBOT_SVC/robot_svc.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart2_rx;

extern TIM_HandleTypeDef htim3;

void ap_init();
void ap_exe();

#endif /* APP_APP_H_ */
