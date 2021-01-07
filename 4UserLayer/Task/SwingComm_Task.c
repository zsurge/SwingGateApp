/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : SwingComm_Task.c
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2021��1��6��
  ����޸�   :
  ��������   : ��բ��ָ�����
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2021��1��6��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/

#define LOG_TAG    "CommTask"
#include "elog.h"
#include "SwingComm_Task.h"
#include "CmdHandle.h"
#include "bsp_uart_fifo.h"

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define SWINGCOMM_TASK_PRIO		(tskIDLE_PRIORITY + 6) 
#define SWINGCOMM_STK_SIZE 		(configMINIMAL_STACK_SIZE*8)

#define SPACE		        			0x00
#define FINISH		       	 			0x55

#define STEP1   0
#define STEP2   10
#define STEP3   20

typedef struct FROMHOST
{
    uint8_t rxStatus;                   //����״̬
    uint8_t rxCRC;                       //У��ֵ.
    uint8_t rxCnt;                     //�ѽ����ֽ���  
    uint8_t rxBuff[32];                 //�����ֽ�
    uint8_t rxPacketState;              //�������ݰ�����״̬ 
}FROMHOST_STRU;

static FROMHOST_STRU rxFromHost;

CMD_BUFF_STRU gCmd4_buff = {0};
uint8_t gdev4Param[11] = {0};

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/
const char *SwingCommTaskName = "vSwingCommTask"; 

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskSwingComm = NULL;  
/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
static void vTaskSwingComm(void *pvParameters);
static void deal_rx_Parse(void);
static void deal_rx_data(void);
static void init_serial_buf(void);





void CreateSwingCommTask(void)
{
    xTaskCreate((TaskFunction_t )vTaskSwingComm,         
                (const char*    )SwingCommTaskName,       
                (uint16_t       )SWINGCOMM_STK_SIZE, 
                (void*          )NULL,              
                (UBaseType_t    )SWINGCOMM_TASK_PRIO,    
                (TaskHandle_t*  )&xHandleTaskSwingComm);
}


static void vTaskSwingComm(void *pvParameters)
{ 
    BaseType_t xReturn = pdTRUE;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50); /* �������ȴ�ʱ��Ϊ100ms */ 

    const char readDevState[7] = { 0x02,0x00,0x06,0x01,0x05,0x03,0x03 };
    
    CMD_BUFF_STRU *ptCmd = &gCmd4_buff;
    
    memset(&gCmd4_buff,0x00,sizeof(CMD_BUFF_STRU));   
    

    while(1)
    { 

        xReturn = xQueueReceive( xComm4Queue,    /* ��Ϣ���еľ�� */
                                 (void *)&ptCmd,  /*�����ȡ���ǽṹ��ĵ�ַ */
                                 xMaxBlockTime); /* ��������ʱ�� */
        if(pdTRUE == xReturn)
        {  
            //����ָ��
            dbh("ptCmd->cmd", ptCmd->cmd, ptCmd->cmd_len);
            
            RS485_SendBuf(COM4, ptCmd->cmd, ptCmd->cmd_len); 
        }
        else
        {
            //���Ͳ�ѯ        
            RS485_SendBuf(COM4, (uint8_t *)readDevState, 7); 
        }

        vTaskDelay(50);    
        
        deal_rx_Parse();
        deal_rx_data(); 
        
        xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_2);   
        
    }  
}





void deal_rx_Parse(void)
{
    uint8_t ch = 0;
    uint16_t dataLen = 0;
    
    
    memset(&rxFromHost,0x00,sizeof(FROMHOST_STRU));   
    
    while(RS485_Recv(COM4,&ch,1))
    {       
       switch (rxFromHost.rxStatus)
        {                
            case STEP1:
                if(0x02 == ch) /*���հ�ͷ*/
                {
                    rxFromHost.rxBuff[0] = ch;                    
                    rxFromHost.rxCnt = 1;
                    rxFromHost.rxCRC = ch;
                    rxFromHost.rxStatus = STEP2;
                }
                break;
           case STEP2:
                if (0x03 == ch) 
                {
                    rxFromHost.rxStatus = STEP3;
                }
                rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch; //ETX
                rxFromHost.rxCRC ^=  ch;
                break;
          case STEP3:

//                dbh("recv", rxFromHost.rxBuff, rxFromHost.rxCnt);
                
                dataLen = ((uint16_t)rxFromHost.rxBuff[1] << 8) |(uint16_t)(rxFromHost.rxBuff[2]);;

                if(dataLen == rxFromHost.rxCnt && rxFromHost.rxCRC == ch)
                {
//                    log_d("-----------CMD OK-----------\r\n");
                     rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch;
                     rxFromHost.rxPacketState = FINISH;                    
                     rxFromHost.rxStatus = STEP1;
                     break;
                }
                else
                {                
                    log_d("-----------CRC ERR-----------\r\n");
                    rxFromHost.rxPacketState = FINISH;  
                    rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch;
                    rxFromHost.rxStatus = STEP2;
                    rxFromHost.rxCRC ^=  ch;
                }
                break;         
            default:
                if (rxFromHost.rxPacketState == SPACE) 
                {
                    rxFromHost.rxPacketState = FINISH;
                    rxFromHost.rxStatus = STEP1;
                } 
         }
    }  
}

void deal_rx_data(void)
{
    uint8_t buf[256] = {0};

    memset(buf,0x00,sizeof(buf));  
    
    if (FINISH == rxFromHost.rxPacketState)
    {   
        if((((uint16_t)rxFromHost.rxBuff[1] << 8) |(uint16_t)(rxFromHost.rxBuff[2])) == rxFromHost.rxCnt-1)                                   //����02���ݰ�
        {   
            if(rxFromHost.rxBuff[5] != 0x4f)
            {
                log_d("cmd excute error\r\n");
            }   
            
            switch(rxFromHost.rxBuff[4])
            {
                case 0x0B:
                     if(rxFromHost.rxBuff[2] == 0x11)
                     {
                         memcpy(gdev4Param,rxFromHost.rxBuff+5,11);
                     }                
                     break;
                default:
                     break;
                
            }
            
        }
        else
        {
            init_serial_buf();
        }
    }
}

void init_serial_buf(void)
{ 
    memset(&rxFromHost,0x00,sizeof(FROMHOST_STRU));
}






