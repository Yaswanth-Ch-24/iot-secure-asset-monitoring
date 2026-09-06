#ifndef DHT11_H
#define DHT11_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    uint8_t temperature;   /* Celsius */
    uint8_t humidity;      /* % RH    */
    uint8_t valid;         /* 1=OK 0=error */
} DHT11_Data;

void      DHT11_Init(TIM_HandleTypeDef *htim);
DHT11_Data DHT11_Read(void);

/* Internal helpers */
static void DHT11_SetOutput(void);
static void DHT11_SetInput(void);
static void delay_us(uint16_t us);

#endif /* DHT11_H */
