#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"

/* ── Peripheral Handles ─────────────────────────── */
extern SPI_HandleTypeDef  hspi1;   /* RC522 RFID   */
extern I2C_HandleTypeDef  hi2c1;   /* DS1307 RTC   */
extern UART_HandleTypeDef huart1;  /* Debug UART   */
extern UART_HandleTypeDef huart2;  /* ESP8266 WiFi */
extern TIM_HandleTypeDef  htim1;   /* Delay timer  */

/* ── GPIO Pin Definitions ───────────────────────── */
/* RC522 RFID — SPI1 */
#define RC522_CS_PIN     GPIO_PIN_4
#define RC522_CS_PORT    GPIOA
#define RC522_RST_PIN    GPIO_PIN_7
#define RC522_RST_PORT   GPIOC

/* PIR Motion Sensor */
#define PIR_PIN          GPIO_PIN_0
#define PIR_PORT         GPIOC

/* DHT11 Temp/Humidity */
#define DHT11_PIN        GPIO_PIN_9
#define DHT11_PORT       GPIOA

/* LEDs */
#define LED_GREEN_PIN    GPIO_PIN_0    /* Access OK   */
#define LED_GREEN_PORT   GPIOB
#define LED_RED_PIN      GPIO_PIN_1    /* Access Deny */
#define LED_RED_PORT     GPIOB
#define LED_YELLOW_PIN   GPIO_PIN_2    /* Motion      */
#define LED_YELLOW_PORT  GPIOB

/* Buzzer */
#define BUZZER_PIN       GPIO_PIN_1
#define BUZZER_PORT      GPIOC

/* ── Function Prototypes ────────────────────────── */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_TIM1_Init(void);
void Error_Handler(void);

#endif /* __MAIN_H */
