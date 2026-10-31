# 电控组第二次培训作业

**牛云飞**

***20243003496***



核心代码（参考ai写的）

思路：1，初始化变量  2，启动定时器 3，写控制流水的亮灭逻辑 4，通过回调函数让cpu执行命令

```
 /* USER CODE BEGIN 0 */
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint32_t system_tick = 0;      // 记录系统走过的毫秒数
uint8_t led_step = 0;          // 记录当前流水灯走到了第几步 (0, 1, 2)
uint32_t last_switch_time = 0; // 记录上一次切换灯的时间
/* USER CODE END PFP */
/* USER CODE END 0 */

 /* USER CODE BEGIN 2 */
    MX_GPIO_Init();
    MX_TIM2_Init();
    // 启动定时器TIM2，并开启中断
    HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

while (1)
  {
    /* USER CODE END WHILE */
    // 检查时间是否过去了 500ms（500毫秒切换一次灯）
    if ((system_tick - last_switch_time) >= 500) {
      // 1. 先熄灭所有的灯（安全起见，把3个灯全关了）
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

      // 2. 根据 step 决定点亮哪个灯
      if (led_step == 0) {
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
      } else if (led_step == 1) {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
      } else {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
      }

      // 3. 步进到下一个灯（0 -> 1 -> 2 -> 0 无限循环）
      led_step++;
      if (led_step >= 3) {
        led_step = 0;
      }

      // 更新上一次的切换时间
      last_switch_time = system_tick;
    }

   
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}


/* USER CODE BEGIN 4 */
// 这是一个定时器中断回调函数，每 1ms 自动被硬件触发一次
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) // 确认是定时器2触发的
  {
    system_tick++; // 每过 1ms，系统计数器 +1
  }
}
/* USER CODE END 4 */
```

