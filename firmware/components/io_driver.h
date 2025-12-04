#ifndef IO_DRIVER_H_
#define IO_DRIVER_H_

#include "main.h"

#define BUZZER_ON() GPIO_SetBits(PORT_BUZZER, PIN_BUZZER)
#define BUZZER_OFF() GPIO_ResetBits(PORT_BUZZER, PIN_BUZZER)
#define RELAY_ON		GPIO_SetBits(PORT_RELAY, PINMASK_RELAY_ALL)
#define RELAY_OFF	GPIO_ResetBits(PORT_RELAY, PINMASK_RELAY_ALL)

typedef enum
{
	BUZZER_IDLE = 0x00,
	BUZZER_ON = 0x01,
	BUZZER_OFF
} BuzzerState_t;

typedef struct
{
	BuzzerState_t stateBuzzer;
	bool isBusy;
	uint8_t times;		 // so lan beep con lai
	uint16_t durationMs; // thoi gian ON moi beep
	uint16_t intervalMs; // thoi gian nghi giua 2 beep
	uint32_t counterMs;	 // moc thoi gian
} __attribute__((packed)) BuzzerFSM_t;

extern BuzzerFSM_t buzzer;

void IO_Init(void);
void IO_SetRelay(uint16_t GPIO_Pin, BitAction BitVal);
void IO_SetBuzzer(uint16_t GPIO_Pin, BitAction BitVal);

void IO_BuzzerStart(uint8_t times, uint16_t durationMs, uint16_t counterMs);
void IO_BuzzerUpdate(void);
bool IO_BuzzerIsBusy(void);

#endif /* IO_DRIVER_H_ */
