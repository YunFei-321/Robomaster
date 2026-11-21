/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (软件 I2C 版 + 结构体解析 + DMA反馈)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "OLED.h"          /* ★ 软件 I2C 驱动 */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* ★ 作业要求2：自行定义结构体接收数据包 */
typedef struct {
    uint8_t Header;    // 帧头 AA
    uint8_t Run;       // 运行标志
    uint8_t Dir;       // 方向标志
    uint8_t Step_L;    // 间隔低字节
    uint8_t Step_H;    // 间隔高字节
    uint8_t Checksum;  // 校验和
} FlowLight_Packet_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t  g_is_running = 1;
uint8_t  g_direction  = 0;
uint16_t g_step_ms    = 300;
uint8_t  g_current_led_idx = 0;
uint8_t  g_last_led_idx    = 0;
uint32_t g_tick_count      = 0;

uint8_t  g_uart_rx_buf[6];      // 串口接收缓冲
uint8_t  g_uart_tx_buf[64];     // 串口发送缓冲
uint32_t g_oled_tick = 0;
volatile uint8_t g_need_refresh = 1; /* ★ 软件I2C防卡死：OLED刷新标志 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Print_Status_To_UART(void);
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
  /* USER CODE BEGIN 2 */

  // /* ★ 所有初始化完成后，再读一次寄存器 */
  // uint32_t crl = GPIOB->CRL;
  // char dbg[64];
  // sprintf(dbg, "Final CRL=0x%08X PB0=%X PB6=%X PB7=%X\r\n",
  //         crl, (crl>>0)&0x0F, (crl>>24)&0x0F, (crl>>28)&0x0F);
  // HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 100);
  // HAL_Delay(3000);



  OLED_Init();             /* ★ 软件 I2C: 无参数 */
  /* ★ 强制修复 PB0 */
  GPIO_InitTypeDef fix = {0};
  fix.Pin   = GPIO_PIN_0;
  fix.Mode  = GPIO_MODE_OUTPUT_PP;
  fix.Pull  = GPIO_NOPULL;
  fix.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &fix);

  OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);

  HAL_UART_Transmit(&huart2, (uint8_t*)"HELLO\r\n", 7, 1000);
  Print_Status_To_UART();

  /* 开启串口中断接收, 每次等待接收 6 个字节 */
  HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 1. 流水灯逻辑（不受OLED影响） */
    if (g_is_running && HAL_GetTick() - g_tick_count >= g_step_ms) {
      g_tick_count = HAL_GetTick();

      switch (g_last_led_idx) {
      case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); break;
      case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); break;
      // case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); break;  LED_BLUE_GPIO_Port
      case 2: HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, GPIO_PIN_0, GPIO_PIN_RESET); break;
      default: break;
      }

      switch (g_current_led_idx) {
      case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); break;
      case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET); break;
      case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;
      default: break;
      }

      g_last_led_idx = g_current_led_idx;
      if (g_direction == 0) {
        g_current_led_idx = (g_current_led_idx + 1) % 3;
      } else {
        g_current_led_idx = (g_current_led_idx - 1 + 3) % 3;
      }

      g_need_refresh = 1; /* LED状态变了，标记需要刷新OLED */
    }

    /* ★ 2. OLED 独立刷新（解决软件I2C卡顿的核心） */
    if (g_need_refresh && (HAL_GetTick() - g_oled_tick >= 200)) {
      g_oled_tick = HAL_GetTick();
      g_need_refresh = 0;
      OLED_Show_Status(g_is_running, g_direction, g_step_ms, g_current_led_idx + 1);
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

/* ★ 作业要求2 & 3：结构体解析 + 校验和计算 */
void Process_Packet(FlowLight_Packet_t *pkt)
{
    uint8_t calc_check = pkt->Header + pkt->Run + pkt->Dir + pkt->Step_L + pkt->Step_H;

    /* 校验通过才执行 */
    if (pkt->Header == 0xAA && calc_check == pkt->Checksum)
    {
        g_is_running = pkt->Run;
        g_direction  = pkt->Dir;
        g_step_ms    = ((uint16_t)pkt->Step_H << 8) | pkt->Step_L;

        /* 限制间隔范围 100~2000ms */
        if (g_step_ms < 100) g_step_ms = 100;
        if (g_step_ms > 2000) g_step_ms = 2000;

        Print_Status_To_UART(); /* 作业要求4：反馈状态 */
        g_need_refresh = 1;     /* 标记刷新OLED */
    }
    /* 校验失败：什么都不做（作业要求3） */
}

/* 串口接收回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    FlowLight_Packet_t *pkt = (FlowLight_Packet_t *)g_uart_rx_buf;
    Process_Packet(pkt);  /* ★ 交给结构体处理函数 */

    HAL_UART_Receive_IT(&huart2, g_uart_rx_buf, 6); /* 重新挂接收 */
  }
}

/* ★ 作业要求4（选做）：使用 DMA 发送反馈，替代 HAL_UART_Transmit_IT */
void Print_Status_To_UART(void)
{
  char *state_str = g_is_running ? "RUN" : "STOP";
  char *dir_str   = (g_direction == 0) ? "FWD" : "REV";

  uint16_t len = sprintf((char*)g_uart_tx_buf,
                         "STATE=%s,DIR=%s,STEP=%d,LED=%d\r\n",
                         state_str, dir_str, g_step_ms, g_current_led_idx + 1);

  HAL_UART_Transmit_DMA(&huart2, g_uart_tx_buf, len); /* ★ DMA发送 */
}

/* 按键回调 */
/* 按键回调（带软件消抖） */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_key1_tick = 0;
  static uint32_t last_key2_tick = 0;

  if (GPIO_Pin == GPIO_PIN_12) {        /* KEY1 运行/暂停 */
    if (HAL_GetTick() - last_key1_tick > 200) {   /* ★ 200ms 内只响应一次 */
      last_key1_tick = HAL_GetTick();
      g_is_running = !g_is_running;
      Print_Status_To_UART();
      g_need_refresh = 1;
    }
  } else if (GPIO_Pin == GPIO_PIN_13) { /* KEY2 正/反向 */
    if (HAL_GetTick() - last_key2_tick > 200) {   /* ★ 200ms 内只响应一次 */
      last_key2_tick = HAL_GetTick();
      g_direction = !g_direction;
      Print_Status_To_UART();
      g_need_refresh = 1;
    }
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
