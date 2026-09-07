#ifndef _LED_H
#define _LED_H

#include <inttypes.h>
#include <stm32f411xe.h>

enum ONOFF_enum { OFF, ON };
typedef uint8_t ONOFF_t;

typedef struct LED_t {
  GPIO_TypeDef* GPIO;
  uint16_t pin_mask;
  uint16_t blink;
  uint32_t last_tick;
  ONOFF_t state; 
  ONOFF_t enable; 
} LED_t;

void LED_Init(LED_t*, GPIO_TypeDef*, uint16_t);
void LED_Handler(LED_t*,uint16_t);

#endif