# 电控组第三次培训作业

*牛云飞   20243003496*



### 一、参考资料和驱动来源
OLED驱动：由参考群文件提供

参考资料：SSD1306 OLED 数据手册

### 二、串口反馈格式说明

STATE=RUN,DIR=FWD,STEP=300,LED=1

- STATE：RUN(运行) / STOP(暂停)
- DIR：FWD(正向) / REV(反向)
- STEP：相邻LED切换间隔，单位ms
- LED：当前点亮的LED编号(1~3)
  以 \r\n 结尾，通过 HAL_UART_Transmit_IT() 中断发送。
- 指令格式：6字节数据包：AA | 状态 | 方向 | 时间低 | 时间高 | 校验和
  校验和 = 前5字节累加和的低8位

### 三、选做说明
**软件I²C驱动OLED，及串口DMA的使用**

思路；不使用硬件外设，靠代码控制高低电平演示，可以自选引脚配置，但是会使cpu忙碌

代码



```
#include "OLED.h"          /* ★ 软件 I2C 驱动 */

OLED_Init();             /* ★ 软件 I2C: 无参数 */

/*Configure GPIO pins : PB6 PB7 */
GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  /* ★ 开漏输出 */
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


void Print_Status_To_UART(void)
{
  char *state_str = g_is_running ? "RUN" : "STOP";
  char *dir_str   = (g_direction == 0) ? "FWD" : "REV";

  uint16_t len = sprintf((char*)g_uart_tx_buf,
                         "STATE=%s,DIR=%s,STEP=%d,LED=%d\r\n",
                         state_str, dir_str, g_step_ms, g_current_led_idx + 1);

  HAL_UART_Transmit_DMA(&huart2, g_uart_tx_buf, len); / DMA发送 /
}
```

#### **硬件I²C驱动**OLED

思路；直接使用硬件i2c来实现与oled的控制，不必时刻占用cpu

代码

```
#include "dma.h"
#include "i2c.h"

OLED_Init(&hi2c1);//指向硬件i2c通道
OLED_Show_Status();
 
```















