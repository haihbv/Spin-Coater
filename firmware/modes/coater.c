#include "coater.h"

CoaterControl_t gCoaterControl = {COATER_IDLE, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0};

static PID_TypeDefStruct sPidMotor;

/**
 * @name Coater_IsIdle
 * @brief Kiểm tra spin coater có đang ở trạng thái nghỉ hay không
 * @warning có thể dùng để chặn không cho ramp mới khi ramp cũ chưa kết thúc
 * @return true nếu đang nghỉ, false nếu đang ramp
 */
bool Coater_IsIdle(void)
{
    return (gCoaterControl.stateCoater == COATER_IDLE);
}

/**
 * @name Coater_PIDInit
 * @brief Khởi tạo thông số PID cho spin coater
 * @param pidConfig: thông số PID
 */
void Coater_PIDInit(CoaterPid_t pidConfig)
{
    PID_Init(&sPidMotor, pidConfig.kp, pidConfig.ki, pidConfig.kd, pidConfig.maxOutput);
}

/**
 * @name Coater_RampUp
 * @param targetRPM: tốc độ đích (RPM)
 * @param steps: số bước ramp
 * @param rampTimeMs: thời gian ramp tổng (ms)
 * @brief Bắt đầu quá trình ramp up đến tốc độ targetRPM trong rampTimeMs
 * @note Chỉ bắt đầu ramp khi đang ở trạng thái nghỉ (idle)
 */
void Coater_RampUp(float targetRPM, uint32_t steps, uint32_t rampTimeMs)
{
    if (gCoaterControl.stateCoater != COATER_IDLE)
    {
        return; // nếu đang ramp cũ thì không làm gì cả
    }

    if (steps == 0u || rampTimeMs == 0u)
    {
        return; // tránh chia cho 0 return luôn
    }

    float currentRPM = Encoder_GetRPM(COATER_ENCODER_AVG_SAMPLE); // đọc RPM hiện tại từ encoder với mẫu được define

    gCoaterControl.stateCoater = COATER_RAMP_UP;
    gCoaterControl.targetRPM = targetRPM;             // mục tiêu tốc độ RPM mong muốn
    gCoaterControl.startRPM = currentRPM;             // tốc độ RPM hiện tại khi bắt đầu ramp
    gCoaterControl.currentRPM = currentRPM;           // setpoint ban đầu = RPM hiện tại, tránh nhảy setpoint đột ngột
    gCoaterControl.steps = steps;                     // số bước ramp
    gCoaterControl.delayPerStep = rampTimeMs / steps; // mỗi bước cách nhau bao lâu (ms)
    gCoaterControl.lastTick = millis();
    gCoaterControl.doneSteps = 0u;
}

/**
 * @name Coater_RampDown
 * @brief Giống với Coater_RampUp nhưng là giảm tốc
 */
void Coater_RampDown(float targetRPM, uint32_t steps, uint32_t rampTimeMs)
{
    if (gCoaterControl.stateCoater != COATER_IDLE)
    {
        return;
    }

    if (steps == 0u || rampTimeMs == 0u)
    {
        return;
    }

    float currentRPM = Encoder_GetRPM(COATER_ENCODER_AVG_SAMPLE);

    gCoaterControl.stateCoater = COATER_RAMP_DOWN;
    gCoaterControl.targetRPM = targetRPM;
    gCoaterControl.startRPM = currentRPM;
    gCoaterControl.currentRPM = currentRPM;
    gCoaterControl.steps = steps;
    gCoaterControl.delayPerStep = rampTimeMs / steps;
    gCoaterControl.lastTick = millis();
    gCoaterControl.doneSteps = 0u;
}

/**
 * @name Coater_UpdateRampProfile
 * @brief Cập nhật profile ramp dựa trên thời gian đã trôi qua
 * @note Hàm này được gọi bên trong Coater_Update()
 */
static void Coater_UpdateRampProfile(void)
{
    if (gCoaterControl.stateCoater == COATER_IDLE)
    {
        return; // nếu đang nghỉ thì không ramp
    }

    uint32_t now = millis();
    if ((now - gCoaterControl.lastTick) < gCoaterControl.delayPerStep)
    {
        return; // chưa đủ thời gian cho 1 step mới thì return luôn
    }

    // thời điểm đã đủ cho 1 step
    gCoaterControl.lastTick = now;

    float progress = 0.0f;
    if (gCoaterControl.steps > 0u)
    {
        progress = (float)gCoaterControl.doneSteps / (float)gCoaterControl.steps; // tính tiến độ ramp
    }

    // camp progress trong khoảng 0.0 -> 1.0
    if (progress < 0.0f)
        progress = 0.0f;
    if (progress > 1.0f)
        progress = 1.0f;

    float start = gCoaterControl.startRPM;
    float target = gCoaterControl.targetRPM;

    // ramp tuyến tính theo RPM
    float rpmProfile = start + (target - start) * progress;

    // clamp theo chiều ramp
    if (gCoaterControl.stateCoater == COATER_RAMP_UP &&
        rpmProfile > target)
    {
        rpmProfile = target;
    }
    if (gCoaterControl.stateCoater == COATER_RAMP_DOWN &&
        rpmProfile < target)
    {
        rpmProfile = target;
    }

    gCoaterControl.currentRPM = rpmProfile; // setpoint RPM PID phải bám tại thời điểm này

    gCoaterControl.doneSteps++;
    if (gCoaterControl.doneSteps >= gCoaterControl.steps)
    {
        gCoaterControl.currentRPM = gCoaterControl.targetRPM;

        if (gCoaterControl.stateCoater == COATER_RAMP_DOWN &&
            gCoaterControl.currentRPM < 0.1f)
        {
            gCoaterControl.currentRPM = 0.0f;
        }

        gCoaterControl.stateCoater = COATER_IDLE; // chuyển về trạng thái nghỉ khi hoàn thành ramp
    }
}

/**
 * @name Coater_Update
 * @brief Cập nhật điều khiển PID cho spin coater
 * @param targetRpm: tốc độ mục tiêu (RPM) khi không ramp
 * @note Hàm này cần được gọi liên tục trong hàm main loop
 */
void Coater_Update(float targetRpm)
{
    /**
     * @name Coater_UpdateRampProfile
     * @note nếu đang ramp, hàm này có thể thay đổi gCoaterControl.currentRpm
     * @note nếu nghỉ, thì không làm gì cả
     */
    Coater_UpdateRampProfile();

    /**
     * @name Chọn setpoint để PID bám theo
     * @note Nếu đang ramp thì bám theo gCoaterControl.currentRpm
     * @note Nếu nghỉ thì bám theo targetRpm truyền vào Coater_Update()
     */
    float setpoint;

    if (gCoaterControl.stateCoater != COATER_IDLE)
    {
        setpoint = gCoaterControl.currentRPM;
    }
    else
    {
        setpoint = targetRpm;
        gCoaterControl.targetRPM = targetRpm; // lưu lại nếu cần
    }

    // nếu setpoint gần 0 tắt motor
    if (setpoint <= 0.1f)
    {
        Esc_SetDuty(ESC_MIN_PWM);
        gCoaterControl.currentRPM = 0.0f;
        return;
    }

    float currentRPM = Encoder_GetRPM(COATER_ENCODER_AVG_SAMPLE); // đọc RPM hiện tại từ encoder với lấy trung bình 50 mẫu

    uint16_t baseDuty = Esc_RpmToDutyLogistic(setpoint); // tính duty ước lượng trước sao cho nếu không có PID thì motor cx gần đạt setpoint

    /**
     * @name PID_Update
     * @brief Cập nhật PID để tính toán duty cuối cùng
     * @param pid: con trỏ đến cấu trúc PID
     * @param setpoint: RPM mục tiêu
     * @param currentRPM: RPM hiện tại
     */
    float pidOutput = PID_Update(&sPidMotor, setpoint, currentRPM, (float)baseDuty);

    Esc_SetDuty((uint16_t)pidOutput); // gửi duty cuối cùng đến ESC
}
