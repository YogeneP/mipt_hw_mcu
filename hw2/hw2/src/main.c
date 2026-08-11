#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim1;

void SystemClock_Config(void);

void main() {
    //breaking down HAL_Init():
    //enabling prefetch, data cache and instuction cache - F4xx supports them
    SET_BIT(FLASH->ACR, FLASH_ACR_ICEN|FLASH_ACR_DCEN|FLASH_ACR_ICEN);

    // set NVIC to no subpriorities mode, HAL-default (all 4 available bits are used for priorities)
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); 

    //Initialization of Tick interrupt, 1 ms
    


    HAL_Init();

    SystemClock_Config();

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);


}