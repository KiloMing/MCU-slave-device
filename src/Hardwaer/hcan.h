#ifndef __HCAN_H__
#define __HCAN_H__

#include "main.h"
#include "can.h"

#define	CAN_NUM   &hcan



void CAN_Start(CAN_HandleTypeDef *hcan);
void can_SendCmd(uint8_t *cmd, uint8_t len);
void CAN_SendEXData(CAN_HandleTypeDef* hcan,uint16_t ID,uint8_t *pData,uint16_t Len);
void CAN_SendData(CAN_HandleTypeDef* hcan,uint16_t ID,uint8_t *pData,uint16_t Len);
void CAN_Rx_Callback(CAN_HandleTypeDef *hcan);








#endif
