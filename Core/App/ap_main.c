
#include "app.h"


void ap_init() {
	lcd_init(&hi2c1);
	dht11_init();
	UART_COM_Init(&huart2);
	RtcClock_Init(&hrtc);
	Fan_Init(&htim3);
	Robot_Init();
}

void ap_exe() {

//	TempHumid_Execute();
//	RtcClock_Execute();
//	Fan_Execute();
	Robot_Execute();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART2) {
		UART_COM_RxEventHandler(Size);
	}
}
