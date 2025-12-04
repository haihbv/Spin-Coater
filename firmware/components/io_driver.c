#include "io_driver.h"

BuzzerFSM_t buzzer;

void IO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Pin = PIN_RELAY_1 | PIN_RELAY_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(PORT_RELAY, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PIN_BUZZER;
	GPIO_Init(PORT_BUZZER, &GPIO_InitStructure);

	RELAY_ON;
	// GPIO_ResetBits(PORT_RELAY, PINMASK_RELAY_ALL); // Tat relay
	BUZZER_OFF();		 // Tat buzzer

	buzzer.stateBuzzer = BUZZER_IDLE;
	buzzer.isBusy = false;
	buzzer.times = 0;
	buzzer.durationMs = 0;
	buzzer.intervalMs = 0;
	buzzer.counterMs = 0;
}

void IO_SetRelay(u16 GPIO_Pin, BitAction BitVal)
{
	if (BitVal == Bit_RESET)
	{
		GPIOB->BSRR = GPIO_Pin;
	}
	else
	{
		GPIOB->BRR = GPIO_Pin;
	}
}

void IO_SetBuzzer(u16 GPIO_Pin, BitAction BitVal)
{
	if (BitVal == Bit_RESET)
	{
		GPIOA->BSRR = GPIO_Pin;
	}
	else
	{
		GPIOA->BRR = GPIO_Pin;
	}
}

void IO_BuzzerStart(u8 times, u16 durationMs, u16 intervalMs)
{
	if (buzzer.isBusy == true)
	{
		return;
	}

	buzzer.times = times;
	buzzer.durationMs = durationMs;
	buzzer.intervalMs = intervalMs;
	buzzer.counterMs = millis();
	buzzer.isBusy = true;
	buzzer.stateBuzzer = BUZZER_ON;

	BUZZER_ON(); // on Buzzer
}
void IO_BuzzerUpdate(void)
{
	if (buzzer.isBusy == false)
	{
		return;
	}

	switch (buzzer.stateBuzzer)
	{
	case BUZZER_IDLE:
	{
		buzzer.isBusy = false;
		BUZZER_OFF(); // OFF
		break;
	}
	case BUZZER_ON:
	{
		if (millis() - buzzer.counterMs >= buzzer.durationMs)
		{
			BUZZER_ON(); // ON
			buzzer.counterMs = millis();
			buzzer.stateBuzzer = BUZZER_OFF;
			if (buzzer.times > 0)
				buzzer.times--;
		}
		break;
	}
	case BUZZER_OFF:
	{
		if (millis() - buzzer.counterMs >= buzzer.intervalMs)
		{
			if (buzzer.times == 0)
			{
				buzzer.stateBuzzer = BUZZER_IDLE;
				buzzer.isBusy = false;
				BUZZER_OFF();
			}
			else
			{
				BUZZER_OFF();
				buzzer.counterMs = millis();
				buzzer.stateBuzzer = BUZZER_ON;
			}
		}
		break;
	}
	}
}

bool IO_BuzzerIsBusy(void)
{
	return buzzer.isBusy;
}
