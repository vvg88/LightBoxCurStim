#include "main.h"

extern void DevInit(void);

int main(void)
{
	//DBGMCU->CR = DBGMCU_CR_DBG_TIM2_STOP | DBGMCU_CR_DBG_TIM3_STOP | DBGMCU_CR_DBG_TIM4_STOP | DBGMCU_CR_DBG_TIM1_STOP | DBGMCU_CR_DBG_TIM8_STOP;
	SaveLastRstReason();
	DevInit();
	
	USART_SendData(USART1, 0x55);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	while (1)
	{
		
	}
}

