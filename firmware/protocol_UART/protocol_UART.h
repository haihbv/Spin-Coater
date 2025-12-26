/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef __PROTOCOL_UART_H__
#define __PROTOCOL_UART_H__

#include "main.h"

typedef enum
{
    MODE_IDLE = 0,
    MODE_ANALOG,
    MODE_DIGITAL,
    MODE_RAMP
} CoaterMode_e;

extern CoaterMode_e gCurrentMode;
extern bool gIsRunning; // Flag để kiểm soát việc gửi MRPM

void UART_ProcessMasterFrame(FrameMaster_t *frameMaster);
void UART_ParserFromEsp(void);
void UART_SendMeasureRpm(void);
void UART_SendRampFinish(void);

#endif /* __PROTOCOL_UART_H__ */
