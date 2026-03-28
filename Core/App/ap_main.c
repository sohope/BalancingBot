/*
 * ap_main.c
 *
 *  Created on: Mar 28, 2026
 *      Author: sohope
 */

#include "ap_main.h"
#include "PID.h"

typedef enum {
	CMD_STOP = 0,
	CMD_FWD,
	CMD_BWD,
	CMD_LEFT,
	CMD_RIGHT
}MoveCmd_t;

typedef struct
{
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

PID_Controller bal_pid;
Move_Synthesizer move_sync;

//Driver 계층
 extern float Driver_Get_Angle(void);
 extern float Driver_Get_Rate(void);
 extern void Driver_Set_Motor(float left_pwm, float right_pwm);


 void ap_timer_10ms_callback()
 {
 	bal_pid.current_angle = Driver_Get_Angle();
 	bal_pid.current_rate= Driver_Get_Rate();

 	PID_Compute(&bal_pid);

 	move_sync.balance_output = bal_pid.balance_output;
 	Move_Compute(&move_sync);

 	if(move_sync.left_cmd > 100.0f) move_sync.left_cmd = 100.0f;
 	else if(move_sync.left_cmd< -100.0f) move_sync.left_cmd = -100.0f;

 	if(move_sync.right_cmd > 100.0f) move_sync.right_cmd = 100.0f;
 	else if(move_sync.right_cmd< -100.0f) move_sync.right_cmd = -100.0f;

 	Driver_Set_Motor(move_sync.left_cmd, move_sync.right_cmd);
 }

 void Move_Compute(Move_Synthesizer *sync)
 {
	 sync->move_output = 0.0f;
	 sync->turn_output = 0.0f;

	 if (sync->move_cmd == CMD_FWD)
		 {
		 sync->move_output = sync->move_speed_cmd * sync->move_gain;
		 }
	 else if (sync->move_cmd == CMD_BWD)
	 	 {
			 sync->move_output = -(sync->move_speed_cmd * sync->move_gain);
		 }
	 else if (sync->move_cmd == CMD_LEFT)
		 {
		 sync->turn_output = sync->move_speed_cmd * sync->turn_gain;
		 }
 	 else if (sync->move_cmd == CMD_RIGHT)
 		 {
 		 sync->turn_output = -(sync->move_speed_cmd * sync->turn_gain);
 		 }

	 sync->left_cmd = sync->balance_output + sync->move_output + sync->turn_output;
	 sync->right_cmd = sync->balance_output + sync->move_output - sync->turn_output;
 }

void ap_init()
{
	PID_Init(&bal_pid, 15.0f, 1.0f, 0.5f); //임의의 초기 게인
	bal_pid.dt = 0.01f; // 10ms 주기 세팅

	move_sync.move_cmd = CMD_STOP;
	move_sync.move_speed_cmd = 0.0f;
	move_sync.turn_gain = 0.5f;
	move_sync.move_gain = 0.5f;
}

void ap_exe()
{

}



















