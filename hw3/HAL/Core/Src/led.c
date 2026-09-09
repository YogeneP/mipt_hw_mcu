#include <led.h>

void LED_Init(LED_t* led, GPIO_TypeDef* gpio_port, uint16_t pin_mask) {
    led->GPIO = gpio_port;
    led->pin_mask = pin_mask;
    led->blink = 0;
    led->last_tick = 0;
    led->state = OFF;
    led->enable = OFF;
}

void LED_Handler(LED_t *led,uint16_t tick) {
  if(led->enable) {
    if(led->blink) {
      if((tick - led->last_tick) >= led->blink) {
        led->state = !led->state;
        led->last_tick = tick;
      }
    } else { 
      led->state = ON;  
    }
  } else {
    led->state = OFF;
  }
  (led->GPIO)->BSRR = ((uint32_t)(led->pin_mask) << ((led->state) * 16U));
}