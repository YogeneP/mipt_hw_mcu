/*
Leightweight single UART implementation for STM32F4xx family 
DO NOT enable the USART instance in Cube MX, no HAL features used
Port configuration:
- word length 8 bit
- no parity check
- 1 stop bit
*/

#ifndef _BM_STM32F4XX_UART_H
#define _BM_STM32F4XX_UART_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "stm32f4xx.h"

/* Exact MCU model, the full list - stm32f4xx.h */
#ifndef STM32F411xE
#define STM32F411xE
#endif // #ifndef STM32F411xE

/* End Of Package reception mark */
#define EOP "EOP"

/* USART instance configuration */
#define BM_UART_N 1 // e.g. 1 means USART1 as CMSIS reference  
#define BM_UART_BUS APB2 // Refer to to MCU datasheet "USART feature comparison"
#define BM_UART_GPIO_PORT A // Refer to to MCU datasheet "Pin definitions"
#define BM_UART_GPIO_BUS AHB1 // Refer to to MCU datasheet "Block diagram", "Register boundary addresses" or "Peripheral current consumption" 
#define BM_UART_TX_PIN_N 9 // Refer to to MCU datasheet "Pin definitions"
#define BM_UART_RX_PIN_N 10 // Refer to to MCU datasheet "Pin definitions"
#define BM_UART_PINS_AF 7U // Refer to MCU datasheet "Alternate function mapping"
#define BM_UART_BUS_CLOCK 100000000ULL // Refer to RCC setup
#define BM_UART_NVIC_PRIORITY 10 // 0...15 by default, less number - higher priority

#define BM_UART_INST _CONCAT2(USART, BM_UART_N)
#define BM_UART_GPIO _CONCAT2(GPIO, BM_UART_GPIO_PORT)
#define BM_UART_TX_PIN _CONCAT2(BM_UART_GPIO, BM_UART_TX_PIN_N)
#define BM_UART_RX_PIN _CONCAT2(BM_UART_GPIO, BM_UART_RX_PIN_N)
#define BM_UART_IRQ_N _CONCAT3(USART,BM_UART_N,_IRQn) 

/* CMSIS references composing helpers */
#define _UNEXP(a) a
#define _JOIN2(a,b) a##b
#define _CONCAT2(a,b) _JOIN2(a,b)
#define _JOIN3(a,b,c) a##b##c
#define _CONCAT3(a,b,c) _JOIN3(a,b,c)
#define _JOIN4(a,b,c,d) a##b##c##d
#define _CONCAT4(a,b,c,d) _JOIN4(a,b,c,d)
#define _JOIN5(a,b,c,d,e) a##b##c##d##e
#define _CONCAT5(a,b,c,d,e) _JOIN5(a,b,c,d,e)

extern int32_t BM_UART_rx_res;
extern char BM_UART_rx_buf[256];

uint32_t BM_UART_Init(uint64_t baud_rate);
uint32_t BM_UART_Transmit(char *txbuf, size_t txbuf_len);
int32_t BM_UART_Receive(char* rxbuf, size_t rxbuf_len);

#endif //#define __BM_STM32F4XX_UART_H