#include "main.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();

    // Запуск таймера PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    uint8_t buffer[100];  // Буфер для отримання команд
    uint8_t response[100];  // Буфер для відповіді
    uint16_t index = 0;  // Індекс для буфера

    // Відправка стартового повідомлення
    uint8_t startMessage[] = "UART Brightness Control Active\r\n";
    HAL_UART_Transmit(&huart2, startMessage, sizeof(startMessage), HAL_MAX_DELAY);

    while (1)
    {
        uint8_t data;
        // Прийом символів через UART
        if (HAL_UART_Receive(&huart2, &data, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            // Якщо отримано символ нового рядка (\n), обробляємо команду
            if (data == '\n' || data == '\r')
            {
                buffer[index] = '\0';  // Завершення рядка
                index = 0;

                // Перевірка команди "L=xx"
                if (tolower(buffer[0]) == 'l' && buffer[1] == '=')
                {
                    int brightness = -1;
                    if (sscanf((char*)&buffer[2], "%d", &brightness) == 1 && brightness >= 0 && brightness <= 99)
                    {
                        // Зміна яскравості світлодіода
                        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, brightness * 10);

                        // Формування відповіді
                        snprintf((char*)response, sizeof(response), "Brightness set to %d\r\n", brightness);
                        HAL_UART_Transmit(&huart2, response, strlen((char*)response), HAL_MAX_DELAY);
                    }
                    else
                    {
                        // Некоректне значення
                        snprintf((char*)response, sizeof(response), "Error: Invalid value\r\n");
                        HAL_UART_Transmit(&huart2, response, strlen((char*)response), HAL_MAX_DELAY);
                    }
                }
                else
                {
                    // Некоректна команда
                    snprintf((char*)response, sizeof(response), "Error: Invalid command\r\n");
                    HAL_UART_Transmit(&huart2, response, strlen((char*)response), HAL_MAX_DELAY);
                }
            }
            else
            {
                // Зберігаємо символ у буфер
                if (index < sizeof(buffer) - 1)
                {
                    buffer[index++] = data;
                }
                else
                {
                    // Якщо буфер переповнений, скидаємо його
                    index = 0;
                    snprintf((char*)response, sizeof(response), "Error: Command too long\r\n");
                    HAL_UART_Transmit(&huart2, response, strlen((char*)response), HAL_MAX_DELAY);
                }
            }
        }
    }
}

// Налаштування тактування системи
void SystemClock_Config(void)
{
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

// Налаштування GPIO (PA5 для LED)
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Увімкнути тактування для GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Налаштування PA5 як вихід
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2; // PWM через TIM2
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Налаштування UART2
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

// Налаштування TIM2 для PWM
static void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 84 - 1;  // Таймер працює на 1 МГц
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999;  // 1 кГц частота PWM
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  // Початкова яскравість
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

// Функція обробки помилок
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
    }
}
