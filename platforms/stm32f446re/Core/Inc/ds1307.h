#ifndef DS1307_H
#define DS1307_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define DS1307_I2C_ADDR  (0x68 << 1)   /* 7-bit addr 0x68, shifted for HAL */

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;      /* 1=Mon ... 7=Sun */
    uint8_t date;
    uint8_t month;
    uint8_t year;     /* Last 2 digits e.g. 26 for 2026 */
} DS1307_Time;

void DS1307_Init(I2C_HandleTypeDef *hi2c);
void DS1307_SetTime(const DS1307_Time *t);
void DS1307_GetTime(DS1307_Time *t);

/* BCD helpers */
static uint8_t DecToBcd(uint8_t val);
static uint8_t BcdToDec(uint8_t val);

#endif /* DS1307_H */
