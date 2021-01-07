/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : rs485Reader_Task.c
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��9��29��
  ����޸�   :
  ��������   : 485����������
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��9��29��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/

#define LOG_TAG    "BarCode"
#include "elog.h"


#include "bsp_uart_fifo.h"
#include "rs485Reader_Task.h"

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
 
#define RS485READER_TASK_PRIO		(tskIDLE_PRIORITY + 7)
#define RS485READER_STK_SIZE 		(configMINIMAL_STACK_SIZE*4)


#define UNFINISHED		        	    0x00
#define FINISHED          	 			0x55

#define STARTREAD		        	    0x00
#define ENDREAD         	 			0xAA

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/
const char *rS485ReaderTaskName = "vRS485ReadTask";  

typedef struct FROMREADER
{
    uint8_t rxBuff[512];               //�����ֽ���    
    uint8_t rxStatus;                   //����״̬
    uint16_t rxCnt;                     //�����ֽ���
}FROMREADER_STRU;

static FROMREADER_STRU gReaderData;


/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskRs485Reader = NULL;

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
static void vTaskRs485Reader(void *pvParameters);
static  uint8_t parseReader(void);


void CreateRs485ReaderTask(void)
{
    //��ȡ�������ݲ�����
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
        //��ȡ485���ݣ�����û�������˳������ض�
        if(!RS485_Recv(COM3,&ch,1))
        {            
            return UNFINISHED;
        }
        
        //��ȡ���������ݵ�BUFF
        gReaderData.rxBuff[gReaderData.rxCnt++] = ch;
        
//        log_d("ch = %c,gReaderData.rxBuff = %c \r\n",ch,gReaderData.rxBuff[gReaderData.rxCnt-1]);
         
        //���һ���ֽ�Ϊ�س��������ܳ���Ϊ510�󣬽�����ȡ
        if(gReaderData.rxBuff[gReaderData.rxCnt-1] == 0x0A || gReaderData.rxCnt >=510)
        {   
            
           if(gReaderData.rxBuff[gReaderData.rxCnt-1] == 0x0A)
           {
//                dbh("gReaderData.rxBuff", (char*)gReaderData.rxBuff, gReaderData.rxCnt);  
//                log_d("gReaderData.rxBuff = %s,len = %d\r\n",gReaderData.rxBuff,gReaderData.rxCnt-1);
                return FINISHED;
           }

            //��û�������һ���ֽڣ������ܳ���λ����գ��ض�������
            memset(&gReaderData,0xFF,sizeof(FROMREADER_STRU));
        }
        
    }   

   
}


