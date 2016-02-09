#include "main.h"

//uint8_t i = 0, j = 0, k = 0, l = 0;
// Счетчик импульсов в трейне
uint8_t impCntr = 0;
// Признак первого импульса в трейне
bool isFirstImp = true;

// Обработчик вызывается при завершении трейна
void TIM1_UP_IRQHandler(void)
{
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Enable);
	isFirstImp = true;
	impCntr = 0;
}

// Обработчик вызывается в начале каждого импульса в трейне
void TIM8_TRG_COM_IRQHandler(void)
{
	TIM8->EGR |= TIM_EGR_UG;			// Сгенерировать событие обновления TIM8
	TIM8->BDTR |= TIM_BDTR_MOE;		// Разрешить выходы каналов 2 и 3
	TIM_ClearITPendingBit(TIM8, TIM_IT_Trigger);
	if (isFirstImp)								// При первом импульсе поменять выходной сигнал триггера
	{															// таймера TIM1
		isFirstImp = false;
		TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC2Ref);
	}
	TIM1->CNT = 0;		// Установить счетчик в 0, чтобы для отсчета второго и последующих периодов импульсов в трейне оба таймера (TIM1 и TIM8)
}										// начинали отсчет с 0, иначе TIM1 запаздывает на 1 цикл и период увеличивается на 1 мс.

// Обработчик вызывается после формирования каждого импульса
void TIM8_CC_IRQHandler(void)
{
	TIM8->CR1 &= ~TIM_CR1_CEN;			// Остановить таймер
	TIM8->BDTR &= ~TIM_BDTR_MOE;		// Запретить выходы каналов 2 и 3
	TIM_ClearITPendingBit(TIM8, TIM_IT_CC1);
	if (impCntr++ == TIM1->RCR)			// После последнего импульса сгенерировать событие Update TIM1, чтобы избежать формирования лишнего импульса
		TIM1->EGR |= TIM_EGR_UG;
}
