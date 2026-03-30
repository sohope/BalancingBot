/*
 * uart_protocol.h
 *
 *  Created on: Mar 25, 2026
 *      Author: kccistc
 */

#ifndef DRIVER_UART_COM_UART_PROTOCOL_H_
#define DRIVER_UART_COM_UART_PROTOCOL_H_

#include "stm32f4xx_hal.h"
#include "string.h"
//frame define
#define PROTO_SOF          0xAA
#define PROTO_MAX_PAYLOAD  32
#define PROTO_HEADER_SIZE 3 // SOF+ LEN+ CMD
#define PROTO_CRC_SIZE     1
#define PROTO_MAX_PKT_SIZE (PROTO_HEADER_SIZE + PROTO_MAX_PAYLOAD + PROTO_CRC_SIZE)

//PC -> STM 32 TEMP HUMID CMD CODE
#define CMD_TEMP_HUMID_MIN 0x10
#define CMD_REQUEST_DATA 0x10
#define CMD_SET_INTERVAL 0x11
#define CMD_SET_BACKLIGT 0x12
#define CMD_PING 0x13
#define CMD_TEMP_HUMID_MAX 0x1F

//PC -> STM 32 RTC CMD CODE
#define CMD_RTC_MIN     0x20
#define CMD_GET_TIME 	0x20
#define CMD_SET_TIME    0x21
#define CMD_GET_DATE    0x22
#define CMD_SET_DATE    0x23
#define CMD_RTC_MAX     0x2F

// PC -> STM32 FAN CMD CODE
#define CMD_FAN_MIN			0x30
#define CMD_FAN_SET_MODE	0x30
#define CMD_FAN_SET_SPEED	0x31
#define CMD_FAN_SET_STATUS	0x32
#define CMD_FAN_MAX			0x3F

// PC -> STM32 BALANCING ROBOT CMD CODE
#define CMD_ROBOT_MIN       0x40
#define CMD_ROBOT_MOVE      0x40
#define CMD_ROBOT_SET_PID   0x41
#define CMD_ROBOT_SET_MODE  0x42
#define CMD_ROBOT_SET_ALPHA 0x43
#define CMD_ROBOT_MAX       0x4F


//STM32 -> RESPONSE CODE
#define CMD_SENSOR_DATA 0x81
#define CMD_ACK	        0x82
#define CMD_ERROR	    0x83
#define CMD_PONG	    0x84
#define CMD_TIME_DATA   0x85
#define CMD_DATE_DATA   0x86
#define CMD_FAN_STATUS	0x87
#define CMD_ROBOT_TELEMETRY 0x88

//ERROR CODE
#define ERR_DHT11_TIMEOUT  0x01
#define ERR_DHT11_CHECKSUM 0x02
#define ERR_INVALID_CMD    0x03
#define ERR_INVALID_CRC    0x04
#define ERR_INVALID_LEN    0x05

typedef enum {
	PARSE_STATE_SOF = 0,
	PARSE_STATE_LEN,
	PARSE_STATE_CMD,
	PARSE_STATE_PAYLOAD,
	PARSE_STATE_CRC
} ParseState_t;

typedef struct {
	uint8_t len;
	uint8_t cmd;
	uint8_t payload[PROTO_MAX_PAYLOAD];
} Packet_t;

typedef struct {
	ParseState_t state;
	uint8_t len;
	uint8_t cmd;
	uint8_t payload[PROTO_MAX_PAYLOAD];
	uint8_t payloadIdx;
} ParseCtx_t;

uint16_t Proto_BuildPacket(uint8_t *buf, uint8_t cmd, const uint8_t *payload,
		uint8_t payloadLen);
uint8_t Proto_CRC8(uint8_t *data, uint16_t len);

#endif /* DRIVER_UART_COM_UART_PROTOCOL_H_ */
