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
	uint8_t impCnt;					// Число импульсов в трейне
	uint8_t outNum;					// Номер выхода
	int16_t firstAmp;				// Амплитуда первого импульса
	int16_t secAmp;					// Амплитуда второго импульса
	uint16_t firstDur;			// Длительность первого импульса
	uint16_t secDur;				// Длительность второго импульса
	uint16_t impPeriod;			// Период импульсов в трейне
	uint16_t stimInterval;	// Интервал Между стимулами
} TStimTabItem;

/* Тип обработчика команд */
typedef void (*TcommHandler)(const TCommReply * const comm);

#define MODULE_ADR 0			// Адрес модуля

#define AMPL_MAX					1000		// Максимальное значение амплитуды 
#define OUTPUT_NUM_MAX		1				// Максимальное значение номера выхода внешнего коммутатора
#define IMP_DURATION_MIN	100			// Минимальное значение длительности импульса
#define IMP_DURATION_MAX	5000		// Максимальное значение длительности импульса
#define IMP_PERIOD_MIN		2000		// Минимальное значение периода импульсов в трейне (2 мс)
#define IMP_PERIOD_MAX		50000		// Максимальное значение периода импульсов в трейне	(50 мс)
#define STIM_INTERV_MIN		20			// Минимальный интервал между стимулами
#define STIM_INTERV_MAX		5000		// Максимальный интервал между стимулами

#define IS_AMPL_OK(ampVal)	 	 			(abs(ampVal) <= AMPL_MAX)																													// Проверить значение амплитуды
#define IS_NUMOUT_OK(outNum) 	 			((outNum == 0) || (outNum == 1) || (outNum == 0x80) || (outNum == 0x81))					// Проверить номер выхода
#define IS_DURATION_OK(durVal) 			((durVal == 0) || ((durVal >= IMP_DURATION_MIN) && (durVal <= IMP_DURATION_MAX)))	// Проверить значение длительности импульса
#define IS_PERIOD_OK(perVal)	 			((perVal >= IMP_PERIOD_MIN) && (perVal <= IMP_PERIOD_MAX))												// Проверить значение периода импульсов в трейне
#define IS_IMP_COUNT_OK(impCntVal)	((impCntVal >= 0) || (impCntVal <= 255))																					// Проверить значение количества импульсов в трейне
#define IS_STIM_INTERV_OK(stimIntervVal) ((stimIntervVal >= STIM_INTERV_MIN) && (stimIntervVal <= STIM_INTERV_MAX))		// Проверить значение 
#define IS_DUR_PERIOD_OK(durVal, perVal) ((durVal + MIN_DURATION_PERIOD_DIFF) <= perVal)																// Проверить, что в трейне длительность импульсов меньше их периода
#define IS_STIMDUR_STIMINTERV_OK(stimDurVal, stimIntervVal) (stimDurVal <= stimIntervVal)							// Проверить, что длительность стимула меньше интервала до следующего стимула

#define UNDERVALUED 0
#define OVERVALUED 	1
#define ERROR_CODE_OFFSET   1
#define ELEMENT_NUM_OFFSET  5

#define WRONG_FIR_AMPL						1		// Неверное значение амплитуды первого импульса
#define WRONG_SEC_AMPL						2		// Неверное значение амплитуды второго импульса
#define WRONG_FIR_IMP_DURATION		3		// Неверное значение длительности первого импульса
#define WRONG_SEC_IMP_DURATION		4		// Неверное значение длительности второго импульса
#define WRONG_IMP_PERIOD					5		// Неверное значение периода импульсов в трейне
#define WRONG_OUTPUT							6		// Неверное значение номера выхода коммутатора
#define WRONG_IMPCNTR							7		// Неверное значение количества импульсов в трейне
#define WRONG_STIM_INTERVAL				8		// Неверное значение интервала до след-го стимула
#define WRONG_DUR_PERIOD					9		// Неверное значение длительности или периода имп-ов в трейне (имп-сы накладываются друг на друга)
#define WRONG_STIMDUR_STIMINTERV	10	// Неверное значение длительности стимула или межстимульного инервала (стимулы наложатся друг на друга)

/* Макрос, возвращающий код ошибки при проверке таблицы стимуляции.
 * Возвращаются 2 байта. Структура их следующая:
 * |0111 - признак кода ошибки в таблице стимуляции|xx - не определены|yyyyy - номер неверного элемента в таблице стимуляции|zzzz - код ошибки|1 бит - перебор или недобор|*/
#define STIM_TABLE_ERROR(elemNum, code, type) ((0x7000) | (elemNum << ELEMENT_NUM_OFFSET) | (code << ERROR_CODE_OFFSET) | (type))

/* Макрос включения-отключения умножителя напряжения для зарядки накопительного конденсатора */
#define HIGH_VOLTAGE(state) ((state == DISABLE) ? (GPIOB->BSRR |= GPIO_BSRR_BS12) : (GPIOB->BRR |= GPIO_BRR_BR12))
/* Управление включением первого выхода */
#define OUT1_ENABLE(state) ((state == DISABLE) ? (GPIOC->BSRR |= GPIO_BSRR_BS9) : (GPIOC->BRR |= GPIO_BRR_BR9))
/* Управление включением второго выхода */
#define OUT2_ENABLE(state) ((state == DISABLE) ? (GPIOC->BSRR |= GPIO_BSRR_BS10) : (GPIOC->BRR |= GPIO_BRR_BR10))

/* Время передачи команды и получения ответа от вилочкового стимулятора (мкс).
 * Передача осуществляется на скорости 1200 Бит/с.
 * Передается байт команды, максимальная задержка 1.05 мс, передается байт ответа => Тпер = (1 / 1,2 * 10) * 2 + 1,05 = 17,717 (мс) */
#define COMM_TO_PERIPH_TIME (17717 + 1000)

/* Адреса вилочкового электрода */
#define FORK_STIM_COMM_ADR 		  0x80
#define FORK_STIM_RESP_ADR 		  0
/* Выбор светодиода */
#define ALL_LEDS_OFF 0
#define LEFT_LED_ON  0x01
#define RIGHT_LED_ON 0x02

/* Константа для сопротивления 27 Ом */
#define _27_OHM ((30.0 * 300.0) / (30.0 + 300.0))
/* Константа для сопротивления 15.2 Ом */
#define _15_OHM ((15.0 * 300.0) / (15.0 + 300.0))
/* Опорное напряжение АЦП и ЦАП */
#define V_REF 3.3
/* Установить диапазон амплитуды */
#define SET_AMPLITUDE_RANGE(value) ((value > 0) ? (GPIOC->BSRR |= GPIO_BSRR_BS6) : (GPIOC->BRR |= GPIO_BRR_BR6))
/* Коэффициент преобразования для перевода значений тока в дискреты ЦАП */
#define AMP_TO_VOLT_DAC(ampVal) ((ampVal > 0) ? (0.0001 / (V_REF / 4096 / _27_OHM/*_15_OHM*/)) : (0.00001 / (V_REF / 4096 / 300.0)))
/* Коэффициент коррекции выходного напряжения ЦАП из-за наличия смещения */
#define AMP_SHIFT_CORRECT (5.0 * 15000.0 / 1015000.0 * 4096 / V_REF)
/* Макрос получения байта для синхросигнала */
#define MAKE_SYNCHRO_SIGNAL(val) ((val << 2) | (0x80))

/* Время переключения реле, задающих полярность.
 * Определяется как Ton_max + Toff_max = 500 мкс + 200 мкс = 700 мкс. */
 #define OUTPUT_SWITCH_TIME 700
 /* Минимальная разница длительности и периода импульсов в трейне */
#define MIN_DURATION_PERIOD_DIFF 100
/* Минимальная разница длительности стимула и интервала до след стимула */
#define MIN_STIM_DUR_INTERV_DIFF 200

/* Поправка на длительности стимулов, зависящая от амплитуды
	 * |>0,4 мА|6  мкс|
	 * |0,3 мА |8  мкс|
	 * |0,2 мА |14 мкс|
	 * |0,1 мА |27 мкс|
	 */
	 #define IMP_DURATION_ADD(stimAmp) ((stimAmp < -30) ? 6 : ((stimAmp == -30) ? 14 : ((stimAmp == -20) ? 20 : ((stimAmp == -10) ? 33 : 6))))
	 
#define GET_STIM_DURATION(impDur, impPeriod, impCount) ((impPeriod * impCount) + impDur)

/* Управление выводом сброса микросхемы watchdog */
#define RESET_WATCHDOG(newState) ((newState == SET) ? (GPIOA->BSRR = GPIO_Pin_1) : (GPIOA->BRR = GPIO_Pin_1))

int16_t GetMaxAmp(const TStimTabItem * const stimTabItem);

#endif
