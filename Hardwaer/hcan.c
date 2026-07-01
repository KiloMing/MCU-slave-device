#include "hcan.h"
#include <string.h>

#define CAN_SEND_TIMEOUT 10

static void CAN_Filter_ParamsInit(CAN_FilterTypeDef *sFilterConfig)
{
    sFilterConfig->FilterIdHigh = 0;						
    sFilterConfig->FilterIdLow = 0;							
    sFilterConfig->FilterMaskIdHigh = 0;					
    sFilterConfig->FilterMaskIdLow = 0;						
    sFilterConfig->FilterFIFOAssignment = CAN_FILTER_FIFO0;	
    sFilterConfig->FilterBank = 0;							
    sFilterConfig->FilterMode = CAN_FILTERMODE_IDMASK;		
    sFilterConfig->FilterScale = CAN_FILTERSCALE_32BIT;		
    sFilterConfig->FilterActivation = ENABLE;				
    sFilterConfig->SlaveStartFilterBank = 0;
}

void CAN_Rx_Callback(CAN_HandleTypeDef *hcan)
{

}

void CAN_Start(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef sFilterConfig;
    CAN_Filter_ParamsInit(&sFilterConfig);
    HAL_CAN_ConfigFilter(hcan, &sFilterConfig);
    HAL_CAN_ActivateNotification(hcan ,CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Start(hcan);
}

// ✅ 适配电机无ACK、固定0x6B校验，完美发多条指令
void can_SendCmd(uint8_t *cmd, uint8_t len)
{
    uint8_t i = 0, j = 0, k = 0, l = 0, packNum = 0;
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8] = {0};
    uint32_t mailbox;
    uint32_t sendTick;

    if(cmd == NULL || len < 2) return;

    // 轻量重置，不影响发送
    HAL_CAN_AbortTxRequest(&hcan, CAN_TX_MAILBOX0|CAN_TX_MAILBOX1|CAN_TX_MAILBOX2);
    j = len - 2;

    while(i < j)
    {
        k = j - i;
        memset(TxData, 0, 8);

        // 帧头配置
        TxHeader.StdId = 0x00;
        TxHeader.ExtId = ((uint32_t)cmd[0] << 8) | (packNum & 0xFF);
        TxHeader.IDE = CAN_ID_EXT;
        TxHeader.RTR = CAN_RTR_DATA;
        TxHeader.TransmitGlobalTime = DISABLE;

        // 你的原始分包逻辑（完全匹配电机）
        TxData[0] = cmd[1];
        if(k < 7)
        {
            for(l=0; l<k; l++,i++) TxData[l+1] = cmd[i+2];
            TxHeader.DLC = k + 1;
        }
        else
        {
            for(l=0; l<7; l++,i++) TxData[l+1] = cmd[i+2];
            TxHeader.DLC = 8;
        }

        // 发送，忽略ACK错误（电机不回复，正常！）
        sendTick = HAL_GetTick();
        while(HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &mailbox) != HAL_OK)
        {
            if(HAL_GetTick()-sendTick > CAN_SEND_TIMEOUT) break;
            HAL_Delay(1);
        }
        packNum++;
    }
}

