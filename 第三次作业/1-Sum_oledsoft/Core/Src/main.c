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
// #include "OLED.h"
#include <stdio.h>
#include "OLED_SoftI2C.h"
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
  // OLED_Init(&hi2c1);  //硬件驱动
  // OLED_Show_Status();  //软件驱动
  OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);
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
      OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);  // ★加参数

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



void Sync_Status(void)
{
  // OLED_Show_Status();        // 刷新OLED
  OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);
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

    // 校验通过才执行
    if (g_uart_rx_buf[0] == 0xAA && g_uart_rx_buf[5] == checksum)
    {
      g_is_running = g_uart_rx_buf[1];              // 0=STOP, 1=RUN
      g_direction  = g_uart_rx_buf[2];              // 0=FWD, 1=REV


      //   收到 2C 01 → (0x01 << 8) | 0x2C = 300ms
      g_step_ms = ((uint16_t)g_uart_rx_buf[4] << 8) | g_uart_rx_buf[3];

      //OLED和串口同步更新
      Sync_Status();
    }

    HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);   // 重新挂接收
  }
}

//发送可读字符串，内容与OLED完全一致
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
