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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "bm_stm32f4xx_uart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define COMMAND_MAX_LENGTH 7 // pure chars count, no '\0' considerd
#define COMMANDS_LIST_LENGTH 5
#define TRANSMISSION_OVER_DELAY 10
#define HANDSHAKE_MESS "HELLO"
#define HANDSHAKE_I 0
#define AYS_MESS "AYS"          // At Your Service
#define AYS_I 1
#define LED_ON_MESS "LED_ON"
#define LED_ON_I 2
#define LED_OFF_MESS "LED_OFF"
#define LED_OFF_I 3
#define LED_TG_MESS "LED_TG"
#define LED_TG_I 4
#define RX_BUFFER_LENGTH 64

#define BUTTON_DEBOUNCE_TIME 10

#define LED_ON  LED_GPIO_Port->BSRR = (LED_Pin << 16U);
#define LED_OFF LED_GPIO_Port->BSRR = LED_Pin
#define LED_TOGGLE LED_GPIO_Port->BSRR = LED_Pin << ((LED_GPIO_Port->ODR & LED_Pin) ? 16U : 0U) 
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
const char* commands_list[COMMANDS_LIST_LENGTH] = {
  HANDSHAKE_MESS,
  AYS_MESS, 
  LED_ON_MESS,
  LED_OFF_MESS,
  LED_TG_MESS
};
char rx_buf[2][RX_BUFFER_LENGTH] = {{ 0 } , { 0 }}; // Flip-flop buffer
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
int checkCommand(const char* comms_list[], uint8_t comm_list_length, char* rx_str, size_t rx_str_len );
void blink(uint8_t count, uint32_t tick);
/* USER CODE BEGIN PFP */

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
  /* USER CODE BEGIN 2 */
  BM_UART_Init(115200);
  int command_index = UINT8_MAX;
  unsigned int buf_i = 0;
  int ays_received = 0; 
  int handshake_received = 0;
  LED_ON;

  BM_UART_Receive(rx_buf[buf_i], RX_BUFFER_LENGTH);
  do {
    while (BM_UART_rx_res <= 0) {
      BM_UART_Transmit((char*)commands_list[HANDSHAKE_I], strlen(commands_list[HANDSHAKE_MESS]));
      HAL_Delay(500);
    }
    buf_i = !buf_i;
    BM_UART_Receive(rx_buf[buf_i], RX_BUFFER_LENGTH);
    do {
      command_index = checkCommand(commands_list, COMMANDS_LIST_LENGTH, rx_buf[!buf_i], BM_UART_rx_res); 
      if (command_index == HANDSHAKE_I) break; //HANDSHAKE
    } while (command_index < 0);
    if (command_index == HANDSHAKE_I) {
      handshake_received = 1;
      break; //HANDSHAKE
  } while (1)
  
  blink(6, HAL_GetTick());

  BM_UART_Transmit((char*)commands_list[AYS_I], strlen(commands_list[AYS_MESS]));
  HAL_Delay(2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  // ***** MIRROR THE ARDUINO STATE MACHINE *****
  while (1) 
  {
    if (BM_UART_rx_res > 0) {
      buf_i = !buf_i;
      BM_UART_Receive(rx_buf[buf_i], RX_BUFFER_LENGTH);
      do {
        command_index = checkCommand(commands_list, COMMANDS_LIST_LENGTH, rx_buf[!buf_i], BM_UART_rx_res); 
        switch (command_index) {
          case HANDSHAKE_I:
            blink(6, HAL_GetTick());
            ays_received = 0;
            BM_UART_Transmit((char*)commands_list[HANDSHAKE_I], strlen(commands_list[HANDSHAKE_MESS]));
            HAL_Delay(2);
            BM_UART_Transmit((char*)commands_list[AYS_I], strlen(commands_list[AYS_MESS]));
            HAL_Delay(2);
            break;
          case LED_ON_I:
            blink(1, HAL_GetTick());
            LED_ON;
            break;
          case LED_OFF_I:
            blink(2, HAL_GetTick());
            LED_OFF;
            break;
          case LED_TG_I:
            blink(3, HAL_GetTick());
            LED_TG_I;
            break;
          case AYS_I:
            blink(4, HAL_GetTick());
            if(ays_received && handshake_received) {
                handshake_received = 0;
                ays_received = 0;
                BM_UART_Transmit((char*)commands_list[HANDSHAKE_I], strlen(commands_list[HANDSHAKE_MESS]));
                HAL_Delay(2);
            }
            break;
          default:
            blink(1, HAL_GetTick());
        } 
      } while (command_index < 0);
    }
    if(ays_received && buttonHandler(HAL_GetTick())) {

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int checkCommand(const char* comms_list[], uint8_t comm_list_length, char* rx_str, size_t rx_str_len ) {
  static char buf[COMMAND_MAX_LENGTH + 1] = { 0 };
  static uint8_t char_pos = 0;
  static bool overflow = false;
  size_t rx_str_pos = 0;
  char c = 0;

  while((rx_str_pos < rx_str_len) {
    c = *(rx_str + rx_str_pos);
    rx_str_pos++;
    if (c == '\r' || c == '\n' || c == '\0' || c == ' ') {
      overflow = false;
      if(buf[0]) {
        buf[char_pos] = '\0'; 
        for (int i = 0; i < comm_list_length; i++) {
          if (strcmp((const char*)buf, comms_list[i]) == 0) {
            memset(rx_str, 0, rx_str_pos-1);
            char_pos = 0;
            memset(buf, 0, sizeof(buf));  
            return i;
          }
        }  
        char_pos = 0;
        memset(buf, 0, sizeof(buf));
      }
      continue;
    }
    if(overflow) continue;
    if(char_pos > COMMAND_MAX_LENGTH - 1) {
      char_pos = 0;
      memset(buf, 0, sizeof(buf));
      overflow = true;     
    } else {
      buf[char_pos] = c;
      char_pos++;
    }
  }
  return -1;
}

void blink(uint8_t count, uint32_t tick) {
  static uint32_t prev_tick = 0;
  static uint8_t blinks_left = 0;
  if (count > 0) {
    blinks_left = blinks_left + count * 2;
    prev_tick = 0; //first immediate toggle
  }
  if (blinks_left && (tick - prev_tick) > 30) {
    LED_TOGGLE;
    prev_tick = tick;
    blinks_left--;
  } 
}

// throws 1 on the first call after button push (state rise detection)
uint8_t buttonHandler(uint32_t tick) { 
  static uint8_t prev_state = 0;
  static uint32_t prev_tick = 0;
  uint8_t state = 0; 

  // BTN is tied up to HIGH by default
  uint8_t curr_state = (uint8_t)(!((BTN_GPIO_Port->IDR) & BTN_Pin));
  
  if(tick - prev_tick > BTN_DEBOUNCE_PERIOD) {
    prev_tick = tick;
    if(curr_state && !prev_state) {
      state = 1; 
    }
    prev_state = curr_state;
  }
  return state;
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
