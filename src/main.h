#ifndef MAIN_H
#define MAIN_H

//#include "stm32f10x.h"
#include "Utils.h"
//#include "CommHandlers.h"

/* Режим работы модуля */
typedef enum {PASSIVE, ACTIVE} ModuleStateType;

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

typedef void (*TcommHandler)(const TCommReply * const comm);

#define MODULE_ADR 0			// Адрес модуля

#endif
