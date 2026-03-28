/*
 * robot_svc.h
 *
 * Created on: Mar 27, 2026
 * Author: kccistc
 */

#ifndef APP_ROBOT_SVC_ROBOT_SVC_H_
#define APP_ROBOT_SVC_ROBOT_SVC_H_

#include "stm32f4xx_hal.h"
#include "../driver/UART_COM/uart_com.h"
#include "../driver/UART_COM/uart_protocol.h"
// #include "../driver/motor/motor.h"

extern uint8_t g_robot_mode;   // 0: Stop, 1: Balance, 2: Debug
extern int8_t g_target_speed;  // 전진/후진 목표 속도 (-100 ~ 100)
extern int8_t g_target_turn;   // 좌우 조향 목표 (-100 ~ 100)

void Robot_Init(void);
void Robot_Execute(void);
void ROBOT_HandleCmd(Packet_t* pkt);

#endif /* APP_ROBOT_SVC_ROBOT_SVC_H_ */
