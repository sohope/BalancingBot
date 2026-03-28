#include "gyro_imu_svc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static UART_HandleTypeDef *_huart;
static uint8_t initialized_flag = 0;

static void print_uart(const char *str)
{
	HAL_UART_Transmit(_huart, (uint8_t*)str, strlen(str), 100);
}

void GyroImuSvc_Init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
	_huart = huart;

	if (HAL_I2C_IsDeviceReady(hi2c, MPU6050_ADDR, 3, 100) != HAL_OK) {
		print_uart("MPU6050 Not Found on I2C\r\n");
		return;
	}

	MPU6050_Status_t status = MPU6050_Init(hi2c);
	if (status == MPU6050_OK) {
		print_uart("MPU6050 Init OK\r\n");
		initialized_flag = 1;
	} else if (status == MPU6050_ERROR_WHO_AM_I) {
		print_uart("MPU6050 WHO_AM_I Failed\r\n");
	} else {
		print_uart("MPU6050 I2C Error\r\n");
	}
}

void GyroImuSvc_Execute(void)
{
	if (!initialized_flag) return;

	static uint32_t lastRead = 0;
	uint32_t now = HAL_GetTick();
	if ((now - lastRead) < READ_INTERVAL_MS) return;
	lastRead = now;

	MPU6050_RawData_t raw;
	if (MPU6050_ReadRaw(&raw) != MPU6050_OK) return;

	float ax = raw.accel_x / ACCEL_SCALE;
	float ay = raw.accel_y / ACCEL_SCALE;
	float az = raw.accel_z / ACCEL_SCALE;
	float gx = raw.gyro_x  / GYRO_SCALE;
	float gy = raw.gyro_y  / GYRO_SCALE;
	float gz = raw.gyro_z  / GYRO_SCALE;

	static uint32_t lastPrint = 0;
	if ((now - lastPrint) >= PRINT_INTERVAL_MS) {
		lastPrint = now;

		char buf[120];
		sprintf(buf, "AX:%d.%02d AY:%d.%02d AZ:%d.%02d GX:%d.%d GY:%d.%d GZ:%d.%d\r\n",
				(int)ax, abs((int)(ax * 100) % 100),
				(int)ay, abs((int)(ay * 100) % 100),
				(int)az, abs((int)(az * 100) % 100),
				(int)gx, abs((int)(gx * 10) % 10),
				(int)gy, abs((int)(gy * 10) % 10),
				(int)gz, abs((int)(gz * 10) % 10));
		print_uart(buf);
	}
}
