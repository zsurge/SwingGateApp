/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : Led_Task.c
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��2��25��
  ����޸�   :
  ��������   : LED�ƿ���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��2��25��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "led_task.h"
//#include "bsp_tim_pwm.h"
#include "bsp_led.h"
#include "bsp_digitaltube.h"

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define LED_TASK_PRIO	    (tskIDLE_PRIORITY)
#define LED_STK_SIZE 		(configMINIMAL_STACK_SIZE)

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/
const char *ledTaskName = "vLedTask";      //LED��������


/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskLed = NULL;      //LED��


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
static void vTaskLed(void *pvParameters);

void CreateLedTask(void)
{
    //����LED����
    xTaskCreate((TaskFunction_t )vTaskLed,         
                (const char*    )ledTaskName,       
                (uint16_t       )LED_STK_SIZE, 
                (void*          )NULL,              
                (UBaseType_t    )LED_TASK_PRIO,    
                (TaskHandle_t*  )&xHandleTaskLed);
}


//LED������ 
static void vTaskLed(void *pvParameters)
{ 
    while(1)
    {  
        LEDERROR = !LEDERROR;
        
        bsp_HC595Show(1,0,1);
        vTaskDelay(500);
        bsp_HC595Show(0,0,1);    
        
		/* �����¼���־����ʾ������������ */        
		xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_0);  
        vTaskDelay(500); 
    }
} 

