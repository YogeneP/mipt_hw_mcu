#include "stm32f4xx_hal.h"

// System Clock Configuration Prototype
void SystemClock_Config(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();

    // Enable GPIOC clock for the onboard LED (PC13)
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500); // Blink every 500ms
    }
}

// System tick handler required for HAL_Delay()
void SysTick_Handler(void) {
    HAL_IncTick();
}

// Simple clock configuration helper
void SystemClock_Config(void) {
    // Default system configuration fallback
}