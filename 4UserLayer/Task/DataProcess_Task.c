/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : DataProcess_Task.c
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��5��15��
  ����޸�   :
  ��������   : ��ˢ��/QR/Զ���͹��������ݽ��д���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��5��15��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/
/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#define LOG_TAG    "DataProcess"
#include "elog.h"

#include "DataProcess_Task.h"
#include "CmdHandle.h"
#include "bsp_uart_fifo.h"
#include "jsonUtils.h"
#include "malloc.h"
#include "tool.h"
#include "bsp_time.h"

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#define DATAPROC_TASK_PRIO		(tskIDLE_PRIORITY + 6) 
#define DATAPROC_STK_SIZE 		(configMINIMAL_STACK_SIZE*12)

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/
const char *dataProcTaskName = "vDataProcTask"; 

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskDataProc = NULL;  



/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
 
static void vTaskDataProcess(void *pvParameters);

void CreateDataProcessTask(void)
{
    xTaskCreate((TaskFunction_t )vTaskDataProcess,         
                (const char*    )dataProcTaskName,       
                (uint16_t       )DATAPROC_STK_SIZE, 
                (void*          )NULL,              
                (UBaseType_t    )DATAPROC_TASK_PRIO,    
                (TaskHandle_t*  )&xHandleTaskDataProc);
}


static void vTaskDataProcess(void *pvParameters)
{    
    BaseType_t xReturn = pdTRUE;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50); /* �������ȴ�ʱ��Ϊ100ms */ 
    int ret = 0;
    int len = 0;

    uint8_t openLeft[8] = { 0x02,0x00,0x07,0x01,0x06,0x4c,0x03,0x4D };    
    uint8_t openRight[8] = { 0x02,0x00,0x07,0x01,0x06,0x52,0x03,0x53 };
    uint8_t jsonbuff[256] = {0};

    
    CMD_BUFF_STRU *ptCmd = &gCmd_buff;
    READER_BUFF_STRU *ptMsg  = &gReaderRecvMsg;
    memset(&gReaderRecvMsg,0x00,sizeof(READER_BUFF_STRU));   
    memset(&gCmd_buff,0x00,sizeof(CMD_BUFF_STRU));       
    
    while (1)
    {
        //�����·���ɺ�30���޿����·���������һ���������
        if(gCardSortTimer.flag && gCardSortTimer.outTimer == 0)
        {
            log_d("-----start sort-----\r\n");
            gCardSortTimer.flag = 0;
            sortLastPageCard();
        } 

        //��ȡ���������������ݣ�����������ʷ��¼ 
//        while(gRecordIndex.accessRecoIndex-- > 0) һ��һ�����ͣ���������ȫ������
        if(gRecordIndex.accessRecoIndex > 0 && gConnectStatus==1)
        {
            len = readRecord(jsonbuff);
            if(len < RECORD_MAX_LEN && len > 0 )
            {                
                mqttSendData(jsonbuff,len);                    
            }
        }  
        
        xReturn = xQueueReceive( xCardIDQueue,    /* ��Ϣ���еľ�� */
                                 (void *)&ptMsg,  /*�����ȡ���ǽṹ��ĵ�ַ */
                                 xMaxBlockTime); /* ��������ʱ�� */
        if(pdTRUE != xReturn)
        { 
            continue;
        }
        
            //��Ϣ���ճɹ������ͽ��յ�����Ϣ                
            log_d("cardid %02x,%02x,%02x,%02x,devid = %d,mode = %d\r\n",ptMsg->cardID[0],ptMsg->cardID[1],ptMsg->cardID[2],ptMsg->cardID[3],ptMsg->devID,ptMsg->mode);
        
            log_d("======vTaskDataProcess mem perused = %3d%======\r\n",mem_perused(SRAMIN));

            //д��
            if(ptMsg->mode == SORT_CARD_MODE)
            {   

                //���������ҳ����
                sortPageCard();
                
            }
            else if(ptMsg->mode == MANUAL_SORT)
            {
                //����������ݽ�������
                manualSortCard();
            }  
            else if(ptMsg->mode == DEL_CARD_MODE)
            {
                ret = delHead(ptMsg->cardID,CARD_MODE);
                log_d("delHead = %d\r\n",ret);
                
                if(ret != 1)
                {
                   //1.ɾ���û�ʧ��

                }              
            }        
            else if(ptMsg->mode == READMODE) //����
            {      
    //            memcpy(ptMsg->cardID,"\x00\xc2\x84\x94",4);
                log_d("test cardid %02x,%02x,%02x,%02x\r\n",ptMsg->cardID[0],ptMsg->cardID[1],ptMsg->cardID[2],ptMsg->cardID[3]);
                
                ret = readHead(ptMsg->cardID, CARD_MODE);
                log_d("readHead = %d\r\n",ret);
                
                if(ret != NO_FIND_HEAD)
                {
                    log_d("read card success\r\n");     

                    ptCmd->cmd_len = 8;
                    
                    if(ptMsg->devID == 1)
                    {
                        memcpy(ptCmd->cmd,openLeft,ptCmd->cmd_len);                
                    }
                    else
                    {
                        memcpy(ptCmd->cmd,openRight,ptCmd->cmd_len); 
                    } 

        			/* ʹ����Ϣ����ʵ��ָ������Ĵ��� */
        			if(xQueueSend(xCmdQueue,             /* ��Ϣ���о�� */
        						 (void *) &ptCmd,             /* ���ͽṹ��ָ�����ptReader�ĵ�ַ */
        						 (TickType_t)30) != pdPASS )
        			{
                        xQueueReset(xCmdQueue);
                        DBG("send card2  queue is error!\r\n"); 
                        //���Ϳ���ʧ�ܷ�������ʾ
                        //�����Ƕ�����                
                    }
                    else
                    {
                        log_d("2 cardid %02x,%02x,%02x,%02x,devid = %d\r\n",ptMsg->cardID[0],ptMsg->cardID[1],ptMsg->cardID[2],ptMsg->cardID[3],ptMsg->devID);
                        //�������                
                        packetCard(ptMsg->cardID, jsonbuff);

                        //�������ݵ�MQTT������
                        len = strlen((const char*)jsonbuff);
                        log_d("send = %d,buff = %s\r\n",len,jsonbuff);   
                        
                        len = mqttSendData(jsonbuff,len);                    

                        //�����ж�mqttSendData�Ƿ�Ϊ0�����ǣ�����Ҫд�뵽FLASH��ȥ
                        if(len == 0)
                        {
                            //д��¼��FLASH
                            writeRecord(jsonbuff,  strlen((const char*)jsonbuff));
                        } 
                    }
                }
                else
                {
                    log_d("read card error: not find card\r\n");
                }  
            }
            else if(ptMsg->mode == REMOTE_OPEN_MODE) //Զ�̿���
            {
                //���Ϳ���ָ��

                log_d("read card success\r\n");     

                ptCmd->cmd_len = 8;  
                memcpy(ptCmd->cmd,openLeft,ptCmd->cmd_len);  
                
    			/* ʹ����Ϣ����ʵ��ָ������Ĵ��� */
    			if(xQueueSend(xCmdQueue,             /* ��Ϣ���о�� */
    						 (void *) &ptCmd,             /* ���ͽṹ��ָ�����ptReader�ĵ�ַ */
    						 (TickType_t)30) != pdPASS )
    			{
                    xQueueReset(xCmdQueue);
                    DBG("send card2  queue is error!\r\n"); 
                    //���Ϳ���ʧ�ܷ�������ʾ
                    //�����Ƕ�����                
                }            
            }
            
      
        /* �����¼���־����ʾ������������ */        
        xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_4); 
        vTaskDelay(30); 

    }

}


