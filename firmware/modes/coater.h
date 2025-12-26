/**
 * @file main.h
 * @brief Header for project Spin Coater.
 * @author haihbv
 * @date December 2025
 */
#ifndef COATER_H_
#define COATER_H_

#include "main.h"

#define COATER_ENCODER_AVG_SAMPLE (50u)

typedef enum
{
	COATER_IDLE = 0, // không ramp, đang đứng yên hoặc giữ tốc độ ổn định
	COATER_RAMP_UP,	 // đang trong quá trình tăng tốc
	COATER_RAMP_DOWN // đang trong quá trình giảm tốc
} CoaterState_e;

typedef struct
{
	CoaterState_e stateCoater;
	float targetRPM;
	float startRPM;
	float currentRPM;
	uint32_t steps;		   // tổng số bước Ramp
	uint32_t delayPerStep; // Thời gian chờ giữa các bước (ms)
	uint32_t lastTick;	   // Thời điểm cập nhật lần cuối (ms)
	uint32_t doneSteps;	   // Số bước hoàn thành
} __attribute__((packed)) CoaterControl_t;

typedef struct
{
	float kp;
	float ki;
	float kd;
	float maxOutput; // giá trị tối đa của output
} CoaterPid_t;

extern CoaterControl_t gCoaterControl; // global

bool Coater_IsIdle(void);
void Coater_Update(float targetRPM);
void Coater_RampUp(float targetRPM, uint32_t steps, uint32_t rampTimeMs);
void Coater_RampDown(float targetRPM, uint32_t steps, uint32_t rampTimeMs);
void Coater_PIDInit(CoaterPid_t pidConfig);

#endif /* COATER_H_ */
