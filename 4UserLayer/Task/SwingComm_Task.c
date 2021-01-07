/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : SwingComm_Task.c
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2021年1月6日
  最近修改   :
  功能描述   : 摆闸门指令解析
  函数列表   :
  修改历史   :
  1.日    期   : 2021年1月6日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

#define LOG_TAG    "CommTask"
#include "elog.h"
#include "SwingComm_Task.h"
#include "CmdHandle.h"
#include "bsp_uart_fifo.h"

/*----------------------------------------------*
 * 宏定义                                       *
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
    uint8_t rxStatus;                   //接收状态
    uint8_t rxCRC;                       //校验值.
    uint8_t rxCnt;                     //已接收字节数  
    uint8_t rxBuff[32];                 //接收字节
    uint8_t rxPacketState;              //整个数据包接收状态 
}FROMHOST_STRU;

static FROMHOST_STRU rxFromHost;

CMD_BUFF_STRU gCmd4_buff = {0};
uint8_t gdev4Param[11] = {0};

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
const char *SwingCommTaskName = "vSwingCommTask"; 

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskSwingComm = NULL;  
/*----------------------------------------------*
 * 内部函数原型说明                             *
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
    BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为pdPASS */
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50); /* 设置最大等待时间为100ms */ 

    const char readDevState[7] = { 0x02,0x00,0x06,0x01,0x05,0x03,0x03 };
    
    CMD_BUFF_STRU *ptCmd = &gCmd4_buff;
    
    memset(&gCmd4_buff,0x00,sizeof(CMD_BUFF_STRU));   
    

    while(1)
    { 

        xReturn = xQueueReceive( xComm4Queue,    /* 消息队列的句柄 */
                                 (void *)&ptCmd,  /*这里获取的是结构体的地址 */
                                 xMaxBlockTime); /* 设置阻塞时间 */
        if(pdTRUE == xReturn)
        {  
            //发送指令
            dbh("ptCmd->cmd", ptCmd->cmd, ptCmd->cmd_len);
            
            RS485_SendBuf(COM4, ptCmd->cmd, ptCmd->cmd_len); 
        }
        else
        {
            //发送查询        
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
                if(0x02 == ch) /*接收包头*/
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
        if((((uint16_t)rxFromHost.rxBuff[1] << 8) |(uint16_t)(rxFromHost.rxBuff[2])) == rxFromHost.rxCnt-1)                                   //解析02数据包
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






