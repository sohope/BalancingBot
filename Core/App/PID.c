/*
 * PID.c
 *
 *  Created on: Mar 28, 2026
 *      Author: kccistc
 */

#include "PID.h"

void PID_Init(PID_Controller *pid, float kp, float ki, float kd)
{
	pid->pid_kp = kp;
	pid->pid_ki = ki;
	pid->pid_kd = kd;
	pid->target_angle = 0.0f;
	pid->i_integral = 0.0f;
}

void PID_Compute(PID_Controller *pid)
{
	//오차 계산
	pid->angle_error = pid->target_angle - pid->current_angle;
	//PID 계산
	pid->p_term = pid->pid_kp * pid->angle_error;
	pid->i_integral += pid->angle_error * pid->dt;
	pid->i_term = pid->pid_ki * pid->i_integral;
	pid->d_term = -(pid->pid_kd * pid->current_rate);

	//출력 합산 및 제한
	float output = pid->p_term + pid->i_term + pid->d_term;

	if(output>100.0f)
	{
		output = 100.0f;
		pid->i_integral -= pid->angle_error *pid->dt;
	}
	else if(output <-100.0f)
	{
		output = -100.0f;
		pid-> i_integral -= pid->angle_error * pid->dt;
	}
	pid->balance_output = output;
}
