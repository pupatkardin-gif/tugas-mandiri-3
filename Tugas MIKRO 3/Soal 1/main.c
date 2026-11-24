#define STM32F411xE
#include "stm32f4xx.h"

/* LED pin configuration */
#define LED1_PIN (1U << 5)   // PA5 - TIM2
#define LED2_PIN (1U << 0)   // PB0 - TIM3
#define LED3_PIN (1U << 1)   // PB1 - TIM4

/* Prescaler: 16 MHz / 16000 = 1 kHz counter clock */
#define TIMER_PSC 15999U

/* Timer initialization */
void Timer_Init(TIM_TypeDef *TIMx, uint32_t arr)
{
    TIMx->CR1 = 0;           // Stop timer
    TIMx->PSC = TIMER_PSC;   // Set prescaler
    TIMx->ARR = arr;         // Set auto-reload
    TIMx->CNT = 0;           // Reset counter

    TIMx->EGR = TIM_EGR_UG;  // Generate update event
    TIMx->SR &= ~TIM_SR_UIF; // Clear update flag

    TIMx->DIER |= TIM_DIER_UIE; // Enable update interrupt
    TIMx->CR1 |= TIM_CR1_CEN;   // Start timer
}

/* TIM2 Interrupt - LED 1 (1 Hz) */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;
        GPIOA->ODR ^= LED1_PIN;
    }
}

/* TIM3 Interrupt - LED 2 (2 Hz) */
void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &= ~TIM_SR_UIF;
        GPIOB->ODR ^= LED2_PIN;
    }
}

/* TIM4 Interrupt - LED 3 (4 Hz) */
void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF)
    {
        TIM4->SR &= ~TIM_SR_UIF;
        GPIOB->ODR ^= LED3_PIN;
    }
}

int main(void)
{
    /* 1. Enable GPIOA and GPIOB clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN;

    /* 2. Configure PA5 as output */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    /* 3. Configure PB0 and PB1 as output */
    GPIOB->MODER &= ~((3U << 0) | (3U << 2));
    GPIOB->MODER |=  ((1U << 0) | (1U << 2));

    /* Configure output type: Push-Pull */
    GPIOA->OTYPER &= ~LED1_PIN;
    GPIOB->OTYPER &= ~(LED2_PIN | LED3_PIN);

    /* Configure low speed */
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));
    GPIOB->OSPEEDR &= ~((3U << 0) | (3U << 2));

    /* Disable pull-up and pull-down */
    GPIOA->PUPDR &= ~(3U << (5 * 2));
    GPIOB->PUPDR &= ~((3U << 0) | (3U << 2));

    /* Initialize LEDs OFF */
    GPIOA->ODR &= ~LED1_PIN;
    GPIOB->ODR &= ~(LED2_PIN | LED3_PIN);

    /* 4. Enable TIM2, TIM3, TIM4 clocks */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN |
                    RCC_APB1ENR_TIM3EN |
                    RCC_APB1ENR_TIM4EN;

    /* 5. Enable timer interrupts in NVIC */
    NVIC->ISER[0] = (1U << TIM2_IRQn) |
                    (1U << TIM3_IRQn) |
                    (1U << TIM4_IRQn);

    /* 6. Initialize each timer */
    Timer_Init(TIM2, 499U);  // LED 1: 1 Hz
    Timer_Init(TIM3, 249U);  // LED 2: 2 Hz
    Timer_Init(TIM4, 124U);  // LED 3: 4 Hz

    /* 7. Main loop */
    while (1)
    {
        /* Timer interrupts operate independently */
        __WFI();
    }
}