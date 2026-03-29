#include "gyro_imu_svc.h"
#include "../PID_SVC/PID_svc.h"
#include "../../Driver/UART_COM/uart_com.h"
#include "../../Driver/Dc_motor/dc_motor.h"
#include <string.h>
#include <math.h>

#define COMP_FILTER_ALPHA  0.98f
#define DEG_PER_RAD        (180.0f / (float)M_PI)
#define DT                 (READ_INTERVAL_MS / 1000.0f)
#define TELEMETRY_INTERVAL_MS  100

static uint8_t initialized_flag = 0;
static float s_angle = 0.0f;
static float s_gyro_rate = 0.0f;

void GyroImuSvc_Init(I2C_HandleTypeDef *hi2c)
{
	if (HAL_I2C_IsDeviceReady(hi2c, MPU6050_ADDR, 3, 100) != HAL_OK) {
		return;
	}

	MPU6050_Status_t status = MPU6050_Init(hi2c);
	if (status == MPU6050_OK) {
		initialized_flag = 1;
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

	// 텔레메트리 패킷 전송 (UART6)
	static uint32_t lastTelemetry = 0;
	if ((now - lastTelemetry) >= TELEMETRY_INTERVAL_MS) {
		lastTelemetry = now;

		extern PID_Controller bal_pid;
		int16_t angle_i    = (int16_t)(s_angle * 10.0f);
		int16_t rate_i     = (int16_t)(s_gyro_rate * 10.0f);
		int16_t pid_out_i  = (int16_t)(bal_pid.balance_output * 10.0f);
		int16_t left_cmd_i = DC_Motor_GetLeftCmd();
		int16_t right_cmd_i = DC_Motor_GetRightCmd();

		uint8_t payload[10];
		payload[0] = (uint8_t)(angle_i >> 8);
		payload[1] = (uint8_t)(angle_i & 0xFF);
		payload[2] = (uint8_t)(rate_i >> 8);
		payload[3] = (uint8_t)(rate_i & 0xFF);
		payload[4] = (uint8_t)(pid_out_i >> 8);
		payload[5] = (uint8_t)(pid_out_i & 0xFF);
		payload[6] = (uint8_t)(left_cmd_i >> 8);
		payload[7] = (uint8_t)(left_cmd_i & 0xFF);
		payload[8] = (uint8_t)(right_cmd_i >> 8);
		payload[9] = (uint8_t)(right_cmd_i & 0xFF);

		UART_COM_SendPacket(CMD_ROBOT_TELEMETRY, payload, 10);
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
