#include "mode.h"

static RampState_e sRampState = RAMP_IDLE; // state hiện tại của Ramp mode
static uint32_t sHoldStart = 0;            // thời điểm bắt đầu giữ ở min/max RPM
static RampParams_t sRampParams = {0};     // tham số ramp hiện tại

#define MODE_ENCODER_AVG_SAMPLES (50U) // đọc encoder trung bình 50 mẫu
#define RPM_MIN_LIMIT (0.0f)
#define RPM_MAX_LIMIT (6000.0f)
#define MODE_RPM_DEADZONE (15.0f) // chênh lệch 15 RPM xem như đủ gần

/**
 * @name clampRPM
 * @brief Giới hạn RPM trong khoảng cho phép
 * @param rpm Giá trị RPM cần giới hạn
 */
static inline float clampRPM(float rpm)
{
    if (rpm < RPM_MIN_LIMIT)
        rpm = RPM_MIN_LIMIT;
    if (rpm > RPM_MAX_LIMIT)
        rpm = RPM_MAX_LIMIT;
    return rpm;
}

/**
 * @name Mode_DoStop
 * @brief Dừng coater và gửi lệnh FINISH nếu cần
 * @param sendFinish true để gửi lệnh FINISH, false không gửi
 * @note Hàm này chỉ được gọi khi coater đã dừng hoàn toàn
 * @note Gửi xong thì đặt state về RAMP_IDLE
 */
static void Mode_DoStop(bool sendFinish)
{
    gIsRunning = false;
    Esc_SetDuty(ESC_MIN_PWM);
    gCoaterControl.currentRPM = 0.0f;

    if (sendFinish)
    {
        UART_SendRampFinish();
    }
    sRampState = RAMP_IDLE;
}

/**
 * @name Mode_AnalogOrDigital
 * @brief Chế độ điều khiển Analog hoặc Digital
 * @param targetRpm Giá trị RPM mục tiêu
 */
void Mode_AnalogOrDigital(float targetRpm)
{
    if (sRampState != RAMP_IDLE)
    {
        return; // tránh xung đột với Ramp mode
    }

    targetRpm = clampRPM(targetRpm);
    gCoaterControl.targetRPM = targetRpm;

    float currentRPM = Encoder_GetRPM(MODE_ENCODER_AVG_SAMPLES);

    if (!Coater_IsIdle())
    {
        return; // nếu spin coater đang chạy thì không can thiệp thêm
    }

    float diff = currentRPM - targetRpm; // tính chênh lệch giữa RPM hiện tại và mục tiêu
    if (diff < -MODE_RPM_DEADZONE)       // if RPM hiện tại nhỏ hơn 15 RPM so với mục tiêu
    {
        // thì ramp up lên targetRpm
        Coater_RampUp(targetRpm, RAMP_DEFAULT_RAMP_UP_STEPS, RAMP_DEFAULT_RAMP_UP_TIME_MS);
    }
    else if (diff > MODE_RPM_DEADZONE) // if RPM hiện tại lớn hơn 15 RPM so với mục tiêu
    {
        // thì ramp down xuống targetRpm
        Coater_RampDown(targetRpm, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
    }
}

/**
 * @name Mode_RampInit
 * @brief Khởi tạo chế độ Ramp với tham số đã cho
 * @param params Tham số Ramp
 */
void Mode_RampInit(RampParams_t params)
{
    if (sRampState != RAMP_IDLE)
    {
        return; // đang có chế độ Ramp khác chạy thì không làm gì cả
        // ví dụ: Mode Analog đang chạy thì chỉ chạy Mode Analog thôi khi vào Mode Ramp thì bị return ngay luôn :3
    }

    // Giới hạn tham số trong khoảng cho phép
    params.minRPM = clampRPM(params.minRPM);
    params.maxRPM = clampRPM(params.maxRPM);

    // Đảm bảo minRPM không lớn hơn maxRPM
    if (params.minRPM > params.maxRPM)
    {
        float temp = params.minRPM;
        params.minRPM = params.maxRPM;
        params.maxRPM = temp;
    }

    // Tránh thời gian giữ bằng 0
    if (params.timeMin == 0U)
    {
        params.timeMin = 1U;
    }
    if (params.timeMax == 0U)
    {
        params.timeMax = 1U;
    }

    sRampParams = params;
    sRampState = RAMP_TO_MIN;
    // Gọi Coater_RampUp để ramp từ RPM hiện tại đến minRPM
    Coater_RampUp(sRampParams.minRPM, RAMP_DEFAULT_RAMP_UP_STEPS, RAMP_DEFAULT_RAMP_UP_TIME_MS);
}

/**
 * @name Mode_RampUpdate
 * @brief Cập nhật state machine của chế độ Ramp
 * @note Hàm này cần được gọi thường xuyên trong main loop
 * @note Xử lý các trạng thái của Ramp mode
 */
void Mode_RampUpdate(void)
{
    switch (sRampState)
    {
    case RAMP_IDLE:
        break;
    case RAMP_TO_MIN: // ramp xong chuyển đến RAMP_HOLD_MIN, ghi lại thời điểm bắt đầu giữ
        if (Coater_IsIdle())
        {
            sRampState = RAMP_HOLD_MIN;
            sHoldStart = millis();
        }
        break;

    case RAMP_HOLD_MIN: // giữ ở minRpm trong timeMin ms, sau đó chuyển sang RAMP_TO_MAX

        if (millis() - sHoldStart >= sRampParams.timeMin)
        {
            Coater_RampUp(sRampParams.maxRPM, RAMP_DEFAULT_RAMP_UP_STEPS, RAMP_DEFAULT_RAMP_UP_TIME_MS);
            sRampState = RAMP_TO_MAX;
        }
        break;

    case RAMP_TO_MAX: // ramp xong chuyển đến RAMP_HOLD_MAX, ghi lại thời điểm bắt đầu giữ
        if (Coater_IsIdle())
        {
            sRampState = RAMP_HOLD_MAX;
            sHoldStart = millis();
        }
        break;

    case RAMP_HOLD_MAX: // giữ ở maxRpm trong timeMax ms, sau đó ramp về 0 và chuyển sang RAMP_FINISH
        if (millis() - sHoldStart >= sRampParams.timeMax)
        {
            Coater_RampDown(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
            sRampState = RAMP_FINISH;
        }
        break;

    case RAMP_FINISH:
        if (Coater_IsIdle())
        {
            Mode_DoStop(true); // true để gửi lệnh FINISH
        }
        break;

    case RAMP_STOP:
        if (Coater_IsIdle())
        {
            Mode_DoStop(false /*true*/); // false để không gửi lệnh FINISH
        }
        break;
    }
}

/**
 * @name Mode_RampStop
 * @brief Dừng chế độ Ramp hiện tại
 * @note Hàm này sẽ ramp coater về 0 RPM và dừng
 */
void Mode_RampStop(void)
{
    if (sRampState != RAMP_IDLE)
    {
        Coater_RampDown(0.0f, RAMP_DEFAULT_RAMP_DOWN_STEPS, RAMP_DEFAULT_RAMP_DOWN_TIME_MS);
        sRampState = RAMP_STOP;
    }
}
