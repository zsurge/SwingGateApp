/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : Reader_Task.c
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2020年2月27日
  最近修改   :
  功能描述   : 处理维根读卡器任务
  函数列表   :
  修改历史   :
  1.日    期   : 2020年2月27日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
 
#define LOG_TAG    "reader"
#include "elog.h"

#include "Reader_Task.h"
#include "CmdHandle.h"
#include "bsp_dipSwitch.h"
#include "tool.h"
#include "LocalData.h"
#include "bsp_Wiegand.h"
#include "bsp_beep.h"






/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define READER_TASK_PRIO	    ( tskIDLE_PRIORITY + 1)
#define READER_STK_SIZE 		(configMINIMAL_STACK_SIZE*8)





typedef union
{
	uint32_t id;        //卡号
	uint8_t sn[4];    //卡号按字符
}CARD_TYPE;


/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
const char *ReaderTaskName = "vReaderTask";  

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskReader = NULL;

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
static void vTaskReader(void *pvParameters);

static void reverseArray(uint8_t *array);

void CreateReaderTask(void)
{
    //跟android通讯串口数据解析
    xTaskCreate((TaskFunction_t )vTaskReader,     
                (const char*    )ReaderTaskName,   
                (uint16_t       )READER_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )READER_TASK_PRIO,
                (TaskHandle_t*  )&xHandleTaskReader);
}

void reverseArray(uint8_t *array) 
{
    int i,temp;
    int size = sizeof(array)/sizeof(array[0]);

    
    for(i=0; i<size/2; i++)
    {
        temp = array[i];
        array[i] = array[size-i-1];
        array[size-i-1] = temp;
    }   

}



static void vTaskReader(void *pvParameters)
{ 
    CARD_TYPE cardDev1,cardDev2;    
    READER_BUFF_STRU *ptReaderBuf = &gReaderMsg;     

    while(1)
    {        
        /* 清零 */
        ptReaderBuf->devID = 0; 
        ptReaderBuf->mode = 0;
        memset(ptReaderBuf->cardID,0x00,sizeof(ptReaderBuf->cardID));  

    
        cardDev1.id = bsp_WeiGenScanf(0);
        cardDev2.id = bsp_WeiGenScanf(1);

        if(cardDev1.id != 0)
        {
            Sound2(50);
            reverseArray(cardDev1.sn);
            
            ptReaderBuf->devID = READER1; 
            ptReaderBuf->mode = READMODE;
            memcpy(ptReaderBuf->cardID,cardDev1.sn,sizeof(cardDev1.sn));   


			/* 使用消息队列实现指针变量的传递 */
			if(xQueueSend(xCardIDQueue,             /* 消息队列句柄 */
						 (void *) &ptReaderBuf,             /* 发送结构体指针变量ptReader的地址 */
						 (TickType_t)10) != pdPASS )
			{
//                xQueueReset(xCardIDQueue); 删除该句，为了防止在下发数据的时候刷卡
                DBG("send card1  queue is error!\r\n"); 
                //发送卡号失败蜂鸣器提示
                //或者是队列满                
            }            

        }
        
        /* 清零 */
        ptReaderBuf->devID = 0; 
        ptReaderBuf->mode = 0;
        memset(ptReaderBuf->cardID,0x00,sizeof(ptReaderBuf->cardID));  

        if(cardDev2.id != 0)
        {
            Sound2(50);
            reverseArray(cardDev2.sn);
            
            ptReaderBuf->devID = READER2; 
            ptReaderBuf->mode = READMODE;            
            memcpy(ptReaderBuf->cardID,cardDev2.sn,sizeof(cardDev2.sn));   

			/* 使用消息队列实现指针变量的传递 */
			if(xQueueSend(xCardIDQueue,             /* 消息队列句柄 */
						 (void *) &ptReaderBuf,             /* 发送结构体指针变量ptReader的地址 */
						 (TickType_t)10) != pdPASS )
			{
//                xQueueReset(xCardIDQueue);删除该句，为了防止在下发数据的时候刷卡
                DBG("send card2  queue is error!\r\n"); 
                //发送卡号失败蜂鸣器提示
                //或者是队列满                
            } 

        }  
        
    	/* 发送事件标志，表示任务正常运行 */        
    	xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_2);       
        
        vTaskDelay(100);        
    }

} 




