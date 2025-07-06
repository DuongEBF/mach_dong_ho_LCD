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
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
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

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
LiquidCrystal_I2C lcd;
Date_time_t date_time;
Button_Typdef btn_OK;
Button_Typdef btn_UP;
Button_Typdef btn_DOWN;
int8_t hour_alarm, min_alarm;
uint32_t setting_state_tick = 0;
uint32_t last_auto_switch = 0;
uint8_t alarm_active = 0;
uint32_t alarm_start_tick = 0;
char bt_rx_buffer[32];
uint8_t bt_rx_index = 0;
uint8_t bt_rx_data;
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
	ALARM_HOUR_STATE,
	ALARM_MIN_STATE
}Clock_state_t;
Clock_state_t clock_state = NORMAL_STATE;
typedef enum
{
	SETTING_STATE,
	ALARM_STATE
}ClockMode_t;
ClockMode_t setting_mode = SETTING_STATE;
//-----------setting_app-------------------//
void parse_bluetooth_cmd(char *cmd)
{
    if(strncmp(cmd, "SETTIME", 7) == 0)
    {
        int h, m, s, d, mo, y;
        if(sscanf(cmd, "SETTIME,%d,%d,%d,%d,%d,%d", &h, &m, &s, &d, &mo, &y) == 6)
        {
            date_time.hour = h;
            date_time.minute = m;
            date_time.second = s;
            date_time.date = d;
            date_time.month = mo;
            date_time.year = y;
            rtc_write_time(&date_time);
        }
    }
    else if(strncmp(cmd, "SETALARM", 8) == 0)
    {
        int h, m;
        if(sscanf(cmd, "SETALARM,%d,%d", &h, &m) == 2)
        {
            hour_alarm = h;
            min_alarm = m;
            ds3231_write_ram(0x08, hour_alarm);
            ds3231_write_ram(0x09, min_alarm);
        }
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART1)
	{
		if(bt_rx_data != '\n' && bt_rx_index < sizeof(bt_rx_buffer) - 1)
		{
			bt_rx_buffer[bt_rx_index++] = bt_rx_data;
		}
		else
		{
			bt_rx_buffer[bt_rx_index] = 0; 
			bt_rx_index = 0;
			parse_bluetooth_cmd(bt_rx_buffer);
		}
		HAL_UART_Receive_IT(&huart1, &bt_rx_data, 1);
	}
}

//-------------handle clock---------------//
uint8_t check_leap_year(uint16_t year)
{
	if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
	{
		return 1;
	}
	return 0;
}
uint8_t get_max_date(uint8_t mon, uint16_t year)
{
	switch(mon)
	{
		case 1: case 3: case 5: case 7: case 8: case 10: case 12:
			return 31;
		case 4: case 6: case 9: case 11:
			return 30;
		case 2:
			if(check_leap_year(year) == 1)
			{
				return 29;
			}
			else 
				return 28;
		default:
			return 0;
	}
}
void up_down_update(int8_t* number, int8_t max, int8_t min, uint8_t plus)
{
	if(plus)
	{
		(*number)++;
		if(*number > max)
		{
			*number = min;
		}
	}
	else
	{
		(*number)--;
		if(*number < min)
		{
			*number = max;
		}
	}
}
//---------------------------check alarm------------------------------//
void check_alarm(void)
{
	if(date_time.hour == hour_alarm && date_time.minute == min_alarm && !alarm_active)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		alarm_active = 1;
		alarm_start_tick = HAL_GetTick();
	}
	if(alarm_active)
	{
		if(HAL_GetTick() - alarm_start_tick >= 600000)
		{
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			alarm_active = 0;
		}
	}
}
//---------------------------adjust------------------------------//
void adjust_time(uint8_t plus)
{
		switch(clock_state)
		{
			case SETTING_HOUR_STATE:
			{
				up_down_update(&date_time.hour, 23, 0, plus);
				rtc_write_time(&date_time);
				break;
			}
			case SETTING_MIN_STATE:
			{
				up_down_update(&date_time.minute, 59, 0, plus);
				rtc_write_time(&date_time);
				break;
			}
			case SETTING_SEC_STATE:
			{
				up_down_update(&date_time.second, 59, 0, plus);
				rtc_write_time(&date_time);
				break;
			}
			case SETTING_DATE_STATE:
			{
				uint8_t max_date = get_max_date(date_time.month, date_time.year);
				up_down_update(&date_time.date, max_date, 1, plus);
				rtc_write_time(&date_time);
				break;
			}
			case SETTING_MON_STATE:
			{
				up_down_update(&date_time.month, 12, 1, plus);
				uint8_t max_date = get_max_date(date_time.month, date_time.year);
				
				if(date_time.date > max_date)
				{
					date_time.date = max_date;
				}
				rtc_write_time(&date_time);
				break;
			}
			case SETTING_YEAR_STATE:
			{
				up_down_update(&date_time.year, 99, 0, plus);
				rtc_write_time(&date_time);
				break;
			}
			case ALARM_HOUR_STATE:
			{
				up_down_update(&hour_alarm, 23, 0, plus);
				break;
			}
			case ALARM_MIN_STATE:
			{
				up_down_update(&min_alarm, 59, 0, plus);
				break;
			}
			default:
				break;
		}
}
void	btn_pressing_callback(Button_Typdef *ButtonX) //nhan la chay
{
	if(clock_state != NORMAL_STATE)
		setting_state_tick = HAL_GetTick(); // Reset tick
	if(ButtonX == &btn_UP)
	{
		adjust_time(1);
	}
	else if(ButtonX == &btn_DOWN)
	{
		adjust_time(0);
	}
}
void btn_press_short_callback(Button_Typdef *ButtonX)
{
	static uint8_t state_setting = 0;
	static uint8_t state_alarm = 0;
	if(ButtonX == &btn_OK)
	{
		if(clock_state == NORMAL_STATE)
		{	
			state_setting = 0;
			state_alarm = 0;
			setting_state_tick = HAL_GetTick();
		}
		if(setting_mode == SETTING_STATE)
		{
			switch(state_setting)
			{
				case 0: clock_state = SETTING_HOUR_STATE; break;
				case 1: clock_state = SETTING_MIN_STATE;  break;
				case 2: clock_state = SETTING_SEC_STATE;  break;
				case 3: clock_state = SETTING_DATE_STATE; break;
				case 4: clock_state = SETTING_MON_STATE;  break;
				case 5: clock_state = SETTING_YEAR_STATE; break;
				case 6: clock_state = NORMAL_STATE;       break;
			}
			if(state_setting < 6) state_setting++;
		}
		else if(setting_mode == ALARM_STATE)
		{
			if(state_alarm < 2) ++state_alarm;
			switch(state_alarm)
			{
				case 0: clock_state = ALARM_HOUR_STATE; break;
				case 1: clock_state = ALARM_MIN_STATE;  break;
				case 2:
					clock_state = NORMAL_STATE;
				  setting_mode = SETTING_STATE;
					break;
			}
		}
		if(clock_state != NORMAL_STATE)
			setting_state_tick = HAL_GetTick();
	}
}

void btn_press_timeout_callback(Button_Typdef *ButtonX)
{
	if(ButtonX == &btn_OK)
	{
		if(clock_state != NORMAL_STATE)
    {	
			setting_state_tick = HAL_GetTick();
		}
		if(setting_mode == SETTING_STATE)
		{
			setting_mode = ALARM_STATE;
			clock_state = ALARM_HOUR_STATE; 
			lcd_clear_display(&lcd);
			lcd_set_cursor(&lcd, 0, 5);
			lcd_printf(&lcd, "ALARM");
		}
		else
		{
			setting_mode = SETTING_STATE;
			clock_state = NORMAL_STATE;
			ds3231_write_ram(0x08, hour_alarm);
			ds3231_write_ram(0x09, min_alarm);
		}
		if(clock_state != NORMAL_STATE)
			setting_state_tick = HAL_GetTick();
	}
}


void auto_next_setting_state(void)
{
	if(setting_mode == SETTING_STATE && clock_state != NORMAL_STATE)
	{
		uint8_t ok_state = HAL_GPIO_ReadPin(btn_OK.GPIOx, btn_OK.GPIO_Pin);
		if(ok_state == 0)
		{
			if(HAL_GetTick() - last_auto_switch > 200)
			{
				switch(clock_state)
				{
					case SETTING_HOUR_STATE: clock_state = SETTING_MIN_STATE; break;
					case SETTING_MIN_STATE:  clock_state = SETTING_SEC_STATE; break;
					case SETTING_SEC_STATE:  clock_state = SETTING_DATE_STATE; break;
					case SETTING_DATE_STATE: clock_state = SETTING_MON_STATE; break;
					case SETTING_MON_STATE:  clock_state = SETTING_YEAR_STATE; break;
					case SETTING_YEAR_STATE: clock_state = NORMAL_STATE; break;
					default: break;
				}
				setting_state_tick = HAL_GetTick();
				last_auto_switch = HAL_GetTick();
			}
		}
		else
		{
			last_auto_switch = HAL_GetTick();
		}
	}
}

//-----------------blink----------------//


void setting_blink(void)
{
		static uint32_t t_blink = 0;
		static uint8_t is_show = 1;
		char line1[16];
		char line2[16];
		if(HAL_GetTick() - t_blink >= 300)
		{
			is_show = !is_show; 
			rtc_read_time(&date_time);
			uint8_t day_of_week = rtc_read_day_of_week(&date_time);
			sprintf(line1,"%02d:%02d:%02d", date_time.hour, date_time.minute, date_time.second);
			sprintf(line2,"%s %02d/%02d/20%02d",arr_day[day_of_week], date_time.date, date_time.month, date_time.year);
			if(is_show)
			{
				lcd_set_cursor(&lcd,0,2);
				lcd_printf(&lcd,"%s", line1);
				lcd_set_cursor(&lcd,1,0);
				lcd_printf(&lcd,"%s",line2);
			}
			else
			{
				switch(clock_state)
				{
					case SETTING_HOUR_STATE:
						line1[0] = line1[1] = ' ';
						break;
					case SETTING_MIN_STATE:
						line1[3] = line1[4] = ' ';
						break;
					case SETTING_SEC_STATE:
						line1[6] = line1[7] = ' ';
						break;
					case SETTING_DATE_STATE:
						line2[3] = line2[4] = ' ';
						break;
					case SETTING_MON_STATE:
						line2[6] = line2[7] = ' ';
						break;
					case SETTING_YEAR_STATE:
						line2[9] = line2[10] = line2[11] = line2[12] = ' ';
						break;
					default:
						break;
				}
				lcd_set_cursor(&lcd,0,2);
				lcd_printf(&lcd,"%s", line1);
				lcd_set_cursor(&lcd,1,0);
				lcd_printf(&lcd,"%s",line2);
			}
			t_blink = HAL_GetTick();
		}
}

void alarm_screen(void)
{
	static uint32_t t_blink = 0;
	static uint8_t is_show = 1;
	char line2[16];
	if(HAL_GetTick() - t_blink >= 300)
	{
		is_show = !is_show;
		sprintf(line2,"%02d:%02d", hour_alarm, min_alarm);
		if(is_show)
		{
			switch(clock_state)
			{
				case ALARM_HOUR_STATE:
					line2[0] = line2[1] = ' ';
					break;
				case ALARM_MIN_STATE:
					line2[3] = line2[4] = ' ';
					break;
				default:
					break;
			}
			lcd_set_cursor(&lcd,1,5);
			lcd_printf(&lcd,"%s",line2);

		}
		else
		{
			lcd_set_cursor(&lcd,1,5);
			lcd_printf(&lcd,"%s",line2);
		}
		t_blink = HAL_GetTick();
	}
}

void clock_handle(void)
{
	switch(clock_state)
	{
		case NORMAL_STATE:
			time_update();
			check_alarm();
			break;
		case SETTING_SEC_STATE:
		case SETTING_MIN_STATE:
		case SETTING_HOUR_STATE:
		case SETTING_DATE_STATE:
		case SETTING_MON_STATE:
		case SETTING_YEAR_STATE:
			setting_blink();
			break;
		case ALARM_HOUR_STATE:
		case ALARM_MIN_STATE:
			alarm_screen();
			break;
	}
	if(clock_state != NORMAL_STATE)
	{
    if(HAL_GetTick() - setting_state_tick > 5000)
    {
			clock_state = NORMAL_STATE;
			setting_mode = SETTING_STATE;
		}
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	DS3231_Init(&hi2c1);
	lcd_init(&lcd,&hi2c1,LCD_ADDR_DEFAULT);
	button_init(&btn_OK, GPIOA, GPIO_PIN_0);
	button_init(&btn_UP, GPIOA, GPIO_PIN_1);
	button_init(&btn_DOWN, GPIOA, GPIO_PIN_2);
	hour_alarm = ds3231_read_ram(0x08);
	min_alarm = ds3231_read_ram(0x09);
	HAL_UART_Receive_IT(&huart1, &bt_rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		button_handle(&btn_DOWN);
		button_handle(&btn_OK);
		button_handle(&btn_UP);
		clock_handle();
		auto_next_setting_state();
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
  huart1.Init.BaudRate = 9600;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
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
