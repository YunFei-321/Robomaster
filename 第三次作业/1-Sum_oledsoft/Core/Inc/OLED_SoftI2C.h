

#define INC_1_SUM1_OLED_SOFTI2C_H
#ifndef __OLED_SOFTI2C_H
#define __OLED_SOFTI2C_H

#include "main.h"
#include <stdint.h>

// ===== 引脚定义，按你的实际接线修改 =====
#define OLED_SCL_PORT   GPIOB
#define OLED_SCL_PIN    GPIO_PIN_6
#define OLED_SDA_PORT   GPIOB
#define OLED_SDA_PIN    GPIO_PIN_7

// ===== 电平控制宏 =====
#define SCL_H()   HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_SET)
#define SCL_L()   HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_RESET)
#define SDA_H()   HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_SET)
#define SDA_L()   HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_RESET)

// ===== 函数声明 =====
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t page, uint8_t col, const char *str);
void OLED_Show_Status(uint8_t is_run, uint8_t dir, uint16_t step_ms, uint8_t led_num);

#endif

