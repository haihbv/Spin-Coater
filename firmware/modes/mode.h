#ifndef __MODE_H__
#define __MODE_H__

#include "main.h"

/**
 * Default ramp parameters
 * RAMP_UP: up 100 steps in 5000 ms
 * RAMP_DOWN: down 120 steps in 6000 ms
 */
#define RAMP_DEFAULT_RAMP_UP_STEPS (100U)
#define RAMP_DEFAULT_RAMP_UP_TIME_MS (5000U)
#define RAMP_DEFAULT_RAMP_DOWN_STEPS (120U)
#define RAMP_DEFAULT_RAMP_DOWN_TIME_MS (6000U)

typedef struct
{
    float minRPM; // RPM thap nhat trong profile
    float maxRPM; // RPM cao nhat trong profile
    u32 timeMin;  // thời gian giữ ở minRPM ms
    u32 timeMax;  // thời gian giữ ở maxRPM ms
} RampParams_t;

typedef enum
{
    RAMP_IDLE = 0, // không chạy profile ramp
    RAMP_TO_MIN,   // đang ramp tới minRPM
    RAMP_HOLD_MIN, // đã ở minRPM, giữ trong timeMin
    RAMP_TO_MAX,   // ramp từ minRPM lên maxRPM
    RAMP_HOLD_MAX, // đã ở maxRPM, giữ trong timeMax
    RAMP_FINISH,   // đã ramp về 0, chờ ramp xong để gửi FINISH
    RAMP_STOP      // lệnh stop giữa chừng, ramp về 0 và dừng
} RampState_e;

void Mode_AnalogOrDigital(float targetRPM);
void Mode_RampInit(RampParams_t params);
void Mode_RampUpdate(void);
void Mode_RampStop(void);

#endif /* __MODE_H__ */
