#include "main.h"
#include "stm32f411xe.h"
#include "stm32f4xx.h"

volatile uint32_t toggle = 0;

int main(void)
{
  // MCU runs in the default reset state mode (HSI, 16MHz), nothing to do with it. 
  // AHB1 has a clock enabled by default

  // GPIO ports enable and config
  SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
  CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MODE0_Msk);
  CLEAR_BIT(GPIOA->PUPDR,GPIO_PUPDR_PUPD0_Msk);
  
  SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
  SET_BIT(GPIOC->PUPDR,GPIO_PUPDR_PUPDR0_0);
  CLEAR_BIT(GPIOC->MODER, GPIO_MODER_MODE13_Msk);
  SET_BIT(GPIOC->MODER, GPIO_MODER_MODE13_0);
  CLEAR_BIT(GPIOC->OTYPER, GPIO_OTYPER_OT13);
  CLEAR_BIT(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED13_Msk);

  //EXTI configuration
  CLEAR_BIT(SYSCFG->EXTICR[0],0U); //PA[0...3] to EXTI0 line
  CLEAR_BIT(EXTI->RTSR, EXTI_RTSR_TR0); //EXTI0 Rise trigger disable
  SET_BIT(EXTI->FTSR, EXTI_FTSR_TR0); //EXTI0 Fall trigger enable
  SET_BIT(EXTI->IMR, EXTI_IMR_IM0); //Mask interrupt on line 0

  //Cortex M4 NVIC configuration
  NVIC_SetPriority(EXTI0_IRQn, 10);
  NVIC_EnableIRQ(EXTI0_IRQn);
  
  while (1)
  {
    __WFI();
  }
}

void EXTI0_IRQHandler(void) {
  if(EXTI->PR == EXTI_PR_PR0) {
      GPIOC->BSRR = (GPIOC->ODR == GPIO_ODR_ODR_13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
  }
  SET_BIT(EXTI->PR, EXTI_PR_PR0); //Pending Register resets by writing '1' to it
}

void SystemInit(void) {
  //dummy, not to modify .s file referring to it
}