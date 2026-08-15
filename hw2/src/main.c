/*Main goal - to avoid use of HAL and disable any unnecessary periphery*/

#include "stm32f4xx.h"

volatile uint8_t led_flag = 0; // tick interrupt triggered - need to toggle

void My_SysTick_Handler(void);

int main(void) {
    //breaking down HAL_Init():
    //enabling prefetch, data cache and instuction cache - F4xx supports them
    SET_BIT(FLASH->ACR, FLASH_ACR_ICEN|FLASH_ACR_DCEN|FLASH_ACR_ICEN);

    // set NVIC to no subpriorities mode, HAL-default (all 4 available bits are used for priorities)
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); 

    //Initialization of Tick interrupt, 0.5 s - to blink LED
    //While core runs at 16MHz HSI -> set SysTicks every 8 000 000 - 1 cycles 
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
    //uwTick doesn't increase anymore - HAL_Delay, SPI, I2C and the rest of uwTick dependent features get freezed

    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
     
    //*** Enable GPIOC
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
    CLEAR_BIT(GPIOC->MODER, 1UL << (2*13 + 1));
    SET_BIT(GPIOC->MODER, 1UL << (2*13));

    GPIOC->BSRR = GPIO_PIN_13;
    
    // Run SysTick
    SET_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);

    while(1) {
        //nothing to do -> sleep
        __WFI();
    }

}

void My_SysTick_Handler(void) {
    led_flag =! led_flag;
    GPIOC->BSRR = GPIO_PIN_13 << (16*led_flag); 
}