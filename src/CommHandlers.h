#ifndef COMM_HANDLERS_H
#define COMM_HANDLERS_H

#include "main.h"

/* Коды команд для статус-рапортов */
typedef enum { ST_RAP_CODE, LONG_COM_DONE_CODE, HIGH_VOLT_CODE, FORK_STIM_CODE, STIM_CUR_CODE } StatusRaportCodes;

size_t GetCommand(uint8_t * const buff);
void CommHandler(const TCommReply * const comm);
void StimTabInit(void);
//void SendReply(const uint8_t * const buff, size_t length);
//void ReturnStatus(const TStatus stat);
//void ReturnValue(const uint8_t cmd, const uint16_t value);

#endif
