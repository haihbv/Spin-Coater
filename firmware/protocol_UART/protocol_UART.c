#include "protocol_UART.h"

#define UART_RX_BUFFER_SIZE (32U)
#define UART_MRPM_SEND_INTERVAL_MS (50U)

CoaterMode_e gCurrentMode = MODE_IDLE;
bool gIsRunning = false;

static void uartSafeStop(void)
{
    float currentRpm = Encoder_GetRPM(COATER_ENCODER_AVG_SAMPLE);
    if (currentRpm > 10.0f)
    {
        Coater_RampDown(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
    }
    else if (currentRpm <= 10.0f)
    {
        // Nếu RPM đã thấp, có thể dừng ngay
        Esc_SetDuty(ESC_MIN_PWM);
    }
    printf("Safe stop coater before switching mode\r\n");
}

void UART_ProcessMasterFrame(FrameMaster_t *frameMaster)
{
    switch (frameMaster->typeMsg)
    {
    case TYPE_MSG_SET_ANALOG:
        if (gCurrentMode != MODE_ANALOG)
        {
            if (gCurrentMode != MODE_IDLE)
            {
                uartSafeStop();
            }
            gCurrentMode = MODE_ANALOG;
            IO_BuzzerStart(1, 100, 200);
            printf("[MODE]: ANALOG\r\n");
        }
        break;

    case TYPE_MSG_SET_DIGITAL:
        if (gCurrentMode != MODE_DIGITAL)
        {
            if (gCurrentMode != MODE_IDLE)
            {
                uartSafeStop();
            }
            gCurrentMode = MODE_DIGITAL;
            IO_BuzzerStart(1, 100, 200);
            printf("[MODE]: DIGITAL\r\n");
        }
        break;

    case TYPE_MSG_SET_RAMP:
        if (gCurrentMode != MODE_RAMP)
        {
            if (gCurrentMode != MODE_IDLE)
            {
                uartSafeStop();
            }
            gCurrentMode = MODE_RAMP;
            IO_BuzzerStart(1, 100, 200);
            printf("[MODE]: RAMP\r\n");
        }
        break;

    /* ---------- Analog Mode ---------- */
    case TYPE_MSG_ANALOG_START:
        if (gCurrentMode != MODE_ANALOG)
        {
            printf("Error: Not in Analog mode\r\n");
            return;
        }

        float rpmAnalog = Convert_From_Bytes_To_U16(frameMaster->data[0], frameMaster->data[1]);
        Mode_AnalogOrDigital(rpmAnalog);
        gIsRunning = true;
        IO_BuzzerStart(2, 100, 200); // Beep khi start
        printf("ANALOG Start: Target RPM = %.2f\r\n", (double)rpmAnalog);
        break;
    case TYPE_MSG_ANALOG_STOP:
        if (gCurrentMode == MODE_ANALOG)
        {
            uartSafeStop();
            gIsRunning = false;
            IO_BuzzerStart(3, 100, 200);
            printf("ANALOG Stop\r\n");
        }
        break;

    /* ---------- Digital Mode ---------- */
    case TYPE_MSG_DIGITAL_START:
        if (gCurrentMode != MODE_DIGITAL)
        {
            printf("Error: Not in Digital mode\r\n");
            return;
        }

        float rpmDigital = Convert_From_Bytes_To_U16(frameMaster->data[0], frameMaster->data[1]);
        Mode_AnalogOrDigital(rpmDigital);
        gIsRunning = true;
        IO_BuzzerStart(2, 100, 200); // Beep khi start
        printf("DIGITAL Start: Target RPM = %.2f\r\n", (double)rpmDigital);
        break;
    case TYPE_MSG_DIGITAL_STOP:
        if (gCurrentMode == MODE_DIGITAL)
        {
            uartSafeStop();
            gIsRunning = false;
            IO_BuzzerStart(3, 100, 200);
            printf("DIGITAL Stop\r\n");
        }
        break;

    /* ---------- Ramp Mode ---------- */
    case TYPE_MSG_RAMP_START:
        if (gCurrentMode != MODE_RAMP)
        {
            printf("Error: Not in Ramp mode\r\n");
            return;
        }

        RampParams_t rampParams = {0};
        rampParams.minRPM = Convert_From_Bytes_To_U16(frameMaster->data[0], frameMaster->data[1]);
        rampParams.timeMin = Convert_From_Bytes_To_U16(frameMaster->data[2], frameMaster->data[3]) * 1000;
        rampParams.maxRPM = Convert_From_Bytes_To_U16(frameMaster->data[4], frameMaster->data[5]);
        rampParams.timeMax = Convert_From_Bytes_To_U16(frameMaster->data[6], frameMaster->data[7]) * 1000;

        Mode_RampInit(rampParams);
        gIsRunning = true;
        IO_BuzzerStart(2, 100, 200); // Beep khi start ramp
        printf("RAMP Start: Min=%.2f | TimeMin=%u ms | Max=%.2f | TimeMax=%u ms\r\n", (double)rampParams.minRPM, rampParams.timeMin, (double)rampParams.maxRPM, rampParams.timeMax);
        break;
    case TYPE_MSG_RAMP_FINISH:
        break;
    case TYPE_MSG_RAMP_STOP:
        if (gCurrentMode == MODE_RAMP)
        {
            Mode_RampStop();
            gIsRunning = false;
            IO_BuzzerStart(3, 100, 200);
            printf("RAMP Stop\r\n");
        }
        break;
    default:
        printf("Error: Unknown message type 0x%02X\r\n", frameMaster->typeMsg);
        break;
    }
}

void UART_ParserFromEsp(void)
{
    static u8 sRxBuffer[UART_RX_BUFFER_SIZE];
    static u16 sRxIndex = 0;

    while (UART2.Available())
    {
        u8 byte = (u8)UART2.GetChar();

        if (sRxIndex >= sizeof(sRxBuffer))
        {
            printf("Uart overflow reset\r\n");
            sRxIndex = 0;
        }

        sRxBuffer[sRxIndex++] = byte;

        /* Frame hop le */
        if (sRxIndex >= 6) // khi nhan du Start Frame (2 byte) + Type Msg (1 byte) + Length Data (1 byte) + Data (0 byte) + CRC (2 byte) = 6 byte
        {
            u16 startFrame = Convert_From_Bytes_To_U16(sRxBuffer[0], sRxBuffer[1]);

            if (startFrame != START_BYTE)
            {
                sRxIndex = 0;
                continue;
            }

            /* Len Data */
            u8 lenData = sRxBuffer[3];

            if (lenData > 12)
            {
                printf("Uart invalid length\r\n");
                sRxIndex = 0;
                continue;
            }

            u16 totalFrameLen = LENGTH_DATA_HEADER + lenData + 2; // 2 byte CRC nua

            if (sRxIndex >= totalFrameLen)
            {
                if (msgCheckCrc(sRxBuffer, totalFrameLen))
                {
                    FrameMaster_t frame;
                    msgDecodeFrameMaster(sRxBuffer, &frame);
                    UART_ProcessMasterFrame(&frame);
                }
                else
                {
                    printf("Error!\r\n");
                }
                sRxIndex = 0;
            }
        }
    }
}

void UART_SendMeasureRpm(void)
{
    static u32 sLastTime = 0;

    if (millis() - sLastTime < UART_MRPM_SEND_INTERVAL_MS)
    {
        return;
    }

    sLastTime = millis();

    if (gIsRunning == false || gCurrentMode == MODE_IDLE)
    {
        return;
    }

    float measuredRpm = Encoder_GetRPM(UART_MRPM_SEND_INTERVAL_MS);

    u8 msgType = '\0';

    switch (gCurrentMode)
    {
    case MODE_IDLE:
        break;
    case MODE_ANALOG:
        msgType = TYPE_MSG_ANALOG_MRPM_UPDATE;
        break;
    case MODE_DIGITAL:
        msgType = TYPE_MSG_DIGITAL_MRPM_UPDATE;
        break;
    case MODE_RAMP:
        msgType = TYPE_MSG_RAMP_MRPM_UPDATE;
        break;
    }

    FrameSlave_t txSlave = {0};
    txSlave.startFrame = START_BYTE;
    txSlave.typeMsg = msgType;

    u8 *rpmBytes = Convert_From_Float_To_Bytes(measuredRpm);
    txSlave.data[0] = rpmBytes[0];
    txSlave.data[1] = rpmBytes[1];
    txSlave.data[2] = rpmBytes[2];
    txSlave.data[3] = rpmBytes[3];

    u8 output[UART_RX_BUFFER_SIZE];
    u8 len = msgCreateFrameSlave(&txSlave, output);

    if (len > 0)
    {
        for (u8 i = 0; i < len; i++)
        {
            UART2.SendChar(output[i]);
        }
    }
}

void UART_SendRampFinish(void)
{
    FrameSlave_t txSlave = {0};
    txSlave.startFrame = START_BYTE;
    txSlave.typeMsg = TYPE_MSG_RAMP_FINISH;

    u8 output[UART_RX_BUFFER_SIZE];
    u8 len = msgCreateFrameSlave(&txSlave, output);

    if (len > 0)
    {
        for (u8 i = 0; i < len; i++)
        {
            UART2.SendChar(output[i]);
        }
    }

    // Beep 3 lan khi ramp finish thanh cong
    IO_BuzzerStart(3, 100, 200);

    printf("Ramp Finish Success\r\n");
}
