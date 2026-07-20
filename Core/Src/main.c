/**
 ******************************************************************************
 * @file    main.c
 * @brief   IoT Secure Asset Management & Environmental Monitoring
 *          Board  : STM32 F446RE (Nucleo-F446RE)
 *          Author : Chlliboina Yaswanth
 *          GitHub : https://github.com/Yaswanth-Ch-24
 ******************************************************************************
 *
 * SYSTEM OVERVIEW
 * ───────────────
 *  - RC522 RFID reader scans cards → SPI1
 *  - Authorized UID list checked → Access Granted / Denied
 *  - DS1307 RTC timestamps every event → I2C1
 *  - DHT11 reads temp + humidity every 5s → GPIO PA9
 *  - HC-SR501 PIR detects motion → GPIO PC0 (EXTI)
 *  - ESP8266 sends logs to server → USART2
 *  - All events logged to debug terminal → USART1
 *
 ******************************************************************************
 */

#include "main.h"
#include "rc522.h"
#include "dht11.h"
#include "ds1307.h"
#include "access_control.h"
#include <stdio.h>
#include <string.h>

/* ── Peripheral Handles ─────────────────────────── */
SPI_HandleTypeDef  hspi1;
I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
TIM_HandleTypeDef  htim1;

/* ── Private Variables ──────────────────────────── */
static RC522_UID    uid;
static DS1307_Time  rtc;
static DHT11_Data   env;
static char         logBuf[256];
static uint8_t      motionFlag = 0;
static uint32_t     lastEnvRead = 0;

/* ── EXTI Callback (PIR Motion) ─────────────────── */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PIR_PIN) {
        motionFlag = 1;
    }
}

/* ── UART Print Helper ──────────────────────────── */
static void UART_Print(const char *msg)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ── WiFi Log (ESP8266) ─────────────────────────── */
static void WiFi_SendLog(const char *msg)
{
    /* Send via ESP8266 AT command: AT+CIPSEND */
    char atCmd[300];
    snprintf(atCmd, sizeof(atCmd), "AT+CIPSEND=%d\r\n", (int)strlen(msg));
    HAL_UART_Transmit(&huart2, (uint8_t *)atCmd, strlen(atCmd), 1000);
    HAL_Delay(100);
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

/* ── Main ───────────────────────────────────────── */
int main(void)
{
    /* HAL + Clock + Peripheral Init */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_TIM1_Init();
    HAL_TIM_Base_Start(&htim1);

    /* Driver Init */
    RC522_Init(&hspi1);
    DHT11_Init(&htim1);
    DS1307_Init(&hi2c1);

    /* Set initial RTC time (run once, then comment out) */
    /* DS1307_Time initTime = {0,30,9,2,15,3,26}; DS1307_SetTime(&initTime); */

    /* Banner */
    UART_Print("\r\n=========================================\r\n");
    UART_Print("  IoT Asset Management System\r\n");
    UART_Print("  STM32 F446RE | Yaswanth Chlliboina\r\n");
    UART_Print("=========================================\r\n");

    DS1307_GetTime(&rtc);
    snprintf(logBuf, sizeof(logBuf),
             "[20%02d-%02d-%02d %02d:%02d:%02d] System initialized\r\n",
             rtc.year, rtc.month, rtc.date,
             rtc.hours, rtc.minutes, rtc.seconds);
    UART_Print(logBuf);

    /* ── Main Loop ─────────────────────────────── */
    while (1)
    {
        /* 1. Read environment every 5 seconds */
        if ((HAL_GetTick() - lastEnvRead) >= 5000)
        {
            lastEnvRead = HAL_GetTick();
            env = DHT11_Read();
            DS1307_GetTime(&rtc);
            if (env.valid)
            {
                snprintf(logBuf, sizeof(logBuf),
                         "[20%02d-%02d-%02d %02d:%02d:%02d] DHT11: Temp=%dC  Humidity=%d%%\r\n",
                         rtc.year, rtc.month, rtc.date,
                         rtc.hours, rtc.minutes, rtc.seconds,
                         env.temperature, env.humidity);
                UART_Print(logBuf);
            }
        }

        /* 2. Check PIR motion alert */
        if (motionFlag)
        {
            motionFlag = 0;
            DS1307_GetTime(&rtc);
            AccessControl_MotionAlert(&rtc);

            /* Yellow LED + Buzzer ON for 2s */
            HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
            HAL_Delay(2000);
            HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
        }

        /* 3. Poll RFID */
        if (RC522_IsCardPresent() == RC522_OK)
        {
            if (RC522_ReadCardUID(&uid) == RC522_OK)
            {
                DS1307_GetTime(&rtc);

                snprintf(logBuf, sizeof(logBuf),
                         "[20%02d-%02d-%02d %02d:%02d:%02d] RFID scan detected...\r\n",
                         rtc.year, rtc.month, rtc.date,
                         rtc.hours, rtc.minutes, rtc.seconds);
                UART_Print(logBuf);

                snprintf(logBuf, sizeof(logBuf),
                         "[20%02d-%02d-%02d %02d:%02d:%02d] Card UID: %02X %02X %02X %02X\r\n",
                         rtc.year, rtc.month, rtc.date,
                         rtc.hours, rtc.minutes, rtc.seconds,
                         uid.uid[0], uid.uid[1], uid.uid[2], uid.uid[3]);
                UART_Print(logBuf);

                AccessResult result = AccessControl_CheckUID(&uid);
                AccessControl_LogEvent(&uid, result, &rtc);

                if (result == ACCESS_GRANTED)
                {
                    AccessControl_GrantAccess();
                    snprintf(logBuf, sizeof(logBuf),
                             "[20%02d-%02d-%02d %02d:%02d:%02d] ACCESS GRANTED  >> Green LED ON\r\n",
                             rtc.year, rtc.month, rtc.date,
                             rtc.hours, rtc.minutes, rtc.seconds);
                }
                else
                {
                    AccessControl_DenyAccess();
                    snprintf(logBuf, sizeof(logBuf),
                             "[20%02d-%02d-%02d %02d:%02d:%02d] ACCESS DENIED   >> Red LED ON >> Buzzer ON\r\n",
                             rtc.year, rtc.month, rtc.date,
                             rtc.hours, rtc.minutes, rtc.seconds);
                }
                UART_Print(logBuf);
                WiFi_SendLog(logBuf);

                HAL_Delay(2000); /* Debounce */
            }
        }

        HAL_Delay(100);
    }
}

/* ── Error Handler ──────────────────────────────── */
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
