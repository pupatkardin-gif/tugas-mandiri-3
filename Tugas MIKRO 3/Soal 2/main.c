
#define STM32F411xE
#include "stm32f4xx.h"

/* Pin configuration */
#define BUTTON_PIN (1U << 0)   // PA0
#define LED_PIN    (1U << 13)  // PC13

/* Semaphore flag */
volatile uint8_t button_semaphore = 0;

/* GPIO and EXTI initialization */
void GPIO_EXTI_Init(void)
{
    /* 1. Enable GPIOA, GPIOC and SYSCFG clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOCEN;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* 2. Configure PA0 as input */
    GPIOA->MODER &= ~(3U << 0);

    /* Enable pull-up on PA0 */
    GPIOA->PUPDR &= ~(3U << 0);
    GPIOA->PUPDR |=  (1U << 0);

    /* 3. Configure PC13 as output */
    GPIOC->MODER &= ~(3U << (13 * 2));
    GPIOC->MODER |=  (1U << (13 * 2));

    GPIOC->OTYPER &= ~LED_PIN;

    /* LED initially OFF */
    GPIOC->ODR |= LED_PIN;

    /* 4. Map EXTI0 to GPIOA */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    /* 5. Configure EXTI0 */
    EXTI->IMR &= ~(1U << 0);
    EXTI->EMR &= ~(1U << 0);

    /* Falling edge trigger */
    EXTI->FTSR |= (1U << 0);
    EXTI->RTSR &= ~(1U << 0);

    /* Clear pending interrupt */
    EXTI->PR = (1U << 0);

    /* 6. Enable EXTI0 interrupt in NVIC */
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* TIM5 debounce initialization */
void TIM5_Init(void)
{
    /* Enable TIM5 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    /* Stop timer */
    TIM5->CR1 = 0;

    /* Configure 20 ms debounce */
    TIM5->PSC = 15999U;
    TIM5->ARR = 19U;

    /* Reset counter */
    TIM5->CNT = 0;

    /* Generate update event to load PSC */
    TIM5->EGR = TIM_EGR_UG;
    TIM5->SR = 0;

    /* Enable update interrupt */
    TIM5->DIER |= TIM_DIER_UIE;

    /* One-pulse mode */
    TIM5->CR1 |= TIM_CR1_OPM;

    /* Enable TIM5 interrupt in NVIC */
    NVIC_EnableIRQ(TIM5_IRQn);
}

/* EXTI0 Interrupt Handler */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1U << 0))
    {
        /* Disable EXTI while debouncing */
        EXTI->IMR &= ~(1U << 0);

        /* Clear EXTI pending flag */
        EXTI->PR = (1U << 0);

        /* Reset TIM5 */
        TIM5->CR1 &= ~TIM_CR1_CEN;
        TIM5->CNT = 0;
        TIM5->SR = 0;

        /* Load timer configuration */
        TIM5->EGR = TIM_EGR_UG;
        TIM5->SR = 0;

        /* Start one-shot debounce timer */
        TIM5->CR1 |= TIM_CR1_OPM | TIM_CR1_CEN;
    }
}

/* TIM5 Interrupt Handler */
void TIM5_IRQHandler(void)
{
    if (TIM5->SR & TIM_SR_UIF)
    {
        /* Clear update interrupt flag */
        TIM5->SR &= ~TIM_SR_UIF;

        /* Check whether button is still pressed */
        if (!(GPIOA->IDR & BUTTON_PIN))
        {
            button_semaphore = 1U;
        }

        /* Clear EXTI pending flag */
        EXTI->PR = (1U << 0);

        /* Re-enable EXTI0 */
        EXTI->IMR |= (1U << 0);
    }
}

int main(void)
{
    /* Initialize GPIO and EXTI */
    GPIO_EXTI_Init();

    /* Initialize debounce timer */
    TIM5_Init();

    /* Enable EXTI0 */
    EXTI->IMR |= (1U << 0);

    while (1)
    {
        /* Check semaphore */
        if (button_semaphore)
        {
            /* Safely consume semaphore */
            __disable_irq();
            button_semaphore = 0U;
            __enable_irq();

            /* Process valid button press */
            GPIOC->ODR ^= LED_PIN;
        }
    }
}