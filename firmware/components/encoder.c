#include "encoder.h"

/**
 * @defgroup encoder_constants
 * @{
 * @brief Nhóm các hằng số cho thông số ENCODER đang sử dụng
 * @param CPR: Counts Per Revolution = số xung trên mỗi vòng quay
 */

#define ENCODER_LINES 16u	 // số đường quét trên 1 vòng quay của encoder
#define ENCODER_QUAD_MULT 4u // hệ số Quadrature multiplier
#define ENCODER_CPR (ENCODER_LINES * ENCODER_QUAD_MULT)

/** @} */ // encoder_constants

static int32_t sTotalCount = 0;	  // số xung tích lũy có hướng
static uint16_t sLastCounter = 0; // thêm một biến bộ đếm lúc trước để phát hiện tràn số
static uint16_t sPulseRate = 0;	  // số xung trong 1 chu kỳ lẫy mẫu

/**
 * @name Encoder_Diff16
 * @brief Tính hiệu số giữa hai giá trị của bộ đếm 16-bit, xử lý tràn số
 * @param prev: giá trị bộ đếm lúc trước
 * @param now: giá trị bộ đếm hiện tại
 * @retval Hiệu số có dấu giữa hai giá trị bộ đếm
 */
static inline int32_t Encoder_Diff16(uint16_t prev, uint16_t now)
{
	// giả sửa prev = 65530, now = 10 -> diff = 10 - 65530 = -65520 (tràn)
	// nhưng thực chất là đã đếm được 16 xung
	int32_t diff = (int32_t)now - (int32_t)prev;

	if (diff > 32767)
	{
		diff -= 65536;
	}
	// nhảy xuống lệnh else if này vì diff = -65520 < -32768
	// và thực hiện diff = -65520 + 65536 = 16 (đúng với khi ta tính bằng tay)
	else if (diff < -32768)
	{
		diff += 65536;
	}
	return diff;
}

/**
 * @name Encoder_Init
 * @brief Cấu hình cho encoder sử dụng mode quadrature x4 với Timer2
 */
void Encoder_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Pin = PIN_ENCODER_A | PIN_ENCODER_B;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PORT_ENCODER, &GPIO_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

	TIM_ICInitTypeDef TIM_ICInitStructure = {0};
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 6;

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	TIM_SetCounter(TIM2, 0);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);

	sLastCounter = 0;
	sTotalCount = 0;
	sPulseRate = 0;

	TIM_Cmd(TIM2, ENABLE);
}

/**
 * @name Encoder_GetCount
 * @brief Lấy giá trị của bộ đếm hiện tại
 * @return u16_t: Giá trị bộ đếm hiện tại
 */
uint16_t Encoder_GetCount(void)
{
	return TIM_GetCounter(TIM2);
}

/**
 * @name Encoder_GetTotalCount
 * @brief Lấy giá trị tổng số xung đã đếm, xử lý tràn số
 * @return int32_t: Giá trị tổng số xung đã đếm
 */
int32_t Encoder_GetTotalCount(void)
{
	uint16_t currentCount = TIM_GetCounter(TIM2);
	int32_t diff = Encoder_Diff16(sLastCounter, currentCount);

	sLastCounter = currentCount;
	sTotalCount += diff;
	return sTotalCount;
}

/**
 * @name Encoder_GetPulseCount
 * @brief Lấy số xung trong một khoảng thời gian mẫu
 * @param sampleTimeMs: Thời gian lấy mẫu (ms)
 * @return uint16_t: Số xung trong khoảng thời gian mẫu
 */
uint16_t Encoder_GetPulseCount(uint32_t sampleTimeMs)
{
	static uint32_t lastTime = 0;	// thời điểm lấy mẫu lần trước
	static uint16_t lastCount = 0;	// giá trị counter lần trước
	static bool isFirstCall = true; // Đánh dấu lần đầu tiên gọi hàm

	uint32_t currentTime = millis();
	uint32_t elapsed = currentTime - lastTime; // thời gian trôi qua từ lúc lấy mẫu trước đó

	// Lần đầu tiên gọi hàm sau reset: khởi tạo và trả về 0
	if (isFirstCall)
	{
		isFirstCall = false;
		lastTime = currentTime;
		lastCount = TIM_GetCounter(TIM2);
		sPulseRate = 0;
		return 0;
	}

	if (elapsed < sampleTimeMs) // chưa đủ thời gian lấy mẫu thì trả về giá trị cũ
	{
		return sPulseRate;
	}

	uint16_t currentCount = TIM_GetCounter(TIM2);			// fetch counter hiện tại
	int32_t diff = Encoder_Diff16(lastCount, currentCount); // lấy xung kể cả khi tràn số

	lastCount = currentCount; // cập nhật giá trị counter để dùng cho lần sau

	uint32_t pulses = (uint32_t)((diff >= 0) ? diff : -diff); // vì xung có thể âm hoặc dương nên lấy trị tuyệt đối để đo xung

	if (elapsed == 0u) // tránh chia 0 (undefined behavior)
	{
		return sPulseRate;
	}

	uint32_t minAllowed = (sampleTimeMs * 4u) / 5u; // 80 %

	if (elapsed < minAllowed)
	{
		return sPulseRate; // bỏ qua nếu thời gian lấy mẫu chưa đủ 80% thời gian yêu cầu
	}

	// Xử lý trường hợp elapsed quá lớn (millis overflow hoặc lần đầu sau reset)
	uint32_t maxAllowed = sampleTimeMs * 3u; // Giới hạn 3x thời gian mẫu
	if (elapsed > maxAllowed)
	{
		// Nếu quá lâu không đọc, reset và trả 0
		lastTime = currentTime;
		sPulseRate = 0;
		return 0;
	}

	if (elapsed != sampleTimeMs)
	{
		pulses = (uint32_t)((pulses * sampleTimeMs) / elapsed); // hiệu chỉnh số xung về đúng thời gian mẫu yêu cầu
	}

	// lưu kết quả và cập nhật thời gian lần cuối lấy mẫu
	sPulseRate = (uint16_t)pulses;
	lastTime = currentTime;
	return sPulseRate;
}

/**
 * @name Encoder_GetRPM
 * @brief Lấy giá trị tốc độ quay hiện tại (RPM)
 * @param sampleTimeMs: Thời gian lấy mẫu (ms)
 * @return float: Tốc độ quay hiện tại (RPM)
 */
float Encoder_GetRPM(uint32_t sampleTimeMs)
{
	uint16_t pulse = Encoder_GetPulseCount(sampleTimeMs);
	float revPerSample = (float)pulse / (float)ENCODER_CPR;
	float rpm = revPerSample * (60000.0f / (float)sampleTimeMs);
	return rpm;
}
