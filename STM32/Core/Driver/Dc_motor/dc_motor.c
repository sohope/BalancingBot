/*
 * dc_motor.c
 *
 *  Created on: Mar 29, 2026
 *      Author: kccistc
 */

#include "dc_motor.h"

#define DC_MOTOR_LEFT_PWM_CHANNEL   TIM_CHANNEL_1
#define DC_MOTOR_RIGHT_PWM_CHANNEL  TIM_CHANNEL_2

#define DC_MOTOR_DEADBAND           15U

#define DC_MOTOR_LEFT_REVERSE       0
#define DC_MOTOR_RIGHT_REVERSE      0

static TIM_HandleTypeDef *s_htim_pwm = NULL;

static int16_t  s_left_cmd  = 0;
static int16_t  s_right_cmd = 0;
static uint16_t s_left_pwm  = 0;
static uint16_t s_right_pwm = 0;

static uint16_t DC_Motor_GetPwmMax(void)
{
    if (s_htim_pwm == NULL) {
        return 0;
    }

    return (uint16_t)__HAL_TIM_GET_AUTORELOAD(s_htim_pwm);
}

static uint16_t DC_Motor_ClampAbsToPwm(int32_t value)
{
    uint16_t pwm_max = DC_Motor_GetPwmMax();

    if (value < 0) {
        value = -value;
    }

    if ((uint32_t)value < DC_MOTOR_DEADBAND) {
        return 0;
    }

    if ((uint32_t)value > pwm_max) {
        return pwm_max;
    }

    return (uint16_t)value;
}

static void DC_Motor_SetLeftDir(int8_t dir)
{
    if (dir > 0) {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    }
    else if (dir < 0) {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    }
}

static void DC_Motor_SetRightDir(int8_t dir)
{
    if (dir > 0) {
        HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);
    }
    else if (dir < 0) {
        HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);
    }
}

static void DC_Motor_ApplyLeft(int16_t cmd)
{
    int16_t applied_cmd = cmd;
    int8_t dir = 0;
    uint16_t pwm = 0;

#if (DC_MOTOR_LEFT_REVERSE == 1)
    applied_cmd = -applied_cmd;
#endif

    if (applied_cmd > 0) {
        dir = 1;
    }
    else if (applied_cmd < 0) {
        dir = -1;
    }
    else {
        dir = 0;
    }

    pwm = DC_Motor_ClampAbsToPwm(applied_cmd);

    if (pwm == 0U) {
        dir = 0;
    }

    DC_Motor_SetLeftDir(dir);
    __HAL_TIM_SET_COMPARE(s_htim_pwm, DC_MOTOR_LEFT_PWM_CHANNEL, pwm);

    s_left_cmd = cmd;
    s_left_pwm = pwm;
}

static void DC_Motor_ApplyRight(int16_t cmd)
{
    int16_t applied_cmd = cmd;
    int8_t dir = 0;
    uint16_t pwm = 0;

#if (DC_MOTOR_RIGHT_REVERSE == 1)
    applied_cmd = -applied_cmd;
#endif

    if (applied_cmd > 0) {
        dir = 1;
    }
    else if (applied_cmd < 0) {
        dir = -1;
    }
    else {
        dir = 0;
    }

    pwm = DC_Motor_ClampAbsToPwm(applied_cmd);

    if (pwm == 0U) {
        dir = 0;
    }

    DC_Motor_SetRightDir(dir);
    __HAL_TIM_SET_COMPARE(s_htim_pwm, DC_MOTOR_RIGHT_PWM_CHANNEL, pwm);

    s_right_cmd = cmd;
    s_right_pwm = pwm;
}

void DC_Motor_Init(TIM_HandleTypeDef *htim_pwm)
{
    s_htim_pwm = htim_pwm;

    HAL_TIM_PWM_Start(s_htim_pwm, DC_MOTOR_LEFT_PWM_CHANNEL);
    HAL_TIM_PWM_Start(s_htim_pwm, DC_MOTOR_RIGHT_PWM_CHANNEL);

    DC_Motor_StopAll();
}

void DC_Motor_StopAll(void)
{
    if (s_htim_pwm == NULL) {
        return;
    }

    DC_Motor_SetLeftDir(0);
    DC_Motor_SetRightDir(0);

    __HAL_TIM_SET_COMPARE(s_htim_pwm, DC_MOTOR_LEFT_PWM_CHANNEL, 0);
    __HAL_TIM_SET_COMPARE(s_htim_pwm, DC_MOTOR_RIGHT_PWM_CHANNEL, 0);

    s_left_cmd = 0;
    s_right_cmd = 0;
    s_left_pwm = 0;
    s_right_pwm = 0;
}

void DC_Motor_SetLeftRight(int16_t left_cmd, int16_t right_cmd)
{
    if (s_htim_pwm == NULL) {
        return;
    }

    DC_Motor_ApplyLeft(left_cmd);
    DC_Motor_ApplyRight(right_cmd);
}

uint16_t DC_Motor_GetLeftPwm(void)
{
    return s_left_pwm;
}

uint16_t DC_Motor_GetRightPwm(void)
{
    return s_right_pwm;
}

int16_t DC_Motor_GetLeftCmd(void)
{
    return s_left_cmd;
}

int16_t DC_Motor_GetRightCmd(void)
{
    return s_right_cmd;
}

