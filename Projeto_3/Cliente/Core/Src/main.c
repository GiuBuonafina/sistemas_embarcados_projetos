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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  CLIENTE_IDLE,              // esperando o clique do operador
  CLIENTE_AGUARDA_CONTADOR,  // comando enviado, esperando 1 byte (contador)
  CLIENTE_AGUARDA_TABELA,    // contador recebido, esperando 100 bytes (tabela) por DMA
  CLIENTE_PISCANDO,          // LED piscando a 1 Hz, uma vez por basculada
  CLIENTE_ENVIA_RELATORIO,   // mandar o relatorio ao PC
  CLIENTE_ENVIA_TABELA       // relatorio enviado, falta mandar a tabela ao PC
} EstadoCliente;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CMD_COLETA                 0x5A   // comando combinado com a estacao de campo
#define TABELA_TAM                 100    // tamanho fixo da tabela (igual no servidor)
#define CHUVA_POR_BASCULADA_CENT   25     // 0,25 mm = 25 centesimos de mm
#define LIMITE_CHUVA_CENT          200    // 2,00 mm: a partir daqui suspende a irrigacao
#define MEIO_PERIODO_MS            500    // 1 Hz = 500 ms ligado + 500 ms desligado
#define TIMEOUT_COLETA_MS          2000   // se a estacao nao responder em 2 s, desiste
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
volatile EstadoCliente estado_cliente = CLIENTE_IDLE;

uint8_t  comando_tx = CMD_COLETA;
volatile uint8_t contador_recebido = 0;        // basculadas enviadas pela estacao
uint8_t  tabela_recebida[TABELA_TAM];          // tabela de talhoes recebida por DMA

volatile uint8_t liberado_para_coletar = 1;    // 0 enquanto uma coleta esta em andamento
volatile uint32_t tempo_inicio_coleta = 0;

uint16_t piscadas_feitas = 0;
uint32_t tempo_ultimo_toggle = 0;
uint8_t  led_ligado = 0;

char msg_relatorio[96];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Interrupcao externa: o operador pediu uma coleta (botao B1) */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13 && liberado_para_coletar)
  {
    liberado_para_coletar = 0;                       // bloqueia novos cliques
    tempo_inicio_coleta = HAL_GetTick();
    estado_cliente = CLIENTE_AGUARDA_CONTADOR;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&contador_recebido, 1);  // prepara para ouvir a resposta
    HAL_UART_Transmit_IT(&huart1, &comando_tx, 1);                   // envia 0x5A para a estacao
  }
}

/* Terminou uma RECEPCAO na USART1 (por interrupcao ou por DMA) */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (estado_cliente == CLIENTE_AGUARDA_CONTADOR)
    {
      // chegou o byte do contador; agora espera os 100 bytes da tabela por DMA
      estado_cliente = CLIENTE_AGUARDA_TABELA;
      HAL_UART_Receive_DMA(&huart1, tabela_recebida, TABELA_TAM);
    }
    else if (estado_cliente == CLIENTE_AGUARDA_TABELA)
    {
      // tabela completa: comeca a piscar o LED
      piscadas_feitas = 0;
      led_ligado = 0;
      tempo_ultimo_toggle = HAL_GetTick();
      estado_cliente = CLIENTE_PISCANDO;
    }
  }
}

/* Terminou uma TRANSMISSAO na USART2 (para o PC) */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2 && estado_cliente == CLIENTE_ENVIA_TABELA)
  {
    // o relatorio ja saiu; agora envia a tabela por DMA
    HAL_UART_Transmit_DMA(&huart2, tabela_recebida, TABELA_TAM);
    estado_cliente = CLIENTE_IDLE;
    liberado_para_coletar = 1;                       // libera para a proxima coleta
  }
}
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Mensagem inicial por POLLING: HAL_UART_Transmit espera terminar */
  const char banner[] =
    "\r\n=== Central de Irrigacao (NUCLEO-L476RG) ===\r\n"
    "Aperte o botao azul para coletar a chuva da estacao de campo.\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t *)banner, sizeof(banner) - 1, 200);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  /* ---- 1) LED piscando a 1 Hz, uma piscada por basculada (sem HAL_Delay) ---- */
	  if (estado_cliente == CLIENTE_PISCANDO)
	  {
	    if (contador_recebido == 0)
	    {
	      estado_cliente = CLIENTE_ENVIA_RELATORIO;        // nao choveu: nada a piscar
	    }
	    else if ((HAL_GetTick() - tempo_ultimo_toggle) >= MEIO_PERIODO_MS)
	    {
	      tempo_ultimo_toggle = HAL_GetTick();
	      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	      led_ligado = !led_ligado;

	      if (led_ligado == 0)                             // completou um ciclo: ligou e desligou
	      {
	        piscadas_feitas++;
	        if (piscadas_feitas >= contador_recebido)
	        {
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	          estado_cliente = CLIENTE_ENVIA_RELATORIO;
	        }
	      }
	    }
	  }

	  /* ---- 2) Relatorio ao PC (USART2, por interrupcao) ---- */
	  else if (estado_cliente == CLIENTE_ENVIA_RELATORIO)
	  {
	    uint32_t chuva_cent = (uint32_t)contador_recebido * CHUVA_POR_BASCULADA_CENT;
	    snprintf(msg_relatorio, sizeof(msg_relatorio),
	             "Basculadas = %03u | Chuva = %lu.%02lu mm | Irrigacao: %s\r\n",
	             (unsigned)contador_recebido,
	             (unsigned long)(chuva_cent / 100), (unsigned long)(chuva_cent % 100),
	             (chuva_cent >= LIMITE_CHUVA_CENT) ? "SUSPENSA" : "LIBERADA");
	    estado_cliente = CLIENTE_ENVIA_TABELA;             // a tabela sai no TxCpltCallback
	    HAL_UART_Transmit_IT(&huart2, (uint8_t *)msg_relatorio, strlen(msg_relatorio));
	  }

	  /* ---- 3) Timeout: a estacao nao respondeu (fio solto, placa desligada...) ---- */
	  else if ((estado_cliente == CLIENTE_AGUARDA_CONTADOR || estado_cliente == CLIENTE_AGUARDA_TABELA)
	           && (HAL_GetTick() - tempo_inicio_coleta) > TIMEOUT_COLETA_MS)
	  {
	    HAL_UART_AbortReceive(&huart1);
	    estado_cliente = CLIENTE_IDLE;
	    liberado_para_coletar = 1;
	    static const char erro[] = "ERRO: estacao de campo nao respondeu. Confira TX/RX/GND.\r\n";
	    HAL_UART_Transmit_IT(&huart2, (uint8_t *)erro, sizeof(erro) - 1);
	  }
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_CENTRAL_GPIO_Port, LED_CENTRAL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_CENTRAL_Pin */
  GPIO_InitStruct.Pin = LED_CENTRAL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_CENTRAL_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
