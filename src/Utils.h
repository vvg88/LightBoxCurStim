#ifndef UTILS_H
#define UTILS_H

#include "stm32f10x.h"
#include <stdbool.h>
#include <stddef.h>

/* Структура очереди событий с одним приоритетом */
typedef struct
{
	__IO int8_t Start;
	__IO int8_t End;
	__IO bool IsFull;
	__IO uint16_t Events[8];
} EventsQueueStruct;

#define MAX_EVENT_PRIORITY		4										// Максимальный приоритет события
#define MAX_QUEUE_CAPACITY		8													// Максимальная емкость очереди
#define MIN_EVENT_INDX				0													// Минимальный индекс события
#define MAX_EVENT_INDX				MAX_QUEUE_CAPACITY - 1		// Максимальный индекс события
	
#define ELEMENTS_OF(arr) (sizeof(arr)/sizeof(arr[0]))

void SaveLastRstReason(void);
void InitQueues(void);
bool EnQueue(uint8_t priority, uint16_t value);
bool DeQueue(uint16_t * retVal, uint8_t * priority);

#endif

