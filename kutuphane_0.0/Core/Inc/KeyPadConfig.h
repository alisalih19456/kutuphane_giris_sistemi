#include "stm32f4xx_hal.h"
#ifndef	_KEYPADCONFIG_H
#define	_KEYPADCONFIG_H

#define           _KEYPAD_DEBOUNCE_TIME_MS        20
#define           _KEYPAD_USE_FREERTOS            0

const GPIO_TypeDef* _KEYPAD_COLUMN_GPIO_PORT[] =
{
  GPIOA,  // PA0
  GPIOA,  // PA1
  GPIOA,  // PA4
  
};

const uint16_t _KEYPAD_COLUMN_GPIO_PIN[] =
{
  GPIO_PIN_0,  // PA0
  GPIO_PIN_1,  // PA1
  GPIO_PIN_4,  // PA4
  
};

const GPIO_TypeDef* _KEYPAD_ROW_GPIO_PORT[] =
{
  GPIOC,  // PC0
  GPIOC,  // PC1
  GPIOC,  // PC2
  GPIOC   // PC3
};

const uint16_t _KEYPAD_ROW_GPIO_PIN[] =
{
  GPIO_PIN_0,  // PC0
  GPIO_PIN_1,  // PC1
  GPIO_PIN_2,  // PC2
  GPIO_PIN_3   // PC3
};

#endif