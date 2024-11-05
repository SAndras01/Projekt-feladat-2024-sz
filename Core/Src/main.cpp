/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "GPIO.hpp"
#include "MAX31865.hpp"
#include "MS.hpp"
#include "stdio.h"
#include "string.h"
#include <inttypes.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum{
	MEAS,
	COMM
} states;

typedef enum{
	IDLE,
	INIT,
	READOUT,
} commStates;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define Buffer_Size 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint32_t measFrequency = 3; //[s]
uint32_t idleTime = 0;
MeasEntry currentMeas;

char commandBuffer[Buffer_Size];
char argBuffer[Buffer_Size];


uint32_t timerInterruptCntr = 0;
bool timeInterruptTick = false;

bool displayMeas = true;

states currentState = MEAS;
const char* MEAS_command = "enterMeas";
const char* COMM_command = "enterComm";
const char* GETSTATE_command = "getState";
const char* WHOAMI_command = "whoami";
const char* FREQ_command = "setFrequency";
const char* DISPLAYMEAS_command = "displayMeas";
const char* DISPLAYMEAS_arg_true = "ON";
const char* DISPLAYMEAS_arg_false = "OFF";

commStates currentCommState = IDLE;
const char* CommIDLE_command = "IDLE";
const char* CommINIT_command = "INIT";
const char* CommREADOUT_command = "READOUT";

uint64_t initTimeastamp = 0;

bool measCommand = false;
bool sendComplete = true;

char msg[Buffer_Size];
uint8_t RxData[Buffer_Size];
uint8_t tmp[2];
uint8_t FinalData[Buffer_Size];
uint8_t idx = 0;



GPIO TEMP_SENS_CS(TEMP_SENS_CS_GPIO_Port, TEMP_SENS_CS_Pin);
GPIO TEMP_RDY(TEMP_RDY_GPIO_Port, TEMP_RDY_Pin);

MAX31865 myPT100(&hspi1, &TEMP_SENS_CS, &TEMP_RDY);
MeasurementStorage myMS(&hi2c1, 80);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void i2cScann(I2C_HandleTypeDef *hi2c_p, uint8_t* devicesBuffer_p)
{
	HAL_StatusTypeDef status;

	for(uint8_t i = 0; i<128; i++){ devicesBuffer_p[i] = 0; }

	uint8_t j = 0;
	for(uint8_t i = 0; i<128; i++)
	{
		status = HAL_I2C_IsDeviceReady(hi2c_p, i<<1, 2, 100);
		if(status == HAL_OK)
		{
			devicesBuffer_p[j] = i;
			j++;
		}
	}
}

void handleMessage()
{

	uint8_t matchResult = sscanf((const char*)FinalData, "%s %s", commandBuffer, argBuffer);

	if(matchResult > 0)
	{
		if( strcmp((const char*) commandBuffer, MEAS_command) == 0)
		{
			currentState = MEAS;
		}
		else if( strcmp((const char*) commandBuffer, COMM_command) == 0)
		{
			currentState = COMM;
		}

		else if( strcmp((const char*) commandBuffer, GETSTATE_command) == 0)
		{
			switch (currentState)
			{
				case MEAS:
					snprintf(msg, Buffer_Size, "MEAS\r\n");
					break;
				case COMM:
					snprintf(msg, Buffer_Size, "COMM\r\n");
					break;
				default:
					break;
			}
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		}
		else if( strcmp((const char*) commandBuffer, WHOAMI_command) == 0)
		{
			snprintf(msg, Buffer_Size, "MEAS_STATION\r\n");

			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		}
		else if( strcmp((const char*) commandBuffer, DISPLAYMEAS_command) == 0)
		{
			if(strcmp((const char*) argBuffer, DISPLAYMEAS_arg_true) == 0)
			{
				displayMeas = true;
			}
			else if(strcmp((const char*) argBuffer, DISPLAYMEAS_arg_false) == 0)
			{
				displayMeas = false;
			}
		}
		else if(matchResult == 2 && strcmp((const char*) commandBuffer, FREQ_command) == 0)
		{
			sscanf((const char*)argBuffer, "%lu", &measFrequency);
		}

		//If already in COMM accept COMM command
		else if(currentState == COMM)
		{
			if( strcmp((const char*) commandBuffer, CommIDLE_command) == 0)
			{
				currentCommState = IDLE;
			}
			else if( strcmp((const char*) commandBuffer, CommREADOUT_command) == 0)
			{
				currentCommState = READOUT;
			}
			if(matchResult == 2 && strcmp((const char*) commandBuffer, CommINIT_command) == 0)
			{
				if( sscanf((const char*)argBuffer, "%llu", &initTimeastamp) == 1)
				{
					currentCommState = INIT;
				}
			}
		}
	}//matchresult
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	sendComplete = true;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) // Ellenőrizni, hogy melyik UART periféria váltotta ki a megszakítást
    {
    	if(idx > Buffer_Size)
    	{
    		idx = 0;
    	}

    	memcpy(RxData+idx, tmp, 1);
    	idx++;

    	if (tmp[0] == '\r')
    	{
    		RxData[idx-1] = '\0';//change \r to \0
    		memcpy(FinalData, RxData, Buffer_Size);
    		idx = 0;
    		handleMessage();
    	}


    	HAL_UART_Receive_IT(&huart2, tmp, 1);

    }
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  HAL_ResumeTick();
}
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  myPT100.init();
  myPT100.startContinousMeas();

  uint8_t devices[128];
  i2cScann(&hi2c1, devices);

  myMS.init(12345678987654321012);

/*
  deleteRegion(&hi2c1, 80, 0, 256, 128);

  uint8_t firstTwoPage0[256];
  uint8_t firstTwoPage1[256];
  uint8_t EEPROM_ADDRESS = 80;
  uint16_t memAddress = 0;

  readFromEEPROM(&hi2c1, EEPROM_ADDRESS<<1, memAddress, sizeof(memAddress), firstTwoPage0, sizeof(firstTwoPage0), HAL_MAX_DELAY);

  myMS.init(12345678987654321012);

  MeasEntry newEntry =
  {
		  .measID = 3,
		  .deltaT = 12345,
		  .measData = 7891
  };

  myMS.addEntry(newEntry);
  MeasEntry fetchedEntry;
  myMS.getEntryAt(0, &fetchedEntry);

  readFromEEPROM(&hi2c1, EEPROM_ADDRESS<<1, memAddress, sizeof(memAddress), firstTwoPage1, sizeof(firstTwoPage1), HAL_MAX_DELAY);

  */

  HAL_UART_Receive_IT(&huart2, tmp, 1);

  bool onEntry_meas = true;
  bool onEntry_comm = true;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  MeasEntry entryBuffer;
  while (1)
  {
	  //@todo: bosch laptopból kinézni hogyan is volt a %llu, illetve a command - argument dolog, hogy tudjak initelni
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  /* switch-case -> állapotgép
	   * az állapotot lehessen lekérdezni is, ezt a beérkező parancsokat kezelő fgv.-ben kell kezelni.
	   *
	   * */
		switch (currentState)
		{
			case MEAS:
			{
				if(onEntry_meas)
				{
					HAL_TIM_Base_Start_IT(&htim3);
					onEntry_meas = false;
					onEntry_comm = true;
					currentCommState = IDLE; //The next time COMM state is entered it will be idle
				}

				if(timeInterruptTick)
				{
					float tempMeas = myPT100.getTemp();
					uint32_t data32;
					memcpy(&data32, (const float*)&tempMeas, sizeof(uint32_t));

					if(displayMeas)
					{
						snprintf(msg, Buffer_Size, "%0.3f \r\n", tempMeas);
						HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
					}



					currentMeas.measID = 1;
					currentMeas.deltaT = measFrequency+idleTime;
					idleTime = 0;
					currentMeas.measData = data32;


					myMS.addEntry(currentMeas);

					timeInterruptTick = false;
				}

				HAL_SuspendTick();
				HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
				//...
				HAL_ResumeTick();

				break;
			}
			case COMM:
			{
				if(onEntry_comm)
				{
					//HAL_TIM_Base_Stop_IT(&htim3);
					onEntry_meas = true;
					onEntry_comm = false;
				}

				switch (currentCommState)
				{
					case IDLE:
						//do nothing
						break;
					case INIT:
					{
						myMS.init(initTimeastamp);
						currentCommState = IDLE;
						break;
					}
					case READOUT:
					{
						uint16_t cnt = 0;
						sniprintf(msg, Buffer_Size, "%llu; %u;\r\n", myMS.readTimestamp(), myMS.readCounter());
						HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
						while(true)
						{
							if( !myMS.getEntryAt(cnt, &entryBuffer) )
							{
								break;
							}
							else
							{
								cnt++;
								snprintf(msg, Buffer_Size, "%u, %u, %lu;\r\n", entryBuffer.measID, entryBuffer.deltaT, entryBuffer.measData);
								HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
							}
						}
						snprintf(msg, Buffer_Size, "END\r\n");
						HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
						currentCommState = IDLE;
						break;
					}
					default:
						break;
				}
			}
				break;
			default:
				break;
		}

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 8400-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  HAL_GPIO_WritePin(TEMP_SENS_CS_GPIO_Port, TEMP_SENS_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TEMP_SENS_CS_Pin */
  GPIO_InitStruct.Pin = TEMP_SENS_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TEMP_SENS_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TEMP_RDY_Pin */
  GPIO_InitStruct.Pin = TEMP_RDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TEMP_RDY_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim3)
	{
		timerInterruptCntr++;
		if(currentState != MEAS)
		{
			idleTime++;
		}
		if(( (timerInterruptCntr % measFrequency) == 0 ) && currentState == MEAS)
		{
			timeInterruptTick = true;
			timerInterruptCntr = 0;
		}
		/*char msg2[20];
		snprintf(msg2, 20, "Interrupt Called\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)msg2, 20, HAL_MAX_DELAY);*/
	}
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
