#include "ap_main.h"

//----- 리재 모터 드라이브 추가
#include "ROBOT_SVC/robot_svc.h"
#include "PID_SVC/PID_svc.h"
#define MOTOR_TEST_MODE  01
//#define MOTOR_TEST_MODE  0 테스트 끝나면 0으로
//-----

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

//----- 리재 모터 드라이브 추가
extern TIM_HandleTypeDef htim3;
//-----
//---------리재 test
static void Motor_Test_Execute(void)
{
    static uint8_t test_done = 0;
    static uint32_t start_tick = 0;
    uint32_t tick = 0;

    if (test_done) {
        DC_Motor_StopAll();
        return;
    }

    if (start_tick == 0) {
        start_tick = HAL_GetTick();
    }

    tick = HAL_GetTick() - start_tick;

    // 0~2초 : 전진
    if (tick < 2000) {
        DC_Motor_SetLeftRight(800, 800);
    }
    // 2~3초 : 정지
    else if (tick < 3000) {
        DC_Motor_StopAll();
    }
    // 3~5초 : 후진
    else if (tick < 5000) {
        DC_Motor_SetLeftRight(-800, -800);
    }
    // 5~6초 : 정지
    else if (tick < 6000) {
        DC_Motor_StopAll();
    }
    // 6~8초 : 좌회전
    else if (tick < 8000) {
        DC_Motor_SetLeftRight(-800, 800);
    }
    // 8~9초 : 정지
    else if (tick < 9000) {
        DC_Motor_StopAll();
    }
    // 9~11초 : 우회전
    else if (tick < 11000) {
        DC_Motor_SetLeftRight(800, -800);
    }
    // 11초 이후 : 완전 종료
    else {
        DC_Motor_StopAll();
        test_done = 1;
    }
}
//---------


void ap_init()
{
	GyroImuSvc_Init(&hi2c1, &huart2);
	UART_COM_Init(&huart6);
	Robot_Init();
	pid_svc_init(15.0f, 1.0f, 0.5f);
	//리재추가
	DC_Motor_Init(&htim3);
}

void ap_exe()
{
#if MOTOR_TEST_MODE
	Motor_Test_Execute();
#else
	GyroImuSvc_Execute();
	Robot_Execute();
	pid_svc_exe();
#endif
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART6) {
		UART_COM_RxEventHandler(Size);
	}
}

// 타이머 인터럽트
void ap_timer_10ms_callback()
{
	ap_exe();
}
