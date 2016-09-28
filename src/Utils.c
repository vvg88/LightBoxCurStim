//#include "Utils.h"
#include "main.h"

uint16_t CheckStimTab(const TStimTabItem * const pCheckedTab, uint8_t tabItemsNum);
void ExceptionsInit(void);
void BKPInit(void);

extern TStimTabItem StimTabOne[32];
extern TStimTabItem StimTabTwo[32];
extern TStimTabItem * pUsedStimTab;
extern __IO uint8_t UsedStimTabItemsNum;
//extern bool TabWasChanged;
extern __IO uint16_t StimCount;
extern ModuleStateType ModuleState;
extern int16_t currentStimAmp;

/* Причина последнего сбрса контроллера */
TStatus LastResReason;
/* Очередь событий. Представляет собой массив очередей (в каждой очереди хранятся события одного определенного приоритета) */
EventsQueueStruct EventsQueue[4];
/* Индекс текущего элемента таблицы стимуляции */
__IO uint8_t StimIndex = 0;
/* Признак бифазного стимула */
__IO bool IsStimBiPhase;
/* Байт с синхросигналом стимулятора */
uint8_t SynchroSignal = 0;

int16_t CurrentAmpl = 0;

// Инициализация активного режима
void ActiveModeInit(void)
{
	/* Запуск таймера опроса внешних модулей (вилка) и измерения напряжения на конденсаторе */
	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM6, ENABLE);
	
	/* Разрешить прерывание от приемника USART2 */
	USART_ClearFlag(USART2, USART_FLAG_RXNE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
}

// Проверка таблицы стимуляции
uint16_t CheckStimTab(const TStimTabItem * const pCheckedTab, uint8_t tabItemsNum)
{
	uint8_t nextIndx;
	uint16_t stimDurAdd;
	for (int i = 0; i < tabItemsNum; i++)
	{		
		if (!IS_AMPL_OK(pCheckedTab[i].firstAmp))
			return STIM_TABLE_ERROR(i, WRONG_FIR_AMPL, OVERVALUED);
		if (!IS_AMPL_OK(pCheckedTab[i].secAmp))
			return STIM_TABLE_ERROR(i, WRONG_SEC_AMPL, OVERVALUED);
		if (!IS_NUMOUT_OK(pCheckedTab[i].outNum))
			return STIM_TABLE_ERROR(i, WRONG_OUTPUT, OVERVALUED);
		if (!IS_DURATION_OK(pCheckedTab[i].firstDur))
			return STIM_TABLE_ERROR(i, WRONG_FIR_IMP_DURATION, pCheckedTab[i].firstDur > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED);
		if (!IS_DURATION_OK(pCheckedTab[i].secDur))
			return STIM_TABLE_ERROR(i, WRONG_SEC_IMP_DURATION, pCheckedTab[i].secDur > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED);
		if (!IS_PERIOD_OK(pCheckedTab[i].impPeriod))
			return STIM_TABLE_ERROR(i, WRONG_IMP_PERIOD, pCheckedTab[i].impPeriod > IMP_PERIOD_MAX ? OVERVALUED : UNDERVALUED);
		if (!IS_STIM_INTERV_OK(pCheckedTab[i].stimInterval))
			return STIM_TABLE_ERROR(i, WRONG_STIM_INTERVAL, pCheckedTab[i].stimInterval > STIM_INTERV_MAX ? OVERVALUED : UNDERVALUED);
		
		if ((pCheckedTab[i].impCnt > 0) && !IS_DUR_PERIOD_OK((pCheckedTab[i].firstDur + pCheckedTab[i].secDur), pCheckedTab[i].impPeriod)) //+ IMP_DURATION_ADD(pCheckedTab[i].firstAmp
			return STIM_TABLE_ERROR(i, WRONG_DUR_PERIOD, OVERVALUED);
		
		nextIndx = i + 1;		//(i + 1 == tabItemsNum) ? 0 : i + 1;
		
		if (nextIndx < tabItemsNum)
		{
			stimDurAdd = ((pCheckedTab[i].outNum & ~0x80) != (pCheckedTab[nextIndx].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			if (!IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pCheckedTab[i].firstDur + pCheckedTab[i].secDur, pCheckedTab[i].impPeriod, pCheckedTab[i].impCnt) + stimDurAdd, pCheckedTab[i].stimInterval * 1e3))
				return STIM_TABLE_ERROR(i, WRONG_STIMDUR_STIMINTERV, OVERVALUED);
		}
	}
	return ST_OK;
}

// Запустить стимуляцию
void StartStim(void)
{
	/*if (TabWasChanged == true)		// Если таблица стимуляции была изменена, стимулировать по измененнной таблице
	{															// на которую указывает pUsedStimTab
		pUsedStimTab = pUsedStimTab == StimTabOne ? StimTabTwo : StimTabOne;
		TabWasChanged = false;
	}*/
	//StimIndex = 0;		// Обнулить индекс текущего элем-та
	InitStim();
		
	/* Разрешить прерывания от АЦП2 (измерение тока стимула) */
	ADC_ClearFlag(ADC2, ADC_FLAG_JEOC);
	ADC_ITConfig(ADC2, ADC_IT_JEOC, ENABLE);
	
	/* Разрешить генерацию DMA запросов от сигнала триггера TIM8, чтобы генерировать синхросигналы на первом импульсе трейна */
	TIM_DMACmd(TIM8, TIM_DMA_Trigger, ENABLE);
	
	TIM_ClearFlag(TIM8, TIM_FLAG_Trigger | TIM_FLAG_CC1);
	// Разрешение прерываний для TIM8
	TIM_ITConfig(TIM8, TIM_IT_Trigger | TIM_IT_CC1, ENABLE);
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	// Разрешить прерывание от события Update TIM1
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	
	//// Разрешить вход прерывания!!!
	EXTI->IMR |= EXTI_Line12;
	////TIM_SelectInputTrigger(TIM1, TIM_TS_TI1F_ED); ////TIM_CCxCmd(TIM1, TIM_Channel_1, TIM_CCx_Enable);		// Разрешить вход захвата-сравнения №1 сигнала запуска
	
	ModuleState = STIMULATION;
}

// Провести инициализацию перед новым стимулом
void InitStim(void)
{
	// Установить период имп-ов в трейне
	uint16_t impPeriod = pUsedStimTab[StimIndex].impCnt == 0 ? 
											(pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp) + pUsedStimTab[StimIndex].secDur + MIN_DURATION_PERIOD_DIFF)
											: pUsedStimTab[StimIndex].impPeriod;
	TIM_SetAutoreload(TIM1, impPeriod);		/// * 1e3
	TIM_SetCompare2(TIM1, impPeriod);			/// * 1e3
	
	TIM1->RCR = pUsedStimTab[StimIndex].impCnt;		// Установить число импульсов в трейне impCnt - 1
	TIM1->DIER &= ~TIM_DIER_UIE;									// Чтобы число имп-ов в трейне установилось сразу при изменении, а не через 1 стимул
	TIM1->EGR |= TIM_EGR_UG;											// запретить прерывание TIM1, сгенерировать update TIM1
	TIM1->SR &= ~TIM_SR_UIF;											// очистить флаг
	TIM1->DIER |= TIM_DIER_UIE;										// разрешить прерывание
	
	// Установить интервал до следующего стимула
	TIM5->ARR = pUsedStimTab[StimIndex].stimInterval - 1;
	
	// Если стимул положительный
	if ((pUsedStimTab[StimIndex].outNum & 0x80) == 0)
	{
		SetStimAmpl(pUsedStimTab[StimIndex].firstAmp, false, true);			// Установить амплитуду
		CurrentAmpl = pUsedStimTab[StimIndex].firstAmp;
		TIM_SetCompare2(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));		// Установить длит-ть "+" импульса
		TIM_SelectOCxM(TIM8, TIM_Channel_2, TIM_OCMode_PWM1);						// Установить режим выхода ШИМ1
		TIM_SetCompare1(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));		// Установить общую длит-ть импульса
		TIM_SetCompare4(TIM8, pUsedStimTab[StimIndex].firstDur - 4);		//// Установить момент измерения тока стимула
		TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);								// Разрешить соответствующий выход
		if (pUsedStimTab[StimIndex].secDur > 0)		// Если стимул бифазный
		{
			TIM_SetCompare3(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	// Установить длит-ть "-" импульса
			
			TIM_SelectOCxM(TIM8, TIM_Channel_3, TIM_OCMode_PWM2);					// Установить режим выхода ШИМ2
			TIM_SetCompare1(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp) + pUsedStimTab[StimIndex].secDur);			// Установить общую длит-ть импульса ("+" + "-")
			TIM_SetCompare4(TIM8, (GetMaxAmp(&pUsedStimTab[StimIndex]) == pUsedStimTab[StimIndex].firstAmp ?
														 pUsedStimTab[StimIndex].firstDur - 4 : pUsedStimTab[StimIndex].firstDur + pUsedStimTab[StimIndex].secDur - 4));	// Установить момент измерения тока стимула
			TIM_CCxCmd(TIM8, TIM_Channel_3, TIM_CCx_Enable);				// Разрешить выход
			if (pUsedStimTab[StimIndex].firstAmp != pUsedStimTab[StimIndex].secAmp)		// Если стимул бифазный и амплитуда импульсов разная
			{
				IsStimBiPhase = true;
				//TIM_SetCompare4(TIM1, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	///
				TIM_ClearFlag(TIM8, TIM_FLAG_CC2);
				TIM_ITConfig(TIM8, TIM_IT_CC2, ENABLE);			// Разрешить прерывания от канала СС2 TIM8, чтобы менять диапазон
			}
		}
		else
			TIM_CCxCmd(TIM8, TIM_Channel_3, TIM_CCx_Disable);				// 
	}
	else		// Если стимул отрицательный
	{
		SetStimAmpl(pUsedStimTab[StimIndex].firstAmp, false, true);			// Установить амплитуду
		CurrentAmpl = pUsedStimTab[StimIndex].firstAmp;
		TIM_SetCompare3(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	// Установить длит-ть 1-го импульса
		TIM_SelectOCxM(TIM8, TIM_Channel_3, TIM_OCMode_PWM1);					// Установить режим выхода ШИМ1
		TIM_SetCompare1(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	// Установить общую длит-ть импульса
		TIM_SetCompare4(TIM8, pUsedStimTab[StimIndex].firstDur - 4);	// Установить момент измерения тока стимула
		TIM_CCxCmd(TIM8, TIM_Channel_3, TIM_CCx_Enable);							// Разрешить выход
		if (pUsedStimTab[StimIndex].secDur > 0)		// Если стимул бифазный
		{
			TIM_SetCompare2(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	// Установить длит-ть 2-го импульса
			TIM_SelectOCxM(TIM8, TIM_Channel_2, TIM_OCMode_PWM2);					// Установить режим выхода ШИМ2
			TIM_SetCompare1(TIM8, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp) + pUsedStimTab[StimIndex].secDur);			// Установить общую длит-ть импульса ("+" + "-")
			TIM_SetCompare4(TIM8, (GetMaxAmp(&pUsedStimTab[StimIndex]) == pUsedStimTab[StimIndex].firstAmp ?
														 pUsedStimTab[StimIndex].firstDur - 4 : pUsedStimTab[StimIndex].firstDur + pUsedStimTab[StimIndex].secDur - 4));	// Установить момент измерения тока стимула
			TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);				// Разрешить выход
			if (pUsedStimTab[StimIndex].firstAmp != pUsedStimTab[StimIndex].secAmp)		// Если стимул бифазный и амплитуда импульсов разная
			{
				IsStimBiPhase = true;
				//TIM_SetCompare4(TIM1, pUsedStimTab[StimIndex].firstDur + IMP_DURATION_ADD(pUsedStimTab[StimIndex].firstAmp));	///
				TIM_ClearFlag(TIM8, TIM_FLAG_CC3);
				TIM_ITConfig(TIM8, TIM_IT_CC3, ENABLE);			// Разрешить прерывания от канала СС3 TIM8, чтобы менять диапазон
			}
		}
		else
			TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Disable);
	}
	currentStimAmp = GetMaxAmp(&pUsedStimTab[StimIndex]); // Получить амплитуду для корректной оцифровки с АЦП
	SynchroSignal = MAKE_SYNCHRO_SIGNAL(StimIndex);		// Сформировать синхросигнал
	// Установить активный выход
	if ((pUsedStimTab[StimIndex].outNum & 0x01) == 0)
	{
		OUT1_ENABLE(ENABLE);
		OUT2_ENABLE(DISABLE);
	}
	else
	{
		OUT1_ENABLE(DISABLE);
		OUT2_ENABLE(ENABLE);
	}
}

// Остановить стимуляцию
void StopStim(void)
{
	while (TIM1->CR1 & TIM_CR1_CEN);										// Дождаться завершения стимула
	TIM5->CR1 &= ~TIM_CR1_CEN;		// Остановить TIM5
	/* Запретить вход внешнего прерывания */
	EXTI->IMR &= ~EXTI_Line12;
	//TIM_SelectInputTrigger(TIM1, TIM_TS_ITR0); //TIM_TS_TI1F_ED//TIM_CCxCmd(TIM1, TIM_Channel_1, TIM_CCx_Disable);		// Запретить вход захвата-сравнения №1 сигнала запуска
	// Запретить выходы TIM8
	TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Disable);
	TIM_CCxCmd(TIM8, TIM_Channel_3, TIM_CCx_Disable);
	
	/* Запретить генерацию DMA запросов от сигнала триггера TIM8, чтобы не генерировать синхросигналы */
	TIM_DMACmd(TIM8, TIM_DMA_Trigger, DISABLE);
	
	// Запретить прерывание от события Update TIM1
	TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);
	// Запретить прерывания для TIM8
	TIM_ITConfig(TIM8, TIM_IT_Trigger | TIM_IT_CC1, DISABLE);
	// Запретить прерывание от измерения тока стимула
	ADC_ITConfig(ADC2, ADC_IT_JEOC, DISABLE);
	StimIndex = 0;	// Обнулить индекс текущего элем-та
	ModuleState = ACTIVE;
}

/* Признак, что будет получен актуальный ответ от внешнего модуля
 * т.к. сначала принимается то же, что и передается, а потом возвращается актуальный ответ*/
__IO bool IsActualRespFromPeriph;

// Послать команду периферийному модулю
void SendCommToPeriph(uint16_t modType, uint16_t command)
{
	IsActualRespFromPeriph = false;
	USART_SendData(USART2, modType | command);
	StartDelay(COMM_TO_PERIPH_TIME);
}

/**
  * @brief  Установка амплитуды импульсов стимуляции
  * @param  amplVal: значение амплитуды
  * @retval none
  */
void SetStimAmpl(int16_t amplVal, bool useSoftWareTrig, bool setRange)
{
	if (setRange)
		SET_AMPLITUDE_RANGE(amplVal);
	register uint16_t dacVal = (uint16_t)(abs(amplVal) * AMP_TO_VOLT_DAC(amplVal) + (amplVal == -1 ? 0 : AMP_SHIFT_CORRECT));
	DAC_SetChannel1Data(DAC_Align_12b_R, dacVal);
	if (useSoftWareTrig)
	{
		register uint32_t dacCrReg = DAC->CR;
		SET_BIT(DAC->CR, DAC_CR_TSEL1_0 | DAC_CR_TSEL1_1 | DAC_CR_TSEL1_2);
		SET_BIT(DAC->SWTRIGR, DAC_SWTRIGR_SWTRIG1);
		DAC->CR = dacCrReg;
	}
}

// Определение максимальной амплитуды стимула
int16_t GetMaxAmp(const TStimTabItem * const stimTabItem)
{
	if (stimTabItem->firstDur > 0)		
	{
		if (stimTabItem->secDur > 0)
		{
			if (stimTabItem->firstAmp < 0 && stimTabItem->secAmp < 0)		// Значения для бифазного стимула в зависимости от диапазона
				return stimTabItem->firstAmp < stimTabItem->secAmp ? stimTabItem->firstAmp : stimTabItem->secAmp;
			else
				return stimTabItem->firstAmp > stimTabItem->secAmp ? stimTabItem->firstAmp : stimTabItem->secAmp;
		}
		return stimTabItem->firstAmp;		// Значение для положительного стимула
	}
	else
	{
		if (stimTabItem->secDur > 0)
			return stimTabItem->secAmp;		// Значение для отрицательного стимула
	}
	return 0;
}

/**
  * @brief  Сохранение источника последнего сброса контроллера
  * @param  None
  * @retval None
  */
void SaveLastRstReason(void)
{
	/* Инициализация исключений */
	ExceptionsInit();
	/* Инициализация модуля резервного копирования */
	BKPInit();
	
	/* Определить источник сброса и если сброс программный, значит было исключение и его причина сохранена в регистре резервного копирования */
	if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) == SET)
	{
		LastResReason = (TStatus)BKP_ReadBackupRegister(BKP_DR1);
		BKP_WriteBackupRegister(BKP_DR1, 0x0000);
	}
	else
	{
		/* Если сброс не программный, то установить и сохранить его причину */
		if ((RCC_GetFlagStatus(RCC_FLAG_PORRST) == SET) || (RCC_GetFlagStatus(RCC_FLAG_PINRST) == SET))
		{
			LastResReason = ST_RES_POWER;
		}
		else
		{
			if ((RCC_GetFlagStatus(RCC_FLAG_WWDGRST) == SET) || (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET))
			{
				LastResReason = ST_RES_WDT;
			}
			else
			{
				LastResReason = ST_RES_UNKNOWN;
			}
		}
	}
	
	RCC_ClearFlag();		//RCC->CSR |= RCC_CSR_RMVF;
}

/**
  * @brief  Инициализирует модуль резервного копирования
  * @param  None
  * @retval None
  */
void BKPInit(void)
{
	/* Enable PWR and BKP clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	
	/* Enable write access to Backup domain */
  PWR_BackupAccessCmd(ENABLE);
}

/************************* Обработка исключений *************************/

void HardFault_Handler(void)
{
	/* Сохранить причину сброса в регистре резервного копирования */
	BKP_WriteBackupRegister(BKP_DR1, ST_RES_HARD_FAULT);
	/* Сгенерировать системный сброс */
	NVIC_SystemReset();
}

void MemManage_Handler(void)
{
	/* Сохранить причину сброса в регистре резервного копирования */
	BKP_WriteBackupRegister(BKP_DR1, ST_RES_MEM_MGR_FAULT);
	/* Сгенерировать системный сброс */
	NVIC_SystemReset();
}

void BusFault_Handler(void)
{
	/* Сохранить причину сброса в регистре резервного копирования */
	BKP_WriteBackupRegister(BKP_DR1, ST_RES_BUS);
	/* Сгенерировать системный сброс */
	NVIC_SystemReset();
}

void UsageFault_Handler(void)
{
	/* Сохранить причину сброса в регистре резервного копирования */
	BKP_WriteBackupRegister(BKP_DR1, ST_RES_USAGE_FAULT);
	/* Сгенерировать системный сброс */
	NVIC_SystemReset();
}

/**
  * @brief  Инициализация исключений
  * @param  None
  * @retval None
  */
void ExceptionsInit(void)
{
	/* Разрешить исключения для деления на ноль и обращения к памяти по невыровненному адресу */
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk /*| SCB_CCR_UNALIGN_TRP_Msk*/;
	
	/* Разрешение генерации исключений */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;
}

/************************* Обработка исключений *************************/

/************************** Операции с очередью событий **************************/

/**
  * @brief  Инициализация очереди событий (присвоить указателям на начало и конец очереди значение индекса последнего элемента массива)
  * @param  None
  * @retval None
  */
void InitQueues(void)
{
	int i;
	for (i = 0; i < MAX_EVENT_PRIORITY; i++)
	{
		EventsQueue[i].Start = EventsQueue[i].End = MAX_EVENT_INDX;
	}
}

/**
  * @brief  Добавляет событие в очередь в зависимости от его приоритета
  * @param  priority: приоритет события
	* @param  value: добавляемое событие
  * @retval Успешность постановки события в очередь (если очередь не заполнена - true, иначе - false)
  */
bool EnQueue(uint8_t priority, uint16_t value)
{
	/* Если установлен флаг переполнения, значит очередь переполнена */
	if (EventsQueue[priority - 1].IsFull)
		return false;
	/* Поставить событе в очередь */
	EventsQueue[priority - 1].Events[EventsQueue[priority - 1].End--] = value;
	
	/* Если указатель на конец очереди меньше нуля, присвоить ему максимальное значение */
	if (EventsQueue[priority - 1].End < MIN_EVENT_INDX)
		EventsQueue[priority - 1].End = MAX_EVENT_INDX;
	
	/* Если при добавлении нового элемента указатель начала и конца совпадают, значит очередь переполнена */
	if (EventsQueue[priority - 1].End == EventsQueue[priority - 1].Start)
		EventsQueue[priority - 1].IsFull = true;
	
	return true;
}

/**
  * @brief  Извлечение из очереди события с наивысшим приоритетом
  * @param  retVal: Указатель на переменную, в которую сохраняется извлекаемое значение
  * @retval Успешность извлечения события из очереди (если в очереди были события - true, если очередь пустая - false)
  */
bool DeQueue(uint16_t * retVal, uint8_t * priority)
{
	int i;
	/* Перебрать массив очередей событий с конца, т.к. наиболее приоритетные события имеют наибольший индекс */
	for (i = MAX_EVENT_PRIORITY - 1; i >= 0; i--)
	{
		/* Если очередь не пустая или переполненная */
		if ((EventsQueue[i].Start != EventsQueue[i].End) || EventsQueue[i].IsFull)
		{
			*retVal = EventsQueue[i].Events[EventsQueue[i].Start--];
			*priority = i + 1;
			/* Извлечь значение и если указатель на начало очереди меньше 0, присвоить ему максимальное значение */
			if (EventsQueue[i].Start < MIN_EVENT_INDX)
			{
				EventsQueue[i].Start = MAX_EVENT_INDX;
			}
			/* Если при извлечении очередь была переполнена, присвоить концу очереди корректное значение */
			if (EventsQueue[i].IsFull)
				EventsQueue[i].IsFull = false;
			return true;
		}
	}
	return false;
}

/************************** Операции с очередью событий **************************/

/**
  * @brief  Функция задержки
  * @param  delVal: значние задержки в мкс
  * @retval none
  */
void Delay(uint32_t delVal)
{
	/* Счетчик старшей части для случая, когда delVal * 48 > 2^24 */
	uint8_t cntr = 0;
	
	/* Если значение задержки меньше или равно максимальному */
	if ((delVal * 48) <= SysTick_LOAD_RELOAD_Msk)
	{
		/* Запустить системный таймер и ждать, пока он не обнулится */
		SysTick->LOAD = delVal * 48 - 1;      																				/* set reload register */
		SysTick->VAL = 0;                    																					/* Load the SysTick Counter Value */
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;        	/* Enable SysTick IRQ and SysTick Timer */
		while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) {}
	}
	else
	{
		/* Сохранить в счетчике старшие 8 бит значения */
		cntr = (delVal * 48) >> 24;
		/* Запустить системный таймер */
		SysTick->LOAD = ((delVal * 48) & SysTick_LOAD_RELOAD_Msk) - 1;
		SysTick->VAL = 0;
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
// 		SysTick_Config((delVal * 48) & SysTick_LOAD_RELOAD_Msk);
		/* Декрементировать счетчик старшей части при каждом обновлении таймера */
		while (cntr)
		{
			while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) {}
				cntr--;
		}
	}
	/* Остановить таймер */
	SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief  Функция запуска задержки
  * @param  delVal: значние задержки в мкс, но не более 2^24 / 48
  * @retval Успешность запуска
  */
bool StartDelay(uint32_t delVal)
{
	StopDelay();
	if (SysTick_Config(delVal * 48))
		return false;
	else
		return true;
}

/**
  * @brief  Функция остановки задержки
  * @param  none
  * @retval none
  */
void StopDelay(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}
