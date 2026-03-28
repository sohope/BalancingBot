#include "ap_main.h"
#include "PID_SVC/PID_svc.h"

void ap_init()
{
	// 서비스 초기화 호출
	pid_svc_init(15.0f, 1.0f, 0.5f);
}

void ap_exe()
{
	// 서비스 실행 호출
	pid_svc_exe();
}

// 타이머 인터럽트
void ap_timer_10ms_callback()
{
	ap_exe();
}
