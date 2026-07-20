/**
 * @file  ds1307.c
 * @brief DS1307 RTC I2C driver — STM32 F446RE I2C1 (PB6/PB7)
 */

#include "ds1307.h"

static I2C_HandleTypeDef *_hi2c;
static uint8_t DecToBcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }
static uint8_t BcdToDec(uint8_t v) { return ((v >> 4) * 10) + (v & 0x0F); }

void DS1307_Init(I2C_HandleTypeDef *hi2c) { _hi2c = hi2c; }

void DS1307_SetTime(const DS1307_Time *t)
{
    uint8_t data[8];
    data[0] = 0x00;                    /* Register address */
    data[1] = DecToBcd(t->seconds) & 0x7F;
    data[2] = DecToBcd(t->minutes);
    data[3] = DecToBcd(t->hours)   & 0x3F;
    data[4] = DecToBcd(t->day);
    data[5] = DecToBcd(t->date);
    data[6] = DecToBcd(t->month);
    data[7] = DecToBcd(t->year);
    HAL_I2C_Master_Transmit(_hi2c, DS1307_I2C_ADDR, data, 8, HAL_MAX_DELAY);
}

void DS1307_GetTime(DS1307_Time *t)
{
    uint8_t reg = 0x00;
    uint8_t data[7];
    HAL_I2C_Master_Transmit(_hi2c, DS1307_I2C_ADDR, &reg,  1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive (_hi2c, DS1307_I2C_ADDR, data, 7, HAL_MAX_DELAY);
    t->seconds = BcdToDec(data[0] & 0x7F);
    t->minutes = BcdToDec(data[1]);
    t->hours   = BcdToDec(data[2] & 0x3F);
    t->day     = BcdToDec(data[3]);
    t->date    = BcdToDec(data[4]);
    t->month   = BcdToDec(data[5]);
    t->year    = BcdToDec(data[6]);
}
