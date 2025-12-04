# UART Protocol

```c
#include "main.h"

void Coater_Init(void); // PID_Init cho Spin
void Filter_Init(void);

void setup()
{
    SystemInit();
    Delay.Init();
    UART1.Init(115200);
    UART2.Init(115200);

    Esc_Init();
    Esc_Arm(1000);

    Encoder_Init();

    Filter_Init();
    Coater_Init();

    printf("============== SPIN COATER ==============\r\n");
}

void loop()
{
    // Nhan lenh tu ESP32
    UART_ParserFromEsp();

    // Cap nhat state machine cua Ramp mode
    if (gCurrentMode == MODE_RAMP)
    {
        Mode_RampUpdate();
    }

    // Cap nhat PID controller
    Coater_Update(gCoaterControl.targetRpm);

    // Gui RPM do duoc ve ESP32
    UART_SendMeasureRpm();
}

int main()
{
    setup();
    while (1)
    {
        loop();
    }
}

void Coater_Init(void)
{
    CoaterPid_t motorPid = {0};
    motorPid.kp = 0.1f;
    motorPid.ki = 1.0f;
    motorPid.kd = 0.0f;
    motorPid.maxOutput = 1500.0f;
    Coater_PIDInit(motorPid);
}

void Filter_Init(void)
{
    KalmanFilter_Init(&gKalmanRpm, 1.0f, 1.0f, 0.05f, 2.0f, 0.0f, 1.0f);
}
```
