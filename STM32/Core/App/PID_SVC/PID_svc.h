#ifndef APP_PID_H_
#define APP_PID_H_

typedef struct
{
	float target_angle;
	float current_angle;
	float current_rate;
	float dt;
	float pid_kp;
	float pid_ki;
	float pid_kd;

	float i_integral;

	float angle_error;
	float p_term;
	float i_term;
	float d_term;
	float balance_output;

}PID_Controller;

void pid_svc_init(float kp, float ki, float kd);
void pid_svc_exe(void);
void pid_svc_set_gain(char type, float value);

#endif /* APP_PID_H_ */
