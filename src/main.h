#ifndef MAIN_H
#define MAIN_H

//#include "stm32f10x.h"
#include "Utils.h"
#include <stdlib.h>
//#include "CommHandlers.h"

/* Режим работы модуля */
typedef enum {PASSIVE, ACTIVE, STIMULATION} ModuleStateType;

typedef enum 
{
	ST_OK = 0,                     // Признак успешного завершения операции

	// Группа кодов возможных причин того, что модуль оказался в пассивном состоянии
  ST_RES_POWER       	 = 1,     // Модуль в пассивном состоянии после подачи питания или внешнего сброса.
  ST_RES_WDT         	 = 2,     // Модуль в пассивном состоянии после сброса от сторожевого таймера.
  ST_RES_BUS    			 = 3,     // Модуль в пассивном состоянии после сброса из-за обработанной ошибки встроенного ПО (BUS_FAULT).
  ST_RES_MEM_MGR_FAULT = 4,     // Модуль в пассивном состоянии после сброса из-за обработанной ошибки встроенного ПО (MEM_MANAGE_FAULT).
  ST_RES_USAGE_FAULT   = 5,     // Модуль в пассивном состоянии после сброса из-за обработанной ошибки встроенного ПО (USAGE_FAULT).
  ST_RES_HARD_FAULT    = 6,     // Модуль в пассивном состоянии после сброса из-за обработанной ошибки встроенного ПО (HARD_FAULT).
  ST_RES_UNKNOWN     	 = 7,     // Модуль в пассивном состоянии после сброса по неизвестной причине. |

	// Группа кодов ошибок общая для всех модулей
  ST_CMD_UNKNOWN 		 = 11, 			// Неизвестная команда
  ST_CMD_WRONG_PARAM = 12, 			// Неверные параметры команды
  ST_CMD_UNABLE			 = 13, 			// Команда не может быть выполнена в данном состоянии
  ST_HW_ERROR 			 = 14, 			// Операция не выполнена по причине неисправности аппаратного обеспечения.
	
	WRONG_STIM_TAB_LOAD = 100,							  // Количество загруженных байт таблицы стимуляции не кратно размеру одного элемента таблицы
	STIM_TAB_INDX_OUT_OF_RANGE,								// Индекс таблицы стимуляции вышел за пределы массива (более 31)
	STIM_TAB_INDX_OUT_OF_USED_RANGE,					// Индекс таблицы стимуляции вышел за пределы используемого в данный момент диапазона элементов
	WRONG_HW_VERSION													// Версия аппаратного обеспечения выше той, которая заложена в программе

} TStatus;

/* Структуры */
/* Структура с версией аппаратуры и встроенного ПО */
typedef /*const*/ struct __attribute((packed))
{
	uint16_t StructVer;
	uint16_t InterfaceVer;
	uint16_t HwVersion;        // Номер версии аппаратного обеспечения
	uint16_t FwVersion;        // Номер версии встроенного ПО
} TBlockInfo;

/* Структура для команды или ответа на команду */
#pragma anon_unions
typedef struct __attribute((packed))
{
	uint8_t commNum : 4;							// Номер команды
	uint8_t modAddr : 4;							// Адрес модуля
	union __attribute((packed))
	{
		uint16_t commParam;							// Параметры короткой команды
		struct __attribute((packed))
		{
			uint8_t commLen;							// Длина длинной команды
			uint8_t commIndx;							// Индекс длинной команды
		};
		uint8_t toggleByte;							// Тогл-байт статус-запроса
	};
	uint8_t commData[256];						// Массив данных длинной команды
} TCommReply;

/* Элемент таблицы стимуляции */
typedef struct __attribute((packed))
{
	uint8_t impCnt;				// Число импульсов в трейне
	uint8_t outNum;				// Номер выхода
	int16_t posAmp;				// Амплитуда положительного импульса
	int16_t negAmp;				// Амплитуда отрицательного импульса
	uint16_t posDur;			// Длительность положительного импульса
	uint16_t negDur;			// Длительность отрицательного импульса
	uint16_t impPeriod;		// Период импульсов в трейне
} TStimTabItem;

/* Тип обработчика команд */
typedef void (*TcommHandler)(const TCommReply * const comm);

#define MODULE_ADR 0			// Адрес модуля

#define AMPL_MAX					1000		// Максимальное значение амплитуды 
#define OUTPUT_NUM_MAX		1				// Максимальное значение номера выхода внешнего коммутатора
#define IMP_DURATION_MIN	100			// Минимальное значение длительности импульса
#define IMP_DURATION_MAX	5000		// Максимальное значение длительности импульса
#define IMP_PERIOD_MIN		10			// Минимальное значение периода импульсов в трейне
#define IMP_PERIOD_MAX		10e3		// Максимальное значение периода импульсов в трейне

#define IS_AMPL_OK(ampVal)	 	 (abs(ampVal) <= AMPL_MAX)																				// Проверить значение амплитуды
#define IS_NUMOUT_OK(outNum) 	 ((outNum == 0) || (outNum == 1))																// Проверить номер выхода
#define IS_DURATION_OK(durVal) ((durVal >= IMP_DURATION_MIN) && (durVal <= IMP_DURATION_MAX))	// Проверить значение длительности импульса
#define IS_PERIOD_OK(perVal)	 ((perVal >= IMP_PERIOD_MIN) && (perVal <= IMP_PERIOD_MAX))			// Проверить значение периода импульсов в трейне

#define UNDERVALUED 0
#define OVERVALUED 	1
#define ERROR_CODE_OFFSET   1
#define ELEMENT_NUM_OFFSET  5

#define WRONG_POS_AMPL						1		// Неверное значение амплитуды положительного стимула
#define WRONG_NEG_AMPL						2		// Неверное значение амплитуды отрицательного стимула
#define WRONG_POS_IMP_DURATION		3		// Неверное значение длительности положительного импульса в трейне
#define WRONG_NEG_IMP_DURATION		4		// Неверное значение длительности отрицательного импульса в трейне
#define WRONG_IMP_PERIOD					5		// Неверное значение периода импульсов в трейне
#define WRONG_OUTPUT							6		// Неверное значение номера выхода коммутатора

/* Макрос, возвращающий код ошибки при проверке таблицы стимуляции.
 * Возвращаются 2 байта. Структура их следующая:
 * |0111 - признак кода ошибки в таблице стимуляции|xx - не определены|yyyyy - номер неверного элемента в таблице стимуляции|zzzz - код ошибки|1 бит - перебор или недобор|*/
#define STIM_TABLE_ERROR(elemNum, code, type) ((0x7000) | (elemNum << ELEMENT_NUM_OFFSET) | (code << ERROR_CODE_OFFSET) | (type))

#endif
