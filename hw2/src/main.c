/*Main goal - to avoid use of HAL*/

#include "stm32f4xx.h"

volatile uint8_t led_flag = 0; // tick interrupt triggered - need to toggle

void My_SysTick_Handler(void);

void main() {
    //breaking down HAL_Init():
    //enabling prefetch, data cache and instuction cache - F4xx supports them
    SET_BIT(FLASH->ACR, FLASH_ACR_ICEN|FLASH_ACR_DCEN|FLASH_ACR_ICEN);

    // set NVIC to no subpriorities mode, HAL-default (all 4 available bits are used for priorities)
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); 

    //Initialization of Tick interrupt, 0.5 s - to blink LED
    //While core runs at 16MHz HSI -> assume SysTicks every 8 000 000 - 1 cycles 
    //code gets clock frequency dependent: higher frequency - faster blinking!!! Poor in production development.
    //Actually it's not good to get SysTick fit to blink LED, but fun...  
    SysTick->LOAD  = (uint32_t)(8000000 - 1UL);

    SysTick->VAL   = 0UL; // reset SysTick

    //set SysTick cycle equal to AHB clock cycle; 
    //in the case of higher clock frequency it may happen that 24 bits of SysTick->LOAD may be not enough to set 0.5s interval
    //then -> CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk); and the SysTick frequency = AHB/8, e.g. 16MHz for 96MHz clock  
    SET_BIT(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk);
    
    //enable SysTick interrupt, it will call SysTick_Handler() on firing up originally. 
    //...or something else defined in startup_stm32f411xe.s with index -1 (just before External Interrupts section)
    // My_SysTick_Handler() in my case 
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
     
    //*** keep going
    HAL_RCC_ClockConfig(); 

    SystemClock_Config();

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    
    // Run SysTick
    SET_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);

    while(1) {
        if(led_flag) {
            GPIOC->BSRR = (GPIOC->ODR & GPIO_PIN_13) ?  GPIO_PIN_13 << 16 : GPIO_PIN_13; 
            led_flag = 0;
        }
    }

}

void My_SysTick_Handler(void) {
    led_flag = 1;
}