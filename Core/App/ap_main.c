
#include "ap_main.h"


void ap_init() {


	UART_COM_Init(&huart6);


	Robot_Init();
}

void ap_exe() {


	Robot_Execute();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance == USART6) {
		UART_COM_RxEventHandler(Size);
	}
}
