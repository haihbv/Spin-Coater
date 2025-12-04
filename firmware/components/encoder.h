/**
 * @file encoder.h
 * @brief Header file for encoder.c
 * @author haihbv
 * @date December 4, 2025
 */
#ifndef ENCODER_H_
#define ENCODER_H_

#include "main.h"

void Encoder_Init(void);
uint16_t Encoder_GetCount(void);
int32_t Encoder_GetTotalCount(void);
uint16_t Encoder_GetPulseCount(uint32_t sampleTimeMs);
float Encoder_GetRPM(uint32_t sampleTimeMs);

#endif /* ENCODER_H_ */
