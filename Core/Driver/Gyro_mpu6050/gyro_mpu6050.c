#include "gyro_mpu6050.h"

static I2C_HandleTypeDef *_hi2c;

MPU6050_Status_t MPU6050_WriteReg(uint8_t reg, uint8_t value)
{
	if (HAL_I2C_Mem_Write(_hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100) != HAL_OK)
		return MPU6050_ERROR_I2C;
	return MPU6050_OK;
}

MPU6050_Status_t MPU6050_ReadReg(uint8_t reg, uint8_t *buf, uint16_t len)
{
	if (HAL_I2C_Mem_Read(_hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) != HAL_OK)
		return MPU6050_ERROR_I2C;
	return MPU6050_OK;
}

MPU6050_Status_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
	_hi2c = hi2c;

	// WHO_AM_I 확인
	uint8_t whoami = 0;
	if (MPU6050_ReadReg(MPU6050_REG_WHO_AM_I, &whoami, 1) != MPU6050_OK)
		return MPU6050_ERROR_I2C;
	if (whoami != MPU6050_WHO_AM_I_VAL)
		return MPU6050_ERROR_WHO_AM_I;

	// Wake up: sleep 비트 해제, Gyro X PLL: x축 시계 사용
	if (MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x01) != MPU6050_OK)
		return MPU6050_ERROR_I2C;
	HAL_Delay(10);

	// 기본 샘플링 레이트 = 1000Hz, Division 9 -> 1000Hz/10 = 100Hz 
	if (MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 9) != MPU6050_OK)
		return MPU6050_ERROR_I2C;

	// DLPF = 44Hz bandwidth
	if (MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03) != MPU6050_OK)
		return MPU6050_ERROR_I2C;

	/* Gyro: +-250 deg/s */
	if (MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00) != MPU6050_OK)
		return MPU6050_ERROR_I2C;

	/* Accel: +-2g */
	if (MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00) != MPU6050_OK)
		return MPU6050_ERROR_I2C;

	return MPU6050_OK;
}

MPU6050_Status_t MPU6050_ReadRaw(MPU6050_RawData_t *data)
{
	uint8_t buf[14];

	/* 0x3B ~ 0x48: accel(6) + temp(2) + gyro(6) = 14 bytes */
	if (MPU6050_ReadReg(MPU6050_REG_ACCEL_XOUT_H, buf, 14) != MPU6050_OK)
		return MPU6050_ERROR_I2C;

	data->accel_x = (int16_t)(buf[0] << 8 | buf[1]);
	data->accel_y = (int16_t)(buf[2] << 8 | buf[3]);
	data->accel_z = (int16_t)(buf[4] << 8 | buf[5]);
	data->temp_raw = (int16_t)(buf[6] << 8 | buf[7]);
	data->gyro_x = (int16_t)(buf[8] << 8 | buf[9]);
	data->gyro_y = (int16_t)(buf[10] << 8 | buf[11]);
	data->gyro_z = (int16_t)(buf[12] << 8 | buf[13]);

	return MPU6050_OK;
}
