//
// Created by Administrator on 2026/11/16.
//

#ifndef OLED_H
#define OLED_H

#include "stm32f1xx_hal.h"

// OLED I2C Definitions
#define OLED_I2C_ADDR 0x78
extern I2C_HandleTypeDef hi2c1;

// Global System Status Variables
extern uint8_t g_is_running;
extern uint8_t g_direction;
extern uint16_t g_step_ms;
extern uint8_t g_current_led_idx;

void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Show_Status(void);

#endif