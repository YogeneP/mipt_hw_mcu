#include "bm_stm32f4xx_uart.h"

int32_t BM_UART_rx_res = -1;
char BM_UART_rx_buf[256] = {0};

/*
* Baudrates 2400...230400 tested
*/
uint32_t BM_UART_Init(uint64_t baud_rate) {
  //Enable clock for USART and GPIO 
  SET_BIT(RCC->_CONCAT2(BM_UART_BUS,ENR), _CONCAT5(RCC_,BM_UART_BUS,ENR_USART,BM_UART_N,EN));
  SET_BIT(RCC->_CONCAT2(BM_UART_GPIO_BUS,ENR), _CONCAT5(RCC_,BM_UART_GPIO_BUS,ENR_GPIO,BM_UART_GPIO_PORT,EN));
  
  BM_UART_INST->CR1 = (uint32_t)0x00; // stop and clear

  // GPIO setup
  CLEAR_BIT(BM_UART_GPIO->MODER, (3U << (BM_UART_TX_PIN_N * 2)) | (3U << (BM_UART_RX_PIN_N * 2)));
  // GPIO MODER TX/RX pins alternate function 
  SET_BIT(BM_UART_GPIO->MODER, (2U << (BM_UART_TX_PIN_N * 2)) | (2U << (BM_UART_RX_PIN_N * 2)));
  // GPIO OTYPER TX/RX pins - reset state = 0 PushPull - OK
  // GPIO OSPEEDR TX/RX pins - reset state = 00 Low speed - OK for up to 230400
  // SET_BIT(BM_UART_GPIO->OSPEEDR, (3U << (BM_UART_TX_PIN_N * 2)) | (3U << (BM_UART_RX_PIN_N * 2)));
  // GPIO PUPDR TX/RX pins - reset state 00 - OK
  SET_BIT(BM_UART_GPIO->AFR[BM_UART_TX_PIN_N < 8 ? 0 : 1], BM_UART_PINS_AF << ((BM_UART_TX_PIN_N - (BM_UART_TX_PIN_N < 8 ? 0 : 8U)) * 4)); //AF7 - USART1 TX/RX for PA9/10
  SET_BIT(BM_UART_GPIO->AFR[BM_UART_RX_PIN_N < 8 ? 0 : 1], BM_UART_PINS_AF << ((BM_UART_RX_PIN_N - (BM_UART_RX_PIN_N < 8 ? 0 : 8U)) * 4)); //AF7 - USART1 TX/RX for PA9/10
  
  // E.G. setting the baud rate to 115200
  // USARTDIV = fck/(8 * OVER8)/BR = 100000000/16/115200 = 54.25347(2)
  // DIV_fraction = 0.25347(2) * 16 = 4.0(5) -> 0x04
  // DIV_mantissa = 54 = 0x36

 // BM_UART_INST->BRR = ((uint64_t)BM_UART_BUS_CLOCK + ((uint64_t)baud_rate / 2ULL)) / baud_rate;

  /* This works as well - full math:*/
  uint64_t usartdiv100 = 100 * BM_UART_BUS_CLOCK / 16 / baud_rate;
  uint32_t div_mant = usartdiv100 / 100;
  uint32_t div_fr = ((usartdiv100 % 100 * 16) / 100) & 0x0FU; 
  BM_UART_INST->BRR = (div_mant << 4) | div_fr;
  
  // All the bits 0 state good for me: USART_CR1_M, USART_CR1_PCE, 
  // USART_CR1_PS, USART_CR1_OVER8, USART_CR2_STOP, just enable 
  BM_UART_INST->CR1 = (uint32_t)(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE); 

  // Allow USART IRQ interrupt to reach CPU through NVIC gate
  NVIC_SetPriority(BM_UART_IRQ_N, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), BM_UART_NVIC_PRIORITY, 0));
  NVIC_EnableIRQ(BM_UART_IRQ_N);//_CONCAT3(USART,BM_UART_N,_IRQn));

  return 0;
}

/*  
*   Initiates and proceeds the package transfer in cooperation with USART1_IRQHandler().
*   Non-blocking, overflow protected
*
*   *txdata - a pointer to the buffer of chars start address; 
*            zero (NULL) expected in the case of repetetive call by ISR while buffer is being sending
*   txdata_len - buffer length to send; 
*               reset running transfer if txdata_len == 0;  
*               any non-zero value expected to keep the buffer sending (repetetive call) with *txdata == NULL
*/
uint32_t BM_UART_Transmit(char *txdata, size_t txdata_len)
{
  static char *tx_pos = 0; //encapsulated sending parameters
  static size_t tx_rem = 0;
  static char tx_buf[256];

  if (txdata && txdata_len) { // init condition
    if (tx_rem == 0 && (BM_UART_INST->SR & USART_SR_TC)) { // previous package sent completely 
      memcpy(tx_buf, txdata, txdata_len);
      tx_pos = tx_buf;
      tx_rem = (txdata_len > 256) ? 256 : txdata_len;
    } else { 
      return 1; // busy, transmission rejected
    }
  }

  if (txdata_len == 0) { // reset condition
    tx_rem = 0;
    tx_pos = NULL;
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_TXEIE); // no need, but just to be sure
    return 0;
  }

  if (tx_rem) {
    BM_UART_INST->DR = (uint8_t)(*tx_pos) & (uint8_t)0x00FF;
    tx_rem--;
    tx_pos++;
  }

  if (tx_rem) {    
    SET_BIT(BM_UART_INST->CR1, USART_CR1_TXEIE); 
  } else {
    tx_pos = NULL; // no need, but just to be sure
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_TXEIE); // no need, but just to be sure
  }
  return 0;
}

int32_t BM_UART_Receive(char* rxbuf, size_t rxbuf_len) {
  static char* rx_pos = NULL;
  static size_t rx_buf_cap = 0;
  static size_t rx_bytes = 0;
  static char eop[] = EOP;
  static size_t eop_len = sizeof(eop)-1;
  static size_t eop_prearm = 0;

  if (rxbuf && rxbuf_len) { // init condition
    if (rx_pos == NULL) { // no listening initiated 
      rx_pos = rxbuf;
      rx_buf_cap = rxbuf_len;
      rx_bytes = 0;
      eop_prearm = 0;
    } else { 
      BM_UART_rx_res = 0;
      return 0; // listenig or receiving, new reception rejected
    }
  }

  if (rxbuf_len == 0) { // reset condition
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_RXNEIE);
    rx_pos = NULL;
    rx_buf_cap = 0;
    BM_UART_rx_res = -1; //stopped
    return -1;
  }

  if (BM_UART_INST->SR & USART_SR_RXNE) {
    *rx_pos = (uint8_t)BM_UART_INST->DR & 0xFFU;
    if (*rx_pos == eop[eop_prearm]) {
      eop_prearm++;
      if ( eop_prearm >= eop_len ) {
        CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_RXNEIE); // no need, but just to be sure
        BM_UART_rx_res = rx_bytes + 1 - eop_len;
        rx_pos = NULL;
        rx_bytes = 0;
        rx_buf_cap = 0;
        eop_prearm = 0;
        return BM_UART_rx_res; // package received, length returned
      } 
    } else {
      eop_prearm = 0;
      if (*rx_pos == eop[eop_prearm]) {
        eop_prearm++;
      }
    }  
    rx_bytes++;
    rx_pos++;
  }

  if (rx_bytes <= rx_buf_cap) {  
    SET_BIT(BM_UART_INST->CR1, USART_CR1_RXNEIE); 
  } else {
    rx_pos = NULL; // no need, but just to be sure
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_RXNEIE); // no need, but just to be sure
    BM_UART_rx_res = -2;
    return -2; // buffer overflow
  }
  BM_UART_rx_res = 0;
  return 0;
}

void _CONCAT3(USART, BM_UART_N,_IRQHandler)(void)
{
  if(BM_UART_INST->SR & USART_SR_TXE) {
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_TXEIE);
    BM_UART_Transmit(NULL,1); //repetative call: buf address doesn't matter; len - non zero
  }
  if(BM_UART_INST->SR & USART_SR_RXNE) {
    CLEAR_BIT(BM_UART_INST->CR1, USART_CR1_RXNEIE);
    BM_UART_Receive(NULL,1); //repetative call: buf address doesn't matter; len - non zero
  }
}