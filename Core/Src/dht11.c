/**
 * @file  dht11.c
 * @brief DHT11 bit-bang driver — STM32 F446RE GPIO PA9
 */

#include "dht11.h"
#include "main.h"

static TIM_HandleTypeDef *_htim;

void DHT11_Init(TIM_HandleTypeDef *htim) { _htim = htim; }

static void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(_htim, 0);
    while (__HAL_TIM_GET_COUNTER(_htim) < us);
}

static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = DHT11_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &cfg);
}

static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = DHT11_PIN;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &cfg);
}

DHT11_Data DHT11_Read(void)
{
    DHT11_Data result = {0, 0, 0};
    uint8_t data[5]   = {0};

    /* Start signal: pull low 18ms, release */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);
    DHT11_SetInput();

    /* Wait for DHT11 response (low 80us, high 80us) */
    uint32_t timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET && timeout--);
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET && timeout--);
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET && timeout--);

    /* Read 40 bits (5 bytes) */
    for (int i = 0; i < 40; i++)
    {
        /* Wait for bit start (low ~50us) */
        timeout = 10000;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET && timeout--);

        /* Measure high duration: >40us = '1', <30us = '0' */
        delay_us(40);
        data[i / 8] <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
            data[i / 8] |= 1;

        timeout = 10000;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET && timeout--);
    }

    /* Checksum */
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        result.humidity    = data[0];
        result.temperature = data[2];
        result.valid       = 1;
    }
    return result;
}
