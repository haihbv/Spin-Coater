/**
 * @file esc_driver.h
 * @brief Header file for esc_driver.c
 * @author haihbv
 * @date December 4, 2025
 */
#ifndef ESC_DRIVER_H_
#define ESC_DRIVER_H_

#include "main.h"

#define ESC_MIN_PWM 4500u
#define ESC_MAX_PWM 6000u

void Esc_Init(void);
void Esc_Arm(uint32_t timeoutMs);
void Esc_SetDuty(uint16_t dutyTicks);
uint16_t Esc_GetDuty(void);
float Esc_DutyToRpmLogistic(uint16_t dutyTicks);
uint16_t Esc_RpmToDutyLogistic(float rpm);

/**
 * @name Linear Conversion Functions
 * @brief These functions are for reference only and are not used in control
 */
float Esc_DutyToRpm(uint16_t dutyTicks);
uint16_t Esc_RpmToDuty(float rpm);

#endif /* ESC_DRIVER_H_ */
