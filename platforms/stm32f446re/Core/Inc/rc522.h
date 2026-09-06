#ifndef RC522_H
#define RC522_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ── RC522 Register Map ─────────────────────────── */
#define RC522_REG_COMMAND        0x01
#define RC522_REG_COM_IEN        0x02
#define RC522_REG_DIV_IEN        0x03
#define RC522_REG_COM_IRQ        0x04
#define RC522_REG_DIV_IRQ        0x05
#define RC522_REG_ERROR          0x06
#define RC522_REG_STATUS1        0x07
#define RC522_REG_STATUS2        0x08
#define RC522_REG_FIFO_DATA      0x09
#define RC522_REG_FIFO_LEVEL     0x0A
#define RC522_REG_CONTROL        0x0C
#define RC522_REG_BIT_FRAMING    0x0D
#define RC522_REG_MODE           0x11
#define RC522_REG_TX_CONTROL     0x14
#define RC522_REG_TX_ASK         0x15
#define RC522_REG_CRC_RESULT_H   0x21
#define RC522_REG_CRC_RESULT_L   0x22
#define RC522_REG_T_MODE         0x2A
#define RC522_REG_T_PRESCALER    0x2B
#define RC522_REG_T_RELOAD_H     0x2C
#define RC522_REG_T_RELOAD_L     0x2D
#define RC522_REG_VERSION        0x37

/* ── RC522 Commands ─────────────────────────────── */
#define RC522_CMD_IDLE           0x00
#define RC522_CMD_CALCULATE_CRC  0x03
#define RC522_CMD_TRANSMIT       0x04
#define RC522_CMD_RECEIVE        0x08
#define RC522_CMD_TRANSCEIVE     0x0C
#define RC522_CMD_SOFT_RESET     0x0F

/* ── MIFARE Commands ────────────────────────────── */
#define PICC_CMD_REQA            0x26
#define PICC_CMD_WUPA            0x52
#define PICC_CMD_CT              0x88
#define PICC_CMD_SEL_CL1         0x93
#define PICC_CMD_SEL_CL2         0x95
#define PICC_CMD_HLTA            0x50

/* ── Status Codes ───────────────────────────────── */
typedef enum {
    RC522_OK = 0,
    RC522_NOTAGERR,
    RC522_ERR
} RC522_Status;

/* ── UID Structure ──────────────────────────────── */
typedef struct {
    uint8_t size;
    uint8_t uid[10];
    uint8_t sak;
} RC522_UID;

/* ── Function Prototypes ────────────────────────── */
void       RC522_Init(SPI_HandleTypeDef *hspi);
void       RC522_Reset(void);
RC522_Status RC522_IsCardPresent(void);
RC522_Status RC522_ReadCardUID(RC522_UID *uid);
uint8_t    RC522_ReadRegister(uint8_t reg);
void       RC522_WriteRegister(uint8_t reg, uint8_t value);
void       RC522_SetBitMask(uint8_t reg, uint8_t mask);
void       RC522_ClearBitMask(uint8_t reg, uint8_t mask);
void       RC522_AntennaOn(void);

#endif /* RC522_H */
