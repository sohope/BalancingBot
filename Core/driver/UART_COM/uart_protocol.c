/*
 * uart_protocol.c
 *
 *  Created on: Mar 25, 2026
 *      Author: kccistc
 */

#include "uart_protocol.h"

uint16_t Proto_BuildPacket(uint8_t *buf, uint8_t cmd, const uint8_t *payload,
		uint8_t payloadLen) {
	uint16_t idx = 0;

	//SOF
	buf[idx++] = PROTO_SOF; //0xAA
	//LEN
	buf[idx++] = payloadLen;
	//CMD
	buf[idx++] = cmd;
	//PAYLOAD
	for (int i = 0; i < payloadLen; i++) {
		buf[idx++] = payload[i];
	}
//	if(payloadLen > 0 && payload != NULL){
//		memcpy(&buf[idx],payload,payloadLen);
//		idx += payloadLen;
//	}

	//CRC8 [LEN,CMD,PAYLOAD..)
	buf[idx] = Proto_CRC8(&buf[1], 2 + payloadLen); // len+cmd+playloadLen
	idx++;

	return idx;
}

uint8_t Proto_CRC8(uint8_t *data, uint16_t len) {
	uint8_t checksum = 0;
	for (int i = 0; i < len; i++) {
		checksum ^= data[i];
	}
	return checksum;
}

void Proto_ParseReset(ParseCtx_t *ctx) {
	ctx->state = PARSE_STATE_SOF;
	ctx->len = 0;
	ctx->cmd = 0;
	ctx->payloadIdx = 0;
}

uint8_t Proto_ParseByte(ParseCtx_t *ctx, uint8_t byte, Packet_t *out) {
	switch (ctx->state) {
	case PARSE_STATE_SOF:
		if (byte == PROTO_SOF) {
			ctx->state = PARSE_STATE_LEN;
		}
		break;
	case PARSE_STATE_LEN:
		if (byte > PROTO_MAX_PAYLOAD) {
			Proto_ParseReset(ctx);
		} else {
			ctx->len = byte;
			ctx->state = PARSE_STATE_CMD;
		}
		break;
	case PARSE_STATE_CMD:
		ctx->cmd =byte;
		ctx->payloadIdx =0;
		ctx->state =(ctx->len ==0) ? PARSE_STATE_CRC : PARSE_STATE_PAYLOAD;
		break;
	case PARSE_STATE_PAYLOAD:
		ctx->payload[ctx->payloadIdx++] =byte;
		if(ctx->payloadIdx >= ctx->len){
			ctx->state = PARSE_STATE_CRC;
		}
		break;
	case PARSE_STATE_CRC:
		//CRC : buf =[LEN,CMD,PAYLOAD..]
		uint8_t crcBuf[PROTO_MAX_PAYLOAD +2];
		crcBuf[0] = ctx->len;
		crcBuf[1] = ctx->cmd;
		memcpy(&crcBuf[2],ctx->payload,ctx->len);
		uint8_t expected =Proto_CRC8(crcBuf,2+ctx->len);

		if(byte == expected){
			out ->len = ctx->len;
			out ->cmd = ctx->cmd;
			memcpy(out->payload,ctx->payload,ctx->len);
			Proto_ParseReset(ctx);
			return 1;
		}
		Proto_ParseReset(ctx);
		break;
	default:
		Proto_ParseReset(ctx);
		break;
	}
	return 0;
}
