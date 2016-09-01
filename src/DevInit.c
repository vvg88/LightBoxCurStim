#include "stm32f10x.h"

void DevInit(void);
void RCC_Init(void);
void GPIOinit(void);
void USARTInit(void);
void TIMsInit(void);
void NVICinit(void);
void AdcInit(void);
void DacInit(void);
void SPIInit(void);
void DMAInit(void);
void EXTIInit(void);

extern uint8_t SynchroSignal;

void DevInit()
{
	RCC_Init();
	GPIOinit();
	USARTInit();
	TIMsInit();
	NVICinit();
	AdcInit();
	DacInit();
	SPIInit();
	DMAInit();
	EXTIInit();
} 

/**
  * @brief  Инициализация тактирования необходимых периферийных модулей.
  * @param  None
  * @retval None
  */
void RCC_Init(void)
{
	/* Установка тактирования порта А, порта В и С, таймеров 1 и 8, АЦП1, АЦП2, АЦП3 */
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1 | RCC_APB2Periph_TIM8 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOC |
												 RCC_APB2Periph_GPIOB | RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2 /*| RCC_APB2Periph_SPI1 | RCC_APB2Periph_ADC3*/, ENABLE);
	/* Установить частоту тактирования АЦП 12 МГц */
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	
	/* Установить частоту APB1 6 МГц */
	RCC_PCLK1Config(RCC_HCLK_Div8);
	
	/* Установка тактирования таймера 6 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC | RCC_APB1Periph_TIM6 | RCC_APB1Periph_USART2 | RCC_APB1Periph_TIM5 | RCC_APB1Periph_SPI2, ENABLE);
	/* Разрешить тактирование DMA1 */
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
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
//	GpioInitStruct.GPIO_Pin = GPIO_Pin_8;
//	GpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//	GPIO_Init(GPIOA, &GpioInitStruct);
	
	/* Выходы 2 и 3 таймера 8 */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GpioInitStruct);
	
	/* Выход CUR-STIM */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOB, &GpioInitStruct);
	
	/* Выходы CUR-OUT2-EN, CUR-OUT1-EN, CUR-HIGH-RANGE */
	GPIO_SetBits(GPIOC, GPIO_Pin_9 | GPIO_Pin_10);		// Установить неактивное состояние на линиях
	GPIO_ResetBits(GPIOC, GPIO_Pin_6);
	GpioInitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_9 | GPIO_Pin_10;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GpioInitStruct);
	
	/* Выход CUR-VHIGH-ON */
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GpioInitStruct.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOB, &GpioInitStruct);
	
	///////////////////////////////
//	GpioInitStruct.GPIO_Pin = GPIO_Pin_9;
//	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_Init(GPIOC, &GpioInitStruct);
	///////////////////////////////
	
	/* Выводы USART2 - опрос вилки */
	/* Configure USART2 Rx as input floating */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_3;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GpioInitStruct);
	/* Configure USART1 Tx as alternate function push-pull (PA9) */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_2;
	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GpioInitStruct);
	
	/* Настройка аналоговых входов/выходов */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;		// Линии CUR-HV-SENCE | CUR-STIM-SENCE
  GpioInitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GpioInitStruct);
	GpioInitStruct.GPIO_Pin = GPIO_Pin_0;									// Линии CUR-DAC-OUT
	GPIO_Init(GPIOA, &GpioInitStruct);
	
	// Выход сигнала установки амплитуды второй части бифазного стимула
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);
	GpioInitStruct.GPIO_Pin = GPIO_Pin_8;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GpioInitStruct);
	
	/*GpioInitStruct.GPIO_Pin = GPIO_Pin_11;
	GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GpioInitStruct);*/
	
	/* Настройка входа внешнего прерывания
		 (Используется для установки второй части бифазного стимула) */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_9;
  GpioInitStruct.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOB, &GpioInitStruct);
	/* Connect EXTI9 Line to PB.09 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);
	
	/* Настройка входа внешнего прерывания
	(Используется для запуска отсчета стимулов) */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_12;
  GpioInitStruct.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOC, &GpioInitStruct);
	/* Connect EXTI12 Line to PС.12 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource12);
	
	/* Выход сброса микросхемы watchdog CUR-WDI */
	GpioInitStruct.GPIO_Pin = GPIO_Pin_1;
	GpioInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
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
		
	/* Initialisation of USART init-structure. Use defalt settings exept baud rate, length of word */
	USART_StructInit(&UsartInitStruct);
	UsartInitStruct.USART_BaudRate = 1200;
	UsartInitStruct.USART_WordLength = USART_WordLength_8b;
	
	/* Initialisation of USART2 */
	USART_Init(USART2, &UsartInitStruct);
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
	
	// TIM5 Отсчитывает интервал до следующего стимула
	/***************** Инициализация таймера TIM5 *******************/
	
	TimeBaseStruct.TIM_Period = 20;
  TimeBaseStruct.TIM_Prescaler = 12e3 - 1;
  TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
  TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseStruct.TIM_RepetitionCounter = 0;
	/* Инициализация таймера */
  TIM_TimeBaseInit(TIM5, &TimeBaseStruct);
	
	TIM_ARRPreloadConfig(TIM5, ENABLE);
	/* Выходным сигналом триггера установить сигнал Update */
	TIM_SelectOutputTrigger(TIM5, TIM_TRGOSource_Update);
	
	/***************** Инициализация таймера TIM5 *******************/
	
	// TIM1 Отсчитывает период импульсов в трейне
	/***************** Инициализация таймера TIM1 *******************/
	TimeBaseStruct.TIM_Prescaler = 47;		//48e3;47999;
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
	
	// Канал 4 исп-ся для 
	/*OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	OCInitStruct.TIM_Pulse = 0xF;//0xFFFF;
	TIM_OC4Init(TIM1, &OCInitStruct);*/
	
	/* Установка для таймера TIM1 однократной перезагрузки */
	TIM_SelectOnePulseMode(TIM1, TIM_OPMode_Single);
	// Сигнал разрешения - выход триггера
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Enable);
	// Сигнал запуска от тайймера 5
	TIM_SelectInputTrigger(TIM1, TIM_TS_ITR0); //TIM_TS_TI1F_ED
	
	TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Trigger);		
	// Установить бит MSM для синхронизации с TIM8
	TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);	
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	/***************** Инициализация таймера TIM1 *******************/
	
	// TIM8 Используется для отсчета длительности импульсов
	/***************** Инициализация таймера TIM8 *******************/
	TimeBaseStruct.TIM_Prescaler = 47;
	TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseStruct.TIM_Period = 0xFFFF;
	TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM8, &TimeBaseStruct);
	
	// Настройка каналов ШИМ.
	// Канал ШИМ2 исп-ся в формировании положительных импульсов
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;//TIM_OutputState_Enable;
	OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;//TIM_OCPolarity_High;
	OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Set;
	OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Set;
	OCInitStruct.TIM_Pulse = 100;
	TIM_OC2Init(TIM8, &OCInitStruct);
	
	// Канал ШИМ3 исп-ся в формировании отрицательных импульсов
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;//TIM_OutputState_Enable;
	OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;//TIM_OCPolarity_High;
	OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	OCInitStruct.TIM_Pulse = 100;
	TIM_OC3Init(TIM8, &OCInitStruct);
	
	// Канал 1 используется для генерации события, при котором останавливается таймер	
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_High;
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
	
	// Канал ШИМ4 исп-ся для запуска измерения тока стимула
	OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;//TIM_OutputState_Disable;
	OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	OCInitStruct.TIM_Pulse = 0;
	TIM_OC4Init(TIM8, &OCInitStruct);
	// Установить выходным триггером сигнал OC4Ref для запуска измерения тока стимула
	TIM_SelectOutputTrigger(TIM8, TIM_TRGOSource_Enable);
	/***************** Инициализация таймера TIM8 *******************/
	
	/***************** Инициализация таймера TIM6 *******************/
	TimeBaseStruct.TIM_Period = 30000;
  TimeBaseStruct.TIM_Prescaler = 12 - 1;
  TimeBaseStruct.TIM_ClockDivision = 0;
  TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseStruct.TIM_RepetitionCounter = 0;
	/* Инициализация таймера */
  TIM_TimeBaseInit(TIM6, &TimeBaseStruct);
	/***************** Инициализация таймера TIM6 *******************/
	
	/***************** Инициализация таймера TIM5 *******************/
	
//	TimeBaseStruct.TIM_Period = 20;
//  TimeBaseStruct.TIM_Prescaler = 12e3 - 1;
//  TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
//  TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
//	TimeBaseStruct.TIM_RepetitionCounter = 0;
//	/* Инициализация таймера */
//  TIM_TimeBaseInit(TIM5, &TimeBaseStruct);
//	
//	TIM_ARRPreloadConfig(TIM5, ENABLE);
//	/* Выходным сигналом триггера установить сигнал Update */
//	TIM_SelectOutputTrigger(TIM5, TIM_TRGOSource_Update);
	
	/***************** Инициализация таймера TIM5 *******************/
}

/**
  * @brief  Инициализация прерываний
  * @param  None
  * @retval None
  */
void NVICinit(void)
{
	NVIC_InitTypeDef NVICinitStruct;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
		
	// Разрешить прерывание от обновления TIM1
	NVICinitStruct.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 3;
	NVICinitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от триггера TIM8
	NVICinitStruct.NVIC_IRQChannel = TIM8_TRG_COM_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от OC1 TIM8
	NVICinitStruct.NVIC_IRQChannel = TIM8_CC_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от TIM6
	NVICinitStruct.NVIC_IRQChannel = TIM6_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от USART2
	NVICinitStruct.NVIC_IRQChannel = USART2_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVICinitStruct);
	
	// Разрешить прерывание от ADC2
	NVICinitStruct.NVIC_IRQChannel = ADC1_2_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 4;
	NVIC_Init(&NVICinitStruct);
	
	NVICinitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVICinitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVICinitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVICinitStruct);
}

void AdcInit(void)
{
	ADC_InitTypeDef AdcInitStruct;
	
	/*** Инициализация АЦП 1 (измерение высокого напряжения)***/
	AdcInitStruct.ADC_Mode = ADC_Mode_Independent;
  AdcInitStruct.ADC_ScanConvMode = DISABLE;
  AdcInitStruct.ADC_ContinuousConvMode = DISABLE;
  AdcInitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  AdcInitStruct.ADC_DataAlign = ADC_DataAlign_Right;
  AdcInitStruct.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &AdcInitStruct);
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
	
	ADC_Cmd(ADC1, ENABLE);
	
	/* Enable ADC1 reset calibration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));
  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
	/*** Инициализация АЦП 1 ***/
	
	/*** Инициализация АЦП 2 (измерение тока стимула)***/
	ADC_Init(ADC2, &AdcInitStruct);
	
	/* Инициализация запуска преобразования по событию канала ШИМ4 таймера 8 */
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_9, 1, ADC_SampleTime_7Cycles5);
	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_Ext_IT15_TIM8_CC4);
	GPIO_PinRemapConfig(GPIO_Remap_ADC2_ETRGINJ, ENABLE);		// Разрешить запуск от TIM8_CC4
	ADC_ExternalTrigInjectedConvCmd(ADC2, ENABLE);
	
	ADC_Cmd(ADC2, ENABLE);
	
	/* Enable ADC2 reset calibration register */   
  ADC_ResetCalibration(ADC2);
  /* Check the end of ADC2 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC2));
  /* Start ADC2 calibration */
  ADC_StartCalibration(ADC2);
  /* Check the end of ADC2 calibration */
  while(ADC_GetCalibrationStatus(ADC2));
	/*** Инициализация АЦП 2 ***/
}

// Инициализация ЦАП, кот. исп-ся для установки амплитуды
void DacInit(void)
{
	DAC_InitTypeDef DAC_InitStructure;
	
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T8_TRGO;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable/*DAC_OutputBuffer_Enable*/;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);

  /* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is 
     automatically connected to the DAC converter. */
  DAC_Cmd(DAC_Channel_1, ENABLE);
}

// Иницицализация SPI1
void SPIInit(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 5;
  SPI_Init(SPI2, &SPI_InitStructure);
		
	SPI_Cmd(SPI2, ENABLE);
}

// Инициализация DMA для передачи синхросигнала
void DMAInit(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	DMA_DeInit(DMA2_Channel2);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SPI2->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)(&SynchroSignal);
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA2_Channel2, &DMA_InitStructure);
	
	DMA_Cmd(DMA2_Channel2, ENABLE);
}

void EXTIInit(void)
{
	EXTI_InitTypeDef EXTIInitStruc;
	
	EXTIInitStruc.EXTI_Line = EXTI_Line12;
	EXTIInitStruc.EXTI_LineCmd = ENABLE;
	EXTIInitStruc.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTIInitStruc.EXTI_Trigger = EXTI_Trigger_Rising;
	
	EXTI_Init(&EXTIInitStruc);
	EXTIInitStruc.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTIInitStruc);
}
