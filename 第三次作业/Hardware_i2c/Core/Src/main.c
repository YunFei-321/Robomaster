/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t g_is_running = 1;
uint8_t g_direction = 0;
uint16_t g_step_ms = 300;
uint8_t g_current_led_idx = 0; // 0, 1, 2 (对应3个LED)
uint8_t g_last_led_idx = 0;
uint32_t g_tick_count = 0;
uint8_t g_uart_rx_buf[6];
uint8_t g_uart_tx_buf[64];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Print_Status_To_UART(void);
void Sync_Status(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init(&hi2c1);
  OLED_Show_Status();
  HAL_UART_Transmit(&huart2, (uint8_t*)"HELLO\r\n", 7, 1000);  // 上电问候
  Print_Status_To_UART();

  // 开启串口中断接收，每次等待接收6个字节（完整指令包）
  HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {       //流水灯的逻辑控制
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
      // Sync_Status();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


// // 串口接收回调函数：彻底适配你的指令格式
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
// {
//     if (huart->Instance == USART2)
//     {
//         // 1. 创建临时数组，专门用来重组数据帧
//         uint8_t corrected_buf[6] = {0};
//         corrected_buf[0] = g_uart_rx_buf[0]; // 帧头 AA
//         corrected_buf[1] = g_uart_rx_buf[1]; // 状态位
//         corrected_buf[2] = g_uart_rx_buf[2]; // 方向位
//         corrected_buf[3] = g_uart_rx_buf[4]; // 【把接收到的低位移到标准高位】
//         corrected_buf[4] = g_uart_rx_buf[3]; // 【把接收到的高位移到标准低位】
//         corrected_buf[5] = g_uart_rx_buf[5]; // 校验和
//
//         // 2. 对重组后的数据进行校验
//         uint8_t local_checksum = corrected_buf[0] + corrected_buf[1] + corrected_buf[2] + corrected_buf[3] + corrected_buf[4];
//
//         if (corrected_buf[5] == local_checksum)
//         {
//             // 3. 解析重组后的完美数据
//             g_is_running = corrected_buf[1];
//             g_direction  = corrected_buf[2];
//             // 现在 corrected_buf[3] 和 corrected_buf[4] 已经是标准的高低顺序了
//             g_step_ms    = ((uint16_t)corrected_buf[3] << 8) | corrected_buf[4];
//
//             // 更新屏幕显示，并发送回复指令
//             OLED_Show_Status();
//             Print_Status_To_UART();
//         }
//
//         // 重新开启接收中断
//         HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);
//     }
// }
//
// // 发送状态回复函数：标准的高低位发送
// // void Print_Status_To_UART(void) {
// //     uint8_t state_byte = g_is_running;
// //     uint8_t dir_byte = g_direction;
// //
// //     // 标准拆分
// //     uint8_t step_high = (uint8_t)(g_step_ms >> 8);
// //     uint8_t step_low = (uint8_t)(g_step_ms & 0xFF);
// //
// //     // 拼接前 5 个基础字节
// //     g_uart_tx_buf[0] = 0xAA;
// //     g_uart_tx_buf[1] = state_byte;
// //     g_uart_tx_buf[2] = dir_byte;
// //     g_uart_tx_buf[3] = step_high;  // 300ms的高位 0x01
// //     g_uart_tx_buf[4] = step_low;   // 300ms的低位 0x2C
// //
// //     // 计算校验和
// //     uint8_t checksum = g_uart_tx_buf[0] + g_uart_tx_buf[1] + g_uart_tx_buf[2] + g_uart_tx_buf[3] + g_uart_tx_buf[4];
// //     g_uart_tx_buf[5] = checksum;
// //
// //     // 发送 6 字节数据包
// //     HAL_UART_Transmit(&huart2, g_uart_tx_buf, 6, 1000);
// // }
//
// // ★修改：改为发送可读字符串，内容与OLED屏一致
// void Print_Status_To_UART(void) {
//   char *state_str = g_is_running ? "RUN" : "STOP";
//   char *dir_str   = (g_direction == 0) ? "FWD" : "REV";
//
//   // 拼接成和OLED显示一致的字符串
//   uint16_t len = sprintf((char*)g_uart_tx_buf,
//                          "STATE=%s,DIR=%s,STEP=%d,LED=%d\r\n",
//                          state_str, dir_str, g_step_ms, g_current_led_idx + 1);
//
//   // 发送字符串（长度用实际拼接出来的len）
//   HAL_UART_Transmit(&huart2, g_uart_tx_buf, len, 1000);
// }
// // 按键中断回调
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//     if (GPIO_Pin == GPIO_PIN_12) { // KEY1
//         g_is_running = !g_is_running;
//         OLED_Show_Status();
//         Print_Status_To_UART();
//     } else if (GPIO_Pin == GPIO_PIN_13) { // KEY2
//         g_direction = !g_direction;
//         OLED_Show_Status();
//         Print_Status_To_UART();
//     }
// }



// 统一的"状态同步"函数，串口和OLED永远一起刷新
void Sync_Status(void)
{
  OLED_Show_Status();        // 刷新OLED
  Print_Status_To_UART();    // 串口回复可读字符串
}

// 串口接收回调：解析6字节指令
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {

    //   AA + 状态 + 方向 + 时间低 + 时间高
    uint8_t checksum = g_uart_rx_buf[0] + g_uart_rx_buf[1] +
                       g_uart_rx_buf[2] + g_uart_rx_buf[3] +
                       g_uart_rx_buf[4];

    // 校验通过
    if (g_uart_rx_buf[0] == 0xAA && g_uart_rx_buf[5] == checksum)
    {
      g_is_running = g_uart_rx_buf[1];              // 0=STOP, 1=RUN
      g_direction  = g_uart_rx_buf[2];              // 0=FWD, 1=REV

      // 时间按低字节在前解析
      //   收到 2C 01 → (0x01 << 8) | 0x2C = 300ms
      g_step_ms = ((uint16_t)g_uart_rx_buf[4] << 8) | g_uart_rx_buf[3];

      // OLED和串口同步更新
      Sync_Status();
    }

    HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);   // 重新挂接收
  }
}

// 发送可读字符串，内容与OLED完全一致
void Print_Status_To_UART(void)
{
  char *state_str = g_is_running ? "RUN" : "STOP";
  char *dir_str   = (g_direction == 0) ? "FWD" : "REV";

  uint16_t len = sprintf((char*)g_uart_tx_buf,
                         "STATE=%s,DIR=%s,STEP=%d,LED=%d\r\n",
                         state_str, dir_str, g_step_ms, g_current_led_idx + 1);

  HAL_UART_Transmit_IT(&huart2, g_uart_tx_buf, len);
}

// 按键回调：也走统一同步函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_12) {        // KEY1 运行/暂停
    g_is_running = !g_is_running;
    Sync_Status();
  } else if (GPIO_Pin == GPIO_PIN_13) { // KEY2 正/反向
    g_direction = !g_direction;
    Sync_Status();
  }
}




/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
