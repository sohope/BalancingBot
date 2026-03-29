#include "button.h"

void Button_Init(hBtn_Def *hBtn, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	hBtn->GPIOx = GPIOx;
	hBtn->GPIO_Pin = GPIO_Pin;
	hBtn->prevBtnState = RELEASED;

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = hBtn->GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(hBtn->GPIOx, &GPIO_InitStruct);
}

btnState_t Button_GetState(hBtn_Def *hBtn)
{
//	static btnState_t prevBtnState = RELEASED;
	btnState_t curBtnState = HAL_GPIO_ReadPin(hBtn->GPIOx, hBtn->GPIO_Pin)? RELEASED : PUSHED;

	if(hBtn->prevBtnState == RELEASED && curBtnState == PUSHED)
	{
		HAL_Delay(50);	// debouncing
		hBtn->prevBtnState = PUSHED;
		return ACT_PUSHED;
	}
	else if(hBtn->prevBtnState == PUSHED && curBtnState == RELEASED)
	{
		HAL_Delay(50); // debouncing
		hBtn->prevBtnState = RELEASED;
		return ACT_RELEASED;
	}
	return NO_ACT;
}
