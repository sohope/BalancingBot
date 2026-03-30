/*
 * robot_svc.c
 *
 * Created on: Mar 27, 2026
 * Author: kccistc
 */

#include "robot_svc.h"
#include "../PID_SVC/PID_svc.h"
#include "../Gyro_imu_svc/gyro_imu_svc.h"

// Robot State Variables
uint8_t g_robot_mode = 0;   // 0: Stop, 1: Balance
int8_t g_target_speed = 0;  // -100 ~ 100 (Forward/Backward)
int8_t g_target_turn = 0;   // -100 ~ 100 (Left/Right)

void Robot_Init(void) {
    // TODO: Initialize motors and sensors
    // Motor_Init(&htim);
}

void Robot_Execute(void) {
    if (UART_COM_ROBOT_isRxReady()) {
        ROBOT_HandleCmd(UART_COM_ROBOT_GetPacket());
    }

    // TODO: Telemetry transmission logic (e.g., 10ms tick)
    /*
    if (telemetry_tick_flag) {
        telemetry_tick_flag = 0;
        Robot_SendTelemetry();
    }
    */
}

void ROBOT_HandleCmd(Packet_t* pkt) {
    uint8_t buf[8];
    uint8_t err;

    switch(pkt->cmd) {
        case CMD_ROBOT_MOVE:
            /*
             * payload[0] (Dir)   : 0(Stop), 1(Fwd), 2(Bwd), 3(Left), 4(Right)
             * payload[1] (Speed) : 0 ~ 100
             */
            if (pkt->len < 2) {
                err = ERR_INVALID_LEN;
                UART_COM_SendPacket(CMD_ERROR, &err, 1);
                break;
            }

            uint8_t dir = pkt->payload[0];
            uint8_t spd = pkt->payload[1];

            switch(dir) {
                case 1:  g_target_speed = spd;   break; // Fwd
                case 2:  g_target_speed = -spd;  break; // Bwd
                case 3:  g_target_turn = -spd;   break; // Left Turn
                case 4:  g_target_turn = spd;    break; // Right Turn
                default: g_target_speed = 0; g_target_turn = 0; break; // Stop
            }

            buf[0] = CMD_ROBOT_MOVE;
            UART_COM_SendPacket(CMD_ACK, buf, 1);
            break;

        case CMD_ROBOT_SET_MODE:
            g_robot_mode = pkt->payload[0];
            buf[0] = CMD_ROBOT_SET_MODE;
            UART_COM_SendPacket(CMD_ACK, buf, 1);
            break;

        case CMD_ROBOT_SET_PID:
            if (pkt->len < 3) {
                err = ERR_INVALID_LEN;
                UART_COM_SendPacket(CMD_ERROR, &err, 1);
                break;
            }
            {
                char type = (char)pkt->payload[0];
                int16_t raw_val = (int16_t)((pkt->payload[1] << 8) | pkt->payload[2]);
                float value = (float)raw_val / 100.0f;
                pid_svc_set_gain(type, value);
            }
            buf[0] = CMD_ROBOT_SET_PID;
            UART_COM_SendPacket(CMD_ACK, buf, 1);
            break;

        case CMD_ROBOT_SET_ALPHA:
            if (pkt->len < 2) {
                err = ERR_INVALID_LEN;
                UART_COM_SendPacket(CMD_ERROR, &err, 1);
                break;
            }
            {
                int16_t raw_val = (int16_t)((pkt->payload[0] << 8) | pkt->payload[1]);
                float alpha = (float)raw_val / 100.0f;
                GyroImuSvc_SetAlpha(alpha);
            }
            buf[0] = CMD_ROBOT_SET_ALPHA;
            UART_COM_SendPacket(CMD_ACK, buf, 1);
            break;

        default:
            err = ERR_INVALID_CMD;
            UART_COM_SendPacket(CMD_ERROR, &err, 1);
            break;
    }
}
