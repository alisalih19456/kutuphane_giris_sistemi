/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SSD1306.h"
#include "fonts.h"
#include "ds3231_for_stm32_hal.h"
#include "KeyPad.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// �grenci struct'i
typedef struct {
    char id[8];
    char name[16];
    uint8_t inside;
    DS3231_Time_t entry_time;
} Student;

// �grenci listesi (�rnek)
Student students[] = {
    {"1001", "Ali",    0, {0}},
    {"1002", "Ayse",   0, {0}},
    {"1003", "Zeynep", 0, {0}},
};
uint8_t num_students = sizeof(students) / sizeof(students[0]);

// Keypad buffer
char num_buffer[8] = {0};

// LCD'ye ve UART'a yazmak i�in buffer
char text_buffer[32];

// DS3231 zaman yapisi
DS3231_Time_t rtc_time;


// Keypad tusunu tutacak degisken
char key;
uint8_t key_index = 0;
uint8_t keypad_active = 0;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
int find_student_index(const char* id);
int calc_duration(DS3231_Time_t start, DS3231_Time_t stop);
void UART_Log(char* str);
uint8_t DS3231_GetTime(I2C_HandleTypeDef *hi2c, DS3231_Time_t *time);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  // SSD1306 OLED ekrani baslat
  SSD1306_Init();

  // DS3231 baslat (I2C1 ile)
  DS3231_Init(&hi2c1);

  // Keypad baslat (kendi k�t�phanende gerekmeyebilir, varsa ekle)
  KeyPad_Init();

  // Hosgeldiniz mesaji
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(0, 0);
  SSD1306_Puts("Kutuphaneye Giris", &Font_7x10, 1);
  SSD1306_GotoXY(0, 20);
  SSD1306_Puts("Numara Giriniz:",&Font_7x10, 1);
  SSD1306_UpdateScreen();

  // UART i�in ilk mesaj
  sprintf(text_buffer, "Sistem Basladi\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)text_buffer, strlen(text_buffer), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		key = KeyPad_WaitForKeyGetChar(HAL_MAX_DELAY);
		if (key != 0 && key_index < 4) {
			num_buffer[key_index++] = key;

			// Boşluklu gösterim için buffer oluştur
			char spaced_buffer[9] = {0};
			int j = 0;
			for (int i = 0; i < key_index; ++i) {
				spaced_buffer[j++] = num_buffer[i];
				if (i < key_index - 1) spaced_buffer[j++] = ' ';
			}

			SSD1306_GotoXY(0, 40);
			SSD1306_Puts("        ", &Font_11x18, 1); 
			SSD1306_GotoXY(0, 40);
			SSD1306_Puts(spaced_buffer, &Font_11x18, 1);
			SSD1306_UpdateScreen();
		}

    if (key_index == 4) {
      num_buffer[4] = '\0';
      int idx = find_student_index(num_buffer);

      if (idx >= 0) {
        // �grenci bulundu, RTC'den saati al
        DS3231_GetTime(&hi2c1, &rtc_time); 
				// ——————————————————————————————
				// Düzeltme: 1 saat 3 dakika geri al
				//————————————————————————————————————
				int h = rtc_time.hours;
				int m = rtc_time.minutes;

				// 3 dk geri
				m -= 50;
				if (m < 0) {
						m += 60;
						h -= 1;
				}

				// 1 saat geri
				h -= 7;
				if (h < 0) {
						h += 24;
				}

				// Düzeltmeyi yapılandırıp rtc_time’a geri yaz (isteğe bağlı)
				rtc_time.hours   = (uint8_t)h;
				rtc_time.minutes = (uint8_t)m;
				//———————————————————————————————

				// Ekrana düzeltmiş zamanı yaz
				sprintf(text_buffer, "%s %02d:%02d", students[idx].name, rtc_time.hours, rtc_time.minutes);

        if (students[idx].inside == 0) {
          // Giris islemi
          students[idx].inside = 1;
          students[idx].entry_time = rtc_time;
          SSD1306_Fill(SSD1306_COLOR_BLACK);
          SSD1306_GotoXY(0, 0);
          SSD1306_Puts("GIRIS OK!", &Font_7x10, 1);
          SSD1306_GotoXY(0, 20);
          sprintf(text_buffer, "%s %02d:%02d", students[idx].name, rtc_time.hours, rtc_time.minutes);
          SSD1306_Puts(text_buffer, &Font_7x10, 1);
          SSD1306_UpdateScreen();
          // UART log
          sprintf(text_buffer, "Giris: %s %s %02d:%02d\r\n", students[idx].id, students[idx].name, rtc_time.hours, rtc_time.minutes);
          UART_Log(text_buffer);
        } else {
          // çikis islemi
          students[idx].inside = 0;
          int duration = calc_duration(students[idx].entry_time, rtc_time);
          SSD1306_Fill(SSD1306_COLOR_BLACK);
          SSD1306_GotoXY(0, 0);
          SSD1306_Puts("CIKIS OK!", &Font_11x18, 1);
          SSD1306_GotoXY(0, 20);
          sprintf(text_buffer, "%s %02d dk", students[idx].name, duration);
          SSD1306_Puts(text_buffer, &Font_11x18, 1);
          SSD1306_UpdateScreen();
          // UART log
          sprintf(text_buffer, "Cikis: %s %s, Sure: %d dk\r\n", students[idx].id, students[idx].name, duration);
          UART_Log(text_buffer);
        }
      } else {
        // �grenci yok
        SSD1306_Fill(SSD1306_COLOR_BLACK);
        SSD1306_GotoXY(0, 0);
        SSD1306_Puts("KAYIT YOK!", &Font_11x18, 1);
        SSD1306_UpdateScreen();
        sprintf(text_buffer, "Kullanici bulunamadi: %s\r\n", num_buffer);
        UART_Log(text_buffer);
      }

      key_index = 0;
      memset(num_buffer, 0, sizeof(num_buffer));
      HAL_Delay(2000); // Sonucu ekranda g�ster
      // Ana ekrani geri y�kle
      SSD1306_Fill(SSD1306_COLOR_BLACK);
      SSD1306_GotoXY(0, 0);
      SSD1306_Puts("Kutuphaneye Giris", &Font_7x10, 1);
      SSD1306_GotoXY(0, 20);
      SSD1306_Puts("Numara Giriniz:", &Font_7x10, 1);
      SSD1306_UpdateScreen();
    }
    HAL_Delay(10);
  }
		
	
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// �grenci arama
int find_student_index(const char* id) {
    for (int i = 0; i < num_students; ++i) {
        if (strcmp(students[i].id, id) == 0) {
            return i;
        }
    }
    return -1; // Bulunamadi
}

// S�re hesaplama (saat/dakika olarak)
int calc_duration(DS3231_Time_t start, DS3231_Time_t stop) {
    int start_min = start.hours * 60 + start.minutes;
    int stop_min = stop.hours * 60 + stop.minutes;
    int diff = stop_min - start_min;
    if (diff < 0) diff += 24 * 60; // Gece yarisi ge�mis olabilir
    return diff;
}

// UART ile string g�nderme
void UART_Log(char* str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
