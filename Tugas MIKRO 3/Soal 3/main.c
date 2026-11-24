#define STM32F411xE
#include "stm32f4xx.h"

#define BUTTON_PIN (1U << 0)   // PA0
#define LED_PIN    (1U << 13)  // PC13

void GPIO_Init(void)
{
    // Enable GPIOA and GPIOC clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOCEN;

    // PA0 as input
    GPIOA->MODER &= ~(3U << (0 * 2));

    // PA0 pull-up
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |=  (1U << (0 * 2));

    // PC13 as output
    GPIOC->MODER &= ~(3U << (13 * 2));
    GPIOC->MODER |=  (1U << (13 * 2));

    // Push-pull output
    GPIOC->OTYPER &= ~LED_PIN;

    // LED initially OFF (active-low LED)
    GPIOC->ODR |= LED_PIN;
}

void TIM5_Init(void)
{
    // Enable TIM5 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    // Stop timer during configuration
    TIM5->CR1 = 0;

    // Timer configuration
    TIM5->PSC = 15999U;
    TIM5->ARR = 499U;
    TIM5->CNT = 0;

    // Generate update event
    TIM5->EGR = TIM_EGR_UG;

    // Clear update flag
    TIM5->SR = 0;

    // Enable update interrupt
    TIM5->DIER |= TIM_DIER_UIE;

    // Enable TIM5 interrupt in NVIC
    NVIC_EnableIRQ(TIM5_IRQn);

    // Start timer
    TIM5->CR1 |= TIM_CR1_CEN;
}

void TIM5_IRQHandler(void)
{
    if (TIM5->SR & TIM_SR_UIF)
    {
        // Clear interrupt flag
        TIM5->SR &= ~TIM_SR_UIF;

        // Check whether button is pressed
        if (!(GPIOA->IDR & BUTTON_PIN))
        {
            // Toggle LED
            GPIOC->ODR ^= LED_PIN;
        }
    }
}

int main(void)
{
    GPIO_Init();
    TIM5_Init();

    while (1)
    {
        // Wait for interrupt
        __WFI();
    }
}