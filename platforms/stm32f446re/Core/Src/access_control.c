/**
 * @file    access_control.c
 * @brief   RFID Access Control Logic
 *          Add your card UIDs to authorizedCards[] below.
 */

#include "access_control.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

/* ── Authorized Card UIDs ───────────────────────── */
/* Replace these with your actual card UIDs          */
/* Scan a card and note its UID from UART output     */
const uint8_t authorizedCards[MAX_AUTHORIZED_CARDS][4] = {
    {0xA3, 0xF2, 0xB1, 0x09},   /* Card 1 — Admin      */
    {0x12, 0x34, 0x56, 0x78},   /* Card 2 — User 1     */
    {0xDE, 0xAD, 0xBE, 0xEF},   /* Card 3 — User 2     */
    /* Add more here ... */
};
uint8_t numAuthorizedCards = 3;

/* ── Check if UID is authorized ────────────────── */
AccessResult AccessControl_CheckUID(const RC522_UID *uid)
{
    for (uint8_t i = 0; i < numAuthorizedCards; i++)
    {
        if (memcmp(uid->uid, authorizedCards[i], 4) == 0)
            return ACCESS_GRANTED;
    }
    return ACCESS_DENIED;
}

/* ── Log event via UART ─────────────────────────── */
void AccessControl_LogEvent(const RC522_UID *uid, AccessResult result, const DS1307_Time *t)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "[LOG] UID=%02X%02X%02X%02X Result=%s Time=20%02d-%02d-%02d %02d:%02d:%02d\r\n",
             uid->uid[0], uid->uid[1], uid->uid[2], uid->uid[3],
             (result == ACCESS_GRANTED) ? "GRANTED" : "DENIED",
             t->year, t->month, t->date,
             t->hours, t->minutes, t->seconds);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
}

/* ── Grant Access — Green LED 3s ────────────────── */
void AccessControl_GrantAccess(void)
{
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_PORT,   LED_RED_PIN,   GPIO_PIN_RESET);
    HAL_Delay(3000);
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
}

/* ── Deny Access — Red LED + Buzzer 1s ─────────── */
void AccessControl_DenyAccess(void)
{
    HAL_GPIO_WritePin(LED_RED_PORT,   LED_RED_PIN,   GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT,    BUZZER_PIN,    GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(LED_RED_PORT,   LED_RED_PIN,   GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT,    BUZZER_PIN,    GPIO_PIN_RESET);
}

/* ── Motion Alert — Yellow LED + Buzzer ─────────── */
void AccessControl_MotionAlert(const DS1307_Time *t)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "[20%02d-%02d-%02d %02d:%02d:%02d] PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON\r\n",
             t->year, t->month, t->date,
             t->hours, t->minutes, t->seconds);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
}
