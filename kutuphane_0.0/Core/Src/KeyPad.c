#include "KeyPad.h"
#include "KeyPadConfig.h"

#if (_KEYPAD_USE_FREERTOS==1)
#include "cmsis_os.h"
#define _KEYPAD_DELAY(x)      osDelay(x)
#else
#define _KEYPAD_DELAY(x)      HAL_Delay(x)
#endif

KeyPad_t	KeyPad;

//#############################################################################################
void	KeyPad_Init(void)
{
  GPIO_InitTypeDef	gpio;
	KeyPad.ColumnSize = sizeof(_KEYPAD_COLUMN_GPIO_PIN) / sizeof(_KEYPAD_COLUMN_GPIO_PIN[0]);
	KeyPad.RowSize = sizeof(_KEYPAD_ROW_GPIO_PIN) / sizeof(_KEYPAD_ROW_GPIO_PIN[0]);

  for(uint8_t	i=0 ; i<KeyPad.ColumnSize ; i++)
  {
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = _KEYPAD_COLUMN_GPIO_PIN[i];
    HAL_GPIO_Init((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], &gpio);
    HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], _KEYPAD_COLUMN_GPIO_PIN[i], GPIO_PIN_SET);
  }
  for(uint8_t	i=0 ; i<KeyPad.RowSize ; i++)
  {
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = _KEYPAD_ROW_GPIO_PIN[i];
    HAL_GPIO_Init((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[i], &gpio);		
  }
}
//#############################################################################################
uint16_t	KeyPad_Scan(void)
{
  uint16_t  key=0;
  for(uint8_t c=0 ; c<KeyPad.ColumnSize ; c++)
  {
    for(uint8_t i=0 ; i<KeyPad.ColumnSize ; i++)
      HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], _KEYPAD_COLUMN_GPIO_PIN[i], GPIO_PIN_SET);
    HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[c], _KEYPAD_COLUMN_GPIO_PIN[c], GPIO_PIN_RESET);
    _KEYPAD_DELAY(5);
    for(uint8_t r=0 ; r<KeyPad.RowSize ; r++)
    {
      if(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
      {
        _KEYPAD_DELAY(_KEYPAD_DEBOUNCE_TIME_MS);
        if(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
        {
          key |= 1<<c;					
          key |= 1<<(r+8);
          while(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
            _KEYPAD_DELAY(5);
          return key;
        }
      }			
    }		
  }
  return key;
}
//#############################################################################################
uint16_t	KeyPad_WaitForKey(uint32_t  Timeout_ms)
{	
  uint16_t  keyRead;
  while(Timeout_ms==0)
  {
    keyRead = KeyPad_Scan();
		if(keyRead!=0)
		{
			KeyPad.LastKey = keyRead;
			return keyRead;	
		}
		_KEYPAD_DELAY(_KEYPAD_DEBOUNCE_TIME_MS);	
	}
	uint32_t	StartTime = HAL_GetTick();
	while(HAL_GetTick()-StartTime < Timeout_ms)
	{
		keyRead = KeyPad_Scan();
		if(keyRead!=0)
		{
			KeyPad.LastKey = keyRead;
			return keyRead;	
		}
		_KEYPAD_DELAY(_KEYPAD_DEBOUNCE_TIME_MS);	
	}
	KeyPad.LastKey=0;
	return 0;
}
//#############################################################################################
char KeyPad_WaitForKeyGetChar(uint32_t Timeout_ms)
{
    uint16_t code = KeyPad_WaitForKey(Timeout_ms);
    switch (code)
    {
        // satir 0 (row=0 ? bit 8)
        case (1<<8)  | (1<<0): return '1';  // 0x0101 = 257
        case (1<<8)  | (1<<1): return '2';  // 0x0102 = 258
        case (1<<8)  | (1<<2): return '3';  // 0x0104 = 260

        // satir 1 (row=1 ? bit 9)
        case (1<<9)  | (1<<0): return '4';  // 0x0201 = 513
        case (1<<9)  | (1<<1): return '5';  // 0x0202 = 514
        case (1<<9)  | (1<<2): return '6';  // 0x0204 = 516

        // satir 2 (row=2 ? bit 10)
        case (1<<10) | (1<<0): return '7';  // 0x0401 = 1025
        case (1<<10) | (1<<1): return '8';  // 0x0402 = 1026
        case (1<<10) | (1<<2): return '9';  // 0x0404 = 1028

        // satir 3 (row=3 ? bit 11)
        case (1<<11) | (1<<0): return '*';  // 0x0801 = 2049
        case (1<<11) | (1<<1): return '0';  // 0x0802 = 2050
        case (1<<11) | (1<<2): return '#';  // 0x0804 = 2052

        default: return 0;
    }
}
