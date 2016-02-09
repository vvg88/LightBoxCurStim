//#include "Utils.h"
#include "main.h"

uint16_t CheckStimTab(const TStimTabItem * const pCheckedTab, uint8_t tabItemsNum);
void ExceptionsInit(void);
void BKPInit(void);

/* Причина последнего сбрса контроллера */
TStatus LastResReason;
/* Очередь событий. Представляет собой массив очередей (в каждой очереди хранятся события одного определенного приоритета) */
EventsQueueStruct EventsQueue[4];

uint16_t CheckStimTab(const TStimTabItem * const pCheckedTab, uint8_t tabItemsNum)
{
	//uint8_t prevIndx = 0;
	
	for (int i = 0; i < tabItemsNum; i++)
	{
		//prevIndx = (i == 0) ? (tabItemsNum - 1) : (i - 1);
		
		if (!IS_AMPL_OK(pCheckedTab[i].posAmp))
			return STIM_TABLE_ERROR(i, WRONG_POS_AMPL, OVERVALUED);
		if (!IS_AMPL_OK(pCheckedTab[i].negAmp))
			return STIM_TABLE_ERROR(i, WRONG_NEG_AMPL, OVERVALUED);
		if (!IS_NUMOUT_OK(pCheckedTab[i].outNum))
			return STIM_TABLE_ERROR(i, WRONG_OUTPUT, OVERVALUED);
		if (!IS_DURATION_OK(pCheckedTab[i].posDur))
			return STIM_TABLE_ERROR(i, WRONG_POS_IMP_DURATION, pCheckedTab[i].posDur > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED);
		if (!IS_DURATION_OK(pCheckedTab[i].negDur))
			return STIM_TABLE_ERROR(i, WRONG_NEG_IMP_DURATION, pCheckedTab[i].negDur > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED);
		if (!IS_PERIOD_OK(pCheckedTab[i].impPeriod))
			return STIM_TABLE_ERROR(i, WRONG_IMP_PERIOD, pCheckedTab[i].impPeriod > IMP_PERIOD_MAX ? OVERVALUED : UNDERVALUED);
	}
	return ST_OK;
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
