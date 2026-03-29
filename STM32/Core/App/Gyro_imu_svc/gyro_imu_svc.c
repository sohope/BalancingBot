#include "gyro_imu_svc.h"
#include "../PID_SVC/PID_svc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define COMP_FILTER_ALPHA  0.98f
#define DEG_PER_RAD        (180.0f / (float)M_PI)
#define DT                 (READ_INTERVAL_MS / 1000.0f)

static UART_HandleTypeDef *_huart;
static uint8_t initialized_flag = 0;
static float s_angle = 0.0f;
static float s_gyro_rate = 0.0f;

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

	// Complementary Filter 각도 계산
	float ay = (float)raw.accel_y / ACCEL_SCALE;
	float az = (float)raw.accel_z / ACCEL_SCALE;
	float gx = (float)raw.gyro_x / GYRO_SCALE;

	float accel_angle = atan2f(ay, az) * DEG_PER_RAD;
	s_gyro_rate = gx;
	s_angle = COMP_FILTER_ALPHA * (s_angle + gx * DT) + (1.0f - COMP_FILTER_ALPHA) * accel_angle;

	// 기존 UART 출력용 정수 변환
	int32_t ax_100 = (int32_t)raw.accel_x * 100 / (int32_t)16384;
	int32_t ay_100 = (int32_t)raw.accel_y * 100 / (int32_t)16384;
	int32_t az_100 = (int32_t)raw.accel_z * 100 / (int32_t)16384;
	int32_t gx_10  = (int32_t)raw.gyro_x  * 10  / (int32_t)131;
	int32_t gy_10  = (int32_t)raw.gyro_y  * 10  / (int32_t)131;
	int32_t gz_10  = (int32_t)raw.gyro_z  * 10  / (int32_t)131;

	static uint32_t lastPrint = 0;
	if ((now - lastPrint) >= PRINT_INTERVAL_MS) {
		lastPrint = now;

		extern PID_Controller bal_pid;
		int32_t an_10 = (int32_t)(s_angle * 10.0f);
		int32_t po_10 = (int32_t)(bal_pid.balance_output * 10.0f);

		char buf[160];
		sprintf(buf, "AX:%d.%02d AY:%d.%02d AZ:%d.%02d GX:%d.%d GY:%d.%d GZ:%d.%d AN:%d.%d PO:%d.%d\r\n",
				(int)(ax_100 / 100), abs((int)(ax_100 % 100)),
				(int)(ay_100 / 100), abs((int)(ay_100 % 100)),
				(int)(az_100 / 100), abs((int)(az_100 % 100)),
				(int)(gx_10 / 10), abs((int)(gx_10 % 10)),
				(int)(gy_10 / 10), abs((int)(gy_10 % 10)),
				(int)(gz_10 / 10), abs((int)(gz_10 % 10)),
				(int)(an_10 / 10), abs((int)(an_10 % 10)),
				(int)(po_10 / 10), abs((int)(po_10 % 10)));
		print_uart(buf);
	}
}

float GyroImuSvc_GetAngle(void)
{
	return s_angle;
}

float GyroImuSvc_GetRate(void)
{
	return s_gyro_rate;
}
