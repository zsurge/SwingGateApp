/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : rs485Reader_Task.c
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2020年9月29日
  最近修改   :
  功能描述   : 485读卡器任务
  函数列表   :
  修改历史   :
  1.日    期   : 2020年9月29日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

#define LOG_TAG    "BarCode"
#include "elog.h"


#include "bsp_uart_fifo.h"
#include "rs485Reader_Task.h"

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
 
#define RS485READER_TASK_PRIO		(tskIDLE_PRIORITY + 7)
#define RS485READER_STK_SIZE 		(configMINIMAL_STACK_SIZE*4)


#define UNFINISHED		        	    0x00
#define FINISHED          	 			0x55

#define STARTREAD		        	    0x00
#define ENDREAD         	 			0xAA

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
const char *rS485ReaderTaskName = "vRS485ReadTask";  

typedef struct FROMREADER
{
    uint8_t rxBuff[512];               //接收字节数    
    uint8_t rxStatus;                   //接收状态
    uint16_t rxCnt;                     //接收字节数
}FROMREADER_STRU;

static FROMREADER_STRU gReaderData;


/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskRs485Reader = NULL;

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
static void vTaskRs485Reader(void *pvParameters);
static  uint8_t parseReader(void);


void CreateRs485ReaderTask(void)
{
    //读取条码数据并处理
    xTaskCreate((TaskFunction_t )vTaskRs485Reader,     
                (const char*    )rS485ReaderTaskName,   
                (uint16_t       )RS485READER_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )RS485READER_TASK_PRIO,
                (TaskHandle_t*  )&xHandleTaskRs485Reader);
}


static void vTaskRs485Reader(void *pvParameters)
{ 
        uint8_t sendBuff[512] = {0};
        uint16_t len = 0; 
        uint8_t relieveControl[38] = {0};        

        while(1)
        { 
            memset(sendBuff,0x00,sizeof(sendBuff));
        
            if(parseReader() == FINISHED)
            {
                len = gReaderData.rxCnt;
                memcpy(sendBuff,gReaderData.rxBuff,len);        
                memset(&gReaderData,0x00,sizeof(FROMREADER_STRU));

                log_d("gReaderData.rxBuff =%s,len=%d\r\n",sendBuff,len);

                dbh("gReaderData.rxBuff", sendBuff,  len);
            }

            vTaskDelay(200); 
        }

}

static uint8_t parseReader(void)
{
    uint8_t ch = 0;   
    
    while(1)
    {    
        //读取485数据，若是没读到，退出，再重读
        if(!RS485_Recv(COM3,&ch,1))
        {            
            return UNFINISHED;
        }
        
        //读取缓冲区数据到BUFF
        gReaderData.rxBuff[gReaderData.rxCnt++] = ch;
        
//        log_d("ch = %c,gReaderData.rxBuff = %c \r\n",ch,gReaderData.rxBuff[gReaderData.rxCnt-1]);
         
        //最后一个字节为回车，或者总长度为510后，结束读取
        if(gReaderData.rxBuff[gReaderData.rxCnt-1] == 0x0A || gReaderData.rxCnt >=510)
        {   
            
           if(gReaderData.rxBuff[gReaderData.rxCnt-1] == 0x0A)
           {
//                dbh("gReaderData.rxBuff", (char*)gReaderData.rxBuff, gReaderData.rxCnt);  
//                log_d("gReaderData.rxBuff = %s,len = %d\r\n",gReaderData.rxBuff,gReaderData.rxCnt-1);
                return FINISHED;
           }

            //若没读到最后一个字节，但是总长到位后，清空，重读缓冲区
            memset(&gReaderData,0xFF,sizeof(FROMREADER_STRU));
        }
        
    }   

   
}


