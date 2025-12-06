/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_ThreadCreation/Src/main.c
  * @author  MCD Application Team
  * @version V1.2.2
  * @date    25-May-2015
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stm32f1xx_hal.h>
#include <../CMSIS_RTOS/cmsis_os.h>
#include "xv7011.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
osThreadId LEDThread2Handle;
osThreadId GyroThreadHandle;
SPI_HandleTypeDef hspi2;

/* Global variables ----------------------------------------------------------*/
XV7011_Data_t g_xv7011_data;
float g_temperature;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void LED_Thread2(void const *argument);
static void Gyro_Task(void const *argument);
static void SPI2_Init(void);
static void SPI2_CS_Init(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  System Clock Configuration
  * The system Clock is configured as follow : 
  *            System Clock source        = PLL (HSE)
  *     SYSCLK(Hz)      = 72000000
  *            HCLK(Hz)  = 72000000
  *       AHB Prescaler            = 1
  * APB1 Prescaler                 = 2
  *            APB2 Prescaler      = 1
  *HSE Frequency(Hz)         = 8000000
  *            HSE PREDIV     = 1
  *      PLLMUL        = 9
  * Flash Latency(WS)          = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 8MHz * 9 = 72MHz
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		// Error Handler
		while(1);
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 72MHz
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 36MHz (APB1 max 36MHz)
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // 72MHz

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		// Error Handler
		while(1);
	}
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
	HAL_Init();  
	
	/* Configure the system clock to 72 MHz */
	SystemClock_Config();
	
	/* Initialize SPI2 and CS pin */
	SPI2_CS_Init();
	SPI2_Init();
	
	__GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = GPIO_PIN_9 | GPIO_PIN_9;

	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Thread definition */
	/* Gyro task - higher priority, 256 stack size, 10ms period */
	osThreadDef(GYRO, Gyro_Task, osPriorityAboveNormal, 0, 256);
	
	/* LED2 task - normal priority */
	osThreadDef(LED2, LED_Thread2, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  
	/* Start threads */
	GyroThreadHandle = osThreadCreate(osThread(GYRO), NULL);
	LEDThread2Handle = osThreadCreate(osThread(LED2), NULL);
  
	/* Start scheduler */
	osKernelStart();

	  /* We should never get here as control is now taken by the scheduler */
	for (;;)
		;
}

void SysTick_Handler(void)
{
	HAL_IncTick();
	osSystickHandler();
}

/**
  * @brief  Toggle LED2 thread
  * @param  argument not used
  * @retval None
  */
static void LED_Thread2(void const *argument)
{
	uint32_t count;
	(void) argument;
  
	for (;;)
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
		osDelay(200);
	}
}

/**
  * @brief  Gyro Task - Read and process gyroscope data
  * @param  argument not used
  * @retval None
  * @note   Priority: Above Normal, Stack: 256 bytes, Period: 10ms
  */
static void Gyro_Task(void const *argument)
{
	(void) argument;
	
	/* Initialize global temperature variable */
	g_temperature = 0.0f;
	
	/* Initialize XV7011 gyroscope sensor */
	if (XV7011_Init() == 0) {
		/* Initialization successful */
		osDelay(100);
	} else {
		/* Initialization failed - error handler */
		while(1) {
			osDelay(1000);
		}
	}
	
	/* Main task loop */
	for (;;)
	{
		/* Read all XV7011 data (angular rate and temperature) */
		XV7011_ReadAllData(&g_xv7011_data);
		
		/* Integrate angular rate to calculate angle offset */
		/* Angular rate unit: бу/s, Task period: 10ms = 0.01s */
		/* Angle offset = angular_rate * dt * 4 */
		float angle_offset = g_xv7011_data.angular_rate * 0.01f * 4.0f;  // бу/s * 0.01s * 4 = degrees
		
		/* Accumulate angle offset to global temperature variable */
		g_temperature += angle_offset;
		
		/* Task period: 10ms */
		osDelay(10);
	}
}

/**
  * @brief  SPI2 Initialization Function
  * @param  None
  * @retval None
  * @note   SPI2 pins: PB13(SCK), PB14(MISO), PB15(MOSI)
  */
static void SPI2_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Enable clocks */
	__HAL_RCC_SPI2_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* SPI2 GPIO Configuration
	 * PB13 ------> SPI2_SCK
	 * PB14 ------> SPI2_MISO
	 * PB15 ------> SPI2_MOSI
	 */
	GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_14;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* SPI2 parameter configuration */
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 36MHz/8 = 4.5MHz
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		// Error Handler
		while(1);
	}
}

/**
  * @briefSPI2 CS Pin Initialization Function
  * @param  None
  * @retval None
  * @note   CS pin: PB12
  */
static void SPI2_CS_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Enable GPIOB clock */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Configure GPIO pin : PB12 as CS */
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Set CS high (idle state) */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
	while (1)
	{
	}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
