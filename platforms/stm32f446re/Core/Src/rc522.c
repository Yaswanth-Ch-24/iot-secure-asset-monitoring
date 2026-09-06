/**
 * @file    rc522.c
 * @brief   RC522 RFID driver for STM32 F446RE via SPI1
 */

#include "rc522.h"
#include "main.h"
#include <string.h>

static SPI_HandleTypeDef *_hspi;

/* ── CS Pin helpers ─────────────────────────────── */
#define RC522_CS_LOW()   HAL_GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, GPIO_PIN_RESET)
#define RC522_CS_HIGH()  HAL_GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, GPIO_PIN_SET)

void RC522_Init(SPI_HandleTypeDef *hspi)
{
    _hspi = hspi;
    RC522_CS_HIGH();
    RC522_Reset();
    RC522_WriteRegister(RC522_REG_T_MODE,      0x80);
    RC522_WriteRegister(RC522_REG_T_PRESCALER, 0xA9);
    RC522_WriteRegister(RC522_REG_T_RELOAD_H,  0x03);
    RC522_WriteRegister(RC522_REG_T_RELOAD_L,  0xE8);
    RC522_WriteRegister(RC522_REG_TX_ASK,      0x40);
    RC522_WriteRegister(RC522_REG_MODE,        0x3D);
    RC522_AntennaOn();
}

void RC522_Reset(void)
{
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    RC522_WriteRegister(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    HAL_Delay(50);
}

void RC522_AntennaOn(void)
{
    uint8_t val = RC522_ReadRegister(RC522_REG_TX_CONTROL);
    if ((val & 0x03) != 0x03)
        RC522_SetBitMask(RC522_REG_TX_CONTROL, 0x03);
}

uint8_t RC522_ReadRegister(uint8_t reg)
{
    uint8_t tx = ((reg << 1) & 0x7E) | 0x80;
    uint8_t rx = 0;
    RC522_CS_LOW();
    HAL_SPI_Transmit(_hspi, &tx, 1, 100);
    HAL_SPI_Receive(_hspi,  &rx, 1, 100);
    RC522_CS_HIGH();
    return rx;
}

void RC522_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { (reg << 1) & 0x7E, value };
    RC522_CS_LOW();
    HAL_SPI_Transmit(_hspi, tx, 2, 100);
    RC522_CS_HIGH();
}

void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    RC522_WriteRegister(reg, RC522_ReadRegister(reg) | mask);
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    RC522_WriteRegister(reg, RC522_ReadRegister(reg) & ~mask);
}

/* ── Detect if a card is in the field ──────────── */
RC522_Status RC522_IsCardPresent(void)
{
    uint8_t  cmd  = PICC_CMD_REQA;
    uint8_t  validBits = 7;
    uint32_t irqEn   = 0x77;
    uint32_t waitIRq = 0x30;

    RC522_WriteRegister(RC522_REG_COM_IEN,     (uint8_t)irqEn | 0x80);
    RC522_ClearBitMask(RC522_REG_COM_IRQ,      0x80);
    RC522_SetBitMask(RC522_REG_FIFO_LEVEL,     0x80);
    RC522_WriteRegister(RC522_REG_COMMAND,     RC522_CMD_IDLE);
    RC522_WriteRegister(RC522_REG_FIFO_DATA,   cmd);
    RC522_WriteRegister(RC522_REG_BIT_FRAMING, validBits);
    RC522_WriteRegister(RC522_REG_COMMAND,     RC522_CMD_TRANSCEIVE);
    RC522_SetBitMask(RC522_REG_BIT_FRAMING,    0x80);

    uint16_t i = 2000;
    uint8_t  irqVal;
    do {
        irqVal = RC522_ReadRegister(RC522_REG_COM_IRQ);
        i--;
    } while ((i != 0) && !(irqVal & 0x01) && !(irqVal & waitIRq));

    RC522_ClearBitMask(RC522_REG_BIT_FRAMING, 0x80);

    if (i == 0)              return RC522_NOTAGERR;
    if (!(irqVal & waitIRq)) return RC522_NOTAGERR;
    if (RC522_ReadRegister(RC522_REG_ERROR) & 0x1B) return RC522_ERR;
    return RC522_OK;
}

/* ── Read the card UID ──────────────────────────── */
RC522_Status RC522_ReadCardUID(RC522_UID *uid)
{
    uint8_t buffer[9];

    /* Anti-collision */
    RC522_WriteRegister(RC522_REG_COMMAND,     RC522_CMD_IDLE);
    RC522_SetBitMask(RC522_REG_FIFO_LEVEL,     0x80);
    buffer[0] = PICC_CMD_SEL_CL1;
    buffer[1] = 0x20;
    RC522_WriteRegister(RC522_REG_FIFO_DATA,   buffer[0]);
    RC522_WriteRegister(RC522_REG_FIFO_DATA,   buffer[1]);
    RC522_WriteRegister(RC522_REG_BIT_FRAMING, 0x00);
    RC522_WriteRegister(RC522_REG_COMMAND,     RC522_CMD_TRANSCEIVE);
    RC522_SetBitMask(RC522_REG_BIT_FRAMING,    0x80);
    HAL_Delay(10);
    RC522_ClearBitMask(RC522_REG_BIT_FRAMING,  0x80);

    uint8_t n = RC522_ReadRegister(RC522_REG_FIFO_LEVEL);
    if (n < 4) return RC522_ERR;

    uid->size = n > 4 ? 4 : n;
    for (uint8_t i = 0; i < uid->size; i++)
        uid->uid[i] = RC522_ReadRegister(RC522_REG_FIFO_DATA);

    return RC522_OK;
}
