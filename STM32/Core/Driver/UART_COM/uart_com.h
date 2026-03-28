/*
 * uart_com.h
 *
 * Created on: Mar 25, 2026
 * Author: kccistc
 */

#ifndef DRIVER_UART_COM_UART_COM_H_
#define DRIVER_UART_COM_UART_COM_H_

#include "stm32f4xx_hal.h"
#include "uart_protocol.h" // Packet_t 구조체가 정의되어 있을 것으로 예상됨

#define UART_RX_DMA_BUF_SIZE 64

void Proto_ParseReset(ParseCtx_t *ctx);
uint8_t Proto_ParseByte(ParseCtx_t *ctx, uint8_t byte, Packet_t *out);

void UART_COM_Init(UART_HandleTypeDef *huart);


uint8_t UART_COM_TempHumid_isRxReady(void);
Packet_t* UART_COM_TempHumid_GetPacket(void);


uint8_t UART_COM_RTC_isRxReady(void);
Packet_t* UART_COM_RTC_GetPacket(void);

uint8_t UART_COM_FAN_isRxReady(void);
Packet_t* UART_COM_FAN_GetPacket(void);

uint8_t UART_COM_ROBOT_isRxReady(void);
Packet_t* UART_COM_ROBOT_GetPacket(void);

/* 공통 송신 및 수신 이벤트 핸들러 */
void UART_COM_SendPacket(uint8_t cmd, const uint8_t *payload, uint8_t payloadLen);
void UART_COM_RxEventHandler(uint16_t size);

#endif /* DRIVER_UART_COM_UART_COM_H_ */
