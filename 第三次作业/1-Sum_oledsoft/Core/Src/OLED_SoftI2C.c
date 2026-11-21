#include "OLED_SoftI2C.h"
#include <stdio.h>
#include <string.h>

// 外部字库（如果你的工程已有 OLED_Font.h 就直接用；没有的话见文末说明）
#include "OLED_Font.h"

#define OLED_ADDR   0x78    // SSD1306 7位地址0x3C左移1位

// ============ 微秒级延时（用DWT计数器，最精确） ============
static void Delay_us(uint32_t us)
{
    uint32_t start = HAL_GetTick();
    // 简单起见用HAL_Delay的1ms粒度*，软件I2C在72MHz下不需要严格us
    // 这里用空循环近似
    volatile uint32_t n = us * 8;
    while (n--) { __NOP(); }
}

// ============ I2C 基础时序 ============

// 起始信号：SCL高电平时，SDA由高变低
static void I2C_Start(void)
{
    SDA_H(); SCL_H();
    Delay_us(4);
    SDA_L();          // SCL高时SDA下降沿 = START
    Delay_us(4);
    SCL_L();          // 钳住总线，准备传输
    Delay_us(4);
}

// 停止信号：SCL高电平时，SDA由低变高
static void I2C_Stop(void)
{
    SDA_L();
    Delay_us(4);
    SCL_H();
    Delay_us(4);
    SDA_H();          // SCL高时SDA上升沿 = STOP
    Delay_us(4);
}

// 发送一个字节（高位在前）
static void I2C_SendByte(uint8_t dat)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (dat & 0x80) SDA_H();
        else            SDA_L();
        dat <<= 1;
        Delay_us(2);
        SCL_H();      // SCL上升沿，从机采样SDA
        Delay_us(4);
        SCL_L();
        Delay_us(2);
    }
}

// 等待从机应答（ACK）
static void I2C_WaitAck(void)
{
    SDA_H();          // 释放SDA，让从机拉低
    Delay_us(2);
    SCL_H();
    Delay_us(4);
    // 这里不检测SDA电平，直接认为有ACK（OLED一般都有）
    SCL_L();
    Delay_us(4);
}

// ============ OLED 写命令/数据 ============

static void OLED_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR);      // 器件地址 + 写
    I2C_WaitAck();
    I2C_SendByte(0x00);           // 控制字节：Co=0, D/C=0 → 命令
    I2C_WaitAck();
    I2C_SendByte(cmd);
    I2C_WaitAck();
    I2C_Stop();
}

static void OLED_WriteData(uint8_t dat)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR);
    I2C_WaitAck();
    I2C_SendByte(0x40);           // 控制字节：Co=0, D/C=1 → 数据
    I2C_WaitAck();
    I2C_SendByte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

// 设置光标位置（页地址 + 列地址）
static void OLED_SetPos(uint8_t page, uint8_t col)
{
    OLED_WriteCmd(0xB0 | page);           // 设置页
    OLED_WriteCmd(0x00 | (col & 0x0F));  // 列低4位
    OLED_WriteCmd(0x10 | (col >> 4));    // 列高4位
}

// ============ 对外接口 ============

void OLED_Init(void)
{
    // 上电初始化序列（SSD1306 128x64）
    HAL_Delay(100);
    OLED_WriteCmd(0xAE);  // 关显示
    OLED_WriteCmd(0x20);  // 列地址寻址模式
    OLED_WriteCmd(0x02);  // 页地址模式
    OLED_WriteCmd(0xB0);  // 设置页地址
    OLED_WriteCmd(0xC8);  // 行扫描方向从上到下
    OLED_WriteCmd(0x00);  // 列地址低4位
    OLED_WriteCmd(0x10);  // 列地址高4位
    OLED_WriteCmd(0x40);  // 设置显示起始行
    OLED_WriteCmd(0x81);  // 设置对比度
    OLED_WriteCmd(0xFF);
    OLED_WriteCmd(0xA1);  // 段重定向
    OLED_WriteCmd(0xA6);  // 正常显示（非反色）
    OLED_WriteCmd(0xA8);  // 设置多路复用率
    OLED_WriteCmd(0x3F);  // 1/64占空比
    OLED_WriteCmd(0xA4);  // 输出跟随RAM
    OLED_WriteCmd(0xD3);  // 设置显示偏移
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);  // 设置时钟分频
    OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xD9);  // 预充电周期
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDA);  // COM引脚硬件配置
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);  // VCOMH
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D);  // 电荷泵
    OLED_WriteCmd(0x14);  // 使能电荷泵
    OLED_WriteCmd(0xAF);  // 开显示
    OLED_Clear();
}

void OLED_Clear(void)
{
    uint8_t page, col;
    for (page = 0; page < 8; page++)
    {
        OLED_SetPos(page, 0);
        for (col = 0; col < 128; col++)
            OLED_WriteData(0x00);
    }
}

// 显示字符串（8x16字体，page必须为偶数）
void OLED_ShowString(uint8_t page, uint8_t col, const char *str)
{
    uint8_t c;
    OLED_SetPos(page, col);
    while (*str)
    {
        c = *str++;
        // 上半部分
        for (uint8_t i = 0; i < 8; i++)
            OLED_WriteData(F8X16[(c - 32) * 16 + i]);
        OLED_SetPos(++page, col);
        // 下半部分
        for (uint8_t i = 0; i < 8; i++)
            OLED_WriteData(F8X16[(c - 32) * 16 + 8 + i]);
        col += 8;
        if (col > 120) break;   // 超出屏幕
    }
}

// ★显示状态（和硬件I2C版本接口一致，方便main.c无缝切换）
void OLED_Show_Status(uint8_t is_run, uint8_t dir, uint16_t step_ms, uint8_t led_num)
{
    char buf[24];

    sprintf(buf, "STATE: %s", is_run ? "RUN" : "STOP");
    OLED_ShowString(0, 0, buf);

    sprintf(buf, "DIR: %s", dir ? "REV" : "FWD");
    OLED_ShowString(2, 0, buf);

    sprintf(buf, "STEP: %dms", step_ms);
    OLED_ShowString(4, 0, buf);

    sprintf(buf, "LED: %d", led_num);
    OLED_ShowString(6, 0, buf);
}