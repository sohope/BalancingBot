#ifndef DRIVER_GYRO_MPU6050_GYRO_MPU6050_H_
#define DRIVER_GYRO_MPU6050_GYRO_MPU6050_H_

#include "stm32f4xx_hal.h"

/* I2C Address (AD0 = GND) */
#define MPU6050_ADDR			(0x68 << 1)

/* Register Map */
#define MPU6050_REG_SMPLRT_DIV		0x19
#define MPU6050_REG_CONFIG			0x1A
#define MPU6050_REG_GYRO_CONFIG		0x1B
#define MPU6050_REG_ACCEL_CONFIG	0x1C
#define MPU6050_REG_ACCEL_XOUT_H	0x3B
#define MPU6050_REG_GYRO_XOUT_H		0x43
#define MPU6050_REG_PWR_MGMT_1		0x6B
#define MPU6050_REG_WHO_AM_I		0x75

/* WHO_AM_I expected value */
#define MPU6050_WHO_AM_I_VAL	0x68

typedef enum {
	MPU6050_OK = 0,
	MPU6050_ERROR_I2C,
	MPU6050_ERROR_WHO_AM_I
} MPU6050_Status_t;

typedef struct {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	int16_t temp_raw;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
} MPU6050_RawData_t;

MPU6050_Status_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
MPU6050_Status_t MPU6050_ReadRaw(MPU6050_RawData_t *data);
MPU6050_Status_t MPU6050_WriteReg(uint8_t reg, uint8_t value);
MPU6050_Status_t MPU6050_ReadReg(uint8_t reg, uint8_t *buf, uint16_t len);

#endif /* DRIVER_GYRO_MPU6050_GYRO_MPU6050_H_ */
