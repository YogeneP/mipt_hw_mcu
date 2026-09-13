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
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define BAUD_RATE 115200ULL
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t spi1Complete = 0;
volatile uint8_t spi4Complete = 0;

char str[256] = {0};
uint8_t str_len = 0;
uint8_t spi1_buffer[16] = {0};
uint8_t spi4_buffer[16] = {0};
char inttostrbuf[11] = {0};
LED_t led;
char* baudrate_str = NULL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
char* uint32_to_str(uint32_t val, char* buf);
const char* spi_state_to_str(HAL_SPI_StateTypeDef state);

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
  MX_SPI4_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  LED_Init(&led, LED_GPIO_Port, LED_Pin);
  BM_UART_Init(BAUD_RATE); // Custom self-made bare-metal init, no HAL USART drivers needed, disabled in CubeMX 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  baudrate_str = uint32_to_str(BAUD_RATE, inttostrbuf);
  str_len = strlen(baudrate_str);
  BM_UART_Transmit("Serial output initiated: \0", 26);
  HAL_Delay(2);
  BM_UART_Transmit(baudrate_str, str_len);
  HAL_Delay(2);
  BM_UART_Transmit(" 8-N-1 \n\0", 9);
  HAL_Delay(2);
  led.enable = ON;

  HAL_SPI_Receive_IT(&hspi4, spi4_buffer, 16);
  HAL_Delay(1);

  BM_UART_Transmit("One way communication:\n",24);
  HAL_Delay(2);
  BM_UART_Transmit("Sending from SPI1 (master) to SPI4 (slave) ABCDEF0123456789\n",61);
  HAL_Delay(2);
  // Just HAL_SPI_Transmit() performs an unhandled behaviour because of parasitic bytes
  // flow backwards from slave to master and cause an Overrun fault because are not consumed
  HAL_SPI_TransmitReceive(&hspi1, (const uint8_t *)"ABCDEF0123456789", spi1_buffer, 16, 5); 

  HAL_Delay(5);

  if (spi4Complete) {
    BM_UART_Transmit("Received by SPI4: ",19);
    HAL_Delay(2);
    BM_UART_Transmit((char*)spi4_buffer, sizeof(spi4_buffer));
    HAL_Delay(2);
    BM_UART_Transmit("\n", 2);
    HAL_Delay(1);
  } else {
    BM_UART_Transmit("SPI comm timeout!\n",19);
    HAL_Delay(2);
  }

  BM_UART_Transmit("Sending from SPI4 (slave) to SPI1 (master) 9876543210FEDCBA\n",61);
  HAL_Delay(4);
  
  HAL_SPI_Transmit_IT(&hspi4, (const uint8_t *)"9876543210FEDCBA", 16);
  HAL_Delay(1);
  // HAL_SPI_receive() calls HAL_SPI_TransmitReceive() for master 
  // from stm32f4xx_hal_spi.c background automatically if Full Duplex mode is selected
  HAL_SPI_Receive(&hspi1, spi1_buffer, 16, 5);
  HAL_Delay(5);
  if (spi4Complete) {
    BM_UART_Transmit("Received by SPI1: ",19);
    HAL_Delay(2);
    BM_UART_Transmit((char*)spi1_buffer, sizeof(spi1_buffer));
    HAL_Delay(2);
    BM_UART_Transmit("\n", 2);
    HAL_Delay(1);
  } else {
    BM_UART_Transmit("SPI comm timeout!\n",19);
    HAL_Delay(2);
  }

  memset(spi1_buffer,0,16);
  memset(spi4_buffer,0,16);
 
  // Switching the roles of the interfaces: 
  // let SPI4 to be a master; SPI1 - slave
  NVIC_DisableIRQ(SPI1_IRQn);
  NVIC_DisableIRQ(SPI4_IRQn);

  HAL_SPI_DeInit(&hspi1);
  HAL_SPI_DeInit(&hspi4);
  HAL_Delay(1);

  __HAL_RCC_SPI1_FORCE_RESET();
  __HAL_RCC_SPI4_FORCE_RESET();
  HAL_Delay(1);
  __HAL_RCC_SPI1_RELEASE_RESET();
  __HAL_RCC_SPI4_RELEASE_RESET();
  HAL_Delay(1);

  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;

  if(hspi1.State != HAL_SPI_STATE_RESET) {
    BM_UART_Transmit("SPI1 is not in RESET state before init!\n",41);
  }
  if(hspi4.State != HAL_SPI_STATE_RESET) {
    BM_UART_Transmit("SPI4 is not in RESET state before init!\n",41);
  }

  HAL_SPI_Init(&hspi1);
  HAL_SPI_Init(&hspi4);

  NVIC_ClearPendingIRQ(SPI1_IRQn);
  NVIC_ClearPendingIRQ(SPI4_IRQn);

  NVIC_EnableIRQ(SPI1_IRQn);
  NVIC_EnableIRQ(SPI4_IRQn);
  HAL_Delay(1);
  
  /* HOT SWITCH WITHOUT DeInit 
  * The transmission actually works, but the LL config gets away from the HAL traced attributes 
  * and causes some background HAL fault
  * I've tried to fix it manually, but the fault still appears 
  * The main difference from the HAL reInit, that GPIO pins are not to be deinitialized, 
  * using them as is 
  */
 /*
  __HAL_SPI_DISABLE(&hspi1);
  __HAL_SPI_DISABLE(&hspi4);

  volatile uint32_t tmpreg;
  tmpreg = SPI1->SR;
  tmpreg = SPI1->DR;
  tmpreg = SPI4->SR;
  tmpreg = SPI4->DR;
  (void)tmpreg;

  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  

  // Clear the Master (MSTR) bit on SPI1, Set it on SPI4
  // Ensure Software Slave Management (SSM) remains active
  SPI1->CR1 = (SPI1->CR1 & ~SPI_CR1_MSTR) | SPI_CR1_SSM; 
  SPI4->CR1 = (SPI4->CR1 | SPI_CR1_MSTR)  | SPI_CR1_SSM;

  // Master (SPI4) needs SSI=1 to prevent Mode Faults. Slave (SPI1) needs SSI=0.
  SET_BIT(SPI4->CR1, SPI_CR1_SSI);
  CLEAR_BIT(SPI1->CR1, SPI_CR1_SSI);

  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi4.Init.Mode = SPI_MODE_MASTER;

  __HAL_SPI_ENABLE(&hspi1);
  __HAL_SPI_ENABLE(&hspi4);
  HAL_Delay(5);

  // There was a try to flush the DR and SR registers while the SPI instances had been configured
  // in uni-directional mode (Transmit only/Receive only) but it doesn't help, only the switch to the 
  // Full Duplex mode led to success.
  
  volatile uint32_t flush_dr;
  volatile uint32_t flush_sr;
  uint32_t count = 10000000;
  
  while ((SPI1->SR & (SPI_SR_OVR | SPI_SR_RXNE)) && count ) {
      flush_dr = SPI1->DR;
      flush_sr = SPI1->SR;
      count--;
  }
  
  if (SPI1->SR & (SPI_SR_OVR | SPI_SR_RXNE)) {
    BM_UART_Transmit((char *)"SPI1 DR/SR register flush fault!\n\0", 34);
  }

   count = 100;
  while ((SPI4->SR & (SPI_SR_OVR | SPI_SR_RXNE)) && count ) {
      flush_dr = SPI4->DR;
      flush_sr = SPI4->SR;
      count--;
  }
  (void)flush_dr;
  (void)flush_sr;
  
  if (SPI4->SR & (SPI_SR_OVR | SPI_SR_RXNE)) {
    BM_UART_Transmit((char *)"SPI4 DR/SR register flush fault!\n\0", 34);
  }

  // Clear tracking errors so HAL driver handles accept incoming requests cleanly
  hspi1.State     = HAL_SPI_STATE_READY;
  hspi4.State     = HAL_SPI_STATE_READY;
  hspi1.ErrorCode = HAL_SPI_ERROR_NONE;
  hspi4.ErrorCode = HAL_SPI_ERROR_NONE;
  // Flush the NVIC channels 
  NVIC_ClearPendingIRQ(SPI1_IRQn);
  NVIC_ClearPendingIRQ(SPI4_IRQn);
  HAL_Delay(1);
  */

  spi1Complete = 0;
  spi4Complete = 0;
  // Check HAL state
  if (hspi1.State != HAL_SPI_STATE_READY) {
    BM_UART_Transmit("HAL SPI1 Handle Blocked! SPI1 state = ", 38);
    HAL_Delay(5);
    const char* spi1state = spi_state_to_str(hspi1.State);
    BM_UART_Transmit((char*)spi1state, strlen((char*)spi1state));
    HAL_Delay(2);
  }
  if (hspi4.State != HAL_SPI_STATE_READY) {
    BM_UART_Transmit("HAL SPI4 Handle Blocked! SPI4 state = ", 38); 
    HAL_Delay(5);
    const char* spi4state = spi_state_to_str(hspi4.State);
    BM_UART_Transmit((char*)spi4state, strlen((char*)spi4state));
    HAL_Delay(2);
  }

  BM_UART_Transmit("The master-slave roles are switched.\n",38);
  HAL_Delay(3);
  BM_UART_Transmit("Two ways communication, package exchange between:\n",51);
  HAL_Delay(4);
  BM_UART_Transmit("SPI4 (master, sending 0123456789ABCDEF);\n",42);
  HAL_Delay(3);
  BM_UART_Transmit("SPI1 (slave, sending FEDCBA9876543210);\n",41);
  HAL_Delay(3);
  HAL_SPI_TransmitReceive_IT(&hspi1, (const uint8_t *)"FEDCBA9876543210", spi1_buffer, 16);
  HAL_Delay(1);
  HAL_SPI_TransmitReceive(&hspi4, (const uint8_t *)"0123456789ABCDEF", spi4_buffer, 16, 5);
  HAL_Delay(5);
  if (spi1Complete) {
    BM_UART_Transmit("Received by SPI1\n",18);
    HAL_Delay(2);
    BM_UART_Transmit((char*)spi1_buffer, sizeof(spi1_buffer));
    HAL_Delay(2);
    BM_UART_Transmit("\n", 2);
    HAL_Delay(2);
    BM_UART_Transmit("Received by SPI4\n",18);
    HAL_Delay(2);
    BM_UART_Transmit((char*)spi4_buffer, sizeof(spi4_buffer));
    HAL_Delay(2);
    BM_UART_Transmit("\n", 2);
    HAL_Delay(1);
  } else {
    BM_UART_Transmit("SPI comm timeout!\n",19);
    HAL_Delay(2);
  }

  while(1)
  { 
    led.blink = 200;
    LED_Handler(&led, HAL_GetTick());
    HAL_Delay(1);
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

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  if(hspi->Instance == SPI1) {
    spi1Complete = 1;
  }
  if(hspi->Instance == SPI4) {
    spi4Complete = 1;
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  if(hspi->Instance == SPI1) {
    spi1Complete = 1;
  }
  if(hspi->Instance == SPI4) {
    spi4Complete = 1;
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if(hspi->Instance == SPI1) {
    spi1Complete = 1;
  }
  if(hspi->Instance == SPI4) {
    spi4Complete = 1;
  }
}

// Convert a uint32_t into a null-terminated string array
char* uint32_to_str(uint32_t val, char* buf) {
    int i = 10;
    buf[i] = '\0';     
    if (val == 0) {
        buf[--i] = '0';
    } else {
        while (val > 0 && i > 0) {
            buf[--i] = '0' + (val % 10);
            val /= 10;
        }
    }
    return &buf[i]; // Return the pointer to where the number actually begins
}

const char* spi_state_to_str(HAL_SPI_StateTypeDef state) {
  switch (state) {
    case HAL_SPI_STATE_RESET:      return "RESET\n";       // 0x00
    case HAL_SPI_STATE_READY:      return "READY\n";       // 0x01
    case HAL_SPI_STATE_BUSY:       return "BUSY\n";        // 0x02
    case HAL_SPI_STATE_BUSY_TX:    return "BUSY_TX\n";     // 0x03
    case HAL_SPI_STATE_BUSY_RX:    return "BUSY_RX\n";     // 0x04
    case HAL_SPI_STATE_BUSY_TX_RX: return "BUSY_TX_RX\n";  // 0x05
    case HAL_SPI_STATE_ERROR:      return "ERROR\n";       // 0x06
    case HAL_SPI_STATE_ABORT:      return "ABORT\n";       // 0x07
    default:                       return "CRITICAL_ERR\n";
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
