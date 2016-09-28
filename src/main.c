#include "main.h"
#include "CommHandlers.h"

extern void DevInit(void);
extern ModuleStateType ModuleState;

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
	1
};

int main(void)
{
	TCommReply Command = {0};		// Reply = {0};
		
	//DBGMCU->CR = /*DBGMCU_CR_DBG_TIM2_STOP | DBGMCU_CR_DBG_TIM3_STOP | DBGMCU_CR_DBG_TIM4_STOP |*/ DBGMCU_CR_DBG_TIM1_STOP | DBGMCU_CR_DBG_TIM8_STOP;
	SaveLastRstReason();
	DevInit();
	InitQueues();
	SetStimAmpl(0, true, true);
	StimTabInit();
	
	////////////////////////////
	// Установить активный режим
//	ActiveModeInit();
//	ModuleState = ACTIVE;
//	StartStim();				// Старт стимуляции
	////////////////////////////

	while (1)
	{
		RESET_WATCHDOG(SET);
		GetCommand((uint8_t*)&Command);
		RESET_WATCHDOG(RESET);
		CommHandler(&Command);
		//SendReply((uint8_t*)&Reply, replySize);
	}
}
