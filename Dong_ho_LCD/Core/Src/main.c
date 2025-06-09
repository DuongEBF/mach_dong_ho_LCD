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
#include "LiquidCrystal_I2C.h"
#include "DS3231.h"
#include "Button.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
LiquidCrystal_I2C lcd;
Date_time_t date_time;
Button_Typdef btn_OK;
Button_Typdef btn_UP;
Button_Typdef btn_DOWN;
volatile const char arr_day[][3] ={
{"T2"},
{"T3"},
{"T4"},
{"T5"},
{"T6"},
{"T7"},
{"CN"}};


void time_update(void)
{
		static uint32_t t_update = 0;
		if(HAL_GetTick() - t_update >= 500)
		{
			rtc_read_time(&date_time);
			uint8_t day_of_week = rtc_read_day_of_week(&date_time);
			lcd_set_cursor(&lcd,0,2);
			lcd_printf(&lcd,"%02d:%02d:%02d", date_time.hour, date_time.minute, date_time.second);
			lcd_set_cursor(&lcd,1,0);
			lcd_printf(&lcd,"%s %02d/%02d/20%02d",arr_day[day_of_week], date_time.date, date_time.month, date_time.year);
			t_update = HAL_GetTick();
		}
}

typedef enum
{
	NORMAL_STATE,
	SETTING_HOUR_STATE,
	SETTING_MIN_STATE,
	SETTING_SEC_STATE,
	SETTING_DATE_STATE,
	SETTING_MON_STATE,
	SETTING_YEAR_STATE,
	ALARM_STATE
}Clock_state_t;
Clock_state_t clock_state = NORMAL_STATE;
void	btn_pressing_callback(Button_Typdef *ButtonX) //nhan la chay
{
	if(ButtonX == &btn_UP)
	{
		switch(clock_state)
		{
			case SETTING_HOUR_STATE:
				date_time.hour++;
				if(date_time.hour > 23)
				{
					date_time.hour = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_MIN_STATE:
				date_time.minute++;
				if(date_time.minute > 59)
				{
					date_time.minute = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_SEC_STATE:
				date_time.second++;
				if(date_time.second > 59)
				{
					date_time.second = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_DATE_STATE:
				date_time.date++;
				if(date_time.date > 30)
				{
					date_time.date = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_MON_STATE:
				date_time.month++;
				if(date_time.month > 12)
				{
					date_time.month = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_YEAR_STATE:
				date_time.year++;
				rtc_write_time(&date_time);
				break;
			default:
				break;
		}
	}
	else if(ButtonX == &btn_DOWN)
	{
		switch(clock_state)
		{
			case SETTING_HOUR_STATE:
				date_time.hour--;
				if(date_time.hour > 23)
				{
					date_time.hour = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_MIN_STATE:
				date_time.minute--;
				if(date_time.minute > 59)
				{
					date_time.minute = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_SEC_STATE:
				date_time.second--;
				if(date_time.second > 59)
				{
					date_time.second = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_DATE_STATE:
				date_time.date--;
				if(date_time.date > 30)
				{
					date_time.date = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_MON_STATE:
				date_time.month--;
				if(date_time.month > 12)
				{
					date_time.month = 0;
				}
				rtc_write_time(&date_time);
				break;
			case SETTING_YEAR_STATE:
				date_time.year--;
				rtc_write_time(&date_time);
				break;
			default:
				break;
		}
	}
}
void btn_press_short_callback(Button_Typdef *ButtonX ) //nhan roi nha ra moi chay
{
	static uint8_t i = 0;
	if(ButtonX == &btn_OK)
	{
		if(i > 6)
		{
			i = 0;
		}
		switch(i)
		{
			case 0:
				clock_state = SETTING_HOUR_STATE;
				break;
			case 1:
				clock_state = SETTING_MIN_STATE;
				break;
			case 2:
				clock_state = SETTING_SEC_STATE;
				break;
			case 3:
				clock_state = SETTING_DATE_STATE;
				break;
			case 4:
				clock_state = SETTING_MON_STATE;
				break;
			case 5:
				clock_state = SETTING_YEAR_STATE;
				break;
			case 6:
				clock_state = NORMAL_STATE;
				break;
			default:
				break;
		}
		i++;
	}
}

void btn_press_timeout_callback(Button_Typdef *ButtonX) //nhan giu moi chay
{
	if(ButtonX == &btn_OK)
	{
		clock_state = ALARM_STATE;
	}
}


void clock_handle(void)
{
	switch(clock_state)
	{
		case NORMAL_STATE:
			time_update();
			break;
		case SETTING_HOUR_STATE:
			break;
		case SETTING_MIN_STATE:
			break;
		case SETTING_SEC_STATE:
			break;
		case SETTING_DATE_STATE:
			break;
		case SETTING_MON_STATE:
			break;
		case SETTING_YEAR_STATE:
			break;
		default:
			break;
	}
}
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
  /* USER CODE BEGIN 2 */
	DS3231_Init(&hi2c1);
	lcd_init(&lcd,&hi2c1,LCD_ADDR_DEFAULT);
	button_init(&btn_OK, GPIOA, GPIO_PIN_0);
	button_init(&btn_UP, GPIOA, GPIO_PIN_1);
	button_init(&btn_DOWN, GPIOA, GPIO_PIN_2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		time_update();
		button_handle(&btn_DOWN);
		button_handle(&btn_OK);
		button_handle(&btn_UP);
		clock_handle();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  hi2c1.Init.ClockSpeed = 100000;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PA0 PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
