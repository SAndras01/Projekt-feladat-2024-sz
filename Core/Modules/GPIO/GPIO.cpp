/*
 * GPIO.cpp
 *
 *  Created on: Sep 21, 2024
 *      Author: Andris
 */

#include "GPIO.hpp"

GPIO::GPIO(GPIO_TypeDef* GPIOx_p, uint16_t GPIO_Pin_p)
{
	this->GPIOx = GPIOx_p;
	this->GPIO_Pin = GPIO_Pin_p;
}

void GPIO::digitalWrite(GPIO_PinState state_p)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, state_p);
}

GPIO_PinState GPIO::digitalRead()
{
	return HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
}
