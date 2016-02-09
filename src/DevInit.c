#include "stm32f10x.h"

void DevInit(void);
void RCC_Init(void);
void GPIOinit(void);
void USARTInit(void);
void TIMsInit(void);
void NVICinit(void);

void DevInit()
{
	RCC_Init();
	GPIOinit();
	USARTInit();
	TIMsInit();
	NVICinit();
}

/**
  * @brief  Инициализация тактирования необходимых периферийных модулей.
  * @param  None
  * @retval None
  */
void RCC_Init(void)
{
	/* Установка тактирования порта А, порта В и С, таймеров 1 и 8, АЦП1, АЦП2, АЦП3 */
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1 | RCC_APB2Periph_TIM8 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOC
												 /*RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2 | RCC_APB2Periph_ADC3*/, ENABLE);
	/* Установить частоту тактирования АЦП 12 МГц */
//	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	
	/* Установить частоту APB1 6 МГц */
	RCC_PCLK1Config(RCC_HCLK_Div8);
	
	/* Установка тактирования таймера 3, 4 и 2 */
	/*RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 |  | RCC_APB1Periph_TIM5 |
												 RCC_APB1Periph_DAC | RCC_APB1Periph_TIM6 | RCC_APB1Periph_USART2 | RCC_APB1Periph_SPI2, ENABLE);*/
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
	
	/* Вход 1 таймера 1 */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_8;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GpioInitStruct);
	
	/* Выходы 2 и 3 таймера 8 */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GpioInitStruct);
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

/**
  * @brief  Инициализация таймеров
  * @param  None
  * @retval None
  */
void TIMsInit(void)
{
	/* Базовая структура инициализации */
	TIM_TimeBaseInitTypeDef  TimeBaseStruct;
	/* Структура инициализации PWM */
	TIM_OCInitTypeDef  OCInitStruct;
	/* Структура инициализации для входа захвата */
	TIM_ICInitTypeDef ICInitStruct;
	/* Структура инициализации регистра BDTR TIM8 */
	TIM_BDTRInitTypeDef BdtrInitStruct;
	
	// TIM1 Отсчитывает период импульсов в трейне
	
	TimeBaseStruct.TIM_Prescaler = 48e3;
	TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseStruct.TIM_Period = 10;
	TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseStruct.TIM_RepetitionCounter = 0;  // Число повторов определяет число импульсов в трейне
	TIM_TimeBaseInit(TIM1, &TimeBaseStruct);
	
	// Канал 1 исп-ся как вход запуска таймера
	ICInitStruct.TIM_Channel = TIM_Channel_1;
	ICInitStruct.TIM_ICFilter = 0;
	ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
	ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInit(TIM1, &ICInitStruct);
	
	// Канал 2 исп-ся для отсчета периода импульсов в трейне
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	OCInitStruct.TIM_Pulse = 10;
	TIM_OC2Init(TIM1, &OCInitStruct);
	
	/* Установка для таймера TIM1 однократной перезагрузки */
	TIM_SelectOnePulseMode(TIM1, TIM_OPMode_Single);
	
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Enable);
	// Сигнал запуска со входа 1
	TIM_SelectInputTrigger(TIM1, TIM_TS_TI1F_ED); 
	// Сигнал разрешения - выход триггера
	TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Trigger);		
	// Установить бит MSM для синхронизации с TIM8
	TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);	
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	// Разрешить прерывание от события Update
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	
	// TIM1
	
	// TIM8 Используется для отсчета длительности импульсов
	
	TimeBaseStruct.TIM_Prescaler = 48;
	TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseStruct.TIM_Period = 0xFFFF;
	TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM8, &TimeBaseStruct);
	
	// Настройка каналов ШИМ.
	// Канал ШИМ2 исп-ся в формировании положительных импульсов
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;//TIM_OutputState_Disable;
	OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;//TIM_OCPolarity_High;
	OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Set;
	OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Set;
	OCInitStruct.TIM_Pulse = 100;
	TIM_OC2Init(TIM8, &OCInitStruct);
	
	// Канал ШИМ3 исп-ся в формировании отрицательных импульсов
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;//TIM_OutputState_Disable;
	OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;//TIM_OCPolarity_High;
	OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	OCInitStruct.TIM_Pulse = 100;
	TIM_OC3Init(TIM8, &OCInitStruct);
	
	// Канал 1 используется для генерации события, при котором останавливается таймер	
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	OCInitStruct.TIM_Pulse = 200;
	TIM_OC1Init(TIM8, &OCInitStruct);
	
	// Инициализация регистра BDTR
	TIM_BDTRStructInit(&BdtrInitStruct);
	BdtrInitStruct.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRConfig(TIM8, &BdtrInitStruct);
	
//	TIM_SelectOnePulseMode(TIM8, TIM_OPMode_Single);
	// В кач-ве сигнала триггера исп-ся сигнал разрешения TIM1 
	// при запуске, а потом исп-ся сигнал OC1 для формирования трейнов
	TIM_SelectInputTrigger(TIM8, TIM_TS_ITR0);
	
	TIM_SelectSlaveMode(TIM8, TIM_SlaveMode_Trigger);
	
	TIM_ClearFlag(TIM8, TIM_FLAG_Trigger | TIM_FLAG_CC1);
	// Разрешение прерываний
	TIM_ITConfig(TIM8, TIM_IT_Trigger | TIM_IT_CC1, ENABLE);
	
//	TIM_CtrlPWMOutputs(TIM8, ENABLE);
	
	// TIM8
}

/**
  * @brief  Инициализация прерываний
  * @param  None
  * @retval None
  */
void NVICinit(void)
{
	NVIC_InitTypeDef NVICinitStruct;
	// Разрешить прерывание от обновления TIM1
	NVICinitStruct.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVICinitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от триггера TIM8
	NVICinitStruct.NVIC_IRQChannel = TIM8_TRG_COM_IRQn;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от OC1 TIM8
	NVICinitStruct.NVIC_IRQChannel = TIM8_CC_IRQn;
	NVIC_Init(&NVICinitStruct);
}
