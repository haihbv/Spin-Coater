#include "main.h"

void coater_setup_pid_init(void);
void setup()
{
    SystemInit();
    Delay.Init();

    IO_Init();
    RELAY_ON;

    Esc_Init();
    Esc_Arm(1000);

    Encoder_Init();

    coater_setup_pid_init();

    UART1.Init(115200);
    UART2.Init(115200);

    DelayMs(3000);

    // Beep bao hieu he thong da san sang
    IO_BuzzerStart(2, 100, 200);

    // printf("==== SYSTEM COATER READY ====\r\n");
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
    Coater_Update(gCoaterControl.targetRPM);

    // Cap nhat buzzer
    IO_BuzzerUpdate();
    // Gui RPM do duoc ve ESP32
    UART_SendMeasureRpm();

    // printf("DUTY: %u | Target: %.2f | MRPM: %.2f | ERROR: %.2f | \r\n", Esc_GetDuty(), (double)2000, (double)Encoder_GetRPM(50), (double)(2000 - Encoder_GetRPM(50)));
}

int main()
{
    setup();
    while (1)
    {
        loop();
    }
}

void coater_setup_pid_init(void)
{
    CoaterPid_t motorPid = {0};
    motorPid.kp = 0.1f;
    motorPid.ki = 1.0f;
    motorPid.kd = 0.0f;
    motorPid.maxOutput = 1500.0f;
    Coater_PIDInit(motorPid);
}
