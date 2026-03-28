#ifndef APP_GYRO_IMU_SVC_GYRO_IMU_SVC_H_
#define APP_GYRO_IMU_SVC_GYRO_IMU_SVC_H_

#include "../../Driver/Gyro_mpu6050/gyro_mpu6050.h"

#define ACCEL_SCALE			16384.0f
#define GYRO_SCALE			131.0f
#define TEMP_SCALE			340.0f
#define TEMP_OFFSET			36.53f

#define READ_INTERVAL_MS	10
#define PRINT_INTERVAL_MS	500

void GyroImuSvc_Init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);
void GyroImuSvc_Execute(void);

#endif /* APP_GYRO_IMU_SVC_GYRO_IMU_SVC_H_ */
