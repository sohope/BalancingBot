#include "PID_svc.h"
#include "../Gyro_imu_svc/gyro_imu_svc.h"

// ==========================================
// 1. 내부에서만 쓸 구조체들
// ==========================================
typedef enum { CMD_STOP = 0, CMD_FWD, CMD_BWD, CMD_LEFT, CMD_RIGHT } MoveCmd_t;

typedef struct {
	float balance_output;
	MoveCmd_t move_cmd;
	float move_speed_cmd;
	float turn_gain;
	float move_gain;
	float move_output;
	float turn_output;
	float left_cmd;
	float right_cmd;
} Move_Synthesizer;

// ==========================================
// 2. 전역 변수 (밖에서 절대 접근 못하게 static 처리)
// ==========================================
PID_Controller bal_pid;
static Move_Synthesizer move_sync;

// TODO: 동료 모터 드라이버 구현 후 교체
static void Driver_Set_Motor(float left_pwm, float right_pwm) {
	(void)left_pwm;
	(void)right_pwm;
}

// ==========================================
// 3. 내부 계산용 함수들 (원래 PID.c에 있던 로직들)
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

static void Move_Compute_Internal(void) {
	move_sync.move_output = 0.0f;
	move_sync.turn_output = 0.0f;

	if (move_sync.move_cmd == CMD_FWD) move_sync.move_output = move_sync.move_speed_cmd * move_sync.move_gain;
	else if (move_sync.move_cmd == CMD_BWD) move_sync.move_output = -(move_sync.move_speed_cmd * move_sync.move_gain);
	else if (move_sync.move_cmd == CMD_LEFT) move_sync.turn_output = move_sync.move_speed_cmd * move_sync.turn_gain;
	else if (move_sync.move_cmd == CMD_RIGHT) move_sync.turn_output = -(move_sync.move_speed_cmd * move_sync.turn_gain);

	move_sync.left_cmd = move_sync.balance_output + move_sync.move_output + move_sync.turn_output;
	move_sync.right_cmd = move_sync.balance_output + move_sync.move_output - move_sync.turn_output;
}

// ==========================================
// 4. 외부로 열어주는 진짜 API 함수들 (사장님이 부르는 버튼)
// ==========================================
void pid_svc_init(float kp, float ki, float kd) {
	bal_pid.pid_kp = kp;
	bal_pid.pid_ki = ki;
	bal_pid.pid_kd = kd;
	bal_pid.target_angle = 0.0f;
	bal_pid.i_integral = 0.0f;
	bal_pid.dt = 0.01f;

	move_sync.move_cmd = CMD_STOP;
	move_sync.move_speed_cmd = 0.0f;
	move_sync.turn_gain = 0.5f;
	move_sync.move_gain = 0.5f;
}

void pid_svc_exe(void) {
	bal_pid.current_angle = GyroImuSvc_GetAngle();
	bal_pid.current_rate = GyroImuSvc_GetRate();

	PID_Compute_Internal();

	move_sync.balance_output = bal_pid.balance_output;
	Move_Compute_Internal();

	if(move_sync.left_cmd > 100.0f) move_sync.left_cmd = 100.0f;
	else if(move_sync.left_cmd < -100.0f) move_sync.left_cmd = -100.0f;
	if(move_sync.right_cmd > 100.0f) move_sync.right_cmd = 100.0f;
	else if(move_sync.right_cmd < -100.0f) move_sync.right_cmd = -100.0f;

	Driver_Set_Motor(move_sync.left_cmd, move_sync.right_cmd);
}

void pid_svc_set_gain(char type, float value) {
	switch(type) {
		case 'P': bal_pid.pid_kp = value; break;
		case 'I': bal_pid.pid_ki = value; bal_pid.i_integral = 0.0f; break;
		case 'D': bal_pid.pid_kd = value; break;
	}
}
