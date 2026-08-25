/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    UART_TX_NEW,
    UART_TX_CONT
} UART_TxStatus_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint32_t UART_Init(void);
uint32_t sendToUSART1(char *txbuf, size_t txbuf_len);
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
  MX_SPI1_Init();
  MX_SPI2_Init();
//  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  UART_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char* str = "Hi! It's working!\n";
  while (1)
  {
    /* USER CODE END WHILE */
    sendToUSART1(str, strlen(str));
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
uint32_t UART_Init(void) {
  //Enable clock for USART1 and GPIO
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
  SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
  
  USART1->CR1 = (uint32_t)0x00; // stop and clear

  //GPIO setup
  CLEAR_BIT(GPIOA->MODER, (3U << (BM_UART_UTX_PINn * 2)) | (3U << (BM_UART_URX_Pin_Nr * 2)));
  SET_BIT(GPIOA->MODER, (2U << (BM_UART_UTX_PINn * 2)) | (1U << (BM_UART_URX_Pin_Nr * 2)));
  // GPIOA_OTYPER pins 9/10 - reset state = 0 PushPull - OK
  // GPIOA_OSPEEDR pins 9/10 - reset state = 00 Low speed - OK for 115200
  // GPIOA_PUPDR pins 9/10 - reset state 00 - OK
  SET_BIT(GPIOA->AFR[1], 7U << (BM_UART_UTX_PINn - 8U)* 4 | 7U << (BM_UART_URX_Pin_Nr - 8U) * 4); //AF7 - USART1 TX/RX for PA9/10

  // Setting the baud rate to 115200
  // USARTDIV = fck/(8 * OVER8)/BR = 100000000/16/115200 = 54.25347(2)
  // DIV_fraction = 0.25347(2) * 16 = 4.0(5) -> 0x04
  // DIV_mantissa = 54 = 0x36
  uint32_t apb2_clock = 100000000;
  uint32_t br = 115200;

  USART1->BRR = (apb2_clock + (br / 2U)) / br;

  uint64_t usartdiv100 = 100 * (uint64_t)apb2_clock / 16 / br;
  uint32_t div_mant = usartdiv100 / 100;
  uint32_t div_fr = ((usartdiv100 % 100 * 16) / 100) & 0x0FU; 
  USART1->BRR = (div_mant << 4) | div_fr;

  // All the bits 0 state good for me: USART_CR1_M, USART_CR1_PCE, 
  // USART_CR1_PS, USART_CR1_OVER8, USART_CR2_STOP, just enable 
  USART1->CR1 = (uint32_t)(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE); 

  // Allow USART1 IRQ interrupt reach CPU
  NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
  NVIC_EnableIRQ(USART1_IRQn);

  return 0;
}



/*  *txbuf - a pointer to the buffer of chars start address; 
*           ignored in the case of repetetive call while buffer is being sending
*   txbuf_len - buffer length to send; 
*               reset sending if 0;  
*               any non-zero value expected to keep the buffer sending (repetetive call)
*/

uint32_t sendToUSART1(char *txbuf, size_t txbuf_len)
{
  static char *tx_pos = 0; //encapsulated sending parameters
  static size_t tx_rem = 0;

  if (tx_rem == 0 && tx_pos == 0 && txbuf) { //init condition
    tx_pos = txbuf;
    tx_rem = txbuf_len;
  }

  if (txbuf_len == 0) { // reset condition
    tx_rem = 0;
    tx_pos = 0;
    CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
    return 0;
  }

  if (tx_rem) {
    USART1->DR = (uint8_t)(*tx_pos) & (uint8_t)0x00FF;
    tx_rem--;
    tx_pos++;
  }

  if (tx_rem) {    
    SET_BIT(USART1->CR1, USART_CR1_TXEIE); 
  } else {
    tx_rem = 0;
    tx_pos = 0;
  }
  return 0;
}

void USART1_ISR(void)
{
  uint32_t status = USART1->SR;

  if(status & USART_SR_TXE) {
    CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
    sendToUSART1(0,1); //repetative call: buf address doesn't matter; len - non zero 
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
#ifdef USE_FULL_ASSERT
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
