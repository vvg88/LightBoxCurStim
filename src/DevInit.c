#include "stm32f10x.h"

void DevInit(void);
void RCC_Init(void);
void GPIOinit(void);
void USARTInit(void);

void DevInit()
{
	RCC_Init();
	GPIOinit();
	USARTInit();
}

/**
  * @brief  Инициализация тактирования необходимых периферийных модулей.
  * @param  None
  * @retval None
  */
void RCC_Init(void)
{
	/* Установка тактирования порта А, порта В и С, таймеров 1 и 8, АЦП1, АЦП2, АЦП3 */
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1
												 /*RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2 | RCC_APB2Periph_ADC3*/, ENABLE);
	/* Установить частоту тактирования АЦП 12 МГц */
//	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	
	/* Установить частоту APB1 24 МГц */
// 	RCC_PCLK1Config(RCC_HCLK_Div2);
	/* Установить частоту APB1 6 МГц */
	RCC_PCLK1Config(RCC_HCLK_Div8);
	
	/* Установка тактирования таймера 3, 4 и 2 */
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM5 |
//												 RCC_APB1Periph_DAC | RCC_APB1Periph_TIM6 | RCC_APB1Periph_USART2 | RCC_APB1Periph_SPI2*/, ENABLE);
	/* Разрешить тактирование DMA1 */
// 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* Выбор источника сигнала на линии MCO */
	RCC_MCOConfig(RCC_MCO_SYSCLK);
}

/**
  * @brief  Инициализация портов ввода/вывода (GPIO).
  * @param  None
  * @retval None
  */
void GPIOinit(void)
{
	GPIO_InitTypeDef GpioInitStruct;
	
	/* Configure USART1 Rx as input floating */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_10;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GpioInitStruct);
	
	/* Configure USART1 Tx as alternate function push-pull (PA9) */
	/* Вывод системной тактовой частоты PA8 */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;
	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GpioInitStruct);
}

/**
  * @brief  Инициализация главного USART (USART1 используется для связи с главным контроллером).
  * @param  None
  * @retval None
  */
void USARTInit(void)
{
	USART_InitTypeDef UsartInitStruct;
	
	/* Initialisation of USART init-structure. Use defalt settings exept baud rate, length of word and ParityBit */
	USART_StructInit(&UsartInitStruct);
	UsartInitStruct.USART_BaudRate = 25000;
	UsartInitStruct.USART_WordLength = USART_WordLength_9b;
	UsartInitStruct.USART_Parity = USART_Parity_No;
	
	/* Initialisation of USARTx */
	USART_Init(USART1, &UsartInitStruct);
	
	/* Enabling of USARTx */
	USART_Cmd(USART1, ENABLE);
}
