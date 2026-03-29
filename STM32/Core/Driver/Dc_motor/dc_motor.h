/*
 * dc_motor.h
 *
 *  Created on: Mar 29, 2026
 *      Author: kccistc
 */

#ifndef __DC_MOTOR_H__
#define __DC_MOTOR_H__

#include "main.h"
#include <stdint.h>

void DC_Motor_Init(TIM_HandleTypeDef *htim_pwm);
void DC_Motor_StopAll(void);
void DC_Motor_SetLeftRight(int16_t left_cmd, int16_t right_cmd);

uint16_t DC_Motor_GetLeftPwm(void);
uint16_t DC_Motor_GetRightPwm(void);
int16_t  DC_Motor_GetLeftCmd(void);
int16_t  DC_Motor_GetRightCmd(void);

#endif /* __DC_MOTOR_H__ */
