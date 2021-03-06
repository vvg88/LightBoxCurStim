#include "main.h"
#include "CommHandlers.h"
#include <string.h>

/* Причина последнего сбрса контроллера */
extern TStatus LastResReason;
/* Структура с версиями */
extern TBlockInfo BlockVersions;

extern uint16_t CheckStimTab(const TStimTabItem * const pCheckedTab, uint8_t tabItemsNum);

/* Состояние работы модуля */
ModuleStateType ModuleState = PASSIVE;
/* Таблица стимуляции 1 */
TStimTabItem StimTabOne[32];
/* Таблица стимуляции 2 */
TStimTabItem StimTabTwo[32];

/* Указатель на таблицу стимуляции, по которой ведется стимуляция */
/*const*/ TStimTabItem * pUsedStimTab = StimTabOne;
/* Количество обходимых элементов в таблице стимуляции */
__IO uint8_t UsedStimTabItemsNum = 0;
/* Признак, что таблица была переписана */
//bool TabWasChanged = false;
/* Признак, что таблица была проверена. Стимуляция не запустится без проверки */
bool TabWasChecked = true;
/* Количество стимулов */
__IO uint16_t StimCount = 0;

void StatReqHandler(const TCommReply * const comm);
void InfoReqHandler(const TCommReply * const comm);
void LoadStimTabHandler(const TCommReply * const comm);
void SaveT0handler(const TCommReply * const comm);
void SaveT1handler(const TCommReply * const comm);
void SetFirstAmp(const TCommReply * const comm);
void SetSecAmp(const TCommReply * const comm);
void SetTrainImpCount(const TCommReply * const comm);
void SetOutNum(const TCommReply * const comm);
void SetFirstImpDuration(const TCommReply * const comm);
void SetSecImpDuration(const TCommReply * const comm);
void SetTrainImpPeriod(const TCommReply * const comm);
void CheckStimTable(const TCommReply * const comm);
void StartStimulation(const TCommReply * const comm);
void SetStimInterval(const TCommReply * const comm);

void ReturnStatus(const TStatus stat);
void ReturnValue(const uint8_t cmd, const uint16_t value);
void ReturnError(const uint16_t errCod);
void ReturnLongReply(const uint8_t * const pBuff, size_t buffSize);
void SendReply(const uint8_t * const buff, size_t length);

/* Обработчики коротких команд */
const TcommHandler CommHandlers[15] =
{
	StatReqHandler,
	SaveT0handler,
	SaveT1handler,
	SetFirstAmp,
	SetSecAmp,
	SetTrainImpCount,
	SetOutNum,
	SetFirstImpDuration,
	SetSecImpDuration,
	SetTrainImpPeriod,
	CheckStimTable,
	StartStimulation,
	SetStimInterval,
	0,
	InfoReqHandler,
};

/* Обработчики длинных команд */
const TcommHandler LongCommHandler[1] = { LoadStimTabHandler };

/**
	* @brief  Обработчик команды
	* @param  comm: указатель на команду
  * @retval none
  */
void CommHandler(const TCommReply * const comm)
{
	if(ModuleState != PASSIVE)				// Если модуль в активном состоянии
	{
		if (comm->commNum < 0x0F)			// Определить тип команды (длинная/короткая)
		{
			if (CommHandlers[comm->commNum] == 0)		// Если обработчика такого нет,
				ReturnStatus(ST_CMD_UNKNOWN);					// Передать ошибку
			else
				CommHandlers[comm->commNum](comm);		// Выполнить соотв-ий обработчик
		}
		else													// Если длинная команда
		{
			// Обработка длинных команд
			if (comm->commIndx < ELEMENTS_OF(LongCommHandler))
				LongCommHandler[comm->commIndx](comm);
			else
				ReturnStatus(ST_CMD_UNKNOWN);
		}
	}
	else
	{
		if((comm->commNum == 0) || ((comm->commNum == 0x0E) && (comm->commParam == 0)))
			CommHandlers[comm->commNum](comm);			// В пассивном состоянии выполнять только команды 0 и 0х0Е
		else
			ReturnStatus(LastResReason);						// На все остальные отправлять статус
	}
}

/* Значение тоггл-байта */
uint8_t ToggleByte;
/* Значение параметра для последнего извлеченного события */
uint16_t LastEventValue;
/* Приоритет последнего извлеченного события */
uint8_t LastEventPriority;
/* Признак, что было послано событие */
bool LastEventWasSent = false;

/**
	* @brief  Обработчик запроса статуса
	* @param  comm: указатель на команду
  * @retval none
  */
void StatReqHandler(const TCommReply * const comm)
{
	if(ModuleState != PASSIVE)
	{
		if (ToggleByte == comm->toggleByte)			// Если тоггл-байт не изменился и до этого была передача события, повторить передачу
		{
			if (LastEventWasSent)
				ReturnValue(LastEventPriority, LastEventValue);
			else
				ReturnStatus(ST_OK);
		}
		else			// Если тоггл-байт отличается и до этого была передача события, сбросить признак последней передачи
		{
			if (LastEventWasSent)
				LastEventWasSent = false;
			/* Извлечь событие из очереди и при его наличии отослать статус-рапорт. В проивном случае отправить "0" */
			if (DeQueue(&LastEventValue, &LastEventPriority))
			{
				ReturnValue(LastEventPriority, LastEventValue);
				LastEventWasSent = true;
			}
			else
				ReturnStatus(ST_OK);
			ToggleByte = comm->toggleByte;
		}
	}
	else
		ReturnStatus(LastResReason);		// В пассивном состоянии отправлять причину сброса
}

/* Переменная для запоминания T0 */
uint16_t T0;
/* Переменная для запоминания T1 */
uint16_t T1;

// Обработчик для сохранения временной переменной Т0
void SaveT0handler(const TCommReply * const comm)
{
	T0 = comm->commParam;
	ReturnStatus(ST_OK);
}

// Обработчик для сохранения временной переменной Т1
void SaveT1handler(const TCommReply * const comm)
{
	T1 = comm->commParam;
	ReturnStatus(ST_OK);
}

// Проверка и установка амплитуды первого импульса
void SetFirstAmp(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{			
			if (!IS_AMPL_OK((int16_t)comm->commParam))		// Проверить устанавливаемое значение
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_FIR_AMPL, OVERVALUED));
			else
			{
				pChangedTab[T0].firstAmp = (int16_t)comm->commParam;
				ReturnStatus(ST_OK);
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверка и установка амплитуды второго импульса
void SetSecAmp(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			if (!IS_AMPL_OK((int16_t)comm->commParam))		// Проверить устанавливаемое значение
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_SEC_AMPL, OVERVALUED));
			else
			{
				pChangedTab[T0].secAmp = (int16_t)comm->commParam;
				ReturnStatus(ST_OK);
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверить и установить число импульсов в трейне
void SetTrainImpCount(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	uint8_t nextIndx;
	uint16_t stimDurAdd;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			nextIndx = (T0 + 1 == UsedStimTabItemsNum) ? 0 : T0 + 1;
			stimDurAdd = ((pChangedTab[T0].outNum & ~0x80) != (pChangedTab[nextIndx].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].firstDur + pChangedTab[T0].secDur, pChangedTab[T0].impPeriod, comm->commParam) + stimDurAdd, pChangedTab[T0].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
			else
			{
				if (((uint8_t)comm->commParam > 0) && !IS_DUR_PERIOD_OK((pChangedTab[T0].secDur + pChangedTab[T0].firstDur), pChangedTab[T0].impPeriod))
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_DUR_PERIOD, OVERVALUED));
				else
				{
					pChangedTab[T0].impCnt = (uint8_t)comm->commParam;
					ReturnStatus(ST_OK);
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверить и установить номер выхода
void SetOutNum(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	//uint8_t nextIndx, prevIndx;
	uint16_t stimDurAddNext, stimDurAddPrev;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{			
			stimDurAddNext = ((comm->commParam & ~0x80) != (pChangedTab[T0 + 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			stimDurAddPrev = ((comm->commParam & ~0x80) != (pChangedTab[T0 - 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if (!IS_NUMOUT_OK(comm->commParam))
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_OUTPUT, OVERVALUED));
			else
			{
				if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].secDur + pChangedTab[T0].firstDur, pChangedTab[T0].impPeriod, pChangedTab[T0].impCnt) + stimDurAddNext, pChangedTab[T0].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала 
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
				else
				{
					if ((T0 > 0) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0 - 1].secDur + pChangedTab[T0 - 1].firstDur, pChangedTab[T0 - 1].impPeriod, pChangedTab[T0 - 1].impCnt) + stimDurAddPrev, pChangedTab[T0 - 1].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала 
						ReturnError(STIM_TABLE_ERROR(T0 - 1, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
					else
					{
						pChangedTab[T0].outNum = (uint8_t)comm->commParam;
						ReturnStatus(ST_OK);
					}
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверка и установка длительности первого импульса
void SetFirstImpDuration(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	//uint8_t nextIndx;
	uint16_t stimDurAdd;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			//nextIndx = (T0 + 1 == UsedStimTabItemsNum) ? 0 : T0 + 1;
			stimDurAdd = ((pChangedTab[T0].outNum & ~0x80) != (pChangedTab[T0 + 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if (!IS_DURATION_OK(comm->commParam))		// Проверить устанавливаемое значение
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_FIR_IMP_DURATION, comm->commParam > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED));
			else
			{
				if ((pChangedTab[T0].impCnt > 0) && !IS_DUR_PERIOD_OK((pChangedTab[T0].secDur + comm->commParam), pChangedTab[T0].impPeriod)) //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_DUR_PERIOD, OVERVALUED));
				else
				{
					if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].secDur + comm->commParam, pChangedTab[T0].impPeriod, pChangedTab[T0].impCnt) + stimDurAdd, pChangedTab[T0].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
						ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
					else
					{
						pChangedTab[T0].firstDur = comm->commParam;
						ReturnStatus(ST_OK);
					}
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверка и установка длительности второго импульса
void SetSecImpDuration(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	//uint8_t nextIndx;
	uint16_t stimDurAdd;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			//nextIndx = (T0 + 1 == UsedStimTabItemsNum) ? 0 : T0 + 1;
			stimDurAdd = ((pChangedTab[T0].outNum & ~0x80) != (pChangedTab[T0 + 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if (!IS_DURATION_OK(comm->commParam))		// Проверить устанавливаемое значение
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_FIR_IMP_DURATION, comm->commParam > IMP_DURATION_MAX ? OVERVALUED : UNDERVALUED));
			else
			{
				if ((pChangedTab[T0].impCnt > 0) && !IS_DUR_PERIOD_OK((pChangedTab[T0].firstDur + comm->commParam), pChangedTab[T0].impPeriod)) //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_DUR_PERIOD, OVERVALUED));
				else
				{
					if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].firstDur + comm->commParam, pChangedTab[T0].impPeriod, pChangedTab[T0].impCnt) + stimDurAdd, pChangedTab[T0].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
						ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
					else
					{
						pChangedTab[T0].secDur = comm->commParam;
						ReturnStatus(ST_OK);
					}
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Установить период импульсов в трейне
void SetTrainImpPeriod(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;	//(TabWasChanged & (ModuleState == ACTIVE)) ? ((pUsedStimTab == StimTabOne) ? StimTabTwo : StimTabOne) : pUsedStimTab;
	//uint8_t nextIndx;
	uint16_t stimDurAdd;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			//nextIndx = (T0 + 1 == UsedStimTabItemsNum) ? 0 : T0 + 1;
			stimDurAdd = ((pChangedTab[T0].outNum & ~0x80) != (pChangedTab[T0 + 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if (!IS_PERIOD_OK(comm->commParam))
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_IMP_PERIOD, comm->commParam > IMP_PERIOD_MAX ? OVERVALUED : UNDERVALUED));
			else
			{
				if ((pChangedTab[T0].impCnt > 0) && !IS_DUR_PERIOD_OK((pChangedTab[T0].firstDur + pChangedTab[T0].secDur), comm->commParam)) //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_DUR_PERIOD, OVERVALUED));
				else
				{
					if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].firstDur + pChangedTab[T0].secDur, comm->commParam, pChangedTab[T0].impCnt) + stimDurAdd, pChangedTab[T0].stimInterval * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp
						ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
					else
					{
						pChangedTab[T0].impPeriod = comm->commParam;
						ReturnStatus(ST_OK);
					}
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Установить параметр "Интервал до следующего стимула"
void SetStimInterval(const TCommReply * const comm)
{
	// Определить индекс изменяемой таблицы
	TStimTabItem * const pChangedTab = pUsedStimTab;
	//uint8_t nextIndx;
	uint16_t stimDurAdd;
	
	if (T0 <= 31)		// Проверить индекс
	{
		if (T0 <= UsedStimTabItemsNum - 1)
		{
			//nextIndx = (T0 + 1 == UsedStimTabItemsNum) ? 0 : T0 + 1;
			stimDurAdd = ((pChangedTab[T0].outNum & ~0x80) != (pChangedTab[T0 + 1].outNum & ~0x80)) ? OUTPUT_SWITCH_TIME : MIN_STIM_DUR_INTERV_DIFF;
			
			if (!IS_STIM_INTERV_OK(comm->commParam))		// Проверить устанавливаемое значние
				ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIM_INTERVAL, comm->commParam > STIM_INTERV_MAX ? OVERVALUED : UNDERVALUED));
			else
			{
				if ((T0 < UsedStimTabItemsNum - 1) && !IS_STIMDUR_STIMINTERV_OK(GET_STIM_DURATION(pChangedTab[T0].firstDur + pChangedTab[T0].secDur, pChangedTab[T0].impPeriod, pChangedTab[T0].impCnt) + stimDurAdd, comm->commParam * 1e3))	// Проверить, что длит-ть стимула не превышает межстимульного интервала //+ IMP_DURATION_ADD(pChangedTab[T0].firstAmp)
					ReturnError(STIM_TABLE_ERROR(T0, WRONG_STIMDUR_STIMINTERV, OVERVALUED));
				else
				{
					pChangedTab[T0].stimInterval = comm->commParam;
					ReturnStatus(ST_OK);
				}
			}
		}
		else
			ReturnStatus(STIM_TAB_INDX_OUT_OF_USED_RANGE);
	}
	else
		ReturnStatus(STIM_TAB_INDX_OUT_OF_RANGE);
}

// Проверить таблицу стимуляции
void CheckStimTable(const TCommReply * const comm)
{
	uint16_t stimTabErr = 0;
	
	if (ModuleState != PASSIVE)
	{
		if ((comm->commParam > 0) && (comm->commParam <= 32))
		{
			stimTabErr = CheckStimTab(pUsedStimTab /*== StimTabOne ? StimTabTwo : StimTabOne*/, comm->commParam);
			if (stimTabErr != ST_OK)
				ReturnError(stimTabErr);
			else
			{
				UsedStimTabItemsNum = comm->commParam;
				TabWasChecked = true;
				ReturnStatus(ST_OK);
			}
		}
		else
			ReturnStatus(ST_CMD_WRONG_PARAM);
	}
	else
		ReturnStatus(ST_CMD_UNABLE);
}

// Начать стимуляцию
void StartStimulation(const TCommReply * const comm)
{
	if (ModuleState == STIMULATION)
		StopStim();		// Остановить стимуляцию
	StimCount = comm->commParam;
	if (TabWasChecked)		// Если таблица была проверена
	{
		StartStim();				// Старт стимуляции
		ReturnStatus(ST_OK);
	}
	else
		ReturnStatus(ST_CMD_UNABLE);		// Иначе отправить ошибку
}

/**
	* @brief  Обработчик запроса информации и изменения состояния
	* @param  comm: указатель на команду
  * @retval none
  */
void InfoReqHandler(const TCommReply * const comm)
{
	switch (comm->commParam)
	{
		case 0:			// Перейти в активный режим
			ReturnValue(ST_RAP_CODE, (uint16_t)(-100));
			ActiveModeInit();
			ModuleState = ACTIVE;
			EnQueue(LONG_COM_DONE_CODE, ST_OK);
			break;
		case 1:			// Переслать размер структуры с версиями
			ReturnValue(comm->commNum, sizeof(TBlockInfo));
			break;
		case 2:			// Переслать структуру с версиями
			ReturnLongReply((uint8_t*)&BlockVersions, sizeof(TBlockInfo));
			break;
		case 10:
			ReturnValue(ST_RAP_CODE, (uint16_t)(-100));
			StopStim();
			EnQueue(LONG_COM_DONE_CODE, ST_OK);//ReturnStatus(ST_OK);
			break;
		default:		// Команда не опознана
			ReturnStatus(ST_CMD_UNKNOWN);
			break;
	}
}

/**
	* @brief  Загрузка таблицы стимуляции
	* @param  comm: указатель на команду
  * @retval none
  */
void LoadStimTabHandler(const TCommReply * const comm)
{
	uint8_t * pFirstElem = 0;		// Указатель на элемент таблицы стимуляции с которого производится ее обновление
	
	if (ModuleState == ACTIVE)	//ModuleState != PASSIVE
	{
		/* Сохранить адрес начального элемента для той таблицы, по которой сейчас нет стимуляции.
		 * Индекс начального элемента сохранен в 0-ом байте переданного массива данных */
		//pFirstElem = (pUsedStimTab == StimTabOne) ? (uint8_t *)(&StimTabTwo[comm->commData[0]]) : (uint8_t *)(&StimTabOne[comm->commData[0]]);
		pFirstElem = (uint8_t *)(&pUsedStimTab[comm->commData[0]]);
		
		if ((comm->commLen - 1) % sizeof(TStimTabItem) == 0)
		{
			for (int i = 1; i < comm->commLen; i++)		// Проверить, что элементы таблицы стимуляции были переданы полностью
				pFirstElem[i - 1] = comm->commData[i];		// Скопировать данные во временную таблицу при успешной проверке 
			ReturnStatus(ST_OK);
			//TabWasChanged = true;
			TabWasChecked = false;
		}
		else
			ReturnStatus(WRONG_STIM_TAB_LOAD);				// или выдать ошибку при неуспешной проверке
	}
	else
		ReturnStatus(ST_CMD_UNABLE);
}

/**
	* @brief  Послать статус-рапорт
	* @param  stat: статус
  * @retval none
  */
void ReturnStatus(const TStatus stat)
{
	ReturnValue(ST_RAP_CODE, stat);
}

//int k = 0;
/**
	* @brief  Переслать значение (статус-рапорт или ответ на команду)
	* @param  cmd: номер команды
	* @param  value: передаваемое значение
  * @retval none
  */
void ReturnValue(const uint8_t cmd, const uint16_t value)
{
	TCommReply reply;
	
	reply.commNum = cmd;					// Сформировать ответ
	reply.modAddr = MODULE_ADR;
	reply.commParam = value;
		
	SendReply((uint8_t*)&reply, 3);		// Послать ответ
}

/**
	* @brief  Послать код ошибки таблицы стим-ции
	* @param  errCod: код ошибки
  * @retval none
  */
void ReturnError(const uint16_t errCod)
{
	ReturnValue(ST_RAP_CODE, errCod);
}

/**
	* @brief  Переслать длинный ответ
	* @param  pBuff: указатель на буфр с данными
	* @param  buffSize: размер буфера
  * @retval none
  */
void ReturnLongReply(const uint8_t * const pBuff, size_t buffSize)
{
	TCommReply reply;
	
	reply.commNum = 0x0F;						// Сформировать длиный ответ
	reply.modAddr = MODULE_ADR;
	reply.commLen = buffSize - 1;
	reply.commIndx = pBuff[0];			// В байт, где для команды передается индекс, записать первый байт буфера ответа
	memmove(reply.commData, &pBuff[1], buffSize - 1);//memmove(&reply.commData[1], pBuff, buffSize - 1);
	
	SendReply((uint8_t*)&reply, buffSize + 2);		// Передать ответ
}

/**
	* @brief  Принять команду.
	* @param  buff: указатель на буфер, куда принять команду
  * @retval Длина полученных данных
  */
size_t GetCommand(uint8_t * const buff)
{
	const uint8_t addrMask = MODULE_ADR << 4;
	uint8_t crc = 0;
	uint8_t * pRxBuff = 0;
	uint8_t * pEnd = 0;			// Указатель на конец длинного сообщения
	
	MODIFY_REG(USART1->CR1, 0, USART_CR1_RE);		// Разрешить прием
	while (1)
	{
		register uint16_t usartWord;
		
		while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);			// Принять байт
		usartWord = USART_ReceiveData(USART1);
		
		if (usartWord & 0x100)														// Если адресный байт
		{																									// Обнулить все переменные
			pRxBuff = 0;
			pEnd = 0;
			crc = 0;
			if (addrMask == (uint8_t)(usartWord & 0xF0))		// Если адрес совпал с адресом модуля
			{
				buff[0] = crc = (uint8_t)usartWord;						// Сохранить его
				pRxBuff = &buff[1];
			}
			continue;
		}
		else																					// Если принят обычный байт
		{
			if (pRxBuff == 0)														// Принятый байт не адресован модулю
				continue;																	// Ожидать адресный байт
			if ((buff[0] & 0x0F) != 0x0F)								// Если принимается короткая команда
			{
				if (pRxBuff < &buff[3])										// Принять 2 и 3 байты команды
				{
					*pRxBuff++ = (uint8_t)usartWord;
					crc += (uint8_t)usartWord;							// Вычислить CRC
				}
				else
				{
					if (crc == (uint8_t)usartWord)					// Принять CRC и проверить ее
						break;
					pRxBuff = 0;														// В случае несовпадения, сбросить автомат состояний
				}
			}
			else
			{
				if (pRxBuff == &buff[1])														// Принять длину сообщения длиной команды
					pEnd = &buff[3] + (size_t)(usartWord & 0xFF);			// Вычислить указатель на конец сообщения
				else if (pRxBuff == pEnd)														// Если пришел последний байт длинного сообщения
				{
					if (crc == (uint8_t)usartWord)										// Принять и проверить CRC
						break;
					pRxBuff = 0;
					continue;
				}
				*pRxBuff++ = (uint8_t)usartWord;										// Принять байт длинного сообщения
				crc += (uint8_t)usartWord;
			}
		}
	}
	
	MODIFY_REG(USART1->CR1, USART_CR1_RE, 0);		// Запретить прием
	return pRxBuff - buff;											// Вернуть длину принятой команды
}

/**
	* @brief  Послать ответ на команду.
	* @param  buff: указатель на буфер с передаваемыми данными
	* @param  length: длина буфера
  * @retval None
  */
void SendReply(const uint8_t * const buff, size_t length)
{
	uint8_t crc = 0;
	
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);		// ДОждаться освобождения передатчика
	crc = buff[0];
	USART_SendData(USART1, crc | 0x100);			// Послать первый байт
	
	for (uint8_t i = 1; i < length; i++)			// Передать оставшиеся байты сообщения
	{
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		crc += buff[i];
		USART_SendData(USART1, buff[i]);
	}
	
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, crc);							// Передать CRC
	
}

/**
	* @brief  Инициализировать таблицы стимуляции
	* @param  None
  * @retval None
  */
void StimTabInit()
{
	for (int i = 0; i < 32; i++)
	{
		StimTabOne[i].impCnt = StimTabTwo[i].impCnt = 0;
		StimTabOne[i].outNum = StimTabTwo[i].outNum = 0;
		StimTabOne[i].firstAmp = StimTabTwo[i].firstAmp = 100;
		StimTabOne[i].secAmp = StimTabTwo[i].secAmp = 100;
		StimTabOne[i].firstDur = StimTabTwo[i].firstDur = 100;
		StimTabOne[i].secDur = StimTabTwo[i].secDur = 0;
		StimTabOne[i].impPeriod = StimTabTwo[i].impPeriod = 2000;
		StimTabOne[i].stimInterval = StimTabTwo[i].stimInterval = 20;
	}
	UsedStimTabItemsNum = 32;
	pUsedStimTab = StimTabOne;
}
