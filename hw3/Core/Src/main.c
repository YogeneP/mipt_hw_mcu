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
#define BAUD_RATE 115200ULL
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
char* uint32_to_str(uint32_t val, char* buf);
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
  /* USER CODE BEGIN 2 */
  BM_UART_Init(BAUD_RATE); // Custom bare-metal init, no HAL USART libraries needed, disabled in CubeMX 
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t led_tick = HAL_GetTick();
  uint32_t led_prev_tick = HAL_GetTick();

  char str[256] = {0};
  uint8_t str_len = 0;
  char inttostrbuf[11] = {0};
  char* baudrate_str = uint32_to_str(BAUD_RATE, inttostrbuf);
  str_len = strlen(baudrate_str);
  memcpy(str, "Serial output initiated: ", 25);
  memcpy(str+25, baudrate_str, str_len);
  memcpy(str+25+strlen(baudrate_str)," 8-N-1 \r\n", 9);
  str_len += (25 + 9);

  uint32_t err_count = 0;
  uint32_t res = 0;
  char* err_count_str = NULL;
  BM_UART_Transmit(str, str_len);
  memset(str,0,sizeof(str));
  str_len = 77;
  memcpy(str,"Overflow demo: sending next string while the previous one is on the way...\r\n\0", str_len);
  while(1)
  { 
    /* STAGE1 - tx overflow*/   
    if (err_count_str == NULL) { //using as flag to miss STAGE1
      res = BM_UART_Transmit(str, str_len); 
      if (res) {
        err_count++; // failed sending attempts
        HAL_Delay(1);
        continue; // repeat STAGE1 until the message is successfuly sent
      } 
    /* STAGE2 - report */
      LED_GPIO_Port->BSRR = LED_Pin << 16U;
      // Assembling report, just once (err_count_str != 0 anymore)
      err_count_str = uint32_to_str(err_count, inttostrbuf); 
      memset(str,0,sizeof(str));
      memcpy(str, "TX attempts failed: ", 20);
      str_len = inttostrbuf + sizeof(inttostrbuf) - err_count_str;
      memcpy(str+20, err_count_str, str_len);
      memcpy(str+20+str_len, "\r\n\0", 3);
      str_len += (20+3);
    }
    if (err_count) {
      res = BM_UART_Transmit(str, str_len);
      if(res) {
        continue;
      }
      err_count = 0;
    } 
    /* STAGE3 - reception */
    if (BM_UART_rx_res != 0 ) {
      if (BM_UART_rx_res > 0) {
        str_len = BM_UART_rx_res;
        memcpy(str, BM_UART_rx_buf, str_len);
        err_count = 0;
        err_count_str = NULL;      //enable string back send at stage 1
      }
      BM_UART_Receive(BM_UART_rx_buf, 256);
    }
    /* Blinker */
    led_tick = HAL_GetTick();
    if((led_tick - led_prev_tick) >= 100) {
    uint32_t led_state = LED_GPIO_Port->ODR;
    LED_GPIO_Port->BSRR = ((led_state & LED_Pin) << 16U) | (~led_state & LED_Pin);
      led_prev_tick = led_tick;
    }
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
// Convert a uint32_t into a null-terminated string array
char* uint32_to_str(uint32_t val, char* buf) {
    int i = 10;
    buf[i] = '\0'; // Put the string terminator at the very end of the array
    
    // Handle the specific case where the value is exactly 0
    if (val == 0) {
        buf[--i] = '0';
    } else {
        // Extract digits one by one using remainder math
        while (val > 0 && i > 0) {
            buf[--i] = '0' + (val % 10);
            val /= 10;
        }
    }
    return &buf[i]; // Return the pointer to where the number actually begins
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
