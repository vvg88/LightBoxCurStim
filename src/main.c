#include "main.h"
#include "CommHandlers.h"

extern void DevInit(void);

/* Версии:
	 1 - версия данной структуры
	 0 - версия интерфейса
	 0 - версия аппаратуры
	 0 - версия ВПО */
TBlockInfo BlockVersions =
{
	1,
	0,
	0,
	0
};

int main(void)
{
	TCommReply Command = {0};		// Reply = {0};
	
	//uint8_t commBuff[260];
	//const uint8_t * pReplyBuff = 0;
	//size_t replySize = 0, commSize = 0;
	
	//DBGMCU->CR = DBGMCU_CR_DBG_TIM2_STOP | DBGMCU_CR_DBG_TIM3_STOP | DBGMCU_CR_DBG_TIM4_STOP | DBGMCU_CR_DBG_TIM1_STOP | DBGMCU_CR_DBG_TIM8_STOP;
	SaveLastRstReason();
	DevInit();
	
//	USART_SendData(USART1, 0x55);
//	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	while (1)
	{
		GetCommand((uint8_t*)&Command);
		CommHandler(&Command);
		//SendReply((uint8_t*)&Reply, replySize);
	}
}

