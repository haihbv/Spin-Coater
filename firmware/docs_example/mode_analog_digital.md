# MODE_ANALOG_AND_DIGITAL_CONTROL

```c
#include "main.h"

static const float setpoint = 500.0f; 

static void coater_setup_pid_control(void)
{
 CoaterPid_t pidCfg;
 pidCfg.kp = 0.1f;
 pidCfg.ki = 1.0f;
 pidCfg.kd = 0.0f;
 pidCfg.maxOutput = 1500.0f;
 Coater_PIDInit(pidCfg);
}

static void coater_setup_kalmanfilter_control(void)
{
 KalmanFilter_Init(&gKalmanRpm, 1.0f, 1.0f, 0.05f, 2.0f, 0.0f, 1.0f);
}

void setup()
{
 SystemInit();
 Delay.Init();
 UART1.Init(115200);
 
 Esc_Init();
 Esc_Arm(1500);
 
 Encoder_Init();
 
 coater_setup_pid_control();
 coater_setup_kalmanfilter_control();
}

void loop()
{
 Mode_AnalogOrDigital(setpoint);
 
 Coater_Update(gCoaterControl.targetRpm);
 
 uint16_t nowDuty = Esc_GetDuty();
 float nowRPM = Encoder_GetRPM(COATER_ENCODER_AVG_SAMPLE);
 float error = setpoint - nowRPM;
 
 static uint32_t sDebugTime = 0;
 if (millis() - sDebugTime >= COATER_ENCODER_AVG_SAMPLE)
 {
  printf("SP=%.2f; RPM=%.2f; Err=%.2f; Duty: %d\n", (double)setpoint, (double)nowRPM, (double)error, nowDuty);
  sDebugTime = millis();
 }
}

int main(void)
{
 setup();
 while (1)
 {
  loop();
 }
}

```
