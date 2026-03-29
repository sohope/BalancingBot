#ifndef DRIVER_BUTTON_BUTTON_H_
#define DRIVER_BUTTON_BUTTON_H_

#include "stm32f4xx_hal.h"

typedef enum {
	NO_ACT,
	PUSHED,
	RELEASED,
	ACT_PUSHED,
	ACT_RELEASED
} btnState_t;

typedef struct {

	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	btnState_t prevBtnState;
}hBtn_Def;

void Button_Init(hBtn_Def *hBtn, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
btnState_t Button_GetState(hBtn_Def *hBtn);

#endif /* DRIVER_BUTTON_BUTTON_H_ */
