/*
 * uart_com.c
 *
 *  Created on: Mar 25, 2026
 *      Author: kccistc
 */

#include "uart_com.h"

ParseCtx_t s_parseCtx;
 uint8_t s_rxBuf[UART_RX_DMA_BUF_SIZE];
 uint8_t s_txBuf[PROTO_MAX_PKT_SIZE];
UART_HandleTypeDef *s_huart;
uint8_t rxReadyFlag = 0;

uint8_t s_dht11RxReady =0;
uint8_t s_rtcRxReady =0;
uint8_t s_fanRxReady =0;

Packet_t pkt;
Packet_t s_dht11Pkt;
Packet_t s_rtcPkt;
Packet_t s_fanPkt;

uint8_t s_robotRxReady = 0;
Packet_t s_robotPkt;

void UART_COM_Init(UART_HandleTypeDef *huart) {
	s_huart = huart;
	Proto_ParseReset(&s_parseCtx);
	memset(s_rxBuf, 0, sizeof(s_rxBuf));

	HAL_UARTEx_ReceiveToIdle_DMA(s_huart, s_rxBuf, UART_RX_DMA_BUF_SIZE);
	__HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
}

uint8_t UART_COM_TempHumid_isRxReady() {
	return s_dht11RxReady;
}

Packet_t* UART_COM_TempHumid_GetPacket() {
	s_dht11RxReady = 0;
	return &s_dht11Pkt;
}

uint8_t UART_COM_RTC_isRxReady() {
	return s_rtcRxReady;
}

Packet_t* UART_COM_RTC_GetPacket() {
	s_rtcRxReady = 0;
	return &s_rtcPkt;
}

uint8_t UART_COM_FAN_isRxReady() {
	return s_fanRxReady;
}

Packet_t* UART_COM_FAN_GetPacket() {
	s_fanRxReady = 0;
	return &s_fanPkt;
}

uint8_t UART_COM_ROBOT_isRxReady() {
	return s_robotRxReady;
}

Packet_t* UART_COM_ROBOT_GetPacket() {
    s_robotRxReady = 0;
    return &s_robotPkt;
}

void UART_COM_SendPacket(uint8_t cmd, const uint8_t *payload,
		uint8_t payloadLen) {
	uint16_t pktLen = Proto_BuildPacket(s_txBuf, cmd, payload, payloadLen);
	HAL_UART_Transmit(s_huart, s_txBuf, pktLen, 100);
}

void UART_COM_RxEventHandler(uint16_t size) {
	if (size == 0) {
		HAL_UARTEx_ReceiveToIdle_DMA(s_huart, s_rxBuf, UART_RX_DMA_BUF_SIZE);
		__HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
		return;
	}

	//rxReadyFlag = 0;
	Packet_t tempPkt;
	for (int i = 0; i < size; i++) {
		if (!Proto_ParseByte(&s_parseCtx, s_rxBuf[i], &tempPkt)) continue;

		//파싱완료 : cmd 범위에 따라 서비스별 버퍼에저장
		if(tempPkt.cmd >= CMD_TEMP_HUMID_MIN &&tempPkt.cmd <= CMD_TEMP_HUMID_MAX){
			s_dht11Pkt = tempPkt;
			s_dht11RxReady =1;
		} else if (tempPkt.cmd >= CMD_RTC_MIN &&tempPkt.cmd <= CMD_RTC_MAX){
			s_rtcPkt =tempPkt;
			s_rtcRxReady =1;
		} else if (tempPkt.cmd >= CMD_FAN_MIN &&tempPkt.cmd <= CMD_FAN_MAX){
			s_fanPkt =tempPkt;
			s_fanRxReady =1;
		}else if(tempPkt.cmd >= CMD_ROBOT_MIN && tempPkt.cmd <= CMD_ROBOT_MAX){
		    s_robotPkt = tempPkt;
		    s_robotRxReady = 1;
		}
	}
	memset(s_rxBuf,0,sizeof(s_rxBuf));
	HAL_UARTEx_ReceiveToIdle_DMA(s_huart, s_rxBuf, UART_RX_DMA_BUF_SIZE);
	__HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
}
