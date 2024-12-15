#include "main.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim2;

// Глобальні змінні
volatile uint8_t brightness = 50;  // Актуальна яскравість (50%)
volatile uint8_t ledState = 1;     // Стан світлодіода (1 - увімкнено, 0 - вимкнено)

// Прототипи функцій
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_TIM2_Init(void);
void Error_Handler(void);

// Обробка переривання від кнопки B1
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) { // Якщо переривання від B1

        ledState = !ledState; // Змінюємо стан світлодіода
        if (ledState) {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness * 10); // Відновлення яскравості
        } else {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // Гасимо світлодіод
        }
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();

    // Запуск PWM на TIM2, канал 1
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness * 10); // Встановлюємо початкову яскравість

    uint8_t buffer[100]; // Буфер для отримання команд
    uint8_t response[100]; // Буфер для відповіді
    uint16_t index = 0;

    while (1) {
        uint8_t data;
        // Прийом символів через UART
        if (HAL_UART_Receive(&huart2, &data, 1, HAL_MAX_DELAY) == HAL_OK) {
            if (data == '\n' || data == '\r') {
                buffer[index] = '\0';
                if (index > 0) {
                    if (tolower(buffer[0]) == 'l' && buffer[1] == '=') {
                        int new_brightness = -1;
                        if (sscanf((char *)&buffer[2], "%d", &new_brightness) == 1 && new_brightness >= 0 && new_brightness <= 99) {
                            brightness = new_brightness; // Оновлюємо глобальну яскравість
                            if (ledState) {
                                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness * 10); // Застосовуємо нову яскравість
                            }

                            snprintf((char *)response, sizeof(response), "Brightness set to %d", brightness);
                            HAL_UART_Transmit(&huart2, response, strlen((char *)response), HAL_MAX_DELAY);
                        } else {
                            snprintf((char *)response, sizeof(response), "Error: Invalid value");
                            HAL_UART_Transmit(&huart2, response, strlen((char *)response), HAL_MAX_DELAY);
                        }
                    } else {
                        snprintf((char *)response, sizeof(response), "Error: Invalid command");
                        HAL_UART_Transmit(&huart2, response, strlen((char *)response), HAL_MAX_DELAY);
                    }
                }
                index = 0;
            } else {
                if (index < sizeof(buffer) - 1) {
                    buffer[index++] = data;
                } else {
                    index = 0;
                    snprintf((char *)response, sizeof(response), "Error: Command too long");
                    HAL_UART_Transmit(&huart2, response, strlen((char *)response), HAL_MAX_DELAY);
                }
            }
        }
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Налаштування PA5 для PWM
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Налаштування PC13 як вхід з перериванням для кнопки B1
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Переривання по падінню сигналу
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Увімкнення переривань для PC13
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

void MX_TIM2_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 84 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}
