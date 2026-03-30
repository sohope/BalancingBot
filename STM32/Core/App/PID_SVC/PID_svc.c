#include "PID_svc.h"
#include "../Gyro_imu_svc/gyro_imu_svc.h"
#include "../ROBOT_SVC/robot_svc.h"
#include "../../Driver/Dc_motor/dc_motor.h"

#include <stdlib.h>

// ==========================================
// 1. 설정값
// ==========================================
#define MOVE_ANGLE_GAIN   (-0.05f)  // speed 100 → 최대 5도 기울기
#define TURN_GAIN         0.3f     // turn 100 → 좌우 차이 30
#define RAMP_RATE         0.5f     // target_angle 초당 변화 최대 (도/루프)

// ==========================================
// 2. 전역 변수
// ==========================================
PID_Controller bal_pid;
static float s_ramped_angle = 0.0f;

// ==========================================
// 3. PID 계산
// ==========================================
static void PID_Compute_Internal(void) {
	bal_pid.angle_error = bal_pid.target_angle - bal_pid.current_angle;
	bal_pid.p_term = bal_pid.pid_kp * bal_pid.angle_error;
	bal_pid.i_integral += bal_pid.angle_error * bal_pid.dt;
	bal_pid.i_term = bal_pid.pid_ki * bal_pid.i_integral;
	bal_pid.d_term = -(bal_pid.pid_kd * bal_pid.current_rate);

	float output = bal_pid.p_term + bal_pid.i_term + bal_pid.d_term;

	if(output > 100.0f) { output = 100.0f; bal_pid.i_integral -= bal_pid.angle_error * bal_pid.dt; }
	else if(output < -100.0f) { output = -100.0f; bal_pid.i_integral -= bal_pid.angle_error * bal_pid.dt; }

	bal_pid.balance_output = output;
}

// ==========================================
// 4. API 함수
// ==========================================
void pid_svc_init(float kp, float ki, float kd) {
	bal_pid.pid_kp = kp;
	bal_pid.pid_ki = ki;
	bal_pid.pid_kd = kd;
	bal_pid.target_angle = 0.0f;
	bal_pid.i_integral = 0.0f;
	bal_pid.dt = 0.01f;
}

void pid_svc_exe(void) {
	// 센서 읽기
	bal_pid.current_angle = GyroImuSvc_GetAngle();
	bal_pid.current_rate = GyroImuSvc_GetRate();

	// 이동 명령 → target_angle 서서히 변경 (ramping)
	float desired_angle = (float)g_target_speed * MOVE_ANGLE_GAIN;
	if (s_ramped_angle < desired_angle) {
		s_ramped_angle += RAMP_RATE;
		if (s_ramped_angle > desired_angle) s_ramped_angle = desired_angle;
	} else if (s_ramped_angle > desired_angle) {
		s_ramped_angle -= RAMP_RATE;
		if (s_ramped_angle < desired_angle) s_ramped_angle = desired_angle;
	}
	bal_pid.target_angle = s_ramped_angle;

	// PID 계산
	PID_Compute_Internal();

	// 회전은 PID 출력 위에 좌우 차이로 적용
	float turn = (float)g_target_turn * TURN_GAIN;
	float left_cmd  = bal_pid.balance_output + turn;
	float right_cmd = bal_pid.balance_output - turn;

	// 클램프
	if(left_cmd > 100.0f) left_cmd = 100.0f;
	else if(left_cmd < -100.0f) left_cmd = -100.0f;
	if(right_cmd > 100.0f) right_cmd = 100.0f;
	else if(right_cmd < -100.0f) right_cmd = -100.0f;

	DC_Motor_SetLeftRight((int16_t)left_cmd, (int16_t)right_cmd);
}

void pid_svc_set_gain(char type, float value) {
	switch(type) {
		case 'P': bal_pid.pid_kp = value; break;
		case 'I': bal_pid.pid_ki = value; bal_pid.i_integral = 0.0f; break;
		case 'D': bal_pid.pid_kd = value; break;
	}
}
