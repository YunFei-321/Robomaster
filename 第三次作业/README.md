# 电控组第三次培训作业

牛云飞   20243003496



## 一、参考资料和驱动来源
- OLED驱动：由ai生成
3. 参考资料：SSD1306 OLED 数据手册。

## 二、串口反馈格式说明

STATE=RUN,DIR=FWD,STEP=300,LED=1

- STATE：RUN(运行) / STOP(暂停)
- DIR：FWD(正向) / REV(反向)
- STEP：相邻LED切换间隔，单位ms
- LED：当前点亮的LED编号(1~3)
以 \r\n 结尾，通过 HAL_UART_Transmit_IT() 中断发送。

## 三、控制指令格式说明
6字节数据包：AA | 状态 | 方向 | 时间低 | 时间高 | 校验和
校验和 = 前5字节累加和的低8位

## 四、选做说明
**软件I²C驱动OLED**

思路；不使用硬件外设，靠代码控制高低电平演示，可以自选引脚配置，但是会使cpu忙碌

代码

函数初始化

```
OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);
HAL_UART_Transmit(&huart2, (uint8_t*)"HELLO\r\n", 7, 1000);  // 上电问候
Print_Status_To_UART();
```

流水灯逻辑控制

```
if (g_is_running && HAL_GetTick() - g_tick_count >= g_step_ms) {
  g_tick_count = HAL_GetTick();

  // Turn off last LED
  switch(g_last_led_idx) {
  case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); break; // PA6 LED_BLUE
  case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); break; // PA7
  case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); break; // PB0
  }

  // Turn on current LED
  switch(g_current_led_idx) {
  case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); break;
  case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); break;
  case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;
  }

  g_last_led_idx = g_current_led_idx;

  // Update LED Index (正反转控制，注意是 % 3)
  if (g_direction == 0) {
    g_current_led_idx = (g_current_led_idx + 1) % 3;
  } else {
    g_current_led_idx = (g_current_led_idx - 1 + 3) % 3;
  }
  OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);  // ★加参数
```



### **硬件I²C驱动**OLED

思路；直接使用硬件i2c来实现与oled的控制，不必时刻占用cpu

代码

函数初始化

```
OLED_Init(&hi2c1);
OLED_Show_Status();
HAL_UART_Transmit(&huart2, (uint8_t*)"HELLO\r\n", 7, 1000);  // 上电问候
Print_Status_To_UART();

// 开启串口中断接收，每次等待接收6个字节（完整指令包）
HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);
```

流水灯逻辑控制

```
if (g_is_running && HAL_GetTick() - g_tick_count >= g_step_ms) {
  g_tick_count = HAL_GetTick();

  // Turn off last LED
  switch(g_last_led_idx) {
  case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); break; // PA6 LED_BLUE
  case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); break; // PA7
  case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); break; // PB0
  }

  // Turn on current LED
  switch(g_current_led_idx) {
  case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); break;
  case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); break;
  case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;
  }

  g_last_led_idx = g_current_led_idx;

  // Update LED Index (正反转控制，注意是 % 3)
  if (g_direction == 0) {
    g_current_led_idx = (g_current_led_idx + 1) % 3;
  } else {
    g_current_led_idx = (g_current_led_idx - 1 + 3) % 3;
  }

  OLED_Show_Status();
```

OLED.c文件

通过i2c来发送命令和数据

```
static void OLED_Write_Cmd(uint8_t cmd) {// 命令
    uint8_t data[] = {0x00, cmd};
    HAL_I2C_Master_Transmit(p_hi2c, OLED_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

static void OLED_Write_Data(uint8_t *data, uint16_t len) {//数据发送
    uint8_t header = 0x40;
    HAL_I2C_Mem_Write(p_hi2c, OLED_I2C_ADDR, header, 1, data, len, HAL_MAX_DELAY);
}
```