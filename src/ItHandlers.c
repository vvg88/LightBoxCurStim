#include "main.h"
#include "CommHandlers.h"

extern __IO uint16_t StimCount;
extern __IO uint8_t StimIndex;
extern __IO uint8_t UsedStimTabItemsNum;
extern TStimTabItem * pUsedStimTab;
extern __IO bool IsActualRespFromPeriph;
extern __IO bool IsStimBiPhase;
extern int16_t CurrentAmpl;

uint8_t i = 0, j = 0, k = 0, l = 0;
// Счетчик импульсов в трейне
uint8_t impCntr = 0;
// Признак первого импульса в трейне
bool isFirstImp = true;

// Обработчик вызывается в начале каждого импульса в трейне
void TIM8_TRG_COM_IRQHandler(void)
{
	TIM8->EGR |= TIM_EGR_UG;			// Сгенерировать событие обновления TIM8
	TIM8->BDTR |= TIM_BDTR_MOE;		// Разрешить выходы каналов 2 и 3
	TIM_ClearITPendingBit(TIM8, TIM_IT_Trigger);
	if (isFirstImp)								// При первом импульсе поменять выходной сигнал триггера
	{															// таймера TIM1
//		isFirstImp = false;
		TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC2Ref);
		TIM8->DIER &= ~TIM_DIER_TDE;		// Запретить генерацию DMA запросов
		while ((SPI1->SR & SPI_SR_TXE) != SPI_SR_TXE);
		SPI1->DR = 0;
	}
	if (IsStimBiPhase)	// Для бифазного стимула
	{
		SetStimAmpl(pUsedStimTab[StimIndex].secAmp, false, false);									// Установить амплитуду 2-го импульса
		MODIFY_REG(DAC->CR, DAC_CR_TSEL1_0, DAC_CR_TSEL1_1 | DAC_CR_TSEL1_2);	// Сменить триггер ЦАП (линия EXTI9)
//		if ((pUsedStimTab[StimIndex].outNum & 0x80) == 0)
//			MODIFY_REG(TIM8->CR2, TIM_CR2_MMS, TIM_CR2_MMS_0 | TIM_CR2_MMS_2);	// Установить выходом триггера сигнал OC2REF, чтобы амп-да установилась после СС2
//		else
//			MODIFY_REG(TIM8->CR2, TIM_CR2_MMS, TIM_CR2_MMS_1 | TIM_CR2_MMS_2);	// Установить выходом триггера сигнал OC3REF, чтобы амп-да установилась после СС3
	}
	i++;
//	TIM1->CNT = 0;		// Установить счетчик в 0, чтобы для отсчета второго и последующих периодов импульсов в трейне оба таймера (TIM1 и TIM8)
}										// начинали отсчет с 0, иначе TIM1 запаздывает на 1 цикл и период увеличивается на 1 мс.

// Обработчик вызывается после формирования каждого импульса
void TIM8_CC_IRQHandler(void)
{
	register uint16_t tim8sr = TIM8->SR;
	if (tim8sr & TIM_SR_CC1IF)		// Если прерывание от СС1
	{
		TIM8->CR1 &= ~TIM_CR1_CEN;			// Остановить таймер
		TIM8->BDTR &= ~TIM_BDTR_MOE;		// Запретить выходы каналов 2 и 3
		SetStimAmpl(-1, true, true);					// Установить амп-ду 0,01 мА, чтобы выключить регулятор тока
		SetStimAmpl(CurrentAmpl, false, true);// Установить амп-ду для следующего импульса
		if (IsStimBiPhase)							// Для бифазного стимула
		{
			SetStimAmpl(pUsedStimTab[StimIndex].firstAmp, false, true);		// Установить амп-ду 1-го имп-са
			MODIFY_REG(DAC->CR, DAC_CR_TSEL1, DAC_CR_TSEL1_0);				// Вернуть триггер ЦАП (TIM8_TRGO)
			//MODIFY_REG(TIM8->CR2, TIM_CR2_MMS, TIM_CR2_MMS_0);			// Установить выходом триггера сигнал TIM8_EN
		}
		if (impCntr++ == TIM1->RCR)			// После последнего импульса сгенерировать событие Update TIM1, чтобы избежать формирования лишнего импульса
			TIM1->EGR |= TIM_EGR_UG;
		j++;
	}
	else
	{
		if ((tim8sr & TIM_SR_CC2IF) || (tim8sr & TIM_SR_CC3IF))		// Если прерывание от СС2 или СС3
		{
			GPIOB->BSRR |= GPIO_BSRR_BS8;			// Установить сигнал установки амплитуды 2-ой части бифазного стимула
			SET_AMPLITUDE_RANGE(pUsedStimTab[StimIndex].secAmp);	// Уст-ть диапазон для 2-го импульса бифазного стимула
			//SetStimAmpl(pUsedStimTab[StimIndex].secAmp, true, true);
		}
	}
	TIM_ClearITPendingBit(TIM8, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3);
	GPIOB->BRR |= GPIO_BRR_BR8;			// Сбросить сигнал установки амплитуды 2-ой части бифазного стимула
}

// Обработчик вызывается при завершении трейна
void TIM1_UP_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	TIM1->CR1 &= ~TIM_CR1_CEN;
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Enable);
	SetStimAmpl(-1, true, true);
	isFirstImp = true;
	impCntr = 0;
	TIM8->DIER |= TIM_DIER_TDE;		// Разрешить генерацию DMA запросов
	
	if (IsStimBiPhase)			// Для бифазного стимула
	{
		IsStimBiPhase = false;
		TIM_ITConfig(TIM8, TIM_IT_CC2 | TIM_IT_CC3, DISABLE);		// Запретить прерывания от СС2 и СС3
	}
	
	if (++StimIndex >= UsedStimTabItemsNum)		// Изменить индекс текущего элемента
	{
		StimIndex = 0;
		TIM5->CR1 &= ~TIM_CR1_CEN;		// Остановить TIM5
		EXTI->IMR |= EXTI_Line12;			// Разрешить вход прерывания
	}
	InitStim();						// Инициализировать новый стимул
	if (StimCount != 0)		// Если число стимулов не бесконечно (задается 0)
	{
		if (--StimCount == 0)		// Декрементировать счетчик стимулов и если он = 0,
		{
			StopStim();						// остановить стимуляцию
		}
	}
	k++;
}

// 
void EXTI15_10_IRQHandler(void)
{
	TIM5->EGR |= TIM_EGR_UG;
	TIM5->CR1 |= TIM_CR1_CEN;
	/* Запретить вход внешнего прерывания */
	EXTI->IMR &= ~EXTI_Line12;
	/* Очистить флаг прерывания */
	EXTI->PR |= EXTI_PR_PR0;
}

/* Счетчик для отсчета периода измерения напряжения на накопительном конденсаторе */
__IO uint8_t HighVoltMeasureCntr = 20;

// Обработчик прерывания от TIM6. Опрос вилочкового стимулятора каждые 30 мс
// и измерение и контроль напряжения на конденсаторе раз в 1 с
void TIM6_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
	
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);			// Запустить преобразование
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);		// Ожидать завершения преобразования
	uint16_t voltValue = ADC_GetConversionValue(ADC1) * 3.3 / 4096.0 * (9900000.0 + 75000.0) / 75000.0;
	
	if (voltValue > 305)
		HIGH_VOLTAGE(DISABLE);
	else
	{
		if (voltValue < 285)
			HIGH_VOLTAGE(ENABLE);
	}
	
	if (++HighVoltMeasureCntr == 34)
	{
		EnQueue(HIGH_VOLT_CODE, voltValue);
		HighVoltMeasureCntr = 0;
	}
	if ((pUsedStimTab[StimIndex].outNum & 0x7F) == 0)
		SendCommToPeriph(FORK_STIM_COMM_ADR, (pUsedStimTab[StimIndex].outNum & 0x80) == 0 ? LEFT_LED_ON : RIGHT_LED_ON);
	else
		SendCommToPeriph(FORK_STIM_COMM_ADR, ALL_LEDS_OFF);
}

// Получение ответа от USART2. Ответ от вилочкового стимулятора
void USART2_IRQHandler(void)
{
	uint16_t responseData = USART_ReceiveData(USART2);
	USART_ClearFlag(USART2, USART_FLAG_RXNE);
	if (IsActualRespFromPeriph)
	{
		StopDelay();
		IsActualRespFromPeriph = false;
		
		if ((responseData & 0x1F) != 0)
			EnQueue(FORK_STIM_CODE, responseData);
	}
	else
		IsActualRespFromPeriph = true;
}


/**
  * @brief  Обработчик прерывания от АЦП2.
	* 				Используется для измерения тока стимула
  * @param  None
  * @retval None
  */
void ADC1_2_IRQHandler(void)
{
	//uint16_t stimCurVal;
	ADC_ClearFlag(ADC2, ADC_FLAG_JEOC | ADC_FLAG_EOC | ADC_FLAG_JSTRT);
	if (isFirstImp)			// Определить амплитуду только для первого импульса
	{
		register uint16_t stimCurVal = (uint16_t)((ADC2->JDR1 * V_REF / 4096.0) / (GetMaxAmp(&pUsedStimTab[StimIndex]) > 0 ? _27_OHM : 300.0) * 10000);
		EnQueue(STIM_CUR_CODE, stimCurVal | (StimIndex << 11));
		isFirstImp = false;
	}
}

// Обработчик прерывания от переполнения системного таймера. Останавливает таймер
void SysTick_Handler(void)
{
	StopDelay();
	SysTick->CTRL &= ~SysTick_CTRL_COUNTFLAG_Msk;
}
